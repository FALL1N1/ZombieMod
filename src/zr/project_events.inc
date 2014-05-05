/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          project_events.inc
 *  Type:          Base
 *  Description:   Event hooking and definitions.  Any event-related stuff for your project should go here.
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

#include "zr/libraries/offsetlib"

/**
 * The max number of events that the project can have.
 */
#define PROJECT_MAX_EVENTS 128

/**
 * Event tag.
 */
enum ProjectEvent
{
    INVALID_EVENT = -1  /** Used as an invalid event. */
}

/**
 * Global variables to store each event's ID.
 */

// Cross-game events
new ProjectEvent:g_EvOnAllPluginsLoaded;
new ProjectEvent:g_EvOnPluginEnd;
new ProjectEvent:g_EvOnEventsRegister;
new ProjectEvent:g_EvOnEventsReady;
new ProjectEvent:g_EvOnAllModulesLoaded;
new ProjectEvent:g_EvOnModuleEnable;
new ProjectEvent:g_EvOnMyModuleEnable;
new ProjectEvent:g_EvOnModuleDisable;
new ProjectEvent:g_EvOnMyModuleDisable;
new ProjectEvent:g_EvOnMapStart;
new ProjectEvent:g_EvOnMapEnd;
new ProjectEvent:g_EvOnAutoConfigsBuffered;
new ProjectEvent:g_EvOnConfigsExecuted;
new ProjectEvent:g_EvOnClientAlphaChanged;
new ProjectEvent:g_EvOnClientPutInServer;
new ProjectEvent:g_EvOnClientConnected;
new ProjectEvent:g_EvOnClientDisconnect;
new ProjectEvent:g_EvOnClientCookiesCached;
new ProjectEvent:g_EvOnPlayerRunCmd;
new ProjectEvent:g_EvOnClientPostAdminCheck;

// Custom project events.
new ProjectEvent:g_EvOnClientReady;

/**
 * Datatypes that can be passed with each event to modules.
 * Don't touch this!
 */
enum EventDataTypes
{
    DataType_Cell,
    DataType_CellRef,
    DataType_Float,
    DataType_FloatRef,
    DataType_Array,
    DataType_ArrayRef,
    DataType_String,
    DataType_StringRef,
}

/**
 * Common data type info.
 */

// When forwarding custom events with no data, use this.
stock any:g_CommonEventData1[][] = {{0}};

// When forwarding custom events whose data thats being passed matches these, you can use these.
stock EventDataTypes:g_CommonDataType1[] = {};
stock EventDataTypes:g_CommonDataType2[] = {DataType_Cell};

/**
 * Spawn post event delay time, in seconds.
 */
#define PROJECT_SPAWN_POST_DELAY    0.1

/**
 * Saved timestamp from the player spawn event.
 */
new Float:g_fPlayerSpawnTime[MAXPLAYERS + 1] = {-1.0, ...};

/**
 * Whether a post spawn event should be fired.
 */
new bool:g_SendPostSpawnEvent[MAXPLAYERS + 1];

/**
 * Saves states for the OnPlayerReady event.
 */
static bool:g_PlayerAdminChecked[MAXPLAYERS + 1];
static bool:g_PlayerCookiesLoaded[MAXPLAYERS + 1];
static bool:g_PlayerReadyEventFired[MAXPLAYERS + 1];

/**
 * Resets states for the OnPlayerReady event.
 */
static ResetReadyState(client)
{
    g_PlayerAdminChecked[client] = false;
    g_PlayerCookiesLoaded[client] = false;
    g_PlayerReadyEventFired[client] = false;
}

/**
 * Returns whether the player is ready or not.
 *
 * @param client    Client index.
 * @return          True if ready, false otherwise.
 */
static bool:IsClientReady(client)
{
    if (IsClientConnected(client) &&
        g_PlayerAdminChecked[client] &&
        g_PlayerCookiesLoaded[client] &&
        !g_PlayerReadyEventFired[client])
    {
        return true;
    }
    
    return false;
}

/**
 * Fires the ready event if the client is ready.
 *
 * @param client    Client index.
 */
static CheckReadyState(client)
{
    // Fire ready event if ready.
    if (IsClientReady(client))
    {
        // Forward event to all modules.
        new any:eventdata[1][1];
        eventdata[0][0] = client;
        EventMgr_Forward(g_EvOnClientReady, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
        
        g_PlayerReadyEventFired[client] = true;
    }
}

// **********************************************
//            SourceMod Forwards
// **********************************************

/**
 * All plugins have loaded.
 */
public OnAllPluginsLoaded()
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvOnAllPluginsLoaded, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/**
 * The map has started.
 */
public OnMapStart()
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvOnMapStart, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/**
 * The map has ended.
 */
public OnMapEnd()
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvOnMapEnd, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/**
 * This is called before OnConfigsExecuted but any time after OnMapStart.
 * Per-map settings should be set here. 
 */
public OnAutoConfigsBuffered()
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvOnAutoConfigsBuffered, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/**
 * All convars are set, cvar-dependent code should use this.
 */
public OnConfigsExecuted()
{
    // Forward event to all modules.
    EventMgr_Forward(g_EvOnConfigsExecuted, g_CommonEventData1, 0, 0, g_CommonDataType1);
}

/**
 * Fires every game frame.
 */
public OnGameFrame()
{
    new Float:gameTime = GetGameTime();
    
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
            continue;
        
        /*************************
         *   OnPlayerSpawnPost   *
         *************************/
        // Forward spawn post event if flag for this client is set.
        if (g_SendPostSpawnEvent[client]
            && gameTime - PROJECT_SPAWN_POST_DELAY >= g_fPlayerSpawnTime[client])
        {
            new any:eventdata2[1][1];
            eventdata2[0][0] = client;
            EventMgr_Forward(g_EvPlayerSpawnPost, eventdata2, sizeof(eventdata2), sizeof(eventdata2[]), g_CommonDataType2);
            
            // Reset event flag for this client.
            g_SendPostSpawnEvent[client] = false;
        }
    }
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public OnClientPutInServer(client)
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientPutInServer, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
}

/**
 * Client successfully connected.
 *
 * @param client    The client index.
 */
public OnClientConnected(client)
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientConnected, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Check if ready to fire OnClientReady event.
    CheckReadyState(client);
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public OnClientDisconnect(client)
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientDisconnect, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Clean up after OnClientReady event.
    ResetReadyState(client);
}

/**
 * A client's cookies have been cached from the database.
 * 
 * @param client    The client index.
 */
public OnClientCookiesCached(client)
{
    // This forward is called when clients connect and disconnect, so filter the bad calls out here.
    if (!IsClientConnected(client))
        return;
    
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientCookiesCached, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Check if ready to fire OnClientReady event.
    g_PlayerCookiesLoaded[client] = true;
    CheckReadyState(client);
}

/**
 * Client is authorized.
 *
 * @param client    The client index.
 */
public OnClientPostAdminCheck(client)
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientPostAdminCheck, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Check if ready to fire OnClientReady event.
    g_PlayerAdminChecked[client] = true;
    CheckReadyState(client);
}

/**
 * @brief Called when a clients movement buttons are being processed
 *
 * @param client	Index of the client.
 * @param buttons	Copyback buffer containing the current commands (as bitflags - see entity_prop_stocks.inc).
 * @param impulse	Copyback buffer containing the current impulse command.
 * @param vel		Players desired velocity.
 * @param angles	Players desired view angles.
 * @param weapon	Entity index of the new weapon if player switches weapon, 0 otherwise.
 *  
 * @return 			Plugin_Handled to block the commands from being processed, Plugin_Continue otherwise.
 */
public Action:OnPlayerRunCmd(client, &buttons, &impulse, Float:vel[3], Float:angles[3], &weapon)
{
    if (!IsClientInGame(client))
        return Plugin_Continue;
    
    // Forward event to all modules.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_CellRef, DataType_CellRef, DataType_ArrayRef, DataType_ArrayRef, DataType_CellRef};
    new any:eventdata[sizeof(eventdatatypes)][3];
    
    eventdata[0][0] = client;
    eventdata[1][0] = buttons;
    eventdata[2][0] = impulse;
    for (new x = 0; x < 3; x++)
        eventdata[3][x] = vel[x];
    for (new x = 0; x < 3; x++)
        eventdata[4][x] = angles[x];
    eventdata[5][0] = weapon;
    
    new Action:result = EventMgr_Forward(g_EvOnPlayerRunCmd, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
    
    // Copy event data back to this hook's variables, to let the engine use the new values.
    client = eventdata[0][0];
    buttons = eventdata[1][0];
    impulse = eventdata[2][0];
    for (new x = 0; x < 3; x++)
        vel[x] = eventdata[3][x];
    for (new x = 0; x < 3; x++)
        angles[x] = eventdata[4][x];
    weapon = eventdata[5][0];
    
    return result;
}

// **********************************************
//             Game Event Creating
// **********************************************

/**
 * This is called from the event manager in OnPluginStart.
 * Create the events you want to forward to modules here.
 */
stock EventMgr_CreateEvents()
{
    // Hook events to manage and forward to modules.
    
    // Cross-game events.
    g_EvOnAllPluginsLoaded =        EventMgr_CreateEvent("Event_OnAllPluginsLoaded");       /** All plugins have loaded. */
    g_EvOnPluginEnd =               EventMgr_CreateEvent("Event_OnPluginEnd");              /** Plugin is ending. */
    g_EvOnEventsRegister =          EventMgr_CreateEvent("Event_OnEventsRegister");         /** All events are created, modules should register them here. */
    g_EvOnEventsReady =             EventMgr_CreateEvent("Event_OnEventsReady");            /** All events are created AND registered by here.  Event priority can be changed here. */
    g_EvOnAllModulesLoaded =        EventMgr_CreateEvent("Event_OnAllModulesLoaded");       /** All modules have been registered. */
    g_EvOnModuleEnable =            EventMgr_CreateEvent("Event_OnModuleEnable");           /** A module has been enabled. */
    g_EvOnMyModuleEnable =          EventMgr_CreateEvent("Event_OnMyModuleEnable");         /** The module that hooked this event callback has been enabled. */
    g_EvOnModuleDisable =           EventMgr_CreateEvent("Event_OnModuleDisable");          /** A module has been disabled. */
    g_EvOnMyModuleDisable =         EventMgr_CreateEvent("Event_OnMyModuleDisable");        /** The module that hooked this event callback has been disabled. */
    g_EvOnMapStart =                EventMgr_CreateEvent("Event_OnMapStart");               /** The map has started. */
    g_EvOnMapEnd =                  EventMgr_CreateEvent("Event_OnMapEnd");                 /** The map has ended. */
    g_EvOnAutoConfigsBuffered =     EventMgr_CreateEvent("Event_OnAutoConfigsBuffered");    /** This is called before OnConfigsExecuted but any time after OnMapStart. */
    g_EvOnConfigsExecuted =         EventMgr_CreateEvent("Event_OnConfigsExecuted");        /** All convars are set, cvar-dependent code should use this. */
    g_EvOnClientAlphaChanged =      EventMgr_CreateEvent("Event_OnClientAlphaChanged");     /** Client cookies were cached from the database. */
    g_EvOnClientPutInServer =       EventMgr_CreateEvent("Event_OnClientPutInServer");      /** Client has joined the server. */
    g_EvOnClientConnected =         EventMgr_CreateEvent("Event_OnClientConnected");        /** Client successfully connected. */
    g_EvOnClientDisconnect =        EventMgr_CreateEvent("Event_OnClientDisconnect");       /** Client is disconnecting from the server. */
    g_EvOnClientCookiesCached =     EventMgr_CreateEvent("Event_OnClientCookiesCached");    /** Client cookies were cached from the database. */
    g_EvOnPlayerRunCmd =            EventMgr_CreateEvent("Event_OnPlayerRunCmd");           /** Client's movement buttons are being processed. */
    g_EvOnClientPostAdminCheck =    EventMgr_CreateEvent("Event_OnClientPostAdminCheck");   /** Client is authorized. */
    
    // Custom project events.
    g_EvOnClientReady =             EventMgr_CreateEvent("Event_OnClientReady");            /** Client is connected, admin checked and cookies are loaded. */
}

// **********************************************
//              Game Event Hooking
// **********************************************

/**
 * This is called from the event manager in OnPluginStart.
 * Hook the events you want to forward to modules here.
 */
stock EventMgr_HookEvents()
{
    // Hook events to manage and forward to modules.
    
    // TODO: All mod specific events hooks and callbacks should be moved to its
    //       own mod adapter modules. Otherwise this file will be messed up with
    //       lots of different mod specific callbacks for the same event. For
    //       instance player_hurt, which is slightly different between games.
    //
    //       It's better to keep mod specific stuff in their own files and use
    //       abstracted events created in the zr_core module.
    //
    //       Then we can also properly unhook events so ZR can be disabled
    //       gracefully mid-round.
    
    // CS:S events. (Moved to cssadapter module.)
    /*#if defined PROJECT_GAME_CSS
        HookEvent("round_start", GameEvent_RoundStart);
        HookEvent("round_freeze_end", GameEvent_RoundFreezeEnd);
        HookEvent("round_end", GameEvent_RoundEnd);
        HookEvent("player_team", GameEvent_PlayerTeamPre, EventHookMode_Pre);
        HookEvent("player_team", GameEvent_PlayerTeam);
        HookEvent("player_spawn", GameEvent_PlayerSpawn);
        HookEvent("player_hurt", GameEvent_PlayerHurt);
        HookEvent("player_death", GameEvent_PlayerDeath);
        HookEvent("player_jump", GameEvent_PlayerJump);
        HookEvent("weapon_fire", GameEvent_WeaponFire);
    #endif*/
    
    // HL2DM events.
    #if defined PROJECT_GAME_HL2DM
    
    #endif
    
    // DOD events.
    #if defined PROJECT_GAME_DOD
    
    #endif
    
    // TF2 events.
    #if defined PROJECT_GAME_TF2
    
    #endif
    
    // L4D events.
    #if defined PROJECT_GAME_L4D
    #endif
    
    // L4D2 events.
    #if defined PROJECT_GAME_L4D2
    #endif
}

// **********************************************
//           CS:S Game Event Callbacks
// **********************************************

// (Moved to cssadapter module.)
