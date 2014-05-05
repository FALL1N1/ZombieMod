/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          ztele.inc
 *  Type:          Module
 *  Description:   Player teleport manager.
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

// Include libraries.
#include "zr/libraries/conversionlib"
#include "zr/libraries/menulib"

/**
 * This module's identifier.
 */
new Module:g_moduleZTele;

/**
 * Teleport events.
 */
new ProjectEvent:g_EvOnZTeleTimerStarted;
new ProjectEvent:g_EvOnClientZTele;
new ProjectEvent:g_EvOnZTeleAutoCancelled;

/**
 * Teleport convars.
 */
new Handle:g_hCvarZTele;
new Handle:g_hCvarZTeleRandom;
new Handle:g_hCvarZTeleZombie;
new Handle:g_hCvarZTeleHumanBefore;
new Handle:g_hCvarZTeleHumanAfter;
new Handle:g_hCvarZTeleDelayZombie;
new Handle:g_hCvarZTeleDelayHuman;
new Handle:g_hCvarZTeleMaxZombie;
new Handle:g_hCvarZTeleMaxHuman;
new Handle:g_hCvarZTeleAutoCancel;
new Handle:g_hCvarZTeleAutoCancelDistance;

/**
 * ZTele's button indexes in menus.
 */
new g_iZTeleZMenuButton;
new g_iZTeleZAdminButton;

/**
 * A list of all used spawnpoints on the current map.
 */
new Float:g_vecZTeleSpawnPoints[MAXPLAYERS + 1][3];

/**
 * Current number of spawnpoints in the map.
 */
new g_iZTeleNumSpawnPoints;

/**
 * Array to store the tele count of each client.
 */
new g_iZTeleCount[MAXPLAYERS + 1];

/**
 * Array for storing ZTele timer handles per client.
 */
new Handle:g_hZTeleTimer[MAXPLAYERS + 1];

/**
 * Array to store time left before teleport.
 */
new g_iZTeleTimeLeft[MAXPLAYERS + 1];

/**
 * Location of client when they type !ztele.  This is used for the "auto-cancel" feature.
 */
new Float:g_vecAutoCancelBoundary[MAXPLAYERS + 1][3];

/**
 * Tracks if the round has started yet.
 */
new bool:g_bZTeleRoundInProgress;

/**
 * Register this module.
 */
ZTele_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZTele");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "ztele");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Player teleport manager.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZTele = ModuleMgr_Register(moduledata);
    
    // Create events.
    g_EvOnZTeleTimerStarted = EventMgr_CreateEvent("Event_OnZTeleTimerStarted");
    g_EvOnClientZTele = EventMgr_CreateEvent("Event_OnClientZTele");
    g_EvOnZTeleAutoCancelled = EventMgr_CreateEvent("Event_OnZTeleAutoCancelled");
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnEventsRegister",             "ZTele_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZTele_OnEventsRegister()
{
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnZMenuCreated",               "ZTele_OnZMenuCreated");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnZAdminCreated",              "ZTele_OnZAdminCreated");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnMyModuleEnable",             "ZTele_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnMyModuleDisable",            "ZTele_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnMapStart",                   "ZTele_OnMapStart");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnClientPutInServer",          "ZTele_OnClientPutInServer");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnClientDisconnect",           "ZTele_OnClientDisconnect");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnClientInfected",             "ZTele_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_OnClientHuman",                "ZTele_OnClientHuman");
    
    #if defined PROJECT_GAME_CSS
    
    EventMgr_RegisterEvent(g_moduleZTele, "Event_PlayerSpawn",                  "ZTele_PlayerSpawn");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_PlayerDeath",                  "ZTele_PlayerDeath");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_RoundStart",                   "ZTele_RoundStart");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_RoundFreezeEnd",               "ZTele_RoundFreezeEnd");
    EventMgr_RegisterEvent(g_moduleZTele, "Event_RoundEnd",                     "ZTele_RoundEnd");
    
    #endif
}

/**
 * Plugin is loading.
 */
ZTele_OnPluginStart()
{
    // Register the module.
    ZTele_Register();
    
    // Create cvars.
    g_hCvarZTele =                      Project_CreateConVar("ztele", "1", "Allow clients to use ztele command to teleport back to spawn.");
    g_hCvarZTeleRandom =                Project_CreateConVar("ztele_random", "1", "Teleport back to a random spawnpoint on the map. ['0' = Teleports back to where you spawned during the round]");
    g_hCvarZTeleZombie =                Project_CreateConVar("ztele_zombie", "1", "Allow zombies to use ZTele.");
    g_hCvarZTeleHumanBefore =           Project_CreateConVar("ztele_human_before", "1", "Allow humans to use ZTele before the mother zombie has spawned. [This only applies to ZRC]");
    g_hCvarZTeleHumanAfter =            Project_CreateConVar("ztele_human_after", "1", "Allow humans to use ZTele after the zombies are present.");
    g_hCvarZTeleDelayZombie =           Project_CreateConVar("ztele_delay_zombie", "3.0", "Time between using ZTele command and teleportation for zombies. [Dependency: <prefix>_ztele_zombie]");
    g_hCvarZTeleDelayHuman =            Project_CreateConVar("ztele_delay_human", "3.0", "Time between using ZTele command and teleportation for humans. [Dependency: <prefix>_ztele_human_(before)/(after)]");
    g_hCvarZTeleMaxZombie =             Project_CreateConVar("ztele_max_zombie", "3", "Max number of times a zombie is allowed to use ZTele per round. [Dependency: <prefix>_ztele_zombie]");
    g_hCvarZTeleMaxHuman =              Project_CreateConVar("ztele_max_human", "1", "Max number of times a human is allowed to use ZTele per round. [Dependency: <prefix>_ztele_human_(before)/(after)]");
    g_hCvarZTeleAutoCancel =            Project_CreateConVar("ztele_autocancel", "1", "Automatically cancel ZTele if player moves out of the set boundary.");
    g_hCvarZTeleAutoCancelDistance =    Project_CreateConVar("ztele_autocancel_distance", "6.1 m", "The radius the player can move without ZTele being auto-cancelled.  See top of file for supported units of measurement.");
    
    // Create commands
    RegConsoleCmd("ztele", ZTele_SayCommand, "Teleport to a known location by command.");
    Project_RegConsoleCmd("ztele_force", ZTele_ForceCommand, "ZTele a client. Usage: <prefix>_ztele_force <client>");
}

/**
 * Called when ZMenu is created.
 * 
 * @param hMenu     Handle to ZMenu.
 */
public ZTele_OnZMenuCreated(Handle:hMenu)
{
    // Add ZTele's button to the menu.
    g_iZTeleZMenuButton = MenuLib_AddMenuBtnEx(hMenu, "ZTele zmenu button", "0", true, ITEMDRAW_DEFAULT, ZTele_ZMenuButton, BtnNextMenu_None, "");
}

/**
 * Called when ZAdmin is created.
 * 
 * @param hMenu     Handle to ZAdmin.
 */
public ZTele_OnZAdminCreated(Handle:hMenu)
{
    // Add ZTele's button to the menu.
    g_iZTeleZAdminButton = MenuLib_AddMenuBtnEx(hMenu, "ZTele zadmin button", "0", true, ITEMDRAW_DEFAULT, ZTele_SendClientMenu, BtnNextMenu_None, "");
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ZTele_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Enable ZTele's button to the menu.
    new Handle:hZMenu = MenuLib_FindMenuById("zmenu");
    if (hZMenu != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZMenu, g_iZTeleZMenuButton, MenuBtn_Style, ITEMDRAW_DEFAULT);
    
    new Handle:hZAdmin = MenuLib_FindMenuById("zadmin");
    if (hZAdmin != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZAdmin, g_iZTeleZAdminButton, MenuBtn_Style, ITEMDRAW_DEFAULT);
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ZTele_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    // Disable ZTele's button to the menu.
    new Handle:hZMenu = MenuLib_FindMenuById("zmenu");
    if (hZMenu != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZMenu, g_iZTeleZMenuButton, MenuBtn_Style, ITEMDRAW_DISABLED);
    
    new Handle:hZAdmin = MenuLib_FindMenuById("zadmin");
    if (hZAdmin != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZAdmin, g_iZTeleZAdminButton, MenuBtn_Style, ITEMDRAW_DISABLED);
}

/**
 * The map has started.
 */
public ZTele_OnMapStart()
{
    // Reset spawn point data.
    g_iZTeleNumSpawnPoints = 0;
    for (new spindex = 0; spindex < sizeof(g_vecZTeleSpawnPoints); spindex++)
    {
        for (new i = 0; i < 3; i++)
            g_vecZTeleSpawnPoints[spindex][i] = 0.0;
    }
}

/**
 * Client connected to the server.
 * 
 * @param client    The client index.
 */
public ZTele_OnClientPutInServer(client)
{
    g_hZTeleTimer[client] = INVALID_HANDLE;
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public ZTele_OnClientDisconnect(client)
{
    Util_CloseHandle(g_hZTeleTimer[client]);
}

/**
 * Player has been infected.
 * 
 * @param client    The client index.
 */
public ZTele_OnClientInfected(client)
{
    g_iZTeleCount[client] = 0;
    Util_CloseHandle(g_hZTeleTimer[client]);
}

/**
 * Player has been turned back human.
 * 
 * @param client    The client index.
 */
public ZTele_OnClientHuman(client)
{
    g_iZTeleCount[client] = 0;
    Util_CloseHandle(g_hZTeleTimer[client]);
}

#if defined PROJECT_GAME_CSS

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 */
public ZTele_PlayerSpawn(client)
{
    // Check if the player is on a team. Spawning into the game is also an event in
    // the connection process where players get their observer camera.
    if (!Util_IsClientOnTeam(client))
        return;
    
    // Check if the spawn point is in the array, if not then add it.
    new Float:vecLoc[3];
    GetClientAbsOrigin(client, vecLoc);
    
    // Check if we should find random spawn points or record the spawn point for a specific client.
    if (GetConVarBool(g_hCvarZTeleRandom))
    {
        if (Array_IsVecInArray(vecLoc, g_vecZTeleSpawnPoints, g_iZTeleNumSpawnPoints) == -1)
        {
            // Don't overflow the array.
            if (g_iZTeleNumSpawnPoints < sizeof(g_vecZTeleSpawnPoints))
            {
                // Copy into the spawnpoint array.
                for (new i = 0; i < 3; i++)
                    g_vecZTeleSpawnPoints[g_iZTeleNumSpawnPoints][i] = vecLoc[i];
                
                LogMgr_Print(g_moduleZTele, LogType_Debug, "Spawnpoints", "New ZTele point (%f, %f, %f) being added to array index %d.", vecLoc[0], vecLoc[1], vecLoc[2], g_iZTeleNumSpawnPoints);
                
                g_iZTeleNumSpawnPoints++;
            }
        }
    }
    else
    {
        // Copy the client's spawn point to teleport back to later during the round.
        for (new i = 0; i < 3; i++)
            g_vecZTeleSpawnPoints[client][i] = vecLoc[i];
    }
    
    g_iZTeleCount[client] = 0;
    Util_CloseHandle(g_hZTeleTimer[client]);
}

/**
 * Client has been killed.
 * 
 * @param victim    The index of the killed client.
 * @param attacker  The killer of the victim.
 * @param weapon    The weapon classname used to kill the victim. (No weapon_ prefix)
 * @param headshot  True if the death was by headshot, false if not.
 */
public ZTele_PlayerDeath(victim, attacker, const String:weapon[], bool:headshot)
{
    Util_CloseHandle(g_hZTeleTimer[victim]);
}

/**
 * Round has started.
 */
public ZTele_RoundStart()
{
    g_bZTeleRoundInProgress = false;
}

/**
 * Pre-round freezetime has finished.
 */
public ZTele_RoundFreezeEnd()
{
    g_bZTeleRoundInProgress = true;
}

/**
 * Round has ended.
 * 
 * @param winner    The index of the winning team.
 */
public ZTele_RoundEnd(winner)
{
    g_bZTeleRoundInProgress = false;
}

#endif

// ************************************************************
//  The functions in this section are ordered chronologically.
// ************************************************************

/**
 * Command callback (ztele)
 * Teleport to a known location by command.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:ZTele_SayCommand(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleZTele))
        return Plugin_Continue;
    
    if (!GetConVarBool(g_hCvarZTele))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZTele, false, "Feature is disabled");
        return Plugin_Handled;
    }
    
    if (client == SERVER_INDEX)
    {
        TransMgr_PrintText(SERVER_INDEX, MsgFormat_Plugin, MsgType_Server, INVALID_MODULE, false, "Must be player");
        return Plugin_Handled;
    }
    
    // Start teleportation process.
    ZTele_Client(client);
    
    return Plugin_Handled;
}

/**
 * Checks everything before starting the teleport sequence.
 * 
 * @param client    The client index.
 */
bool:ZTele_Client(client)
{
    // If the client is dead, then stop.
    if (!IsPlayerAlive(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "Must be alive");
        return false;
    }
    
    if (!g_bZTeleRoundInProgress)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZTele restricted round not active");
        return false;
    }
    
    new bool:zombie = TeamMgr_IsClientZombie(client);
    new bool:human = TeamMgr_IsClientHuman(client);
    
    // If zombie cvar is disabled and the client is a zombie, then stop.
    new bool:ztelezombie = GetConVarBool(g_hCvarZTeleZombie);
    if (zombie && !ztelezombie)
    {
        // Tell client they must be human to use this feature.
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZTele restricted zombie");
        return false;
    }
    
    // If zombies are present, get the after value, if not get the before value.
    new bool:ztelehuman = IInfection_AreZombiesPresent() ? GetConVarBool(g_hCvarZTeleHumanAfter) : GetConVarBool(g_hCvarZTeleHumanBefore);
    if (human && !ztelehuman)
    {
        // Tell client that feature is restricted at this time.
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZTele restricted human");
        return false;
    }
    
    // If the tele limit has been reached, then stop.
    new ztelemax = zombie ? GetConVarInt(g_hCvarZTeleMaxZombie) : GetConVarInt(g_hCvarZTeleMaxHuman);
    if (g_iZTeleCount[client] >= ztelemax)
    {
        // Tell client that they have already reached their limit.
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZTele max", ztelemax);
        return false;
    }
    
    // If teleport is already in progress, then stop.
    if (g_hZTeleTimer[client] != INVALID_HANDLE)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZTele in progress");
        return false;
    }
    
    // Get the time the client has to wait before teleporting.
    new time = zombie ? GetConVarInt(g_hCvarZTeleDelayZombie) : GetConVarInt(g_hCvarZTeleDelayHuman);
    ZTele_StartTeleport(client, time);
    
    return true;
}

/**
 * Starts the teleport sequence.
 * 
 * @param client    The client index.
 * @param time      Time between calling this and actual teleportation.
 */
ZTele_StartTeleport(client, time)
{
    if (time > 0)
    {
        // Set boundary for auto-cancel feature.
        ZTele_AutoCancelSetBoundary(client);
        
        g_iZTeleTimeLeft[client] = time;
        
        // Tell client how much time is left until teleport.
        TransMgr_PrintText(client, MsgFormat_None, MsgType_Center, INVALID_MODULE, false, "ZTele countdown", g_iZTeleTimeLeft[client]);
        g_hZTeleTimer[client] = CreateTimer(1.0, ZTele_Timer, client, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);
        
        // Forward event to all modules.
        static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_Cell};
        new any:eventdata[sizeof(eventdatatypes)][1];
        
        eventdata[0][0] = client;
        eventdata[1][0] = time;
        eventdata[2][0] = g_hZTeleTimer[client];
        
        EventMgr_Forward(g_EvOnZTeleTimerStarted, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    }
    else
    {
        ZTele_TeleportClient(client);
        
        new ztelemax = TeamMgr_IsClientZombie(client) ? GetConVarInt(g_hCvarZTeleMaxZombie) : GetConVarInt(g_hCvarZTeleMaxHuman);
        TransMgr_PrintText(client, MsgFormat_None, MsgType_Center, INVALID_MODULE, false, "ZTele countdown end", g_iZTeleCount[client], ztelemax);
    }
}

/**
 * Set move the center of the client's circular boundary to their current position.
 * 
 * @param client    The client index.
 */
ZTele_AutoCancelSetBoundary(client)
{
    GetClientAbsOrigin(client, g_vecAutoCancelBoundary[client]);
}

/**
 * Check if a client has exceeded the defined boundary.
 * See ZTele_AutoCancelSetBoundary to set the center of the circular boundary.
 * 
 * @param client    The client index.
 * 
 * @return          True if they are within the boundary, false if not.
 */
bool:ZTele_AutoCancelCheck(client)
{
    // If client has been running around after using ZTele, then stop timer.
    new Float:vecClient[3];
    GetClientAbsOrigin(client, vecClient);
    
    decl String:strDistance[16];
    GetConVarString(g_hCvarZTeleAutoCancelDistance, strDistance, sizeof(strDistance));
    new Float:flDistance = ConvertLib_DetectAndConvert(ConvLibMeasurement_Distance, strDistance, _:ConvLibDistance_GameUnit, _:ConvLibDistance_GameUnit);
    new Float:distance = GetVectorDistance(vecClient, g_vecAutoCancelBoundary[client]);
    
    // Check if distance has been surpassed.
    if (distance > flDistance)
    {
        // Reset timer handle.
        g_hZTeleTimer[client] = INVALID_HANDLE;
        
        // Tell client teleport has been cancelled.
        TransMgr_PrintText(client, MsgFormat_None, MsgType_Center, INVALID_MODULE, false, "ZTele cancel");
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZTele autocancel", strDistance);
        
        // Forward event to all modules.
        new any:eventdata[1][1];
        eventdata[0][0] = client;
        
        EventMgr_Forward(g_EvOnZTeleAutoCancelled, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType1);
        
        // Stop timer.
        return false;
    }
    
    return true;
}

/**
 * Timer callback, counts down teleport to the client.
 * 
 * @param timer     The timer handle.
 * @param client    The client index.
 */
public Action:ZTele_Timer(Handle:timer, any:client)
{
    // Check if teleport should be auto-cancelled.
    if (GetConVarBool(g_hCvarZTeleAutoCancel))
    {
        // If client has left the boundary then stop the timer.
        if (!ZTele_AutoCancelCheck(client))
        {
            g_hZTeleTimer[client] = INVALID_HANDLE;
            return Plugin_Stop;
        }
    }
    
    // Check if the time has expired.
    g_iZTeleTimeLeft[client]--;
    if (g_iZTeleTimeLeft[client] <= 0)
    {
        ZTele_TeleportClient(client);
        g_iZTeleCount[client]++;
        
        new ztelemax = TeamMgr_IsClientZombie(client) ? GetConVarInt(g_hCvarZTeleMaxZombie) : GetConVarInt(g_hCvarZTeleMaxHuman);
        TransMgr_PrintText(client, MsgFormat_None, MsgType_Center, INVALID_MODULE, false, "ZTele countdown end", g_iZTeleCount[client], ztelemax);
        
        // Stop repeating timer.
        g_hZTeleTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }
    TransMgr_PrintText(client, MsgFormat_None, MsgType_Center, INVALID_MODULE, false, "ZTele countdown", g_iZTeleTimeLeft[client]);
    
    // Allow timer to continue repeating.
    return Plugin_Continue;
}

/**
 * Teleport client to a random spawn location.
 * 
 * @param client    The client index.
 */
ZTele_TeleportClient(client)
{
    // Verify that there is a point to teleport client to.
    if (g_iZTeleNumSpawnPoints <= 0)
    {
        LogMgr_Print(g_moduleZTele, LogType_Error, "ZTeleport", "No available point to teleport client %N to.", client);
        return;
    }
    
    // Check if we should teleport to a random point or teleport back to the client's spawn point.
    if (GetConVarBool(g_hCvarZTeleRandom))
    {
        // Find random spawn point.
        new rand_spindex = GetRandomInt(0, g_iZTeleNumSpawnPoints - 1);
        TeleportEntity(client, g_vecZTeleSpawnPoints[rand_spindex], NULL_VECTOR, Float:{0.0, 0.0, 0.0});
    }
    else
    {
        TeleportEntity(client, g_vecZTeleSpawnPoints[client], NULL_VECTOR, Float:{0.0, 0.0, 0.0});
    }
    
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientZTele, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType1);
}

// **********************************************
//          ZMenu ZTele button callback
// **********************************************

/**
 * Menu callback
 * Called when a certain button in a menu is pressed.  The menu action is always MenuAction_Select.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public ZTele_ZMenuButton(Handle:hMenu, client, slot)
{
    // Teleport the client, but if it fails then re-resend the menu.
    if (!ZTele_Client(client))
        MenuLib_SendMenu(hMenu, client);
}

// **********************************************
//                Admin Callbacks
// **********************************************

/**
 * Menu callback
 * Called when a certain button in a menu is pressed.  The menu action is always MenuAction_Select.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public ZTele_SendClientMenu(Handle:hMenu, client, slot)
{
    new Handle:hClientMenu = MenuLib_CreateClientListMenu(client, "ZTele menu clients title", true, false, INVALID_FUNCTION, ZTele_ClientMenuHandle, BtnNextMenu_LinkBack, "", UTILS_FILTER_ALIVE, ZTele_ClientMenuFilter);
    MenuLib_SendMenu(hClientMenu, client, hMenu);
}

/**
 * Called for each client added to the menu being generated by MenuLib_GenerateClientMenu. 
 *
 * @param menu          Menu handle the clients are being added to.
 * @param client        Index of client that is about to be added to the menu.
 * @param buttontxt     The text that will be shown for this client's button.
 *                      Default is just the client's in-game name.
 * @param info          The button's information string.  Be careful modifying this, it will break MenuLib_GetClientIndex so you will have to interpret this data yourself.
 *
 * @return              Follows rules of a normal hook.  (Plugin_Stop will behave the same as Plugin_Handled)
 */
public Action:ZTele_ClientMenuFilter(Handle:menu, client, String:buttontxt[], String:buttoninfo[])
{
    // Tells the client this client is in the processes of ZTele'ing.
    if (g_hZTeleTimer[client] != INVALID_HANDLE)
    {
        Format(buttontxt, 256, "%s (%T)", buttontxt, "ZTele teleporting", client);
        return Plugin_Changed;
    }
    return Plugin_Continue;
}

/**
 * Menu callback
 * Called when for all MenuAction actions except voting.  See menus.inc.
 * 
 * @param menu      The menulib menu handle.
 * @param action    Action client is doing in menu.  May be any action except voting ones.  MenuAction_Select can be handled in other callbacks.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public ZTele_ClientMenuHandle(Handle:hMenu, MenuAction:action, client, slot)
{
    if (action == MenuAction_Select)
    {
        // Get the client index of the selected client.
        new target = MenuLib_GetClientIndex(hMenu, slot);
        
        // Target is no longer available.
        if (target == 0 || !IsPlayerAlive(target))
            return;
        
        // ZTele the client.
        Util_CloseHandle(g_hZTeleTimer[target]); // Stop the countdown if they were already ztele'ing.
        ZTele_StartTeleport(target, 0);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZTele command successful", target);
        LogMgr_Print(g_moduleZTele, LogType_Normal, "ZTele Command", "\"%L\" teleported \"%L\" to spawn.", client, target);
    }
}

/**
 * Command callback (zr_ztele_force)
 * Force a client to ZTele.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:ZTele_ForceCommand(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleZTele))
        return Plugin_Continue;
    
    // Check if client has access.
    if (!AccessMgr_HasAccess(client, g_moduleZTele))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // If not enough arguments given, then stop.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZTele command syntax", PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    decl String:target[MAX_NAME_LENGTH], String:targetname[MAX_NAME_LENGTH];
    new targets[MAXPLAYERS], bool:tn_is_ml, result;
    
    // Get targetname.
    GetCmdArg(1, target, sizeof(target));
    
    // Find a target.
    result = ProcessTargetString(target, client, targets, sizeof(targets), COMMAND_FILTER_ALIVE, targetname, sizeof(targetname), tn_is_ml);
        
    // Check if there was a problem finding a client.
    if (result <= 0)
    {
        TransMgr_ReplyToTargetError(client, result);
        return Plugin_Handled;
    }
    
    for (new tindex = 0; tindex < result; tindex++)
    {
        // Stop the countdown if they were already ztele'ing.
        Util_CloseHandle(g_hZTeleTimer[targets[tindex]]);
        
        ZTele_StartTeleport(targets[tindex], 0);
        
        // Tell admin the outcome of the command if only 1 client was targetted.
        if (result == 1)
        {
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZTele command successful", targets[tindex]);
        }
        
        LogMgr_Print(g_moduleZTele, LogType_Normal, "ZTele Command", "\"%L\" teleported \"%L\" to spawn.", client, targets[tindex]);
    }
    
    return Plugin_Handled;
}
