/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          functionlib.inc
 *  Type:          Library
 *  Description:   Function-related API.
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

#if defined _functionlib_included
 #endinput
#endif
#define _functionlib_included

/**
 * Stores the trie handle to the function cache.
 */
new Handle:g_trieFuncCache;

/**
 * Checks if a function is available, returning a function ID if so.
 * The return value is cached for future calls.
 * 
 * @param functionname  The function name to check if available.
 * @param altcache      An alternate trie handle to use as the cache.
 * 
 * @return              Function ID if the function is available, INVALID_FUNCTION if it doesn't exist.
 */
stock Function:FuncLib_IsFunctionAvailable(const String:functionname[], Handle:altcache = INVALID_HANDLE)
{
    static bool:cachecreated = false;
    if (!cachecreated)
    {
        g_trieFuncCache = CreateTrie();
        cachecreated = true;
    }
    
    // Check if we should use the alternate cache.
    new Handle:cache = g_trieFuncCache;
    if (altcache != INVALID_HANDLE)
        cache = altcache;
    
    new Function:func;
    if (GetTrieValue(cache, functionname, func))
        return func;
    
    // Get function ID.
    func = GetFunctionByName(GetMyHandle(), functionname);
    if (func == INVALID_FUNCTION)
        return INVALID_FUNCTION;
    
    // Add to cache.
    SetTrieValue(cache, functionname, func);
    return func;
}
