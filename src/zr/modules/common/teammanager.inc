/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:           teammanager.inc
 *  Type:           Module
 *  Description:    Manages virtual teams. This module does nothing by itself.
 *                  Core modules will manage teams through this module. Other
 *                  modules may use functions to get current team, etc.
 *
 *  Copyright (C) 2009-2013  Greyscale, Richard Helgeby
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
 * This module's identifier.
 */
new Module:g_moduleTeamManager;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:TeamMgr_GetIdentifier() { return g_moduleTeamManager; }

/**
 * Maximum number of teams. This is 4 in all mods we support.
 */
#define TEAM_MANAGER_MAX_TEAMS  4

/**
 * Cache for mod specific team index to VTeam mapping.
 *
 * The name is like this on purpose, so the statement will be verbose:
 *      new VTeam:team = TeamMgr_VTeamForIndex[3];
 */
new VTeam:TeamMgr_VTeamForIndex[TEAM_MANAGER_MAX_TEAMS] = {VTeam_Invalid, ...};

/**
 * Cache for VTeam to mod specific team index.
 */
new TeamMgr_IndexForVTeam[VTeam] = {-1, ...};

/**
 * Current client team.
 */
new VTeam:TeamMgr_ClientTeam[MAXPLAYERS];

/*____________________________________________________________________________*/

/**
 * Register this module.
 */
TeamMgr_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Team manager");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "teammanager");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manages virtual teams");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleTeamManager = ModuleMgr_Register(moduledata);
    
    // Create events.
    // (None)
    
    // Register the OnEventsRegister event to register all events in it.
    //EventMgr_RegisterEvent(g_moduleTeamManager, "Event_OnEventsRegister", "TeamMgr_OnEventsRegister");
}

/*____________________________________________________________________________*/

/**
 * Register all events here.
 */
/*public TeamMgr_OnEventsRegister()
{
    // Register all the events needed for this module.
    // (None)
}*/

/*____________________________________________________________________________*/

/**
 * Plugin is loading.
 *
 * Once enabled, this module is supposed to stay enabled.
 */
TeamMgr_OnPluginStart()
{
    // Register the module.
    TeamMgr_Register();
}

/*____________________________________________________________________________*/

/**
 * Gets the client's team.
 *
 * @param client        Client index.
 *
 * @param               Current client team.
 */
VTeam:TeamMgr_GetClientTeam(client)
{
    return TeamMgr_ClientTeam[client];
}

/*____________________________________________________________________________*/

/**
 * Sets the client's team.
 *
 * @param client        Client index.
 * @param team          Team to set.
 */
TeamMgr_SetClientTeam(client, VTeam:team)
{
    TeamMgr_ClientTeam[client] = team;
}

/*____________________________________________________________________________*/

/**
 * Converts mod specific team index to a cached virtual team.
 *
 * @param teamIndex     Team index to convert.
 *
 * @return              Cached virtual team, or VTeam_Invalid if invalid.
 */
stock VTeam:TeamMgr_IndexToTeam(teamIndex)
{
    return TeamMgr_VTeamForIndex[teamIndex];
}

/*____________________________________________________________________________*/

/**
 * Converts a virtual team to a cached mod specific team index.
 *
 * @param team          Virtual team to convert.
 *
 * @return              Cached team index, or -1 on error.
 */
stock TeamMgr_TeamToIndex(VTeam:team)
{
    return TeamMgr_IndexForVTeam[team];
}

/*____________________________________________________________________________*/

/**
 * Returns whether the client has not yet been assigned to a team.
 *
 * @param client        Client index.
 *
 * @return              True if unassigned, false otherwise.
 */
stock bool:TeamMgr_IsClientUnassigned(client)
{
    return TeamMgr_ClientTeam[client] == VTeam_Unassigned;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the client is on the spectator team.
 *
 * @param client        Client index.
 *
 * @return              True if spectator, false otherwise.
 */
stock bool:TeamMgr_IsClientSpectator(client)
{
    return TeamMgr_ClientTeam[client] == VTeam_Spectator;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the client is on the zombie team.
 *
 * @param client        Client index.
 *
 * @return              True if zombie, false otherwise.
 */
bool:TeamMgr_IsClientZombie(client)
{
    return TeamMgr_ClientTeam[client] == VTeam_Zombie;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the client is on the human team.
 *
 * @param client        Client index.
 *
 * @return              True if human, false otherwise.
 */
bool:TeamMgr_IsClientHuman(client)
{
    return TeamMgr_ClientTeam[client] == VTeam_Human;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the client is on the ghost team.
 *
 * @param client        Client index.
 *
 * @return              True if ghost, false otherwise.
 */
stock bool:TeamMgr_IsClientGhost(client)
{
    return TeamMgr_ClientTeam[client] == VTeam_Ghost;
}

/*____________________________________________________________________________*/

/**
 * Returns whether client is playing.
 * 
 * @param client    The client index.
 * 
 * @return          True if client is a zombie or human, false otherwise.
 */
bool:TeamMgr_IsClientOnPlayerTeam(client)
{
    return (TeamMgr_IsClientZombie(client) || TeamMgr_IsClientHuman(client));
}

/*____________________________________________________________________________*/

/**
 * Counts clients on a specific virtual team.
 * 
 * @param team      The team to count clients of.
 * @param alive     (Optional) Only count client if they're alive. Default is
 *                  false
 * 
 * @return          The number of clients on the specified team.  
 */
TeamMgr_CountTeam(VTeam:team, bool:alive = false)
{
    new count;
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
        {
            continue;
        }
        
        // Skip dead players if they must be alive.
        if (alive && !IsPlayerAlive(client))
        {
            continue;
        }
        
        if (team == TeamMgr_ClientTeam[client])
        {
            count++;
        }
    }
    
    return count;
}

/*____________________________________________________________________________*/

/**
 * Moves client to the matching mod specific team it's bound to.
 * 
 * @param client    The client index.
 * @param silent    (Optional) Don't throw error if team is invalid. Useful when
 *                  cleaning up after a client disconnected. Default is false.
 */
TeamMgr_MoveClientToTeam(client, bool:silent = false)
{
    new VTeam:team = TeamMgr_GetClientTeam(client);
    
    // Validate if enabled.
    if (!silent && (team == VTeam_Invalid || team == VTeam_Unassigned))
    {
        ThrowError("Invalid virtual team ID for client %d: %d", client, team);
        return;
    }
    
    IGame_SwitchTeam(client, team);
}

/*____________________________________________________________________________*/

/**
 * Moves all clients to the matching mod specific team they're bound to.
 *
 * @param silent    (Optional) Don't throw error if team is invalid. Useful when
 *                  cleaning up after a client disconnected. Default is true.
 */
TeamMgr_UpdateClientTeams(bool:silent = true)
{
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
        {
            continue;
        }
        
        TeamMgr_MoveClientToTeam(client, silent);
    }
}

/*____________________________________________________________________________*/

/**
 * Updates all connected clients' teams to the specified team.
 *
 * @param team              New team to set on clients.
 * @param playingTeamOnly   (Optional) Only update clients who are on a playing
 *                          team (zombies/humans). Default is true.
 */
stock TeamMgr_SetClientTeams(VTeam:team, bool:playingTeamOnly = true)
{
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
        {
            continue;
        }
        
        new VTeam:currentTeam = TeamMgr_GetClientTeam(client);
        
        // Skip players not on a playing team if enabled.
        if (playingTeamOnly && !IGame_IsPlayerTeam(currentTeam))
        {
            continue;
        }
        
        // Set new team.
        TeamMgr_SetClientTeam(client, team);
    }
}

/*____________________________________________________________________________*/

/**
 * Spawns a client on a given virtual team.
 * Note: This also sets the client's virtual team to the given team.
 * 
 * @param client    The client to spawn.
 * @param team      Virtual team to spawn on.
 */   
TeamMgr_SpawnOnTeam(client, VTeam:team)
{
    // Change virtual team.
    TeamMgr_SetClientTeam(client, team);
    
    // Move client to its matching mod specific team.
    TeamMgr_MoveClientToTeam(client);
    
    // Spawn player in game.
    IGame_RespawnPlayer(client);
}

/*____________________________________________________________________________*/

/**
 * Sets a mapping from VTeam to a team index.
 */
TeamMgr_SetTeamIndex(VTeam:team, teamIndex)
{
    TeamMgr_IndexForVTeam[team] = teamIndex;
    TeamMgr_VTeamForIndex[teamIndex] = team;
}

/*____________________________________________________________________________*/

/**
 * Gets mod specific team index for the virtual team.
 */
TeamMgr_GetTeamIndex(VTeam:team)
{
    return TeamMgr_IndexForVTeam[team];
}

/*____________________________________________________________________________*/

/**
 * Update the game teams associated with the virtual teams, as set by cvar.
 *
 * @param zombieTeamCvar    Handle to cvar with mod specific team name that
 *                          should be assigned to the virtual zombie team.
 */
TeamMgr_UpdateZombieTeam(Handle:zombieTeamCvar)
{
    // Get mod specific team name from cvar.
    new String:zombieTeamName[32];
    GetConVarString(zombieTeamCvar, zombieTeamName, sizeof(zombieTeamName));
    
    // Convert to virtual team.
    new zombieTeamIndex = IGame_StringToTeamIndex(zombieTeamName);
    
    // Check for invalid teams.
    if (!IGame_IsPlayerTeamIndex(zombieTeamIndex))
    {
        // Restore default teams.
        TeamMgr_SetTeamIndex(VTeam_Zombie, IGame_GetDefaultIndexFor(VTeam_Zombie));
        TeamMgr_SetTeamIndex(VTeam_Human, IGame_GetDefaultIndexFor(VTeam_Human));
        TeamMgr_SetTeamIndex(VTeam_Ghost, IGame_GetDefaultIndexFor(VTeam_Ghost));
        
        new String:defaultZombieTeam[32];
        IGame_TeamIndexToString(zombieTeamIndex, defaultZombieTeam, sizeof(defaultZombieTeam));
        
        LogMgr_Print(g_moduleTeamManager, LogType_Error, "Cvar Validation", "Invalid team name (%s) in cvar \"zr_zombie_team\".  Check the description in zombiereloaded.cfg.  Assigning zombies to: %s", zombieTeamName, defaultZombieTeam);
        
        return;
    }
    
    // Associate the zombie virtual team with the given value.
    TeamMgr_SetTeamIndex(VTeam_Zombie, zombieTeamIndex);
    
    // For humans, use the opposite team as the one given for zombies.
    TeamMgr_SetTeamIndex(VTeam_Human, IGame_GetOpposingPlayerTeam(zombieTeamIndex));
}
