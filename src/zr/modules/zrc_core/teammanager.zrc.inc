/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          teammanager.zrc.inc
 *  Type:          Module
 *  Description:   Manages zombie-based teams for ZRClassic gameplay.
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
 * This module's identifier.
 */
new Module:g_moduleZRCTM;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRCTM_GetIdentifier() { return g_moduleZRCTM; }

/**
 * Cvar handles.
 */
new Handle:g_hCvarZRCZombieTeam;

/**
 * Register this module.
 */
ZRCTM_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRC Team Manager");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zrcteammanager");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manages zombie-based teams for ZRClassic gameplay.");
    moduledata[ModuleData_Dependencies][0] = ZRC_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZRCTM = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_OnEventsRegister",             "ZRCTM_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZRCTM_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_OnConfigsExecuted",            "ZRCTM_OnConfigsExecuted");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_OnClientPutInServer",          "ZRCTM_OnClientPutInServer");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_OnEventsReady",                "ZRCTM_OnEventsReady");
    
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_RoundStart",                   "ZRCTM_RoundStart");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_RoundEnd",                     "ZRCTM_RoundEnd");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_PlayerTeamPre",                "ZRCTM_PlayerTeamPre");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_PlayerTeam",                   "ZRCTM_PlayerTeam");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_PlayerSpawn",                  "ZRCTM_PlayerSpawn");
    
    // Register custom events.
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_OnClientInfected",             "ZRCTM_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleZRCTM, "Event_OnClientHuman",                "ZRCTM_OnClientHuman");
}

/**
 * Plugin is loading.
 */
ZRCTM_OnPluginStart()
{
    // Register the module.
    ZRCTM_Register();
    
    // Create cvars.
    g_hCvarZRCZombieTeam  = Project_CreateConVar("zombie_team", "t", "Define the game team that will be the zombie team.  Opposite team will be the human team.  ['t' = Terrorists | 'ct' = Counter-Terrorists]");
}

public ZRCTM_OnEventsReady()
{
    // Get priority over infection module.
    EventMgr_GivePriority("Event_PlayerSpawn", g_moduleZRCTM, ZRCInfect_GetIdentifier());
}

/**
 * All convars are set, cvar-dependent code should use this.
 */
public ZRCTM_OnConfigsExecuted()
{
    TeamMgr_UpdateZombieTeam(g_hCvarZRCZombieTeam);
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public ZRCTM_OnClientPutInServer(client)
{
    // Initialize the client's team.
    TeamMgr_SetClientTeam(client, VTeam_Unassigned);
}

/**
 * Client has been infected.
 * 
 * @param client        The infected client.
 * @param attacker      The zombie that infected the client.
 * @param mzombie       True if the client has been infected as a mother zombie.
 */
public ZRCTM_OnClientInfected(client, attacker, bool:mzombie)
{
    // Move client to zombie team.
    IGame_SwitchTeam(client, VTeam_Zombie);
}

/**
 * Client has turned back to human.
 * 
 * @param client        The client that became human.
 */
public ZRCTM_OnClientHuman(client)
{
    // Only move client to another game team if there are zombies present. It
    // doesn't matter which team they are on when there are no zombies.
    if (g_bZombiesPresent)
    {
        IGame_SwitchTeam(client, VTeam_Human);
    }
}

/**
 * Called when round has started.
 */
public ZRCTM_RoundStart()
{
    // Update the zombie team every round start. This shouldn't be updated
    // real-time with the cvar, or else zombies would be on both teams during
    // the round.
    TeamMgr_UpdateZombieTeam(g_hCvarZRCZombieTeam);
}

/**
 * Called when round has ended.
 *
 * @param winner        Winner team.
 */
public ZRCTM_RoundEnd(VTeam:winner)
{
    // Turn everyone into humans, so they can pickup weapons. Only do this for
    // playing clients. (Already done by infection module.)
    //TeamMgr_SetClientTeams(VTeam_Human, true);
    
    ZRCTM_BalanceTeams();
}

/**
 * Called when player is about to join a team.
 *
 * @param client        Client index.
 * @param team          Team client is about to join.
 * @param oldTeam       Old team before changing team.
 * @param disconnect    Team change because player disconnects.
 *
 * @return              Plugin_Handled to stop broadcast.
 *                      Plugin_Continue to allow broadcast.
 */
public Action:ZRCTM_PlayerTeamPre(client, VTeam:team, VTeam:oldTeam, bool:disconnect)
{
    // Prevent the join message from being broadcasted to clients.
    if (!disconnect && IGame_IsPlayerTeam(team))
    {
        return Plugin_Handled;
    }
    
    return Plugin_Continue;
}

/**
 * Called when player has joined a team.
 *
 * @param client        Client index.
 * @param team          Team client joined.
 * @param oldTeam       Old team before changing team.
 * @param disconnect    Team change because player disconnects.
 */
public ZRCTM_PlayerTeam(client, VTeam:team, VTeam:oldTeam, bool:disconnect)
{
    // Update player team in common team manager.
    
    // Check if player is already assigned to a player team.
    if (TeamMgr_IsClientOnPlayerTeam(client))
    {
        // Do nothing. This callback should only handle unassigned players or
        // spectators.
        return;
    }
    
    // Player joined a player team.
    if (IGame_IsPlayerTeam(team))
    {
        // Assign player to human team. All new players start as humans.
        TeamMgr_SetClientTeam(client, VTeam_Human);
        return;
    }
    
    // Other teams are directly mapped (spectator, unassigned).
    TeamMgr_SetClientTeam(client, team);
    
    // Old code, in case the logic above is incorrect.
    /*
    if (team == TEAM_1)
        TLib_SetClientTeam(client, VTeam_Spectator);
    // If client isn't already in the game, then assign them to human by default.
    else if (team > TEAM_1 && !TLib_IsClientPlaying(client))
        TLib_SetClientTeam(client, VTeam_Human);
    */
}

/**
 * Player has spawned.
 *
 * Note: This event is NOT called when client is spawning in the game (the first
 *       time they see the map, before selecting a team).
 *
 * @param client        Client index.
 */
public ZRCTM_PlayerSpawn(client)
{
    // TODO: Assign player to a team if not assigned. Decide which team
    //       according to spawn team cvar and if zombies are present. Will add
    //       support for spawning late joining players as zombies.
    TeamMgr_SetClientTeam(client, VTeam_Human);
    
    // Move client the right team for their virtual team if there are zombies present.
    if (g_bZombiesPresent)
        TeamMgr_MoveClientToTeam(client);
}


/**
 * Randomly put an equal amount of clients on both teams.
 */
static stock ZRCTM_BalanceTeams()
{
    new bool:team1 = true;
    
    for (new client = 1; client <= MaxClients; client++)
    {
        // Skip players not connected.
        if (!IsClientInGame(client))
            continue;
        
        // Skip players not in zombie or human teams.
        if (!TeamMgr_IsClientOnPlayerTeam(client))
            continue;
        
        // Switch team in mod. The client's virtual team is unchanged.
        if (team1)
            IGame_SwitchTeam(client, VTeam_Zombie);
        else
            IGame_SwitchTeam(client, VTeam_Human);
        
        team1 = !team1;
    }
}
