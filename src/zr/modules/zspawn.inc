/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          zspawn.inc
 *  Type:          Module
 *  Description:   Allows players to spawn into the game late.
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

// Include libraries
#include "zr/libraries/authcachelib"
#include "zr/libraries/menulib"

/**
 * This module's identifier.
 */
new Module:g_moduleZSpawn;

/**
 * Cvar handles.
 */
new Handle:g_hCvarZSpawn;
new Handle:g_hCvarZSpawnTeam;
new Handle:g_hCvarZSpawnTimeLimit;
new Handle:g_hCvarZSpawnTimeLimitRules;

/**
 * ZSpawns's button index in ZMenu.
 */
new g_iZSpawnZMenuButton;

/**
 * Stores the auth cache handle.
 */
new Handle:g_hZSpawnAuthCache;

/**
 * ZSpawn timer handle.
 */
new Handle:g_hZSpawnTimer;

/**
 * Register this module.
 */
ZSpawn_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZSpawn");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zspawn");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Allows players to spawn into the game late.");
    
    // Send this array of data to the module manager.
    g_moduleZSpawn = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_OnEventsRegister",            "ZSpawn_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZSpawn_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_OnZMenuCreated",              "ZSpawn_OnZMenuCreated");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_OnMyModuleEnable",            "ZSpawn_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_OnMyModuleDisable",           "ZSpawn_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_OnMapStart",                  "ZSpawn_OnMapStart");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_OnClientDisconnect",          "ZSpawn_OnClientDisconnect");
    
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_RoundStart",                  "ZSpawn_RoundStart");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_RoundFreezeEnd",              "ZSpawn_RoundFreezeEnd");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_RoundEnd",                    "ZSpawn_RoundEnd");
    EventMgr_RegisterEvent(g_moduleZSpawn, "Event_PlayerDeath",                 "ZSpawn_PlayerDeath");
}

/**
 * Plugin is loading.
 */
ZSpawn_OnPluginStart()
{
    // Register the module.
    ZSpawn_Register();
    
    // Create cvars.
    g_hCvarZSpawn =                 Project_CreateConVar("zspawn", "1", "Allow players to use zspawn command to spawn late into a game.");
    g_hCvarZSpawnTeam =             Project_CreateConVar("zspawn_team", "human", "Team to spawn player on when using ZSpawn. ['0' = Zombie | '1' = Human]");
    g_hCvarZSpawnTimeLimit =        Project_CreateConVar("zspawn_timelimit", "120.0", "The amount of time from round start before ZSpawn changes its behavior. ['0' = No timelimit]");
    g_hCvarZSpawnTimeLimitRules =   Project_CreateConVar("zspawn_timelimit_rules", "1", "How ZSpawn should work after the timelimit has expired. ['-1' = Block ZSpawn | '0' = Spawn as zombie | '1' = Spawn as human | Dependency: zspawn_team_zombie&zspawn_timelimit]");
    
    // Create commands.
    RegConsoleCmd("zspawn", ZSpawn_SayCommand, "Spawn into the game.");
}

/**
 * Called when ZMenu is created.
 * 
 * @param hMenu     Handle to ZMenu.
 */
public ZSpawn_OnZMenuCreated(Handle:hMenu)
{
    // Add ZSpawn's button to the menu.
    g_iZSpawnZMenuButton = MenuLib_AddMenuBtnEx(hMenu, "ZSpawn zmenu button", "0", true, ITEMDRAW_DEFAULT, ZSpawn_ZMenuButton, BtnNextMenu_None, "");
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ZSpawn_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Enable ZTele's button to the menu.
    new Handle:hZMenu = MenuLib_FindMenuById("zmenu");
    if (hZMenu != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZMenu, g_iZSpawnZMenuButton, MenuBtn_Style, ITEMDRAW_DEFAULT);
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ZSpawn_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    Util_CloseHandle(g_hZSpawnTimer);
    
    // Disable ZTele's button to the menu.
    new Handle:hZMenu = MenuLib_FindMenuById("zmenu");
    if (hZMenu != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZMenu, g_iZSpawnZMenuButton, MenuBtn_Style, ITEMDRAW_DISABLED);
}

/**
 * The map has started.
 */
public ZSpawn_OnMapStart()
{
    // Initialize auth cache.
    Util_CloseHandle(g_hZSpawnAuthCache);
    g_hZSpawnAuthCache = AuthCache_Create();
    
    // Reset the timer handle.
    g_hZSpawnTimer = INVALID_HANDLE;
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public ZSpawn_OnClientDisconnect(client)
{
    // If client clicked "cancel" during connection or is a bot, then stop.
    if (!IsClientInGame(client) || IsFakeClient(client))
        return;
    
    // Add client to cache.
    AuthCache_AddClient(g_hZSpawnAuthCache, client);
}

/**
 * Round has started.
 */
public ZSpawn_RoundStart()
{
    // Clear everyone from the cache.
    AuthCache_Clear(g_hZSpawnAuthCache);
    
    // End the timer.
    Util_CloseHandle(g_hZSpawnTimer);
}

/**
 * Pre-round freezetime has finished.
 */
public ZSpawn_RoundFreezeEnd()
{
    // End the timer.
    Util_CloseHandle(g_hZSpawnTimer);
    
    // Get timelimit, and check if it's disabled.
    new Float:timelimit = GetConVarFloat(g_hCvarZSpawnTimeLimit);
    if (timelimit > 0.0)
        g_hZSpawnTimer = CreateTimer(timelimit, ZSpawn_TimeLimitTimer, _, TIMER_FLAG_NO_MAPCHANGE);
}

/**
 * Round has ended.
 * 
 * @param winner    The index of the winning team.
 */
public ZSpawn_RoundEnd(winner)
{
    // End the timer.
    Util_CloseHandle(g_hZSpawnTimer);
}

/**
 * Client has been killed.
 * 
 * @param victim    The index of the killed client.
 * @param attacker  The killer of the victim.
 * @param weapon    The weapon classname used to kill the victim. (No weapon_ prefix)
 * @param headshot  True if the death was by headshot, false if not.
 */
public ZSpawn_PlayerDeath(victim, attacker, const String:weapon[], bool:headshot)
{
    // Add client to cache.
    AuthCache_AddClient(g_hZSpawnAuthCache, victim);
}

/**
 * ZSpawn timer callback.
 * 
 * @param timer     The timer handle.
 */
public Action:ZSpawn_TimeLimitTimer(Handle:timer)
{
    // Clear timer handle variable.
    g_hZSpawnTimer = INVALID_HANDLE;
}

// ************************************************************
//  The functions in this section are ordered chronologically.
// ************************************************************

/**
 * Command callback: zspawn
 * Client command to spawn into the game late.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:ZSpawn_SayCommand(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleZSpawn))
        return Plugin_Continue;
    
    if (!GetConVarBool(g_hCvarZSpawn))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZSpawn, false, "Feature is disabled");
        return Plugin_Handled;
    }
    
    // If client is console, then stop and tell them this feature is for players only.
    if (client == SERVER_INDEX)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZSpawn, false, "Must be player");
        return Plugin_Handled;
    }

    // Check everything before zspawning.
    ZSpawn_Client(client);
    
    return Plugin_Handled;
}

/**
 * Checks everything before zspawning.
 * 
 * @param client    The client index.
 */
bool:ZSpawn_Client(client)
{
    // Check if client is on a team.
    if (!Util_IsClientOnTeam(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZSpawn, false, "Must be on team");
        return false;
    }
    
    // Check if client is alive.
    if (IsPlayerAlive(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZSpawn, false, "Must be dead");
        return false;
    }
    
    // Check if client has already played in this round.
    if (AuthCache_ClientExists(g_hZSpawnAuthCache, client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZSpawn, false, "ZSpawn double spawn");
        return false;
    }
    
    ZSpawn_SpawnClient(client);
    return true;
}

/**
 * ZSpawns a client.
 * 
 * @param client    The client to ZSpawn.
 */
ZSpawn_SpawnClient(client)
{
    // Get respawn team.
    decl String:respawnteam[32];
    GetConVarString(g_hCvarZSpawnTeam, respawnteam, sizeof(respawnteam));
    
    new Float:timelimit = GetConVarFloat(g_hCvarZSpawnTimeLimit);
    if (timelimit == 0 || g_hZSpawnTimer != INVALID_HANDLE)
    {
        new VTeam:team = IGame_VTeamStringToTeam(respawnteam);
        if (team == VTeam_Invalid)
        {
            LogMgr_Print(g_moduleZSpawn, LogType_Error, "Invalid cvar value", "Invalid team \"%s\" defined in cvar \"%s_zspawn_team\".  Assuming human.", respawnteam, PROJECT_CVAR_PREFIX);
            team = VTeam_Human;
        }
        
        TeamMgr_SpawnOnTeam(client, team);
    }
    else
    {
        // Timelimit is expired, new rules.
        new timelimitrules = GetConVarInt(g_hCvarZSpawnTimeLimitRules);
        switch(timelimitrules)
        {
            case -1:
            {
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, g_moduleZSpawn, false, "ZSpawn timelimit", RoundToNearest(timelimit));
            }
            case 0:
            {
                TeamMgr_SpawnOnTeam(client, VTeam_Zombie);
            }
            case 1:
            {
                TeamMgr_SpawnOnTeam(client, VTeam_Human);
            }
            default:
            {
                LogMgr_Print(g_moduleZSpawn, LogType_Error, "Cvar \"zspawn_timelimit_rules\"", "Invalid value \"%d\" defined in this cvar.", timelimitrules);
            }
        }
    }
}

// **********************************************
//          ZMenu ZSpawn button callback
// **********************************************

/**
 * Menu callback
 * Called when a certain button in a menu is pressed.  The menu action is always MenuAction_Select.
 * 
 * @param menu      The menulib menu handle.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
public ZSpawn_ZMenuButton(Handle:hMenu, client, slot)
{
    // Teleport the client, but if it fails then re-resend the menu.
    if (!ZSpawn_Client(client))
        MenuLib_SendMenu(hMenu, client);
}

/**
 * Menu callback (zspawn_force)
 * Forces ZSpawn on a client.
 * 
 * @param menu      The menu handle.
 * @param action    Action client is doing in menu.
 * @param client    The client index.
 * @param slot      The menu slot selected. (starting from 0)
 */
/*
public ZSpawnForceHandle(Handle:menu_zspawn_force, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        // Get the client index of the selected client.
        new target = MenuGetClientIndex(menu_zspawn_force, slot);
        
        // If the target is 0, then the client left before being selected from the menu.
        if (target == 0)
        {
            // Re-send the menu.
            MenuClientList(client, ZSpawnForceHandle, true, true, false, "ZSpawn clients title");
            
            return;
        }
        
        // Get the target's name for future use.
        decl String:targetname[MAX_NAME_LENGTH];
        GetClientName(target, targetname, sizeof(targetname));
        
        // Force ZSpawn on the target.
        new bool:success = ZSpawnClient(target, true);
        
        // Tell admin the outcome of the action.
        if (success)
        {
            TranslationReplyToCommand(client, "ZSpawn command force successful", targetname);
        }
        else
        {
            TranslationReplyToCommand(client, "ZSpawn command force unsuccessful", targetname);
        }
        
        // Re-send the menu.
        MenuClientList(client, ZSpawnForceHandle, true, false, true, "ZSpawn clients title");
    }
    // Client closed the menu.
    if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
        {
            // Re-open admin menu.
            ZAdminMenu(client);
        }
    }
    // Client exited menu.
    if (action == MenuAction_End)
    {
        CloseHandle(menu_zspawn_force);
    }
}
*/
