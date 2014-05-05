/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:           zr_core.inc
 *  Type:           Module
 *  Description:    Game mode-independent stuff. Detects game mod and enables
 *                  appropriate mod adapter for access to mod specific features.
 *
 *                  Provides abstract common game events (round_start/end,
 *                  player_death, etc).
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
new Module:g_moduleZRCore;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRCore_GetIdentifier() { return g_moduleZRCore; }

/**
 * Current game mod. (Enum type defined in game interface.)
 */
new Game:g_Game = Game_Unknown;

/*____________________________________________________________________________*/

/**
 * Generic zombie gameplay events that cores can share.
 */
new ProjectEvent:g_EvOnClientInfected;
new ProjectEvent:g_EvOnClientHuman;

/*____________________________________________________________________________*/

/**
 * Register this module.
 */
ZRCore_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRCore");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "core");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Common events and functions.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZRCore = ModuleMgr_Register(moduledata);
    
    // Create events.
    ZRCore_CreateCommonEvents();
    
    // Register the OnEventsRegister event to register all events in it.
    //EventMgr_RegisterEvent(g_moduleZRCore, "Event_OnEventsRegister",            "ZRCore_OnEventsRegister");
}

/*____________________________________________________________________________*/

/**
 * Register all events here.
 */
public ZRCore_OnEventsRegister()
{
    // Register all the events needed for this module.
    // (None)
}

/*____________________________________________________________________________*/

/**
 * Plugin is loading.
 */
ZRCore_OnPluginStart()
{
    // Get mod and check if supported. A warning is logged if not.
    ZRCore_DetectMod();
    
    // Register the module.
    ZRCore_Register();
    
    // Forward OnPluginStart to appropriate mod adapter module.
    ZRCore_InitializeAdapter();
}

/*____________________________________________________________________________*/

/**
 * Creates common events used in ZR.
 *
 * These are events that are common in mods, but slightly different. Or events
 * that doesn't exist in the base Source engine.
 *
 * Most of the events are based on CS:S, but abstracted or simplified so modules
 * can rely on a common set of events instead of mod specific events.
 *
 * The actual event implementation is done in mod adapter modules. They are
 * responsible for hooking and forwarding them.
 */
ZRCore_CreateCommonEvents()
{
    // Implemented in active infection core module.
    g_EvOnClientInfected =      EventMgr_CreateEvent("Event_OnClientInfected");         /** Client has become a zombie. */
    g_EvOnClientHuman =         EventMgr_CreateEvent("Event_OnClientHuman");            /** Client has become a human. */
    
    // Implemented in active mod adapter module.
    g_EvRoundStart =            EventMgr_CreateEvent("Event_RoundStart");               /** Round has started. */
    g_EvRoundFreezeEnd =        EventMgr_CreateEvent("Event_RoundFreezeEnd");           /** Pre-round freezetime has finished. */
    g_EvRoundEnd =              EventMgr_CreateEvent("Event_RoundEnd");                 /** Round has ended. */
    g_EvPlayerTeamPre =         EventMgr_CreateEvent("Event_PlayerTeamPre");            /** Client has joined a team. (pre) */
    g_EvPlayerTeam =            EventMgr_CreateEvent("Event_PlayerTeam");               /** Client has joined a team. */
    g_EvPlayerSpawn =           EventMgr_CreateEvent("Event_PlayerSpawn");              /** Client has spawned. */
    g_EvPlayerSpawnPost =       EventMgr_CreateEvent("Event_PlayerSpawnPost");          /** Client has spawned (delayed event). */
    g_EvPlayerHurt =            EventMgr_CreateEvent("Event_PlayerHurt");               /** Client has been damaged. */
    g_EvPlayerDeath =           EventMgr_CreateEvent("Event_PlayerDeath");              /** Client has been killed. */
    g_EvPlayerJump =            EventMgr_CreateEvent("Event_PlayerJump");               /** Client has jumped. */
    g_EvWeaponFire =            EventMgr_CreateEvent("Event_WeaponFire");               /** Client has fired a weapon. */
    //g_EvWeaponEntityFire =      EventMgr_CreateEvent("Event_WeaponEntityFire");         /** Client has fired a weapon. (returns weapon entity index) */
}

/*____________________________________________________________________________*/

/**
 * Forwards OnPluginStart to the mod adapter module matching current game mod.
 * 
 * @error   Missing mod adapter, stopping plugin.
 */
ZRCore_InitializeAdapter()
{
    switch (g_Game)
    {
        case Game_CSS:
        {
            CSSAdapter_OnPluginStart();
        }
        /*case Game_CSGO:
        {
            CSGOAdapter_OnPluginStart();
        }
        case Game_CSTF2:
        {
            TF2Adapter_OnPluginStart();
        }*/
        default:
        {
            LogMgr_Print(g_moduleZRCore, LogType_Fatal_Plugin, "Game Mod Adapter", "Zombie:Reloaded doesn't have a mod adapter module for this unsupported game. Refusing to start, continuing would cause errors or crashes.");
        }
    }
}

/*____________________________________________________________________________*/

/**
 * Converts a game folder name to a game ID.
 *
 * @param gameFolder        Game string to convert.
 *
 * @return                  Game ID, or Game_Unknown on error.
 */
Game:ZRCore_StringToGame(const String:gameFolder[])
{
    // Check for supported games.
    if (StrEqual(gameFolder, "cstrike", false))
    {
        return Game_CSS;
    }
    else if (StrEqual(gameFolder, "csgo", false))
    {
        return Game_CSGO;
    }
    else if (StrEqual(gameFolder, "tf", false))
    {
        return Game_TF2;
    }
    
    return Game_Unknown;
}

/*____________________________________________________________________________*/

/**
 * Detects game mod and updates g_Game. Will log a warning if an unsupported
 * game is detected.
 */
ZRCore_DetectMod()
{
    new String:gameFolder[PLATFORM_MAX_PATH];
    GetGameFolderName(gameFolder, sizeof(gameFolder));
    
    g_Game = ZRCore_StringToGame(gameFolder);
    
    // Check if ZR is running on a unsupported game.
    if (g_Game == Game_Unknown)
    {
        LogMgr_Print(g_moduleZRCore, LogType_Error, "Game Mod Adapter", "Warning: Zombie:Reloaded doesn't support this game: %s", gameFolder);
    }
}
