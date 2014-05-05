/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:           querylib.inc
 *  Type:           Library
 *  Description:    This is a general query parser for selecting one or more
 *                  elements from a list (array) and collections based on
 *                  elemens in the list. It supports filters and authorization.
 *
 *  Copyright (C) 2009-2012  Richard Helgeby, Greyscale
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

#if defined _querylib_included
 #endinput
#endif
#define _querylib_included

#include "zr/libraries/utilities"
#include "zr/libraries/arrays"

//#section querylib

/*
DESCRIPTION

This is a general query parser for selecting one or more elements from a list
(array) and collections based on elements from the list.

The query language is based on one or more entries separated by spaces. A entry
may be a name of an element, a filter list, a collection list, or a collection
list with filters. While this language is based on names, it's internally using
indexes to refer to elements. Elements in the backing implementation must have
unique indexes.

The code that use this library may also do some preprocessing on the query if
needed. This could simply be to check if the query is a predefined option.

When the query is executed, all entries are validated to catch typos and
errors. To validate a query, simple execute a query and check the error code
parameter.

Don't consider this parsing algorithm as fast. Save results unless you need a
new result each time (random mode). Don't execute queries in busy events. It's
also recommended to use a name index on elements (ADT Trie with names mapped to
element indexes) for fast named lookup.


CALLBACKS AND FUNCTIONS

Since this library work with abstract element types it depends on callbacks and
functions to do operations on elements like validating names and testing
filters.

To simplify the parse function, these callbacks are provided through a callback
set. Before parsing, a callback set must be created. Then the single returned
handle is passed into the parse function instead of several callback
parameters.

Callback prototypes:

(See prototypes in code below)


ADVANTAGES / DISADVANTAGES

+ Very simple and flexible.
* Named lookups must be fast to not slow down the parser. Using ADT Tries with
  names mapped to indexes will make fast lookup index.
- It doesn't use caches, so some operations may be repeated. Filter names and
  values will be reparsed each time.
- Might be expensive for longer query strings.
  Possible optimization: Preprocess the query and resolve identifiers into
  numeric values. Tokenize the query into simple opcodes.


SYNTAX

The following markup is not a part of the language, but describes parts of the
syntax:
{}      - group parts
|       - "or", "any of these", only part on either side can be used
[]      - optional part
...     - multiple parts is allowed (repeat last part/group)
<>      - identifier (mode, element, filter, collection)

Any markup that's NOT listed above should be considered literal and must be
written in the query string.

Query Syntax:
{s:<select mode> | <element> | f:<filter>[,...] | c:<collection>[,...] | f:<filter>[,...]:collection>[,...]} ...

Example:
my_element c:a_collection s:random f:some_filter,another_filter

This example query results in a list with:
* "my_element"
* all elements in the collection "a_collection"
* a random element that passes two filters ("some_filter" and "another_filters")

Element:
Element identifier. Elements can be explicitly added to the query. Its name
is validated through the callbacks.

Collection:
Collection identifier. Whether collections are supported depends on the code
that use this library. A collection is a named ADT array with element indexes.
A filter may also be applied to a collection before reading it. In addition a
select mode entry can be set before the collection entry to change how the
collection is read.

Select Modes:
How elements are selected. Only used by the following filters and collections.
A query may have several select mode entries so that one part of the query may
use one select mode and another part use another select mode.
first   - Pick first accessible element. Useful for prioritized queries
          Example: Attempt to select most restrictive element before falling
                   back on others.
          Example: Attempt to select VIP model before falling back on public
                   model.
last    - Pick the last accessible element.
random  - Pick a random accessible element.
all     - Get all accessible elements for further processing.

Filters:
Filter name list. The filter names are not defined by this library, but
validated and tested through the callbacks. Filters are passed as a bit field
in the callback. This also implies that there can be no more than 31 filters,
since a cell variable is signed 32 bit value.

A filter cannot have filters that act as operations ("random" filter), such
operations are select modes. Filters must be able to instantly tell whether a
single element passes this filter.

*/

/**
 * @section Callback prototypes.
 */

/**
 * Gets a list of all available elements (indexes).
 *
 * Note: This function may be frequently called. Don't build the element list
 *       every time in this callback, store the list handle.
 *
 * @return          Handle to element list.
 */
functag public Handle:QueryLib_GetElementList();

/**
 * Gets the element index for a element name.
 *
 * Note: This function may be frequently called. Don't do expensive operations
 *       here. Use a name index (ADT Trie with names mapped to element indexes).
 *
 * @param name      Element identifier.
 *
 * @return          Element index, or -1 if not valid.
 */
functag public QueryLib_GetElementIndex(const String:name[]);

/**
 * Gets the filter bit value for a certain filter name.
 *
 * Note: This function may be frequently called. Don't do expensive operations
 *       here. If there's a lot of filter names (or lots of filters start with
 *       the same names, use a name index (ADT Trie with names mapped to filter
 *       values).
 *
 * @param name      Filter name.
 *
 * @return          Filter bit value, or 0 if not valid.
 */
functag public QueryLib_GetFilterValue(const String:name[]);

/**
 * Returns whether a element passed a filter test.
 *
 * Note: This function may be frequently called. Don't do expensive operations
 *       here. Use caches for expensive operations.
 *
 * @param client    Client index. Used to restrict the filter further if
 *                  authorization is required in the filter. The server should
 *                  always be authorized (but filter flags must not be ignored
 *                  for the server).
 * @param element   Element index.
 * @param filter    Filter flags (bit field).
 *
 * @return          True if passed, false otherwise.
 */
functag public bool:QueryLib_PassedFilter(client, element, filter);

/**
 * Gets the collection with the specified name.
 *
 * Note: This function may be frequently called. Don't do expensive operations
 *       here. Use a name index (ADT Trie with names mapped to collections).
 *
 * @param name      Collection identifier.
 *
 * @return          Handle to collection array, or INVALID_HANDLE if not valid.
 */
functag public Handle:QueryLib_GetCollection(const String:name[]);

/**
 * @endsection
 */

/**
 * Query parsing modes.
 */
enum QueryLib_ResultMode
{
    QueryLib_Invalid = -1,  /** Not a valid mode. Used by validators. */
    QueryLib_First,         /** Pick first accessible element. Useful for prioritized queries. */
    QueryLib_Last,          /** Pick first accessible last. Useful for prioritized queries. */
    QueryLib_Random,        /** Pick a random accessible element. */
    QueryLib_List           /** Add accessible elements to a list for further processing. */
}

/**
 * Set of callbacks required by the parser.
 */
enum QueryLib_CallbackSet
{
    Function:QueryLibCall_ElementList,
    Function:QueryLibCall_ElementIndex,
    Function:QueryLibCall_FilterValue,
    Function:QueryLibCall_PassedFilter,
    Function:QueryLibCall_GetCollection
}

/**
 * Entry types.
 */
enum QueryLib_EntryType
{
    QueryLibEntry_Invalid = -1,     /** Not a valid type. Used by validators.*/
    QueryLibEntry_SelectMode,       /** Select mode. */
    QueryLibEntry_Element,          /** Element name. */
    QueryLibEntry_Filter,           /** Filter list. */
    QueryLibEntry_Collection,       /** Collection list. */
    QueryLibEntry_FilterCollection  /** Filtered collection list. */
}

/**
 * @section Parser error codes.
 */
#define QUERYLIB_ERR_SUCCESS                0   /** No errors. Only valid in errCode parameters. */
#define QUERYLIB_ERR_EMPTY                  -1  /** Query string was empty. */
#define QUERYLIB_ERR_NONE_FOUND             -2  /** No element found. */
#define QUERYLIB_ERR_INVALID_FILTER         -3  /** Invalid filter name. */
#define QUERYLIB_ERR_INVALID_COLLECTION     -4  /** Invalid collection name. */
#define QUERYLIB_ERR_INVALID_ELEMENT        -5  /** Invalid element name. */
#define QUERYLIB_ERR_INVALID_ENTRY          -6  /** Invalid entry (syntax error). */
#define QUERYLIB_ERR_INVALID_MODE           -7  /** Invalid select mode. */
#define QUERYLIB_ERR_EMPTY_ENTRY            -8  /** Empty entry (double spaces). */
#define QUERYLIB_ERR_TOO_MANY_ENTRIES       -9  /** Maximum number of entries reached. */
#define QUERYLIB_ERR_NO_CALLBACK            -10 /** No callbacks specified. */
#define QUERYLIB_ERR_NO_LIST                -11 /** No result list specified. */
/**
 * @endsection
 */

/**
 * Separator for entries in query.
 * Note: MUST be just one character. Otherwise parser must logih be updated too.
 */
#define QUERYLIB_ENTRY_SEPARATOR " "

/**
 * Separator between mode, filter and collection types and identifiers.
 * Note: MUST be just one character. Otherwise parser logic must be updated too.
 */
#define QUERYLIB_TYPE_SEPARATOR ":"

/**
 * Separator for filter lists.
 * Note: MUST be just one character. Otherwise parser logic must be updated too.
 */
#define QUERYLIB_LIST_SEPARATOR ","

/**
 * Filter prefix. Must match type separator.
 */
#define QUERYLIB_FILTER_PREFIX "f:"

/**
 * Collection prefix. Must match type separator.
 */
#define QUERYLIB_COLLECTION_PREFIX "c:"

/**
 * Selection mode prefix. Must match type separator.
 */
#define QUERYLIB_MODE_PREFIX "s:"

/**
 * Maximum lenghth of the query string buffer.
 */
#define QUERYLIB_MAX_QUERY_LEN 1024

/**
 * Maximum lenghth of the query string buffer.
 */
#define QUERYLIB_MAX_ENTRY_LEN 256

/**
 * Max length of name buffers.
 */
#define QUERYLIB_IDENTIFIER_LEN 48

/**
 * Maximum number of entries in a query string.
 */
#define QUERYLIB_MAX_ENTRIES 20


/**
 * Parses a query string for a client.
 *
 * @param client    Client index. Used for authorization in filter tests.
 * @param query     Query string.
 * @param callbacks Callback set. Create with QueryLib_CreateCallbackSet.
 * @param mode      Parser result mode.
 * @param list      Optional. List to put elements in if mode is QueryLib_List.
 * @param errCode   Output, optional. Error code.
 * @param errPos    Output, optional. Position in query string where it failed.
 *                  -1 is used if it's a global parser error.
 *
 * @return          List mode: Number of elements added to the list. On error
 *                  an error code is set in the errCode parameter.
 *
 *                  Other modes: Element index if found, or a negative number
 *                  on error (see QUERYLIB_ERR_* defines).
 */
stock QueryLib_ParseQuery(client,
                          const String:query[],
                          Handle:callbacks,
                          QueryLib_ResultMode:mode = QueryLib_First,
                          Handle:list = INVALID_HANDLE,
                          &errCode = 0,
                          &errPos = 0)
{
    new retval;     // Result buffer for helper functions.
    new element = -1;
    new filters = 0;
    new QueryLib_ResultMode:selectMode = mode;
    new bool:asList = (mode == QueryLib_List);  // How the result is _returned_.
    new bool:listSelectMode = asList;           // Whether elements are selected as list or not.
    new listCount;
    new resultCount;
    
    // Reset error variables.
    new tempErrPos = 0;
    errPos = 0;
    errCode = 0;
    
    // Validate mode.
    if (mode == QueryLib_Invalid)
    {
        errPos = -1;
        errCode = QUERYLIB_ERR_INVALID_MODE;
        return asList ? 0 : errCode;
    }
    
    // Validate callbacks.
    if (callbacks == INVALID_HANDLE)
    {
        errPos = -1;
        errCode = QUERYLIB_ERR_NO_CALLBACK;
        return asList ? 0 : errCode;
    }
    
    // Validate list.
    if (asList && list == INVALID_HANDLE)
    {
        errPos = -1;
        errCode = QUERYLIB_ERR_NO_LIST;
        return asList ? 0 : errCode;
    }
    
    // Store callbacks in an array for direct access through all query parsing
    // functions.
    new callbackSet[QueryLib_CallbackSet];
    QueryLib_ReadCallbacks(callbacks, callbackSet);     // "callbacks" is the set handle.
    
    // Do a quick check for single element queries. If the query is longer than
    // the maximum name length it's definitely not a element name.
    if (strlen(query) <= QUERYLIB_IDENTIFIER_LEN)
    {
        element = QueryLib_CallGetElementIndex(callbackSet, query);
        if (element >= 0)
        {
            if (asList)
            {
                PushArrayCell(list, element);
                return 1;
            }
            else
            {
                return element;
            }
        }
    }
    
    // Prepare filtered list.
    new Handle:resultList = asList ? list : CreateArray();
    listCount = GetArraySize(resultList);
    
    // Check if too many entries. (Simple test by counting entry separators.)
    static const String:tempSeparator[] = QUERYLIB_ENTRY_SEPARATOR;
    if (Array_CountChars(query, tempSeparator[0]) > QUERYLIB_MAX_ENTRIES - 1)
    {
        errPos = -1;
        errCode = QUERYLIB_ERR_TOO_MANY_ENTRIES;
        if (!asList) CloseHandle(list);
        return asList ? 0 : errCode;
    }
    
    // Separate entries.
    new entryCount;
    decl String:entries[QUERYLIB_MAX_ENTRIES][QUERYLIB_MAX_ENTRY_LEN];
    entryCount = ExplodeString(query, QUERYLIB_ENTRY_SEPARATOR, entries, QUERYLIB_MAX_ENTRIES, QUERYLIB_MAX_ENTRY_LEN);
    
    // Check if there are no entries.
    if (entryCount == 0)
    {
        errPos = -1;
        errCode = QUERYLIB_ERR_EMPTY;
        if (!asList) CloseHandle(list);
        return asList ? 0 : errCode;
    }
    
    // Loop through entries and parse them.
    for (new entry = 0; entry < entryCount; entry++)
    {
        // Calculate entry position in query string.
        if (entry > 0)
        {
            // Continue after last entry. +1 for the entry separator.
            errPos += strlen(entries[entry - 1]) + 1;
        }
        
        // TODO: If optimization is needed, reduce use of strlen.
        
        // Check for empty entry (double spaces).
        if (strlen(entries[entry]) == 0)
        {
            // (errPos is already set).
            errCode = QUERYLIB_ERR_EMPTY_ENTRY;
            if (!asList) CloseHandle(list);
            return asList ? resultCount : errCode;
        }
        
        // Check entry type.
        new QueryLib_EntryType:entryType = QueryLib_GetEntryType(entries[entry]);
        
        //PrintToServer("[DEBUG] Parsing entry (type id: %d) \"%s\" at position %d", entryType, entries[entry], errPos);
        
        // Parse entry.
        switch (entryType)
        {
            case QueryLibEntry_Invalid:
            {
                // (errPos is already set).
                errCode = QUERYLIB_ERR_INVALID_ENTRY;   // Entry syntax error.
                if (!asList) CloseHandle(list);
                return asList ? resultCount : errCode;
            }
            case QueryLibEntry_SelectMode:
            {
                // Parse select mode. (+2 to go past select mode prefix.)
                new QueryLib_ResultMode:tempMode = QueryLib_StringToResultMode(entries[entry][2]);
                if (tempMode == QueryLib_Invalid)
                {
                    // Invalid select mode name.
                    errPos += 2;
                    errCode = QUERYLIB_ERR_INVALID_MODE;
                    if (!asList) CloseHandle(list);
                    return asList ? resultCount : errCode;
                }
                
                // Set new select mode.
                selectMode = tempMode;
                listSelectMode = (selectMode == QueryLib_List);
            }
            case QueryLibEntry_Element:
            {
                // Get element.
                element = QueryLib_CallGetElementIndex(callbackSet, entries[entry]);
                if (element < 0)
                {
                    // Invalid element name.
                    // (errPos is already set).
                    errCode = QUERYLIB_ERR_INVALID_ELEMENT;
                    if (!asList) CloseHandle(list);
                    return asList ? resultCount : errCode;
                }
                
                // Add to result list if the client has access to the element.
                if (QueryLib_CallPassedFilter(callbackSet,
                                              client,
                                              element,
                                              0))       // No flag requirements, just do client authorization.
                {
                    PushArrayCell(resultList, element);
                    resultCount++;
                    listCount++;
                }
            }
            case QueryLibEntry_Filter:
            {
                // Parse filter list. (+2 to go past filter type prefix.)
                filters = QueryLib_ParseFilters(callbackSet, entries[entry][2], tempErrPos);
                if (filters < 0)
                {
                    // Invalid filter name.
                    errPos += tempErrPos + 2;   // +2 to go past filter type prefix.
                    errCode = QUERYLIB_ERR_INVALID_FILTER;
                    if (!asList) CloseHandle(list);
                    return asList ? resultCount : errCode;
                }
                
                // Get the source list.
                new Handle:sourceList = QueryLib_CallGetList(callbackSet);
                
                // Get filtered model list. Ignore errors, no result is a valid
                // case too.
                retval = QueryLib_GetEntries(client,
                                callbackSet,
                                sourceList,
                                filters,
                                selectMode,
                                resultList,
                                errCode);
                
                //PrintToServer("[DEBUG] Parse filter entry - Retval: %d", retval);
                
                // Only add or update counters if the result was successful.
                if (errCode == 0)
                {
                    if (!listSelectMode)
                    {
                        // Not retrieving a list, add the element manually.
                        PushArrayCell(resultList, retval);
                    }
                    
                    // Update counters.
                    QueryLib_UpdateCounters(listSelectMode, listCount, resultCount, retval);
                }
            }
            case QueryLibEntry_Collection:
            {
                // Parse collection list according to select mode, no filters.
                retval = QueryLib_ParseCollections(client,
                                callbackSet,
                                entries[entry][2],  // Collection list string.
                                0,                  // No filters.
                                selectMode,
                                resultList,
                                errCode,
                                tempErrPos);
                if (errCode < 0)
                {
                    // Invalid collection name.
                    // (errCode is already set.)
                    errPos += tempErrPos + 2;   // +2 to go past collection type prefix.
                    if (!asList) CloseHandle(list);
                    return asList ? resultCount : errCode;
                }
                
                if (!listSelectMode)
                {
                    // Not retrieving a list, add the element manually.
                    PushArrayCell(resultList, retval);
                }
                
                // Update counters.
                QueryLib_UpdateCounters(listSelectMode, listCount, resultCount, retval);
            }
            case QueryLibEntry_FilterCollection:
            {
                // Extract filter and collection list (split types).
                decl String:filterList[QUERYLIB_MAX_ENTRY_LEN];
                decl String:collectionList[QUERYLIB_MAX_ENTRY_LEN];
                filterList[0] = 0;
                collectionList[0] = 0;
                QueryLib_SplitTypes(entries[entry], filterList, collectionList, QUERYLIB_MAX_ENTRY_LEN);
                
                // Parse filter list.
                filters = QueryLib_ParseFilters(callbackSet, filterList, tempErrPos);
                if (filters < 0)
                {
                    // Invalid filter name.
                    errPos += tempErrPos + 2;   // +2 to go past filter type prefix.
                    errCode = QUERYLIB_ERR_INVALID_FILTER;
                    if (!asList) CloseHandle(list);
                    return asList ? resultCount : errCode;
                }
                
                // Parse collection list according to select mode, with filters.
                retval = QueryLib_ParseCollections(client,
                                callbackSet,
                                collectionList,
                                filters,
                                selectMode,
                                resultList,
                                errCode,
                                tempErrPos);
                if (errCode < 0)
                {
                    // Invalid collection name.
                    // (errCode is already set.)
                    errPos += tempErrPos + 2 + strlen(filterList);  // Go past filter type prefix and list.
                    
                    if (!asList) CloseHandle(list);
                    return asList ? resultCount : errCode;
                }
                
                if (!listSelectMode)
                {
                    // Not retrieving a list, add the element manually.
                    PushArrayCell(resultList, retval);
                }
                
                // Update counters.
                QueryLib_UpdateCounters(listSelectMode, listCount, resultCount, retval);
            }
        }
    }
    
    // Get the result value (element, list count or error code). This function
    // will also close the list handle if no longer used.
    return QueryLib_GetResultValue(list, listCount, resultCount, asList, selectMode, errCode);
}

/**
 * Parse a filter flag list into a flag bit field. Invalid filters are skipped.
 *
 * @param callbackSet   Set of callbacks.
 * @param filterList    List of filter flags separated by
 *                      MODEL_FILTER_SEPARATOR.
 * @param errPos        Output, optional. Position in filter list string that
 *                      failed.
 *
 * @return              Filter flags in a bit field. -1 on error (with position
 *                      in errPos.)
 */
// TODO: Make static when testing is done.
stock QueryLib_ParseFilters(callbackSet[QueryLib_CallbackSet],
                                   const String:filterList[],
                                   &errPos = 0)
{
    new filters = 0;
    errPos = 0;
    decl String:filterEntries[QUERYLIB_MAX_ENTRIES][QUERYLIB_IDENTIFIER_LEN];
    
    new count = ExplodeString(filterList, QUERYLIB_LIST_SEPARATOR, filterEntries, QUERYLIB_MAX_ENTRIES, QUERYLIB_IDENTIFIER_LEN);
    
    // Loop through filter entries.
    for (new entry = 0; entry < count; entry++)
    {
        // Calculate entry position in query string.
        if (entry > 0)
        {
            // Continue after last entry. +1 for the filter separator.
            errPos += strlen(filterEntries[entry - 1]) + 1;
        }
        
        // Get filter value.
        new filter = QueryLib_CallGetFilterValue(callbackSet, filterEntries[entry]);
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
 * Parses a collection list entry.
 *
 * @param client        Client that will use the element.
 * @param callbackSet   Set of callbacks.
 * @param sourceList    Collection list string.
 * @param filters       Filter flags.
 * @param selectMode    Optional. Selection mode. Defaults to list.
 * @param resultList    Optional. List to put element indexes in.
 * @param errCode       Output, optional. Error code.
 * @param errPos        Output, optional. Position in entry string where it
 *                      failed.
 *
 * @return              List mode: Number of elements added to the list. On
 *                      error an error code is set in the errCode parameter.
 *
 *                      Other modes: Element index if found, or a negative
 *                      number on error (see QUERYLIB_ERR_* defines).
 */
// TODO: Make static when testing is done.
stock QueryLib_ParseCollections(client,
                                       callbackSet[QueryLib_CallbackSet],
                                       const String:sourceList[],
                                       filters = 0,
                                       QueryLib_ResultMode:selectMode = QueryLib_List,
                                       Handle:resultList = INVALID_HANDLE,
                                       &errCode = 0,
                                       &errPos = 0)
{
    new bool:asList = (resultList != INVALID_HANDLE);
    new bool:listSelectMode = (selectMode == QueryLib_List);
    new sourceListCount = 0;    // Number of collections.
    new resultCount = 0;        // Number of elements added to the list.
    
    // Reset error variables.
    errCode = 0;
    errPos = 0;
    
    // Split collection list.
    decl String:collections[QUERYLIB_MAX_ENTRIES][QUERYLIB_IDENTIFIER_LEN];
    sourceListCount = ExplodeString(sourceList, QUERYLIB_LIST_SEPARATOR, collections, QUERYLIB_MAX_ENTRIES, QUERYLIB_IDENTIFIER_LEN);
    
    //PrintToServer("[DEBUG] ParseCollections - selectMode = %d", selectMode);
    //PrintToServer("[DEBUG] ParseCollections - sourceList = \"%s\"", sourceList);
    //PrintToServer("[DEBUG] ParseCollections - sourceListCount = %d", sourceListCount);
    
    // Check if no collections.
    if (sourceListCount == 0)
    {
        errCode = QUERYLIB_ERR_NONE_FOUND;
        return asList ? 0 : errCode;
    }
    
    // Create temp array.
    new Handle:list = listSelectMode ? resultList : CreateArray();
    
    // Loop through collection entries.
    for (new entry = 0; entry < sourceListCount; entry++)
    {
        // Calculate entry position in collection list.
        if (entry > 0)
        {
            // Continue after last entry. +1 for the list entry separator.
            errPos += strlen(collections[entry - 1]) + 1;
        }
        
        //PrintToServer("[DEBUG] ParseCollections - reading collection \"%s\"", collections[entry]);
        
        // Parse collection name and get list.
        new Handle:collection = QueryLib_CallGetCollection(collections[entry], callbackSet);
        if (collection != INVALID_HANDLE)
        {
            // Add elements in collection to temp array or list (those who pass
            // the filter). Ignore "none found" error because this is a valid
            // case too.
            resultCount += QueryLib_GetEntries(client, callbackSet, collection, filters, QueryLib_List, list);
        }
        else
        {
            // Invalid collection name. Error position is already updated.
            errCode = QUERYLIB_ERR_INVALID_COLLECTION;
            if (!asList) CloseHandle(list);
            return asList ? resultCount : errCode;
        }
    }
    
    new listCount = GetArraySize(list);
    
    //PrintToServer("[DEBUG] ParseCollections - %d elements in temp list.", listCount);
    
    // Get the result value (element, list count or error code). This function
    // will also close the list handle if no longer used.
    return QueryLib_GetResultValue(list, listCount, resultCount, listSelectMode, selectMode, errCode);
}

/**
 * Gets one or more elements from a source list according to the filter and
 * select mode.
 *
 * @param client        Client that will use the element.
 * @param callbackSet   Set of callbacks.
 * @param sourceList    Source list of entry indexes.
 * @param filters       Filter flags.
 * @param selectMode    Optional. Selection mode. Defaults to list.
 * @param resultList    Optional. List to put element indexes in.
 * @param errCode       Output, optional. Error code.
 *
 * @return              List mode: Number of elements added to the list. On
 *                      error an error code is set in the errCode parameter.
 *
 *                      Other modes: Element index if found, or a negative
 *                      number on error (see QUERYLIB_ERR_* defines).
 */
// TODO: Make static when testing is done.
stock QueryLib_GetEntries(client,
                                 callbackSet[QueryLib_CallbackSet],
                                 Handle:sourceList,
                                 filters = 0,
                                 QueryLib_ResultMode:selectMode = QueryLib_List,
                                 Handle:resultList = INVALID_HANDLE,
                                 errCode = 0)
{
    // How elements are returned.
    new bool:asList = (resultList != INVALID_HANDLE);
    
    // Number of elements added to list.
    new resultCount = 0;
    
    // Prepare result list. Use a temp list for non-list select modes.
    new Handle:list = (selectMode == QueryLib_List) ? resultList : CreateArray();
    
    // Reset error code.
    errCode = 0;
    
    // Loop through all entries in the source list.
    new sourceCount = GetArraySize(sourceList);
    for (new entry = 0; entry < sourceCount; entry++)
    {
        new element = GetArrayCell(sourceList, entry);
        
        // Filter test.
        if (!QueryLib_CallPassedFilter(callbackSet, client, element, filters))
        {
            // Didn't pass filter. Skip element.
            continue;
        }
        
        // Add element to result list.
        PushArrayCell(list, element);
        resultCount++;
    }
    
    // Set error code if none found. ---- redundant
    /*if (resultCount == 0)
    {
        errCode = QUERYLIB_ERR_NONE_FOUND;
    }*/
    
    // Get total number of elements in the result list.
    new listCount = GetArraySize(list);
    
    // Get the result value (element, list count or error code). This function
    // will also close the list handle if no longer used (when asList is false).
    return QueryLib_GetResultValue(list, listCount, resultCount, asList, selectMode, errCode);
}

/**
 * Gets the result value according to the specified select mode.
 *
 * Note: This function will close the list handle if asList is false.
 *
 * @param list          List with results.
 * @param listCount     Total number of elements in the result list.
 * @param resultCount   Number of elements that was added to the result list.
 * @param asList        Whether the result is added to a list or returned as a
 *                      single element. When false, the list handle will be
 *                      closed.
 * @param selectMode    Selection mode.
 * @param errCode       Output, optional. Error code.
 *
 * @return              List mode: Number of elements added to the list. On
 *                      error an error code is set in the errCode parameter.
 *
 *                      Other modes: Element index if found, or a negative
 *                      number on error (see QUERYLIB_ERR_* defines).
 */
// TODO: Make static when testing is done.
stock QueryLib_GetResultValue(&Handle:list,
                                    listCount,
                                    resultCount,
                                    bool:asList,
                                    QueryLib_ResultMode:selectMode,
                                    &errCode = 0)
{
    // Check if no elements was found.
    if (resultCount == 0)
    {
        errCode = QUERYLIB_ERR_NONE_FOUND;
        if (!asList)
        {
            // Destroy result list.
            CloseHandle(list);
            list = INVALID_HANDLE;
        }
        return asList ? 0 : errCode;
    }
    
    new retval;
    switch (selectMode)
    {
        case QueryLib_First:
        {
            // Return the first element.
            retval = GetArrayCell(list, 0);
        }
        case QueryLib_Last:
        {
            // Return the last element.
            retval = GetArrayCell(list, listCount - 1);
        }
        case QueryLib_Random:
        {
            // Return a random element.
            new randIndex = Util_GetRandomInt(0, listCount - 1);
            retval = GetArrayCell(list, randIndex);
        }
        case QueryLib_List:
        {
            // Return number of elements added to the list.
            retval = resultCount;
        }
        default:
        {
            // Invalid mode. It should never reach this point. Some elements
            // still might have been added to the list, so return how many, if
            // returning a list.
            errCode = QUERYLIB_ERR_INVALID_MODE;
            retval = asList ? resultCount : errCode;
        }
    }
    
    if (!asList)
    {
        // Destroy result list.
        CloseHandle(list);
        list = INVALID_HANDLE;
    }
    
    return retval;
}

/**
 * Updates the list counters.
 *
 * @param listSelectMode    Whether the result is a list or a single element.
 * @param listCount         Total element counter.
 * @param resultCount       Elements added to the list.
 * @param retval            Result value from a element parser helper function.
 *                          Ignored in list mode.
 *                          Note: Only successful result values, no error codes.
 */
// TODO: Make static when testing is done.
stock QueryLib_UpdateCounters(bool:listSelectMode, &listCount, &resultCount, retval)
{
    if (listSelectMode)
    {
        resultCount += retval;
        listCount += retval;
    }
    else
    {
        resultCount++;
        listCount++;
    }
}

/**
 * Splits a entry with a filter and collection list into two lists.
 *
 * @param entry             The entry string to split. Only works with entries
 *                          of type QueryLibEntry_FilterCollection.
 * @param filterList        Filter list buffer.
 * @param collectionList    Collection list buffer.
 * @param maxlen            Size of buffers (used for both buffers).
 */
// TODO: Make static when testing is done.
stock QueryLib_SplitTypes(const String:entry[],
                                 String:filterList[],
                                 String:collectionList[],
                                 maxlen)
{
    // Only 3 filelds required. (f:filters:collections -> 1:2:3)
    decl String:splitBuffer[3][QUERYLIB_MAX_ENTRY_LEN];
    new count = ExplodeString(entry, QUERYLIB_TYPE_SEPARATOR, splitBuffer, 3, QUERYLIB_MAX_ENTRY_LEN);
    
    if (count != 3)
    {
        // Invalid entry.
        return;
    }
    
    // Write lists.
    strcopy(filterList, maxlen, splitBuffer[1]);
    strcopy(collectionList, maxlen, splitBuffer[2]);
}

/**
 * Creates a set of function callback handles. This set may be reused through
 * different queries.
 *
 * @return      Handle to set. Must be closed with CloseHandle() when no longer
 *              in use.
 */
stock Handle:QueryLib_CreateCallbackSet(QueryLib_GetElementList:elementList,
                                        QueryLib_GetElementIndex:elementIndex,
                                        QueryLib_GetFilterValue:filterValue,
                                        QueryLib_PassedFilter:passedFilter,
                                        QueryLib_GetCollection:getCollection)
{
    static const String:errMsg[] = "Invalid function name for %s";
    
    // Validate functions (to prevent caller passing INVALID_FUNCTION
    // intentionally).
    if (elementList == INVALID_FUNCTION)
    {
        ThrowError(errMsg, "QueryLib_GetElementList");
    }
    if (elementIndex == INVALID_FUNCTION)
    {
        ThrowError(errMsg, "QueryLib_GetElementIndex");
    }
    if (filterValue == INVALID_FUNCTION)
    {
        ThrowError(errMsg, "QueryLib_GetFilterValue");
    }
    if (passedFilter == INVALID_FUNCTION)
    {
        ThrowError(errMsg, "QueryLib_PassedFilter");
    }
    if (getCollection == INVALID_FUNCTION)
    {
        ThrowError(errMsg, "QueryLib_GetCollection");
    }
    
    // Build set.
    new Handle:set = CreateDataPack();
    WritePackCell(set, _:elementList);
    WritePackCell(set, _:elementIndex);
    WritePackCell(set, _:filterValue);
    WritePackCell(set, _:passedFilter);
    WritePackCell(set, _:getCollection);
    
    return set;
}

// TODO: Make static when testing is done.
stock QueryLib_EntryType:QueryLib_GetEntryType(const String:entry[])
{
    // Count number of types (no. of ":").
    new typeCount = Array_CountCharsEx(entry, QUERYLIB_TYPE_SEPARATOR);
    
    // TODO: Optimize repeated StrContains into one scan operation if required.
    
    if (typeCount > 2)
    {
        // No entries with more than two type separators are valid.
        return QueryLibEntry_Invalid;
    }
    else if (typeCount == 0)
    {
        // No type specified. Assume it's a element name.
        return QueryLibEntry_Element;
    }
    else if (typeCount == 2)
    {
        // Filtered collection.
        // Note: This must be tested before regular filters.
        return QueryLibEntry_FilterCollection;
    }
    else if (StrContains(entry, QUERYLIB_COLLECTION_PREFIX) >= 0)
    {
        return QueryLibEntry_Collection;
    }
    else if (StrContains(entry, QUERYLIB_FILTER_PREFIX) >= 0)
    {
        return QueryLibEntry_Filter;
    }
    else if (StrContains(entry, QUERYLIB_MODE_PREFIX) >= 0)
    {
        return QueryLibEntry_SelectMode;
    }
    
    return QueryLibEntry_Invalid;
}

/**
 * Reads a callback set into an array.
 *
 * @param set           Set to read.
 * @param setBuffer     Destination set buffer.
 */
static stock QueryLib_ReadCallbacks(Handle:set, setBuffer[QueryLib_CallbackSet])
{
    ResetPack(set);
    setBuffer[QueryLibCall_ElementList] = Function:ReadPackCell(set);
    setBuffer[QueryLibCall_ElementIndex] = Function:ReadPackCell(set);
    setBuffer[QueryLibCall_FilterValue] = Function:ReadPackCell(set);
    setBuffer[QueryLibCall_PassedFilter] = Function:ReadPackCell(set);
    setBuffer[QueryLibCall_GetCollection] = Function:ReadPackCell(set);
}

/**
 * Makes a call to QueryLib_GetElementList and returns the result.
 */
static stock Handle:QueryLib_CallGetList(callbackSet[QueryLib_CallbackSet])
{
    new Handle:list;
    new Handle:plugin = GetMyHandle();
    Call_StartFunction(plugin, callbackSet[QueryLibCall_ElementList]);
    Call_Finish(list);
    
    return list;
}

/**
 * Makes a call to QueryLib_GetElementIndex and returns the result.
 */
static stock QueryLib_CallGetElementIndex(callbackSet[QueryLib_CallbackSet], const String:name[])
{
    new element;
    new Handle:plugin = GetMyHandle();
    Call_StartFunction(plugin, callbackSet[QueryLibCall_ElementIndex]);
    Call_PushString(name);
    Call_Finish(element);
    
    return element;
}

/**
 * Makes a call to QueryLib_GetFilterValue and returns the result.
 */
static stock QueryLib_CallGetFilterValue(callbackSet[QueryLib_CallbackSet], const String:name[])
{
    new filter;
    new Handle:plugin = GetMyHandle();
    Call_StartFunction(plugin, callbackSet[QueryLibCall_FilterValue]);
    Call_PushString(name);
    Call_Finish(filter);
    
    return filter;
}

/**
 * Makes a call to QueryLib_GetPassFilter and returns the result.
 */
static stock bool:QueryLib_CallPassedFilter(callbackSet[QueryLib_CallbackSet], client, element, filter)
{
    new bool:passed;
    new Handle:plugin = GetMyHandle();
    Call_StartFunction(plugin, callbackSet[QueryLibCall_PassedFilter]);
    Call_PushCell(client);
    Call_PushCell(element);
    Call_PushCell(filter);
    Call_Finish(passed);
    
    return passed;
}

/**
 * Makes a call to QueryLib_GetCollection and returns the result.
 */
static stock Handle:QueryLib_CallGetCollection(const String:name[], callbackSet[QueryLib_CallbackSet])
{
    new Handle:collection;
    new Handle:plugin = GetMyHandle();
    Call_StartFunction(plugin, callbackSet[QueryLibCall_GetCollection]);
    Call_PushString(name);
    Call_Finish(collection);
    
    return collection;
}


/****************************
 *   Conversion functions   *
 ****************************/

/**
 * Converts a mode name into a parser result mode value.
 *
 * @param mode      Mode name to convert.
 *
 * @return          Parser result mode, or QueryLib_Invalid if failed.
 */
public QueryLib_ResultMode:QueryLib_StringToResultMode(const String:mode[])
{
    if (StrEqual(mode, "first", false))
    {
        return QueryLib_First;
    }
    else if (StrEqual(mode, "last", false))
    {
        return QueryLib_Last;
    }
    else if (StrEqual(mode, "random", false))
    {
        return QueryLib_Random;
    }
    else if (StrEqual(mode, "list", false))
    {
        return QueryLib_List;
    }
    
    return QueryLib_Invalid;
}

/**
 * Converts a error code to a user friendly message.
 *
 * @param errCode   Error code to convert
 * @param buffer    Destination string buffer.
 * @param maxlen    Size of buffer.
 *
 * @return          Number of cells written.
 */
public QueryLib_ErrCodeToString(errCode, String:buffer[], maxlen)
{
    switch (errCode)
    {
        case QUERYLIB_ERR_EMPTY:
        {
            return strcopy(buffer, maxlen, "Query string is empty");
        }
        case QUERYLIB_ERR_NONE_FOUND:
        {
            return strcopy(buffer, maxlen, "No element found");
        }
        case QUERYLIB_ERR_INVALID_FILTER:
        {
            return strcopy(buffer, maxlen, "Invalid filter name");
        }
        case QUERYLIB_ERR_INVALID_COLLECTION:
        {
            return strcopy(buffer, maxlen, "Invalid collection name");
        }
        case QUERYLIB_ERR_INVALID_ELEMENT:
        {
            return strcopy(buffer, maxlen, "Invalid element name");
        }
        case QUERYLIB_ERR_INVALID_ENTRY:
        {
            return strcopy(buffer, maxlen, "Invalid entry (syntax error)");
        }
        case QUERYLIB_ERR_INVALID_MODE:
        {
            return strcopy(buffer, maxlen, "Invalid select mode");
        }
        case QUERYLIB_ERR_EMPTY_ENTRY:
        {
            return strcopy(buffer, maxlen, "Empty entry (double spaces)");
        }
        case QUERYLIB_ERR_TOO_MANY_ENTRIES:
        {
            return Format(buffer, maxlen, "Too many entries (max %d)", QUERYLIB_MAX_ENTRIES);
        }
        case QUERYLIB_ERR_NO_CALLBACK:
        {
            return Format(buffer, maxlen, "No callbacks specified");
        }
        case QUERYLIB_ERR_NO_LIST:
        {
            return Format(buffer, maxlen, "No result list specified");
        }
    }
    
    return 0;
}
