/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          voice.inc
 *  Type:          Module
 *  Description:   Modifies mic-communication to match zombie/human teams.
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
new Module:g_moduleVoice;

/**
 * Cvar handles.
 */
new Handle:g_hCvarVoice;

/**
 * Register this module.
 */
Voice_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = true;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Voice");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "voice");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Modifies mic-communication to match zombie/human teams.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleVoice = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleVoice, "Event_OnEventsRegister",             "Voice_OnEventsRegister");
}

/**
 * Register all events here.
 */
public Voice_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleVoice, "Event_OnMyModuleEnable",             "Voice_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleVoice, "Event_OnMyModuleDisable",            "Voice_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleVoice, "Event_OnClientInfected",             "Voice_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleVoice, "Event_OnClientHuman",                "Voice_OnClientHuman");
    
    EventMgr_RegisterEvent(g_moduleVoice, "Event_PlayerTeam",                   "Voice_PlayerTeam");
    EventMgr_RegisterEvent(g_moduleVoice, "Event_RoundEnd",                     "Voice_RoundEnd");
}

/**
 * Plugin is loading.
 */
Voice_OnPluginStart()
{
    // Register the module.
    Voice_Register();
    
    // Create cvars.
    g_hCvarVoice = CreateConVar("zr_voice", "1", "Modify sv_alltalk to obey zombie/human teams instead of t/ct.");
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:Voice_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    if (GetConVarBool(g_hCvarVoice))
        Voice_ReApplyAll();
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:Voice_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    Voice_ReleaseAllOverrides();
}

/**
 * Client has been infected.
 * 
 * @param client        The infected client.
 * @param attacker      The zombie that infected the client.
 * @param mzombie       True if the client has been infected as a mother zombie.
 */
public Voice_OnClientInfected(client, attacker, bool:mzombie)
{
    if (GetConVarBool(g_hCvarVoice))
        Voice_UpdateCommunication(client);
}

/**
 * Client has turned back to human.
 * 
 * @param client        The client that became human.
 */
public Voice_OnClientHuman(client)
{
    if (GetConVarBool(g_hCvarVoice))
        Voice_UpdateCommunication(client);
}

/**
 * Called when player has joined a team.
 *
 * @param client        Client index.
 * @param team          Team client joined.
 * @param oldTeam       Old team before changing team.
 * @param disconnect    Team change because player disconnects.
 */
public Voice_PlayerTeam(client, VTeam:team, VTeam:oldTeam, bool:disconnect)
{
    // Undo overrides if they aren't playing on a team.
    if (team == VTeam_Spectator || team == VTeam_Ghost)
    {
        Voice_ReleaseClientOverrides(client);
    }
}

/**
 * Round has ended.
 * 
 * @param winner    The index of the winning team.
 */
public Voice_RoundEnd(winner)
{
    Voice_ReleaseAllOverrides();
}

/**
 * Re-applies listening/speaking permissions on all clients.
 */
Voice_ReApplyAll()
{
    for (new client = 1; client <= MaxClients; client++)
    {
        Voice_UpdateCommunication(client);
    }
}

/**
 * Release voice overrides on a client.
 * 
 * @param client    The client.
 */
Voice_ReleaseClientOverrides(client)
{
    for (new client2 = 1; client2 <= MaxClients; client2++)
    {
        if (client != client2)
        {
            Voice_OverrideCommunication(client, client2, Listen_Default);
        }
    }
}

/**
 * Release all voice overrides.
 */
Voice_ReleaseAllOverrides()
{
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
            continue;
        
        Voice_ReleaseClientOverrides(client);
    }
}

/**
 * Update communication overrides on this client.
 * 
 * @param client    Client to update.
 */
Voice_UpdateCommunication(client)
{
    new VTeam:team = TeamMgr_GetClientTeam(client);
    new VTeam:team2;
    for (new client2 = 1; client2 <= MaxClients; client2++)
    {
        if (!IsClientInGame(client2))
            continue;
        
        // TODO: Handle ghost team, they are treated as spectators.
        
        team2 = TeamMgr_GetClientTeam(client2);
        if (team == team2)
            Voice_OverrideCommunication(client, client2, Listen_Yes);
        else if (team2 > VTeam_Spectator) // Don't override people not in the game.
            Voice_OverrideCommunication(client, client2, Listen_No);
    }
}

/**
 * Change listening voice override settings between these clients.
 * 
 * @param client1/2     Clients to change communication overrides between.
 */
Voice_OverrideCommunication(client1, client2, ListenOverride:override)
{
    SetListenOverride(client1, client2, override);
    SetListenOverride(client2, client1, override);
}
