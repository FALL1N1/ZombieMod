/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          respawn.inc
 *  Type:          Module
 *  Description:   Respawns players back into the game.
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
#include "zr/libraries/menulib"

/**
 * This module's identifier.
 */
new Module:g_moduleRespawn;

/**
 * Cvar handles.
 */
new Handle:g_hCvarRespawn;
new Handle:g_hCvarRespawnDelay;
new Handle:g_hCvarRespawnCountdown;
new Handle:g_hCvarRespawnTeam;
new Handle:g_hCvarRespawnLimit;
new Handle:g_hCvarRespawnZombieIfSuicide;

/**
 * Respawn's button indexes in menus.
 */
new g_iRespawnZAdminButton;

/**
 * Respawn menus.
 */
new Handle:hRespawnMainMenu;
new Handle:hRespawnTeamMenu;

/**
 * These represent menu paths for collecting values to respawn a client.
 */
enum RespawnMenuPaths
{
    RespawnMenuPath_Clients,    /** Choose client -> Choose team */
    RespawnMenuPath_Team        /** Choose team -> Choose client */
}

/**
 * These represent what team a client selected in the respawn menu.
 */
enum RespawnMenuTeam
{
    RespawnMenuTeam_Cvar,       /** Cvar-defined team. */
    RespawnMenuTeam_Current,    /** Respawn on client's current team. */
    RespawnMenuTeam_Zombies,    /** Respawn as zombie. */
    RespawnMenuTeam_Humans      /** Respawn as human. */
}

/**
 * Store values that depend on what a client chooses in a menu.
 */
new RespawnMenuPaths:g_RespawnMenuPath[MAXPLAYERS + 1];
new RespawnMenuTeam:g_RespawnMenuTeam[MAXPLAYERS + 1];
new g_iRespawnMenuClient[MAXPLAYERS + 1];

/**
 * Per-client respawn timer handles.
 */
new Handle:g_hRespawnTimer[MAXPLAYERS + 1];

/**
 * Number of respawns in the current round.
 */
new g_iRespawnCount[MAXPLAYERS + 1];

/**
 * Handle to store countdown timers.
 */
new Handle:g_hRespawnCountdownTimer[MAXPLAYERS + 1];

/**
 * Keeps track of players who suicide
 */
new bool:g_bDeathBySuicide[MAXPLAYERS + 1];

/**
 * Register this module.
 */
Respawn_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Respawn");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "respawn");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Respawns players back into the game.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleRespawn = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnEventsRegister",           "Respawn_OnEventsRegister");
}

/**
 * Register all events here.
 */
public Respawn_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnZAdminCreated",            "Respawn_OnZAdminCreated");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnZAdminDeleted",            "Respawn_OnZAdminDeleted");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnMyModuleEnable",           "Respawn_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnMyModuleDisable",          "Respawn_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnClientPutInServer",        "Respawn_OnClientPutInServer");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_OnClientDisconnect",         "Respawn_OnClientDisconnect");
    
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_RoundStart",                 "Respawn_RoundStart");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_RoundEnd",                   "Respawn_RoundEnd");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_PlayerSpawn",                "Respawn_PlayerSpawn");
    EventMgr_RegisterEvent(g_moduleRespawn, "Event_PlayerDeath",                "Respawn_PlayerDeath");
}

/**
 * Plugin is loading.
 */
Respawn_OnPluginStart()
{
    // Register the module.
    Respawn_Register();
    
    // Create cvars.
    g_hCvarRespawn = Project_CreateConVar("respawn", "0", "Players will respawn after they are killed.");
    g_hCvarRespawnDelay = Project_CreateConVar("respawn_delay", "5", "Time player will wait after dying before they respawn.");
    g_hCvarRespawnCountdown = Project_CreateConVar("respawn_countdown", "1", "Show a countdown of when the player will spawn again.");
    g_hCvarRespawnTeam = Project_CreateConVar("respawn_team", "zombie", "Team to respawn player on. ['zombie' = zombie team | 'human' = human team]");
    g_hCvarRespawnLimit = Project_CreateConVar("respawn_limit", "0", "Max amount of respawns per round. ['0' = Unlimited respawns]");
    g_hCvarRespawnZombieIfSuicide = Project_CreateConVar("respawn_zombie_if_suicide", "1", "Respawn player as a zombie if player suicides regardless of other settings.");
    
    // Create commands.
    Project_RegConsoleCmd("spawn", Respawn_Command, "Respawn a client on a given team.  Usage: <prefix>_spawn <client> <zombie|human>");
}

/**
 * Called when ZAdmin is created.
 * 
 * @param hMenu     Handle to ZAdmin.
 */
public Respawn_OnZAdminCreated(Handle:hMenu)
{
    // Add Respawn's button to the menu.
    g_iRespawnZAdminButton = MenuLib_AddMenuBtnEx(hMenu, "Respawn zadmin button", "", true, ITEMDRAW_DEFAULT, INVALID_FUNCTION, BtnNextMenu_LinkMenu, "respawn_main");
    
    Respawn_CreateMenus();
}

/**
 * Called when ZAdmin is deleted.
 * 
 * @param hMenu     Handle to ZAdmin.
 */
public Respawn_OnZAdminDeleted(Handle:hMenu)
{
    Respawn_DeleteMenus();
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:Respawn_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    new Handle:hZAdmin = MenuLib_FindMenuById("zadmin");
    if (hZAdmin != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZAdmin, g_iRespawnZAdminButton, MenuBtn_Style, ITEMDRAW_DEFAULT);
    
    Respawn_CreateMenus();
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:Respawn_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    new Handle:hZAdmin = MenuLib_FindMenuById("zadmin");
    if (hZAdmin != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZAdmin, g_iRespawnZAdminButton, MenuBtn_Style, ITEMDRAW_DISABLED);
    
    Respawn_DeleteMenus();
}

Respawn_CreateMenus()
{
    // Build respawn menus.
    
    // Main respawn menu.
    hRespawnMainMenu = MenuLib_CreateMenu("respawn_main", INVALID_FUNCTION, INVALID_FUNCTION, "Respawn menu main title", true, false, false);
    MenuLib_AddMenuBtnEx(hRespawnMainMenu, "Respawn menu main choose client", "", true, ITEMDRAW_DEFAULT, Respawn_BtnSendClientMenu, BtnNextMenu_None, "");
    MenuLib_AddMenuBtnEx(hRespawnMainMenu, "Respawn menu main choose team", "", true, ITEMDRAW_DEFAULT, Respawn_BtnSendTeamMenu, BtnNextMenu_None, "");
    
    // Choose team.
    hRespawnTeamMenu = MenuLib_CreateMenu("respawn_team", Respawn_ChooseTeamPreSend, Respawn_ChooseTeamHandle, "", false, true, false);
    MenuLib_AddMenuBtnEx(hRespawnTeamMenu, "", "", false, ITEMDRAW_DEFAULT, INVALID_FUNCTION, BtnNextMenu_None, "");
    MenuLib_AddMenuBtnEx(hRespawnTeamMenu, "Respawn menu choose team current", "", true, ITEMDRAW_DEFAULT, INVALID_FUNCTION, BtnNextMenu_None, "");
    MenuLib_AddMenuBtnEx(hRespawnTeamMenu, "Respawn menu choose team zombie", "", true, ITEMDRAW_DEFAULT, INVALID_FUNCTION, BtnNextMenu_None, "");
    MenuLib_AddMenuBtnEx(hRespawnTeamMenu, "Respawn menu choose team human", "", true, ITEMDRAW_DEFAULT, INVALID_FUNCTION, BtnNextMenu_None, "");
}

Respawn_DeleteMenus()
{
    // Delete respawn menus.
    MenuLib_DeleteMenu(hRespawnMainMenu);
    MenuLib_DeleteMenu(hRespawnTeamMenu);
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public Respawn_OnClientPutInServer(client)
{
    g_hRespawnTimer[client] = INVALID_HANDLE;
    g_hRespawnCountdownTimer[client] = INVALID_HANDLE;
    g_iRespawnCount[client] = 0;
    g_bDeathBySuicide[client] = false;
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public Respawn_OnClientDisconnect(client)
{
    Util_CloseHandle(g_hRespawnTimer[client]);
    Util_CloseHandle(g_hRespawnCountdownTimer[client]);
}

/**
 * Round has started.
 */
public Respawn_RoundStart()
{
    // Reset respawn counts.
    for (new client = 1; client <= MaxClients; client++)
    {
        g_iRespawnCount[client] = 0;
    }
}

/**
 * Round has ended.
 * 
 * @param winner    The index of the winning team.
 */
public Respawn_RoundEnd(winner)
{
    for (new client = 1; client <= MaxClients; client++)
    {
        if (IsClientInGame(client))
        {
            Util_CloseHandle(g_hRespawnTimer[client]);
            Util_CloseHandle(g_hRespawnCountdownTimer[client]);
        }
    }
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 */
public Respawn_PlayerSpawn(client)
{
    // Reset this variable since they're alive now.
    g_bDeathBySuicide[client] = false;
    
    // Kill the timer that respawns them and the countdown timer.
    Util_CloseHandle(g_hRespawnTimer[client]);
    Util_CloseHandle(g_hRespawnCountdownTimer[client]);
}

/**
 * Client has been killed.
 * 
 * @param victim    The index of the killed client.
 * @param attacker  The killer of the victim.
 * @param weapon    The weapon classname used to kill the victim. (No weapon_ prefix)
 * @param headshot  True if the death was by headshot, false if not.
 */
public Respawn_PlayerDeath(victim, attacker, const String:weapon[], bool:headshot)
{
    // Don't do anything if it's just a client being infected.
    if (Interface_IsImplemented(g_IInfectionGetInfectWeapon))
    {
        decl String:infectweapon[64];
        IInfection_GetInfectWeapon(infectweapon, sizeof(infectweapon));
        if (StrEqual(infectweapon, weapon, false))
            return;
    }
    
    if (TeamMgr_IsClientZombie(victim))
        g_bDeathBySuicide[victim] = (attacker < 0 || attacker > MaxClients);
    
    // Stop the timer just in case it's running. (it never should at this point.)
    Util_CloseHandle(g_hRespawnTimer[victim]);
    Util_CloseHandle(g_hRespawnCountdownTimer[victim]);
    
    // Start timer to respawn them if enabled.
    if (GetConVarBool(g_hCvarRespawn))
    {
        new respawn_limit = GetConVarInt(g_hCvarRespawnLimit);
        if (respawn_limit == 0 || g_iRespawnCount[victim] < respawn_limit)
        {
            new delay = GetConVarInt(g_hCvarRespawnDelay);
            g_hRespawnTimer[victim] = CreateTimer(float(delay), Respawn_Timer, victim, TIMER_FLAG_NO_MAPCHANGE);
            
            // Start countdown if enabled.
            if (GetConVarBool(g_hCvarRespawnCountdown))
            {
                // Create a pack of data to track countdown.
                new Handle:hCountdownData = CreateDataPack();
                WritePackCell(hCountdownData, victim);
                WritePackCell(hCountdownData, 0);
                WritePackCell(hCountdownData, delay);
                g_hRespawnCountdownTimer[victim] = CreateTimer(1.0, Respawn_Countdown, hCountdownData, TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE|TIMER_DATA_HNDL_CLOSE);
                TransMgr_PrintText(victim, MsgFormat_None, MsgType_Center, g_moduleRespawn, false, "Respawn countdown", delay);
            }
        }
    }
}

/**
 * Respawn a client on a given team or their current team.
 * 
 * @param client    The client to respawn.
 * @param team      The team to respawn them on.  Anything other than zombie or human means their current team.
 * 
 * @return          True if respawned as zombie or human, false if they couldn't be respawned.
 */
stock bool:Respawn_Client(client, VTeam:team)
{
    // Set the variable to the team this client should respawn on.
    new VTeam:respawnteam;
    if (team == VTeam_Zombie || team == VTeam_Human)
        respawnteam = team;
    else
        respawnteam = TeamMgr_GetClientTeam(client);
    
    if (respawnteam == VTeam_Zombie || respawnteam == VTeam_Human)
    {
        TeamMgr_SpawnOnTeam(client, respawnteam);
        return true;
    }
    return false;
}

/**
 * Timer callback, respawns a client.
 * 
 * @param timer     The timer handle.
 * @param client    The client index.
 */
public Action:Respawn_Timer(Handle:timer, any:client)
{
    g_hRespawnTimer[client] = INVALID_HANDLE;
    
    if (!IsClientInGame(client))
        return;
    
    if (IsPlayerAlive(client))
        return;
    
    if (!TeamMgr_IsClientOnPlayerTeam(client))
        return;
    
    // If the client suicided and the cvar is enabled, they will be respawned as zombie.
    if (GetConVarBool(g_hCvarRespawnZombieIfSuicide) && g_bDeathBySuicide[client])
        TeamMgr_SpawnOnTeam(client, VTeam_Zombie);
    else
    {
        // Update the client's virtual team with the one defined in this cvar.
        decl String:respawn_team[32];
        GetConVarString(g_hCvarRespawnTeam, respawn_team, sizeof(respawn_team));
        new VTeam:team = IGame_VTeamStringToTeam(respawn_team);
        Respawn_Client(client, team);
        
        // Log an error if client was respawned on their current team.
        if (team == VTeam_Invalid)
            LogMgr_Print(g_moduleRespawn, LogType_Error, "Cvar zr_respawn_team", "Invalid value (%s) set in cvar, spawning player on their current team instead.", respawn_team);
    }
    g_iRespawnCount[client]++;  // Increment their spawn count.
    
    // Print number of respawns left if there's a limit.
    new respawn_limit = GetConVarInt(g_hCvarRespawnLimit);
    if (respawn_limit > 0)
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Center, g_moduleRespawn, false, "Respawn spawns left", respawn_limit - g_iRespawnCount[client]);
}

/**
 * Timer callback
 * Executed every second until a client respawns.
 * 
 * @param timer             The timer handle.
 * @param hCountdownData    Stores length of the timer and seconds that have passed.
 */
public Action:Respawn_Countdown(Handle:timer, Handle:hCountdownData)
{
    // Read the info from the datapack.
    ResetPack(hCountdownData);
    new client = ReadPackCell(hCountdownData);
    if (!IsClientInGame(client))
    {
        g_hRespawnCountdownTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }
    
    if (IsPlayerAlive(client))
    {
        g_hRespawnCountdownTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }
    
    if (!TeamMgr_IsClientOnPlayerTeam(client))
    {
        g_hRespawnCountdownTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }
    
    new counter = ReadPackCell(hCountdownData);
    new total = ReadPackCell(hCountdownData);
    
    counter += 1; // Increment.
    
    // Print if the counter hasn't reached the end.
    if (counter <= total)
    {
        TransMgr_PrintText(client, MsgFormat_None, MsgType_Center, g_moduleRespawn, false, "Respawn countdown", total - counter);
    }
    
    // Stop if the countdown is finished.
    if (counter >= total)
    {
        g_hRespawnCountdownTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }
    
    // Write the new counter value to the datapack.
    ResetPack(hCountdownData);
    WritePackCell(hCountdownData, client);
    WritePackCell(hCountdownData, counter);
    
    return Plugin_Continue;
}

/**
 * Command callback: zr_(re)spawn
 * Spawn a client on a given team.
 * 
 * @param client    The client index. 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:Respawn_Command(client, argc)
{
    // Check if the this core module is disabled, if so then don't do anything with it.
    if (ModuleMgr_IsDisabled(g_moduleRespawn))
        return Plugin_Continue;
    
    // Check if the client has permission to use this.
    if (!AccessMgr_HasAccess(client, g_moduleRespawn))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // If not enough arguments given, then stop.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Respawn command syntax", PROJECT_CMD_PREFIX);
        return Plugin_Handled;
    }
    
    decl String:target[MAX_NAME_LENGTH], String:targetname[MAX_NAME_LENGTH];
    new targets[MAXPLAYERS], bool:tn_is_ml, result;
    
    // Get targetname.
    GetCmdArg(1, target, sizeof(target));
    
    // Find a target.
    result = ProcessTargetString(target, client, targets, sizeof(targets), COMMAND_FILTER_DEAD, targetname, sizeof(targetname), tn_is_ml);
    
    // Check if there was a problem finding a client.
    if (result <= 0)
    {
        TransMgr_ReplyToTargetError(client, result);
        return Plugin_Handled;
    }
    
    // Check optional parameters.
    decl String:strTeam[32];
    GetCmdArg(2, strTeam, sizeof(strTeam));
    new VTeam:team = VTeam_Invalid;
    if (strTeam[0])
    {
        team = VTeam:IGame_VTeamStringToTeam(strTeam);
        if (team == VTeam_Invalid)
        {
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Respawn command invalid team");
            return Plugin_Handled;
        }
    }
    
    for (new tindex = 0; tindex < result; tindex++)
    {
        // Respawn targets.
        if (Respawn_Client(targets[tindex], team))
        {
            if (result == 1)
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Respawn command success", targets[tindex]);
        }
        else
        {
            if(result == 1)
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Respawn command fail", targets[tindex]);
        }
    }
    
    return Plugin_Handled;
}

// **********************************************
//                     Menus
// **********************************************

/**
 * Respawn a client reading from data received via menu.
 * 
 * @param client    Client to respawn.
 * @param team      The team selected from menu.
 */
stock Respawn_MenuRespawnClient(client, RespawnMenuTeam:menu_team)
{
    switch(menu_team)
    {
        case RespawnMenuTeam_Cvar:
        {
            decl String:respawn_team[32];
            GetConVarString(g_hCvarRespawnTeam, respawn_team, sizeof(respawn_team));
            new VTeam:team = IGame_VTeamStringToTeam(respawn_team);
            Respawn_Client(client, team);
            if (team == VTeam_Invalid)
                LogMgr_Print(g_moduleRespawn, LogType_Error, "Cvar zr_respawn_team", "Invalid value (%s) set in cvar, spawning player on their current team instead.", respawn_team);
        }
        case RespawnMenuTeam_Current:
        {
            Respawn_Client(client, VTeam_Invalid);
        }
        case RespawnMenuTeam_Zombies:
        {
            Respawn_Client(client, VTeam_Zombie);
        }
        case RespawnMenuTeam_Humans:
        {
            Respawn_Client(client, VTeam_Human);
        }
    }
}

/**
 * Menu callback
 * Called when a certain button in a menu is pressed.  The menu action is always MenuAction_Select.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public Respawn_BtnSendClientMenu(Handle:hMenu, client, slot)
{
    g_RespawnMenuPath[client] = RespawnMenuPath_Clients;
    Respawn_SendClientMenu(client, hMenu);
}

/**
 * Menu callback
 * Called when a certain button in a menu is pressed.  The menu action is always MenuAction_Select.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public Respawn_BtnSendTeamMenu(Handle:hMenu, client, slot)
{
    g_RespawnMenuPath[client] = RespawnMenuPath_Team;
    MenuLib_SendMenu(hRespawnTeamMenu, client, hMenu);
}

/**
 * Send a menu of a clients to a client.
 * 
 * @param client    The client to send menu to.
 * @param hMenu     Menu to link back to. 
 */
stock Respawn_SendClientMenu(client, Handle:hMenu = INVALID_HANDLE)
{
    new Handle:hClientMenu = MenuLib_CreateClientListMenu(client, "", false, false, Respawn_ClientMenuPreSend, Respawn_ClientMenuHandle, BtnNextMenu_None, "", UTILS_FILTER_TEAM, Respawn_ClientMenuFilter);
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
public Action:Respawn_ClientMenuFilter(Handle:menu, client, String:buttontxt[], String:buttoninfo[])
{
    if (g_RespawnMenuPath[client] == RespawnMenuPath_Clients || g_RespawnMenuTeam[client] != RespawnMenuTeam_Current)
    {
        // Tells if the client is dead or alive.
        if (IsPlayerAlive(client))  Format(buttontxt, 256, "%s (%T)", buttontxt, "ZR alive", client);
        else                        Format(buttontxt, 256, "%s (%T)", buttontxt, "ZR dead", client);
    }
    else
    {
        PrintToChatAll("TLib_GetClientTeam %d", TeamMgr_GetClientTeam(client));
        if (TeamMgr_IsClientZombie(client))     Format(buttontxt, 256, "%s (%T)", buttontxt, "ZR team zombie", client);
        else if (TeamMgr_IsClientHuman(client)) Format(buttontxt, 256, "%s (%T)", buttontxt, "ZR team human", client);
    }
    return Plugin_Changed;
}

/**
 * Menu callback
 * Called right before the menu is displayed to a client.  Menu data changed here will show up in the menu.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 */
public Respawn_ClientMenuPreSend(Handle:hMenu, client)
{
    new String:title[128];
    decl String:team[32];
    switch(g_RespawnMenuPath[client])
    {
        case RespawnMenuPath_Clients:
        {
            Format(title, sizeof(title), "%T", "Respawn menu clients title 1", client);
        }
        case RespawnMenuPath_Team:
        {
            if (g_RespawnMenuTeam[client] == RespawnMenuTeam_Cvar)
            {
                GetConVarString(g_hCvarRespawnTeam, team, sizeof(team));
                Format(title, sizeof(title), "%T", "Respawn menu clients title 2", client, team);
            }
            else if (g_RespawnMenuTeam[client] == RespawnMenuTeam_Current)
                Format(title, sizeof(title), "%T", "Respawn menu clients title 1", client, team);
            else if (g_RespawnMenuTeam[client] == RespawnMenuTeam_Zombies)
                Format(title, sizeof(title), "%T", "Respawn menu clients title 3", client);
            else if (g_RespawnMenuTeam[client] == RespawnMenuTeam_Humans)
                Format(title, sizeof(title), "%T", "Respawn menu clients title 4", client);
        }
    }
    MenuLib_MenuWriteString(hMenu, MenuData_Title, ML_DATA_TITLE, title);
}

/**
 * Menu callback (respawn)
 * Respawns a client.
 * 
 * @param menu      The menu handle.
 * @param action    Action client is doing in menu.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public Respawn_ClientMenuHandle(Handle:hMenu, MenuAction:action, client, slot)
{
    if (action == MenuAction_Select)
    {
        // Get the client index of the selected client.
        new target = MenuLib_GetClientIndex(hMenu, slot);
        
        // Target is no longer available.
        if (target == 0 || !IsPlayerAlive(target))
        {
            Respawn_SendClientMenu(client);
            return;
        }
        
        switch(g_RespawnMenuPath[client])
        {
            case RespawnMenuPath_Clients:
            {
                g_iRespawnMenuClient[client] = target;
                MenuLib_SendMenu(hRespawnTeamMenu, client);
            }
            case RespawnMenuPath_Team:
            {
                Respawn_SendClientMenu(client);
                Respawn_MenuRespawnClient(target, g_RespawnMenuTeam[client]);
            }
        }
    }
}

/**
 * Menu callback
 * Called right before the menu is displayed to a client.  Menu data changed here will show up in the menu.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 */
public Respawn_ChooseTeamPreSend(Handle:hMenu, client)
{
    // Update label that shows cvar value.
    new String:label[64], String:respawn_team[32];
    GetConVarString(g_hCvarRespawnTeam, respawn_team, sizeof(respawn_team));
    Format(label, sizeof(label), "%T", "Respawn menu choose team cvar", client, respawn_team);
    MenuLib_BtnWriteString(hMenu, 0, MenuBtn_Label, ML_DATA_LABEL, label);
    
    new String:title[128];
    switch(g_RespawnMenuPath[client])
    {
        case RespawnMenuPath_Clients:
        {
            Format(title, sizeof(title), "%T", "Respawn menu choose team title 2", client, g_iRespawnMenuClient[client]);
        }
        case RespawnMenuPath_Team:
        {
            Format(title, sizeof(title), "%T", "Respawn menu choose team title 1", client);
        }
    }
    MenuLib_MenuWriteString(hMenu, MenuData_Title, ML_DATA_TITLE, title);
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
public Respawn_ChooseTeamHandle(Handle:hMenu, MenuAction:action, client, slot)
{
    if (action == MenuAction_Select)
    {
        g_RespawnMenuTeam[client] = RespawnMenuTeam:slot;
        
        // Respawn the client
        if (g_RespawnMenuPath[client] == RespawnMenuPath_Clients)
        {
            Respawn_MenuRespawnClient(g_iRespawnMenuClient[client], g_RespawnMenuTeam[client]);
            MenuLib_SendMenu(hMenu, client);
        }
        else if (g_RespawnMenuPath[client] == RespawnMenuPath_Team)
            Respawn_SendClientMenu(client, hMenu);
    }
    else if (action == MenuAction_Cancel)
    {
        if (slot == MenuCancel_ExitBack)
        {
            if (g_RespawnMenuPath[client] == RespawnMenuPath_Clients)
                Respawn_SendClientMenu(client, INVALID_HANDLE);
            else if (g_RespawnMenuPath[client] == RespawnMenuPath_Team)
                MenuLib_MenuGoBack(client);
        }
    }
}
