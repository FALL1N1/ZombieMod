/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          authcachelib.inc
 *  Type:          Library
 *  Description:   API to store auth strings of players in a cache.
 *
 *  Copyright (C) 2009-2011  Greyscale
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
 * This library should be used for managing a list of players identified by their auth string. (steamid).
 * Normally client indexes would suffice, but this uses auth strings to track players that disconnect
 * and come back.
 *  
 * Example Application:  ZSpawn allows new players to spawn into a game late.  With standard
 * methods, a dead player can rejoin the game and be seen as "new" and spawn, when they already
 * played this round.  So ZSpawn can create lists of players that will stop rejoining players
 * from exploiting.
 */

#if defined _authcachelib_included
 #endinput
#endif
#define _authcachelib_included

/**
 * Creates a cache to store clients.
 * 
 * @return  Handle to new cache.
 */
stock Handle:AuthCache_Create()
{
    // Create an array of blocks big enough to store auth IDs.
    return CreateArray(16);
}

/**
 * Clear a cache.
 * 
 * @param cache     The cache to clear.
 */
stock AuthCache_Clear(Handle:cache)
{
    // Clear array.
    ClearArray(cache);
}

/**
 * Add a client to a cache.
 * 
 * @param cache     The cache handle returned by AuthCache_Create.
 * @param client    The client to add.
 * 
 * @return          True if added, false if the client is already in the cache.
 */
stock bool:AuthCache_AddClient(Handle:cache, client)
{
    // Check if client is in the cache.
    if (AuthCache_ClientExists(cache, client))
        return false;
    
    // Get client's auth string.
    decl String:authstring[16];
    GetClientAuthString(client, authstring, sizeof(authstring));
    
    PushArrayString(cache, authstring);
    return true;
}

/**
 * Remove a client from a cache.
 * 
 * @param cache     The cache handle returned by AuthCache_Create.
 * @param client    The client to remove.
 * 
 * @return          True if removed, false if the client doesn't exist in the cache.
 */
stock bool:AuthCache_RemoveClient(Handle:cache, client)
{
    // Get client's auth string.
    decl String:authstring[16];
    GetClientAuthString(client, authstring, sizeof(authstring));
    
    // Check if client is in the cache.
    new index = FindStringInArray(cache, authstring);
    if (index == -1)
        return false;
    
    RemoveFromArray(cache, index);
    return true;
}

/**
 * Check if a client is in a cache.
 * 
 * @param cache     The cache handle returned by AuthCache_Create.
 * @param client    The client to check existence of.
 * 
 * @return          True if the client exists, false otherwise.
 */
stock bool:AuthCache_ClientExists(Handle:cache, client)
{
    // Get client's SteamID.
    decl String:authstring[16];
    GetClientAuthString(client, authstring, sizeof(authstring));
    
    return (FindStringInArray(cache, authstring) != -1);
}
