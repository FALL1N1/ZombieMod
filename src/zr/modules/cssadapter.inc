/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:           cssadapter.inc
 *  Type:           Module
 *  Description:    Handles functions and events specific to CS:S. Implements
 *                  the game interface.
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

#undef REQUIRE_EXTENSIONS
#include <cstrike>
#define REQUIRE_EXTENSIONS

#include "zr/libraries/csslib"

/**
 * This module's identifier.
 */
new Module:g_moduleCSSAdapter;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:CSSAdapter_GetIdentifier() { return g_moduleCSSAdapter; }

/**
 * Default teams to use if not specified in configuration file.
 */
#define CSS_ADAPTER_DEFAULT_ZOMBIE_TEAM     CS_TEAM_T
#define CSS_ADAPTER_DEFAULT_HUMAN_TEAM      CS_TEAM_CT
#define CSS_ADAPTER_DEFAULT_GHOST_TEAM      CS_TEAM_CT

/*____________________________________________________________________________*/

/**
 * Register this module.
 */
CSSAdapter_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Counter-Strike: Source adapter");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "cssadapter");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Handles functions and events specific to CS:S.");
    moduledata[ModuleData_Dependencies][0] = ZRCore_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleCSSAdapter = ModuleMgr_Register(moduledata);
    
    // Create events.
    // (None)
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleCSSAdapter, "Event_OnEventsRegister", "CSSAdapter_OnEventsRegister");
}

/*____________________________________________________________________________*/

/**
 * Register all events here.
 */
public CSSAdapter_OnEventsRegister()
{
    // Register all the events needed for this module.
    //EventMgr_RegisterEvent(g_moduleCSSAdapter, "Event_OnMyModuleDisable", "CSSAdapter_OnMyModuleDisable");
    //EventMgr_RegisterEvent(g_moduleCSSAdapter, "Event_OnMyModuleEnable", "CSSAdapter_OnMyModuleEnable");
}

/*____________________________________________________________________________*/

/**
 * Plugin is loading. Called by zrcore module.
 *
 * Once enabled, this module is supposed to stay enabled.
 */
CSSAdapter_OnPluginStart()
{
    // Register the module.
    CSSAdapter_Register();
    
    CSSAdapter_Enable();
}

/*____________________________________________________________________________*/

/**
 * Enables the CS:S adapter features.
 */
CSSAdapter_Enable()
{
    // Prepare CS:S library.
    CSSLib_SetupSDKCalls();
    
    // Implement the game interface.
    CSSAdapter_ImplementInterface();
    
    // Hook events.
    CSSAdapter_HookEvents();
}

/*____________________________________________________________________________*/

/**
 * Disables the CS:S adapter features.
 */
CSSAdapter_Disable()
{
    // Release the game interface.
    CSSAdapter_ReleaseInterface();
    
    // Unhook events.
    CSSAdapter_UnhookEvents();
}

/*____________________________________________________________________________*/

/**
 * Implements the game interface functions.
 */
CSSAdapter_ImplementInterface()
{
    Interface_Implement(g_IGameStringToTeamIndex, CSSAdapter_StringToTeamIndex);
    Interface_Implement(g_IGameTeamIndexToString, CSSAdapter_TeamIndexToString);
    Interface_Implement(g_IGameIsPlayerTeamIndex, CSSAdapter_IsPlayerTeamIndex);
    Interface_Implement(g_IGameGetDefaultIndexFor, CSSAdapter_GetDefaultIndexFor);
    Interface_Implement(g_IGameGetOpposingPlayerTeam, CSSAdapter_GetOpposingPlayerTeam);
    
    Interface_Implement(g_IGameRespawnPlayer, CSSAdapter_RespawnPlayer);
    Interface_Implement(g_IGameSwitchTeam, CSSAdapter_SwitchTeam);
    Interface_Implement(g_IGameTerminateRound, CSSAdapter_TerminateRound);
    Interface_Implement(g_IGameDropWeapon, CSSAdapter_DropWeapon);
    Interface_Implement(g_IGameFlashlightIsOn, CSSAdapter_FlashlightIsOn);
    Interface_Implement(g_IGameFlashlightOn, CSSAdapter_FlashlightOn);
    Interface_Implement(g_IGameFlashlightOff, CSSAdapter_FlashlightOff);
    Interface_Implement(g_IGameWeaponIsZombieClaws, CSSAdapter_WeaponIsZombieClaws);
}

/*____________________________________________________________________________*/

/**
 * Releases the game interface functions.
 */
stock CSSAdapter_ReleaseInterface()
{
    Interface_Release(g_IGameStringToTeamIndex);
    Interface_Release(g_IGameTeamIndexToString);
    Interface_Release(g_IGameIsPlayerTeamIndex);
    Interface_Release(g_IGameGetDefaultIndexFor);
    Interface_Release(g_IGameGetOpposingPlayerTeam);
    
    Interface_Release(g_IGameRespawnPlayer);
    Interface_Release(g_IGameSwitchTeam);
    Interface_Release(g_IGameTerminateRound);
    Interface_Release(g_IGameDropWeapon);
    Interface_Release(g_IGameFlashlightIsOn);
    Interface_Release(g_IGameFlashlightOn);
    Interface_Release(g_IGameFlashlightOff);
    Interface_Release(g_IGameWeaponIsZombieClaws);
}

/*____________________________________________________________________________*/

/**
 * Hook CS:S events.
 */
CSSAdapter_HookEvents()
{
    HookEvent("round_start", CSSAdapter_OnRoundStart);
    HookEvent("round_freeze_end", CSSAdapter_OnRoundFreezeEnd);
    HookEvent("round_end", CSSAdapter_OnRoundEnd);
    HookEvent("player_team", CSSAdapter_OnPlayerTeamPre, EventHookMode_Pre);
    HookEvent("player_team", CSSAdapter_OnPlayerTeam);
    HookEvent("player_spawn", CSSAdapter_OnPlayerSpawn);
    HookEvent("player_hurt", CSSAdapter_OnPlayerHurt);
    HookEvent("player_death", CSSAdapter_OnPlayerDeath);
    HookEvent("player_jump", CSSAdapter_OnPlayerJump);
    HookEvent("weapon_fire", CSSAdapter_OnWeaponFire);
}

/*____________________________________________________________________________*/

/**
 * Unhook CS:S events.
 */
CSSAdapter_UnhookEvents()
{
    UnhookEvent("round_start", CSSAdapter_OnRoundStart);
    UnhookEvent("round_freeze_end", CSSAdapter_OnRoundFreezeEnd);
    UnhookEvent("round_end", CSSAdapter_OnRoundEnd);
    UnhookEvent("player_team", CSSAdapter_OnPlayerTeamPre, EventHookMode_Pre);
    UnhookEvent("player_team", CSSAdapter_OnPlayerTeam);
    UnhookEvent("player_spawn", CSSAdapter_OnPlayerSpawn);
    UnhookEvent("player_hurt", CSSAdapter_OnPlayerHurt);
    UnhookEvent("player_death", CSSAdapter_OnPlayerDeath);
    UnhookEvent("player_jump", CSSAdapter_OnPlayerJump);
    UnhookEvent("weapon_fire", CSSAdapter_OnWeaponFire);
}

/*____________________________________________________________________________*/

/**
 * The module has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned
 *                      and it's non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and
 *                      Plugin_Continue to allow it.
 */
public Action:CSSAdapter_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    CSSAdapter_Enable();
    return Plugin_Continue;
}

/*____________________________________________________________________________*/

/**
 * The module has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned
 *                      and it's non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and
 *                      Plugin_Continue to allow it.
 */
public Action:CSSAdapter_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    CSSAdapter_Disable();
    return Plugin_Continue;
}


/************************************************
 *           Game Interface Functions           *
 ************************************************/

/**
 * Converts a CS:S specific team name to a mod specific team index.
 *
 * @param teamName      CS:S team name to convert.
 *
 * @return              CS:S team index, or -1 if invalid.
 */
public CSSAdapter_StringToTeamIndex(const String:teamName[])
{
    return CSSLib_StringToTeam(teamName);
}

/*____________________________________________________________________________*/

/**
 * Converts a CS:S team index to a CS:S team name.
 *
 * @param team          Team index to convert.
 * @param buffer        (Output) Buffer to store CS:S team name in.
 * @param maxlen        Size of buffer.
 *
 * @return              Number of cells written.
 */
public CSSAdapter_TeamIndexToString(teamIndex, String:buffer[], maxlen)
{
    return CSSLib_TeamToString(teamIndex, buffer, maxlen);
}

/*____________________________________________________________________________*/

/**
 * Returns whether the CS:S team index is a player team (not spectator or
 * unassigned).
 *
 * @param team          Player team index check.
 *
 * @return              True if it's a player team, false otherwise.
 */
public bool:CSSAdapter_IsPlayerTeamIndex(teamIndex)
{
    return CSSLib_IsPlayerTeam(teamIndex);
}

/*____________________________________________________________________________*/

/**
 * Gets default CS:S team index for the specified virtual team.
 *
 * @param team          Virtual team to get default index for.
 *
 * @return              CS:S team index, or -1 on error.
 */
public CSSAdapter_GetDefaultIndexFor(VTeam:team)
{
    switch (team)
    {
        case VTeam_Unassigned:
        {
            return CS_TEAM_NONE;
        }
        case VTeam_Spectator:
        {
            return CS_TEAM_SPECTATOR;
        }
        case VTeam_Zombie:
        {
            return CSS_ADAPTER_DEFAULT_ZOMBIE_TEAM;
        }
        case VTeam_Human:
        {
            return CSS_ADAPTER_DEFAULT_HUMAN_TEAM;
        }
        case VTeam_Ghost:
        {
            return CSS_ADAPTER_DEFAULT_GHOST_TEAM;
        }
    }
    
    return -1;
}

/*____________________________________________________________________________*/

/**
 * Gets the opposing player team index.
 *
 * @param team          Player team index to invert.
 *
 * @return              Opposing team index, -1 on error.
 */
public CSSAdapter_GetOpposingPlayerTeam(teamIndex)
{
    return CSSLib_GetOpposingPlayerTeam(teamIndex);
}

/*____________________________________________________________________________*/

/**
 * Respawns a client.
 *
 * @param client        Player's index.
 */
public CSSAdapter_RespawnPlayer(client)
{
    CS_RespawnPlayer(client);
}

/*____________________________________________________________________________*/

/**
 * Switches the player's team.
 *
 * @param client        Player's index.
 * @param team          Team index.
 */
public CSSAdapter_SwitchTeam(client, VTeam:team)
{
    // Convert virtual team to CS:S team index.
    new teamIndex = TeamMgr_GetTeamIndex(team);
    
    // Only switch team if not already on that team.
    if (GetClientTeam(client) != teamIndex)
    {
        CS_SwitchTeam(client, teamIndex);
    }
}

/*____________________________________________________________________________*/

/**
 * Forces round to end.
 *
 * @param winner        The winning team (virtual team). Use VTeam_Invalid to
 *                      terminate the round without a winner (round draw, etc).
 * @param               (Optional) Delay in seconds before terminating round.
 *                      Default is 3 seconds.
 */
public CSSAdapter_TerminateRound(VTeam:winner, Float:delay)
{
    // Convert winning team to a round end reason for CS:S.
    new CSRoundEndReason:reason;
    
    if (winner == VTeam_Invalid)
    {
        // No winner. Use GameStart as reason.
        reason = CSRoundEnd_GameStart;
    }
    else
    {
        new teamIndex = TeamMgr_GetTeamIndex(winner);
        reason = CSSLib_TeamToReason(teamIndex);
    }
    
    CS_TerminateRound(delay, reason);
}

/*____________________________________________________________________________*/

/**
 * Forces player to drop or toss their weapon.
 *
 * @param client        Player's index.
 * @param weaponIndex   Index of weapon to drop.
 * @param toos          True to toss weapon (with velocity), or false to just
 *                      drop weapon.
 */
public CSSAdapter_DropWeapon(client, weaponIndex, bool:toss)
{
    CS_DropWeapon(client, weaponIndex, toss);
}

/*____________________________________________________________________________*/

/**
 * Returns whether the player's flashlight is turned on.
 *
 * @param client        Player's index.
 *
 * @return              True if on, false otherwise.
 */
public bool:CSSAdapter_FlashlightIsOn(client)
{
    return CSSLib_FlashlightIsOn(client);
}

/*____________________________________________________________________________*/

/**
 * Turns player's flashlight on.
 *
 * @param client        Player's index.
 */
public CSSAdapter_FlashlightOn(client)
{
    CSSLib_FlashlightOn(client);
}

/*____________________________________________________________________________*/

/**
 * Turns player's flashlight off.
 *
 * @param client        Player's index.
 */
public CSSAdapter_FlashlightOff(client)
{
    CSSLib_FlashlightOff(client);
}

/*____________________________________________________________________________*/

/**
 * Returns whether the specified weapon class is used as zombie claws.
 *
 * @param weaponClass       Weapon class name (without "weapon_" prefix).
 */
public bool:CSSAdapter_WeaponIsZombieClaws(const String:weaponClass[])
{
    // TODO: This could be configurable.
    if (StrEqual(weaponClass, "knife", false))
    {
        return true;
    }
    
    return false;
}


/************************************************
 *                Event Handlers                *
 ************************************************/

/**
 * Round has started.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnRoundStart(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvRoundStart, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/*____________________________________________________________________________*/

/**
 * Pre-round freeze time has finished.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnRoundFreezeEnd(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvRoundFreezeEnd, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/*____________________________________________________________________________*/

/**
 * Round has ended.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnRoundEnd(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = GetEventInt(event, "winner");
    
    EventMgr_Forward(g_EvRoundEnd, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
}

/*____________________________________________________________________________*/

/**
 * Client has joined a team. (pre)
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnPlayerTeamPre(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_Cell, DataType_Cell};
    new any:eventdata[sizeof(eventdatatypes)][1];
    
    eventdata[0][0] = GetClientOfUserId(GetEventInt(event, "userid"));
    eventdata[1][0] = GetEventInt(event, "team");
    eventdata[2][0] = GetEventInt(event, "oldteam");
    eventdata[3][0] = GetEventBool(event, "disconnect");
    
    new Action:result = EventMgr_Forward(g_EvPlayerTeamPre, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    
    // If a module returns Plugin_Handled then stop the event from broadcasting to clients.
    if (result == Plugin_Handled)
        SetEventBroadcast(event, true);
}

/*____________________________________________________________________________*/

/**
 * Client has joined a team.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnPlayerTeam(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_Cell, DataType_Cell};
    new any:eventdata[sizeof(eventdatatypes)][1];
    
    eventdata[0][0] = GetClientOfUserId(GetEventInt(event, "userid"));
    eventdata[1][0] = GetEventInt(event, "team");
    eventdata[2][0] = GetEventInt(event, "oldteam");
    eventdata[3][0] = GetEventBool(event, "disconnect");
    
    EventMgr_Forward(g_EvPlayerTeam, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
}

/*____________________________________________________________________________*/

/**
 * Client has spawned.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnPlayerSpawn(Handle:event, const String:name[], bool:dontBroadcast)
{
    new client = GetClientOfUserId(GetEventInt(event, "userid"));
    
    // Skip clients not on a team. These are clients who just connected and
    // spawned as unassigned spectators.
    if (!Util_IsClientOnTeam(client))
    {
        return;
    }
    
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvPlayerSpawn, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Prepare post spawn data. Used in OnGameFrame.
    g_fPlayerSpawnTime[client] = GetGameTime();
    g_SendPostSpawnEvent[client] = true;
}

/*____________________________________________________________________________*/

/**
 * Client has been damaged.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnPlayerHurt(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Note: This event is called frequently, skipping full array
    //       initialization.
    
    // Forward event to all modules.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_Cell, DataType_Cell, DataType_String, DataType_Cell, DataType_Cell, DataType_Cell};
    decl any:eventdata[sizeof(eventdatatypes)][32];
    eventdata[4][0] = 0;    // Initialize string slot.
    
    eventdata[0][0] = GetClientOfUserId(GetEventInt(event, "userid"));
    eventdata[1][0] = GetClientOfUserId(GetEventInt(event, "attacker"));
    eventdata[2][0] = GetEventInt(event, "health");
    eventdata[3][0] = GetEventInt(event, "armor");
    GetEventString(event, "weapon", eventdata[4], sizeof(eventdata[]));
    eventdata[5][0] = GetEventInt(event, "dmg_health");
    eventdata[6][0] = GetEventInt(event, "dmg_armor");
    eventdata[7][0] = GetEventInt(event, "hitgroup");
    
    EventMgr_Forward(g_EvPlayerHurt, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
}

/*____________________________________________________________________________*/

/**
 * Client has been killed.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnPlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_String, DataType_Cell};
    new any:eventdata[sizeof(eventdatatypes)][32];
    
    eventdata[0][0] = GetClientOfUserId(GetEventInt(event, "userid"));
    eventdata[1][0] = GetClientOfUserId(GetEventInt(event, "attacker"));
    GetEventString(event, "weapon", eventdata[2], sizeof(eventdata[]));
    eventdata[3][0] = GetEventInt(event, "headshot");
    
    EventMgr_Forward(g_EvPlayerDeath, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
}

/*____________________________________________________________________________*/

/**
 * Client jumped.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnPlayerJump(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = GetClientOfUserId(GetEventInt(event, "userid"));
    
    EventMgr_Forward(g_EvPlayerJump, eventdata, 1, 1, g_CommonDataType2);
}

/*____________________________________________________________________________*/

/**
 * Client has fired a weapon.
 *
 * @param event         Handle to event. This could be INVALID_HANDLE if every
 *                      plugin hooking  this event has set the hook mode
 *                      EventHookMode_PostNoCopy.
 * @param name          String containing the name of the event.
 * @param dontBroadcast True if event was not broadcast to clients, false
 *                      otherwise.
 */
public CSSAdapter_OnWeaponFire(Handle:event, const String:name[], bool:dontBroadcast)
{
    // Note: This event is called frequently, skipping full array
    //       initialization.
    
    // Forward event to all modules. (WeaponFire)
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_String};
    decl any:eventdata[sizeof(eventdatatypes)][32];
    eventdata[1][0] = 0;    // Initialize string slot.
    
    eventdata[0][0] = GetClientOfUserId(GetEventInt(event, "userid"));
    GetEventString(event, "weapon", eventdata[1], sizeof(eventdata[]));
    
    EventMgr_Forward(g_EvWeaponFire, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    
    /*
    // Forward event to all modules. (WeaponEntityFire)
    new eventdata2[sizeof(eventdatatypes)][1];
    static EventDataTypes:eventdatatypes2[] = {DataType_Cell, DataType_Cell};
    
    decl String:classname[32];
    new weaponentity;
    
    // Loop through all entities.
    new maxentities = GetMaxEntities();
    for (new entity = MaxClients; entity < maxentities; entity++)
    {
        if (!IsValidEntity(entity))
            continue;
        
        GetEdictClassname(entity, classname, sizeof(classname));
        if (StrContains(classname, eventdata[1], false) == -1)
            continue;
        
        if (eventdata[0][0] == GetEntPropEnt(entity, Prop_Data, "m_hOwner"))
        {
            weaponentity = entity;
            break;
        }
    }
    
    eventdata2[0][0] = eventdata[0][0];
    eventdata2[1][0] = weaponentity;
    
    EventMgr_Forward(g_EvWeaponEntityFire, eventdata2, sizeof(eventdata2), sizeof(eventdata2[]), eventdatatypes2);
    */
}
