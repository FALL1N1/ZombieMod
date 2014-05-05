/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          classfilter.inc
 *  Type:          Module include
 *  Description:   Class manager filtering and list tools.
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

/**
 * Gets the first class index of a class with the specified name (not a case
 * sensitive search), or index (converted from string).
 *
 * @param name      The name or index to search for.
 * @param cache     Optional. Specifies what class cache to read from. If player
 *                  cache is used 'index' will be used as a client index.
 *                  Default is modified cache.
 *
 * @return          The class index if successful. On errors:
 *                  -1: no classes
 *                  -2: no name specified
 *                  -3: invalid class index specified (converted from string)
 *                  -4: invalid class name
 */
stock ClassMgr_GetClass(const String:name[], ClassCacheType:cache = ClassCache_Modified)
{
    // Check if there are no classes.
    if (g_ClassCount == 0)
    {
        return -1;
    }
    
    // Validate name.
    if (strlen(name) == 0)
    {
        return -2;
    }
    
    decl String:currentName[CLASS_NAME_LEN];
    currentName[0] = 0;
    new classIndex;
    
    // Check if it's a class index.
    if (IsCharNumeric(name[0]))
    {
        classIndex = StringToInt(name);
        if (ClassMgr_IsValidIndex(classIndex))
        {
            return classIndex;
        }
        else
        {
            return -3;
        }
    }
    
    // Lookup class name in trie index.
    if (GetTrieValue(g_hClassNameIndex, name, classIndex))
    {
        return classIndex;
    }
    
    // The class name wasn't found in the index.
    return -4;
}

/**
 * Check if a class pass the specified flag filters.
 * 
 * @param index         Index of the class in a class cache or a client index,
 *                      depending on the cache type specified.
 * @param require       Class flags to require. 0 for no filter.
 * @param deny          Class flags to exclude. 0 for no filter.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              True if passed, false otherwise.
 */
stock bool:ClassMgr_FlagFilterMatch(index, require, deny, ClassCacheType:cache)
{
    // Do quick check for optimization reasons: Check if no flags are specified.
    if (require == 0 && deny == 0)
    {
        return true;
    }
    
    new flags = ClsGeneric_GetClassFlags(index, cache);
    new bool:requirePassed = false;
    new bool:denyPassed = false;
    
    // Match require filter.
    if (require == 0 || flags & require)
    {
        // All required flags are set.
        requirePassed = true;
    }
    
    // Match deny filter.
    if (deny == 0 || !(flags & deny))
    {
        // No denied flags are set.
        denyPassed = true;
    }
    
    // Check if required and denied flags passed the filter.
    if (requirePassed && denyPassed)
    {
        // The class pass the filter.
        return true;
    }
    
    // The class didn't pass the filter.
    return false;
}

/**
 * Check if a class pass the specified filter.
 * 
 * @param index         Index of the class in a class cache or a client index,
 *                      depending on the cache type specified.
 * @param filter        Structure with filter settings.
 * @param cache         Optional. Specifies what class cache to read from. If
 *                      player cache is used 'index' will be used as a client
 *                      index. Default is modified cache.
 *
 * @return              True if passed, false otherwise.
 */
stock bool:ClassMgr_FilterMatch(index, filter[ClassFilter], ClassCacheType:cache = ClassCache_Modified)
{
    // Check if the class is disabled and the enabled attribute is NOT ignored.
    if (!filter[ClassFilter_IgnoreEnabled] && !ClsGeneric_IsEnabled(index, cache))
    {
        return false;
    }
    
    // Check if class flags pass the flag filter.
    if (!ClassMgr_FlagFilterMatch(index, filter[ClassFilter_RequireFlags], filter[ClassFilter_DenyFlags], cache))
    {
        return false;
    }
    
    new client = filter[ClassFilter_Client];
    
    // Get authorization mode
    new ClsGeneric_AuthModes:authMode = ClsGeneric_GetAuthMode(index, cache);
    
    // Check if classes with authorization should be excluded.
    if (client < 0 && authMode != ClsGeneric_Disabled)
    {
        return false;
    }
    
    // Check if client must be authorized.
    // Note: Client 0 (the server) always have access.
    if (client > 0)
    {
        if (authMode != ClsGeneric_Disabled)
        {
            decl String:groups[CLASS_STRING_LEN];
            decl String:flags[CLASS_STRING_LEN];
            groups[0] = 0;
            flags[0] = 0;
            
            // Get lists.
            ClsGeneric_GetGroups(index, groups, sizeof(groups), cache);
            ClsGeneric_GetFlags(index, groups, sizeof(groups), cache);
            
            new Auth_Modes:libMode = ClassMgr_ClsAuthModeToAuthMode(authMode);
            return Auth_IsClientAuthorized(client, groups, flags, CLASS_LIST_SEPARATOR, libMode, false);
        }
    }
    
    // The class passed the filter.
    return true;
}

/**
 * Gets all class indexes or from a specified team, and adds them to the
 * specified array.
 * 
 * @param array         The destination ADT array to add class indexes to.
 * @param team          Optional. Team to match. Default is all.
 * @param filter        Optional. Structure with filter settings.
 * @param cache         Optional. Specifies what class cache to read from. If
 *                      player cache is used 'index' will be used as a client
 *                      index. Default is modified cache.
 *
 * @return              Number of classes added.
 */
stock ClassMgr_GetClasses(Handle:array, ClassTeam:team = ClassTeam_All, filter[ClassFilter] = g_ClassNoFilter, ClassCacheType:cache = ClassCache_Modified)
{
    // Validate the array.
    if (array == INVALID_HANDLE)
    {
        return 0;
    }
    
    // Check if there are no classes.
    if (g_ClassCount == 0)
    {
        return 0;
    }
    
    // Store a local boolean that says if the user specified a team filter or not.
    new bool:hasTeamFilter = (team != ClassTeam_All);
    new count;
    
    // Loop through all classes.
    for (new classIndex = 0; classIndex < g_ClassCount; classIndex++)
    {
        // Validate filter settings.
        if (!ClassMgr_FilterMatch(classIndex, filter, cache))
        {
            // The class didn't pass the filter, skip class.
            continue;
        }
        
        // Check team filtering.
        if (hasTeamFilter)
        {
            // Only add classes with matching team ID.
            if (ClassMgr_TeamEqual(classIndex, team, cache))
            {
                // Team ID match. Add class index to array.
                PushArrayCell(array, classIndex);
                count++;
            }
        }
        else
        {
            // No filter. Add any class to the array.
            PushArrayCell(array, classIndex);
            count++;
        }
    }
    
    return count;
}

/**
 * Counts number of classes according to the filter settings. Default is no filter.
 *
 * @param team      Optional. Team to count. Default is all.
 * @param filter    Optional. Structure with filter settings.
 * @param cache     Optional. Specifies what class cache to read from. If
 *                  player cache is used 'index' will be used as a client
 *                  index. Default is modified cache.
 *
 * @return          Number of classes found.
 */
stock ClassMgr_Count(ClassTeam:team = ClassTeam_All, filter[ClassFilter] = g_ClassNoFilter, ClassCacheType:cache = ClassCache_Modified)
{
    // Check if there are no classes.
    if (g_ClassCount == 0)
    {
        return 0;
    }
    
    new Handle:array = CreateArray();
    new count = ClassMgr_GetClasses(array, team, filter, cache);
    CloseHandle(array);
    
    return count;
}

/**
 * Gets a random class index from a specified team or from all classes.
 *
 * @param team      Optional. Team to filter. Default is all.
 * @param filter    Optional. Structure with filter settings.
 * @param cache     Optional. Specifies what class cache to read from. If
 *                  player cache is used 'index' will be used as a client
 *                  index. Default is modified cache.
 *
 * @return          The class index if successful, or -1 on error.
 */
stock ClassMgr_GetRandomClass(ClassTeam:team = ClassTeam_All, filter[ClassFilter] = g_ClassNoSpecialClasses, ClassCacheType:cache = ClassCache_Modified)
{
    new Handle:array;
    new count;
    new randNum;
    new classIndex;
    
    // Get a class list according to the filter settings.
    array = CreateArray();
    count = ClassMgr_GetClasses(array, team, filter, cache);
    
    // Check if classes were found.
    if (count)
    {
        // Get a random index from the new class array.
        randNum = GetRandomInt(0, count - 1);
        
        // Return the value at the random index.
        classIndex = GetArrayCell(array, randNum);
        CloseHandle(array);
        return classIndex;
    }
    else
    {
        // Failed to get a random class.
        CloseHandle(array);
        return -1;
    }
}

/**
 * Gets the first class index, or the first class index with the specified team
 * ID.
 *
 * @param team      Optional. Team to filter. Default is all.
 * @param filter    Optional. Structure with filter settings.
 * @param cache     Optional. Specifies what class cache to read from. If
 *                  player cache is used 'index' will be used as a client
 *                  index. Default is modified cache.
 *
 * @return          Class index. On errors:
 *                  -1: no classes,
 *                  -2: no matching class found
 */
stock ClassMgr_GetFirstClass(ClassTeam:team = ClassTeam_All, filter[ClassFilter] = g_ClassNoSpecialClasses, ClassCacheType:cache = ClassCache_Modified)
{
    // Check if there are no classes.
    if (g_ClassCount == 0)
    {
        return -1;
    }
    
    new bool:hasTeamFilter = (team != ClassTeam_All);
    
    // Loop through all classes.
    for (new classIndex = 0; classIndex < g_ClassCount; classIndex++)
    {
        // Validate filter settings.
        if (!ClassMgr_FilterMatch(classIndex, filter, cache))
        {
            // The class is didn't pass the filter, skip class.
            continue;
        }
        
        // Check team filtering.
        if (hasTeamFilter)
        {
            if (ClassMgr_TeamEqual(classIndex, team, cache))
            {
                // Team match. Return the class index.
                return classIndex;
            }
        }
        else
        {
            // No team filter. Return the class index.
            return classIndex;
        }
    }
    
    return -2;
}

/**
 * Gets the first class marked as default for the specified team.
 *
 * @param team      Team to get default class for.
 * @param filter    Optional. Structure with filter settings. Default is to
 *                  deny classes with special flags (ZR_CLASS_SPECIALFLAGS).
 * @param cache     Optional. Specifies what class cache to read from. If
 *                  player cache is used 'index' will be used as a client
 *                  index. Default is modified cache.
 *
 * @return          Class index. On errors:
 *                  -1: no classes found,
 *                  -2: no default class found
 */
stock ClassMgr_GetDefaultClass(ClassTeam:team, filter[ClassFilter] = g_ClassNoSpecialClasses, ClassCacheType:cache = ClassCache_Modified)
{
    new Handle:array;
    new count;
    new classIndex;
    
    // Get all classes from the specified team.
    array = CreateArray();
    count = ClassMgr_GetClasses(array, team, filter, cache);
    
    // Check if failed.
    if (!count)
    {
        CloseHandle(array);
        return -1;
    }
    
    // Loop through all classes and return the first class marked as team default.
    for (new i = 0; i < count; i++)
    {
        // Get class index from the array.
        classIndex = GetArrayCell(array, i);
        
        // Check if the current class is marked as team default.
        if (ClsGeneric_GetTeamDefault(classIndex, cache))
        {
            // Default class found.
            CloseHandle(array);
            return classIndex;
        }
    }
    
    CloseHandle(array);
    return -2;
}

/**
 * Gets the default class index for the specified team configured to be used
 * when players join the server.
 *
 * @param team      Team to get default spawn class for.
 * @param filter    Optional. Structure with filter settings. Default is to
 *                  deny classes with special flags (ZR_CLASS_SPECIALFLAGS).
 * @param cache     Optional. Specifies what class cache to read from. If
 *                  player cache is used 'index' will be used as a client
 *                  index. Default is modified cache.
 *
 * @return          Class index. On errors it will try to fall back on the first
 *                  class in the team. On critical errors:
 *                  -1: invalid team,
 *                  -2: unable to find random class,
 *                  -3: unable to fall back on another class
 */
stock ClassMgr_GetDefaultSpawnClass(ClassTeam:team, filter[ClassFilter] = g_ClassNoSpecialClasses, ClassCacheType:cache = ClassCache_Modified)
{
    decl String:className[CLASS_NAME_LEN];
    className[0] = 0;
    new classIndex;
    
    // Get the default class configured for the specified team.
    switch (team)
    {
        case ClassTeam_Zombies:
        {
            GetConVarString(g_hCvarClassesDefaultZombie, className, sizeof(className));
        }
        case ClassTeam_Humans:
        {
            GetConVarString(g_hCvarClassesDefaultHuman, className, sizeof(className));
        }
        default:
        {
            // Invalid team.
            return -1;
        }
    }
    
    // Check if a class was specified.
    if (strlen(className) > 0)
    {
        // Check if the user configured a random default class.
        if (StrEqual(className, "random", false))
        {
            // Get a list of all classes with the specified team ID. Deny
            // classes with special flags.
            classIndex = ClassMgr_GetRandomClass(team, filter, cache);
            
            // Validate the result, in case there were errors.
            if (ClassMgr_IsValidIndex(classIndex))
            {
                return classIndex;
            }
            else
            {
                // Invalid index. The ClassMgr_GetRandomClass function is pretty
                // failsafe. So if we can't get a class index here, it's a
                // critical error. No reason to fall back on other solutions.
                return -2;
            }
        }
        else
        {
            // The user set a spesific class.
            
            // Try to get the class index with the specified class name.
            classIndex = ClassMgr_GetClass(className, cache);
            
            // Validate the class.
            if (ClassMgr_IsValidIndex(classIndex) && ClassMgr_TeamEqual(classIndex, team, cache))
            {
                return classIndex;
            }
            else
            {
                // The class index is invalid or the teams didn't match.
                // Because it's user input, we'll fall back to the first class
                // in the specified team, and log a warning.
                classIndex = ClassMgr_GetFirstClass(team, filter, cache);
                
                decl String:teamString[32];
                teamString[0] = 0;
                ClassMgr_TeamToString(team, teamString, sizeof(teamString));
                
                LogMgr_Print(ClassMgr_GetIdentifier(), LogType_Error, "Default Spawn Class", "Warning: Failed to set \"%s\" as default class for team \"%s\". The class doesn't exist or the teams doesn't match. Falling back to the first available class in the team. Check spelling in <prefix>_classes_default_* cvars.", className, teamString);
                
                // Validate the new index.
                if (ClassMgr_IsValidIndex(classIndex))
                {
                    // Log a warning.
                    return classIndex;
                }
                else
                {
                    // Something went wrong. This is a critical error. There's
                    // probably missing classes with no special flags set.
                    return -3;
                }
            }
        }
    }
    else
    {
        // Blank class name, get the default class and return the index.
        return ClassMgr_GetDefaultClass(team, filter, cache);
    }
}
