/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          modelparser.inc
 *  Type:          Module include
 *  Description:   Model query parser for the model db.
 *
 *  Copyright (C) 2009-2011  Greyscale, Richard Helgeby
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 */

/*
Query syntax:

<option>
or
{<model> | <filter>[,...] | c:<collection> | <filter>[,...]:collection>} ...

Options:
default | no_change

Filters:
random | no_auth | zombie | human | both_teams | public | admin | mother_zombie
*/

/**
 * Query parsing modes. Default and NoChange is not handled by this module.
 */
enum ModelParserMode
{
    ModelParser_Invalid = -1,
    ModelParser_Default,    /** No query. Use default CS:S model that player selected when connecting. */
    ModelParser_NoChange,   /** No model change at all. Compatibility mode for other plugins. */
    ModelParser_First,      /** Pick first accessible model. Useful for prioritized queries. */
    ModelParser_Random,     /** Pick a random accessible model. */
    ModelParser_List        /** Add accessible models to a list for further processing. */
}

/**
 * @section Model parse error codes.
 */
#define MODEL_ERR_SUCCESS               0   /** No errors. */
#define MODEL_ERR_EMPTY                 1   /** Query string was empty. */
#define MODEL_ERR_NOT_FOUND             2   /** No model found. */
#define MODEL_ERR_INVALID_FILTER        3   /** Invalid filter name. */
#define MODEL_ERR_INVALID_COLLECTION    4   /** Invalid collection name. */
#define MODEL_ERR_INVALID_MODEL         5   /** Invalid model or filter name. */
#define MODEL_ERR_EMPTY_ENTRY           6   /** Empty entry (double spaces). */
/**
 * @endsection
 */

/**
 * Separator for filter lists.
 * Note: MUST be just one character. Otherwise parser must be updated too.
 */
#define MODEL_FILTER_SEPARATOR ","

/**
 * Prefix to separate filter list and collection name.
 * Note: MUST be just one character. Otherwise parser must be updated too.
 */
#define MODEL_COLLECTION_PREFIX ":"

/**
 * Maximum lenghth of the query string buffer.
 */
#define MODEL_MAX_QUERY_LEN 320

/**
 * Max length of filter name buffers.
 */
#define MODEL_FILTER_NAME_LEN 16


/**
 * Parses a model query string for a client.
 *
 * @param client    Client index. Used for authorization.
 *                  The server will always get access.
 * @param query     Model query string.
 * @param mode      Parser mode.
 * @param list      Optional. List to put models in if mode is ModelParser_List.
 * @param errCode   Output, optional. Error code.
 * @param errPos    Output, optional. Position in query string where it failed.
 *
 * @return          -1 if it's a predefined option (mode).
 *
 *                  List mode: Number of models added to the list.
 *
 *                  Other modes: Model index if found, or a negative number:
 *                  -2 if empty query string,
 *                  -3 if empty entry,
 *                  -4 if invalid collection entry,
 *                  -5 if invalid filter entry,
 *                  -6 if no model found.
 *                  -7 if invalid parser mode.
 */
ModelDB_ParseQuery(client, const String:query[], ModelParserMode:mode = ModelParser_First, Handle:list = INVALID_HANDLE, &errCode = 0, &errPos = 0)
{
    new model = -1;
    new filters = 0;
    new bool:asList = (mode == ModelParser_List);
    new listCount = 0;
    
    // Reset error variables.
    new tempErrPos = 0;
    errPos = 0;
    errCode = 0;
    
    new entryCount;
    decl String:entries[MODEL_MAX_ENTRIES][MODEL_STRING_LEN];
    
    // Check if it's a predefined option.
    new ModelParserMode:option = ModelDB_StringToParserMode(query);
    if(option == ModelParser_Default || option == ModelParser_NoChange)
    {
        // Predefined option. Do nothing and leave it to other modules/plugins.
        return asList ? 0 : -1;
    }
    
    // Do a quick check for single model queries. If the query is longer than
    // the maximum name length it's definitely not a model name.
    if (strlen(query) <= MODEL_NAME_LEN)
    {
        model = ModelMgr_GetModelIndex(query);
        if (model >= 0)
        {
            if (asList)
            {
                PushArrayCell(list, model);
                return 1;
            }
            else
            {
                return model;
            }
        }
    }
    
    // Prepare filtered list.
    new Handle:filtered = asList ? list : CreateArray();
    
    // Separate entries.
    entryCount = ZR_ExplodeStringEx(query, MODEL_ENTRY_SEPARATOR, entries, MODEL_MAX_ENTRIES, MODEL_STRING_LEN);
    
    // Check if failed.
    if (entryCount == 0)
    {
        errCode = MODEL_ERR_EMPTY;
        return asList ? 0 : -2;
    }
    
    // Loop through entries.
    for (new entry = 0; entry < entryCount; entry++)
    {
        // Calculate entry position in query string.
        if (entry > 0)
        {
            // Continue after last entry.
            errPos += strlen(entries[entry - 1]);
            
            // MODEL_ENTRY_SEPARATOR, 1 character.
            errPos++;
        }
        
        // Check for empty entry (double spaces).
        if (strlen(entries[entry]) == 0)
        {
            // (errPos is already set).
            errCode = MODEL_ERR_EMPTY_ENTRY;
            return asList ? 0 : -3;
        }
        
        // Determine entry type.
        if (StrContains(entries[entry], MODEL_COLLECTION_PREFIX) >= 0)
        {
            // It's a collection. The collection parser also parses filters
            // on collections.
            model = ModelDB_ParseCollection(client, entries[entry], asList, list, errCode, tempErrPos);
            
            // Check for parse errors.
            if (errCode > 0)
            {
                errPos += tempErrPos;
                return asList ? 0 : -4;
            }
            
            if (asList)
            {
                listCount += model;
            }
            else if (model >= 0)
            {
                PushArrayCell(filtered, model);
                listCount++;
            }
        }
        else
        {
            // It's a model or filter. Test if it's a model.
            model = ModelMgr_GetModelIndex(entries[entry]);
            
            if (model >= 0)
            {
                // It's a model. Add to array/list.
                PushArrayCell(filtered, model);
                listCount++;
            }
            else
            {
                // It's a filter list. Parse it and get a model.
                filters = ModelDB_ParseFilters(entries[entry], tempErrPos);
                
                // Check for parse errors.
                if (filters < 0)
                {
                    // Calculate position.
                    errPos += tempErrPos;
                    errCode = MODEL_ERR_INVALID_MODEL;  // Invalid model or filter name.
                    return asList ? 0 : -5;
                }
                
                model = ModelDB_GetModel(client, filters, _, asList, list);
                
                // Note: No need to check for errors here. No model in return
                //       is valid too.
                
                if (asList)
                {
                    listCount += model;
                }
                else if (model >= 0)
                {
                    // Add model to filtered list.
                    PushArrayCell(filtered, model);
                    listCount++;
                }
            }
        }
    }
    
    // Check if using a list.
    if (asList)
    {
        // Stop here and return number of models added to the list.
        return listCount;
    }
    
    // Check if no models found.
    if (listCount == 0)
    {
        return -6;
    }
    
    switch (mode)
    {
        case ModelParser_First:
        {
            // Return the first model.
            return GetArrayCell(filtered, 0);
        }
        case ModelParser_Random:
        {
            // Return a random model.
            new randIndex = GetRandomInt(0, listCount - 1);
            return GetArrayCell(filtered, randIndex);
        }
    }
    
    // Invalid mode.
    return -7;
}

/**
 * Parse a filter flag list into a flag bit field. Invalid filters are skipped.
 *
 * @param filterList    List of filter flags separated by
 *                      MODEL_FILTER_SEPARATOR.
 * @param errPos        Output, optional. Position in filter list string that
 *                      failed.
 *
 * @return              Filter flags in a bit field. -1 on error (with position
 *                      in errPos.)
 */
ModelDB_ParseFilters(const String:filterList[], &errPos = 0)
{
    new filters = 0;
    errPos = 0;
    decl String:filterEntries[MODEL_MAX_ENTRIES][MODEL_FILTER_NAME_LEN];
    
    new count = ZR_ExplodeStringEx(filterList, MODEL_FILTER_SEPARATOR, filterEntries, MODEL_MAX_ENTRIES, MODEL_FILTER_NAME_LEN);
    
    for (new entry = 0; entry < count; entry++)
    {
        // Calculate entry position in query string.
        if (entry > 0)
        {
            // Continue after last entry.
            errPos += strlen(filterEntries[entry - 1]);
            
            // MODEL_FILTER_SEPARATOR, 1 character.
            errPos++;
        }
        
        new filter = ModelDB_StringToFilter(filterEntries[entry]);
        if (filter > 0)
        {
            filters |= filter;
        }
        else
        {
            // Invalid filter name. Error position is already updated.
            return -1;
        }
    }
    
    return filters;
}

/**
 * Parses a collection entry in a model query.
 *
 * @param client    Client that will use the model. Used for authorization.
 *                  The server will always get access.
 * @param entry     Collection query entry.
 * @param asList    Optional. Return all accessible models in a list.
 *                  Defaults to false.
 * @param list      Optional. List to put models in.
 * @param errCode   Output, optional. Error code.
 * @param errPos    Output, optional. Position in entry string where it failed.
 *
 * @return          If asList is true; number of models added to list.
 *                  Otherwise model index, or negative number on error:
 *                  -1 if no models found,
 *                  -2 if invalid filter name,
 *                  -3 if invalid collection name.
 */
ModelDB_ParseCollection(client, const String:entry[], bool:asList = false, Handle:list = INVALID_HANDLE, &errCode = 0, &errPos = 0)
{
    new model = -1;
    new filters;
    decl String:filterBuffer[MODEL_STRING_LEN];
    
    // Reset error variables.
    errCode = 0;
    errPos = 0;
    
    new pos = SplitString(entry, MODEL_COLLECTION_PREFIX, filterBuffer, sizeof(filterBuffer));
    
    // Do a quick check to see if there's no filter (just c:collection).
    if (StrEqual(filterBuffer, "c", false))
    {
        // No filters.
        filters = 0;
    }
    else
    {
        // Get filter flags.
        filters = ModelDB_ParseFilters(filterBuffer, errPos);
        
        // Check if failed.
        if (filters < 0)
        {
            // Invalid filter name. Error position is already updated.
            errCode = MODEL_ERR_INVALID_FILTER;
            return asList ? 0 : -2;
        }
    }
    
    // Get collection. The collection name is the other part of the entry.
    new collection = ModelMgr_GetCollectionIndex(entry[pos]);
    
    // Check for errors.
    if (collection < 0)
    {
        // Invalid collection name.
        errPos = pos;
        errCode = MODEL_ERR_INVALID_COLLECTION;
        return asList ? 0 : -3;
    }
    
    // Get list.
    new Handle:collectionList = ModelCollectionData[collection][ModelCollection_Array];
    
    // Get model(s) from list.
    model = ModelDB_GetModel(client, filters, collectionList, asList, list);
    
    return model;   // Returns list count if writing to list, -1 if no models found.
}

/**
 * Adds a marker (^) at the specified position on a new line below.
 *
 * Note: Does not work with multiline strings as source.
 *
 * @param pos       Position of marker (zero based).
 * @param buffer    Destination string buffer.
 * @param maxlen    Size of buffer.
 * 
 * @return          Number of cells written.
 */
ModelDB_GetStringMarker(pos, String:buffer[], maxlen)
{
    // Check if the marker is outside the buffer space.
    if (pos > maxlen - 2)
    {
        // Outside buffer, stop here. The marker won't be visible anyways.
        return 0;
    }
    
    // Write spaces before marker.
    for (new i = 0; i < pos; i++)
    {
        buffer[i] = ' ';
    }
    
    // Write marker.
    buffer[pos] = '^';
    
    // Terminate string.
    buffer[pos + 1] = 0;
    
    // +1 for the terminator, +1 to get off zero-based number.
    return pos + 2;
}

/**
 * Breaks a string into pieces and stores each piece into an array of buffers.
 *
 * Note: This is a fixed version so it returns correct number of strings
 *       retreived (it doesn't fix all cases, but good enough for us). The SM
 *       developers doesn't have a official fix yet - so we fork it.
 *
 * @param text				The string to split.
 * @param split				The string to use as a split delimiter.
 * @param buffers			An array of string buffers (2D array).
 * @param maxStrings		Number of string buffers (first dimension size).
 * @param maxStringLength	Maximum length of each string buffer.
 * @return					Number of strings retrieved.
 */
stock ZR_ExplodeStringEx(const String:text[], const String:split[], String:buffers[][], maxStrings, maxStringLength)
{
	new reloc_idx, idx, total;
	
	if (maxStrings < 1 || split[0] == '\0')
	{
		return 0;
	}
	
	while ((idx = SplitString(text[reloc_idx], split, buffers[total], maxStringLength)) != -1)
	{
		reloc_idx += idx;
		if (text[reloc_idx] == '\0')
		{
			total++;        // Bug fix by Richard.
			break;
		}
		if (++total >= maxStrings)
		{
			return total;
		}
	}
	
	if (text[reloc_idx] != '\0' && total <= maxStrings - 1)
	{
		strcopy(buffers[total++], maxStringLength, text[reloc_idx]);
	}
	
	return total;
}


/****************************
 *   Conversion functions   *
 ****************************/

/**
 * Converts a mode name into a parser mode value.
 *
 * @param mode      Mode name to convert.
 *
 * @return          Parser mode, or ModelParser_Invalid if failed.
 */
public ModelParserMode:ModelDB_StringToParserMode(const String:mode[])
{
    if (strlen(mode) == 0)
    {
        return ModelParser_Invalid;
    }
    else if (StrEqual(mode, "default", false))
    {
        return ModelParser_Default;
    }
    else if (StrEqual(mode, "no_change", false))
    {
        return ModelParser_NoChange;
    }
    else if (StrEqual(mode, "first", false))
    {
        return ModelParser_First;
    }
    else if (StrEqual(mode, "random", false))
    {
        return ModelParser_Random;
    }
    
    return ModelParser_Invalid;
}

/**
 * Converts a error code to a user friendly message.
 *
 * @param errCode   Error code to convert
 * @param buffer    Destination string buffer.
 * @param maxlen    Size of buffer.
 */
public ModelDB_ErrCodeToString(errCode, String:buffer[], maxlen)
{
    switch (errCode)
    {
        case MODEL_ERR_EMPTY:
        {
            return strcopy(buffer, maxlen, "Query string is empty");
        }
        case MODEL_ERR_NOT_FOUND:
        {
            return strcopy(buffer, maxlen, "No model found");
        }
        case MODEL_ERR_INVALID_FILTER:
        {
            return strcopy(buffer, maxlen, "Invalid filter name");
        }
        case MODEL_ERR_INVALID_COLLECTION:
        {
            return strcopy(buffer, maxlen, "Invalid collection name");
        }
        case MODEL_ERR_INVALID_MODEL:
        {
            return strcopy(buffer, maxlen, "Invalid model or filter name");
        }
        case MODEL_ERR_EMPTY_ENTRY:
        {
            return strcopy(buffer, maxlen, "Empty entry (double spaces)");
        }
    }
    
    return 0;
}
