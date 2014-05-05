/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          infection.zrc.inc
 *  Type:          Module
 *  Description:   Handles infection for ZRClassic.
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

// Include infection interface.
#include "zr/interfaces/infection.interface"

/**
 * The weapon classname to fire player_death with.
 * Default: "zombie_claws_of_death" for compatibility.
 */
#define ZOMBIE_WEAPON_CLASSNAME "zombie_claws_of_death"

/**
 * Implementation for IInfection_GetInfectWeapon
 */
public ZRCInfect_GetInfectWeapon(String:weapon[], maxlen) { strcopy(weapon, maxlen, ZOMBIE_WEAPON_CLASSNAME); }

/**
 * This module's identifier.
 */
new Module:g_moduleZRCInfect;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRCInfect_GetIdentifier() { return g_moduleZRCInfect; }

/**
 * Cvar handles.
 */
new Handle:g_hCvarMZombieMinTime;
new Handle:g_hCvarMZombieMaxTime;
new Handle:g_hCvarMZombieRatio;
new Handle:g_hCvarMZombieBlockConsecutive;
new Handle:g_hCvarMZombieCountdown;
new Handle:g_hCvarMZombieCountdownStart;

/**
 * Variable/function set to check/set if zombies are present.
 */
new bool:g_bZombiesPresent;
public bool:ZRCInfect_AreZombiesPresent() { return g_bZombiesPresent; }
stock ZRCInfect_SetZombiesPresent(bool:zombiespresent) { g_bZombiesPresent = zombiespresent; }

/**
 * Timer handles for mother zombie selection and round timer.
 */
new Handle:g_hMotherInfect;
new Handle:g_hMotherInfectCountdown;

/**
 * Keeps track of who the mother zombies are for the current round.
 */
new bool:g_bMotherZombie[MAXPLAYERS + 1];
public bool:ZRCInfect_IsClientMZombie(client) { return g_bMotherZombie[client]; }

/**
 * Keeps track of if a client was mother zombie in the previous round.
 * See cvar g_hCvarMZombieBlockConsecutive.
 */
new bool:g_bWasMotherZombie[MAXPLAYERS + 1];

/**
 * Keeps track of the round freeze state.
 */
new bool:ZRCInfect_RoundFreeze;

/**
 * Register this module.
 */
ZRCInfect_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRC Infection Handler");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zrcinfection");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Handles infection for ZRClassic gameplay.");
    moduledata[ModuleData_Dependencies][0] = ZRC_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZRCInfect = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnEventsRegister",         "ZRCInfect_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZRCInfect_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnMyCoreActivate",         "ZRCInfect_OnMyCoreActivate");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnMyCoreShutdown",         "ZRCInfect_OnMyCoreShutdown");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnMapStart",               "ZRCInfect_OnMapStart");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnMapEnd",                 "ZRCInfect_OnMapEnd");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnClientPutInServer",      "ZRCInfect_OnClientPutInServer");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_OnClientDisconnect",       "ZRCInfect_OnClientDisconnect");
    
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_RoundStart",               "ZRCInfect_RoundStart");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_RoundFreezeEnd",           "ZRCInfect_RoundFreezeEnd");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_RoundEnd",                 "ZRCInfect_RoundEnd");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_PlayerSpawn",              "ZRCInfect_PlayerSpawn");
    EventMgr_RegisterEvent(g_moduleZRCInfect, "Event_PlayerHurt",               "ZRCInfect_PlayerHurt");
}

/**
 * Plugin is loading.
 */
ZRCInfect_OnPluginStart()
{
    // Register the module.
    ZRCInfect_Register();
    
    // Create cvars.
    g_hCvarMZombieMinTime =             CreateConVar("zrc_mzombie_min_time", "1.0", "Minimum time from the start of the round until picking the mother zombie(s).");
    g_hCvarMZombieMaxTime =             CreateConVar("zrc_mzombie_max_time", "1.0", "Maximum time from the start of the round until picking the mother zombie(s).");
    g_hCvarMZombieRatio =               CreateConVar("zrc_mzombie_ratio", "5", "Number of humans to infect as mother zombies in proportion to number of humans on the server. ['0' = Always one mother zombie]");
    g_hCvarMZombieBlockConsecutive =    CreateConVar("zrc_mzombie_block_consecutive", "1", "Prevent a player from being chosen as mother zombie two rounds in a row.");
    g_hCvarMZombieCountdown =           CreateConVar("zrc_mzombie_countdown", "1", "Shows a countdown to players until the outbreak.");
    g_hCvarMZombieCountdownStart =      CreateConVar("zrc_mzombie_countdown_start", "15", "Start showing countdown when there are lessthan or equal to this amount of seconds before infection.");
    
    // Create commands.
    RegConsoleCmd("zrc_infect", ZRCInfect_InfectCommand, "Turn a player into a zombie. Usage: zrc_infect <#userid|name> [respawn - 1/0]");
    RegConsoleCmd("zrc_human", ZRCInfect_HumanCommand, "Turn a player into a human. Usage: zrc_human <#userid|name> [respawn - 1/0] [protect - 1/0]");
}

/**
 * Forwarded to the core module being shutdown before ZRC_OnMyCoreActivate and ZRC_OnGameRulesApplied.
 */
public ZRCInfect_OnMyCoreShutdown(Module:module)
{
    if (module == g_moduleZRCRoot)
    {
        // Release the infection interface.
        Interface_Release(g_IInfectionGetInfectWeapon);
        Interface_Release(g_IInfectionAreZombiesPresent);
        Interface_Release(g_IInfectionIsClientMZombie);
        Interface_Release(g_IInfectionClientToZombie);
        Interface_Release(g_IInfectionClientToHuman);
    }
}

/**
 * Forwarded to the core module being activated after ZRC_OnMyCoreShutdown but before ZRC_OnGameRulesApplied.
 */
public ZRCInfect_OnMyCoreActivate(Module:module)
{
    if (module == g_moduleZRCRoot)
    {
        // Implement the infection interface.
        Interface_Implement(g_IInfectionGetInfectWeapon, GetFunctionByName(GetMyHandle(), "ZRCInfect_GetInfectWeapon"));
        Interface_Implement(g_IInfectionAreZombiesPresent, GetFunctionByName(GetMyHandle(), "ZRCInfect_AreZombiesPresent"));
        Interface_Implement(g_IInfectionIsClientMZombie, GetFunctionByName(GetMyHandle(), "ZRCInfect_IsClientMZombie"));
        Interface_Implement(g_IInfectionClientToZombie, GetFunctionByName(GetMyHandle(), "ZRCInfect_ClientToZombie"));
        Interface_Implement(g_IInfectionClientToHuman, GetFunctionByName(GetMyHandle(), "ZRCInfect_ClientToHuman"));
    }
}

/**
 * The map has started.
 */
public ZRCInfect_OnMapStart()
{
    // Stop timers.
    Util_CloseHandle(g_hMotherInfect);
    Util_CloseHandle(g_hMotherInfectCountdown);
}

/**
 * The map has ended.
 */
public ZRCInfect_OnMapEnd()
{
    // Stop timers.
    Util_CloseHandle(g_hMotherInfect);
    Util_CloseHandle(g_hMotherInfectCountdown);
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public ZRCInfect_OnClientPutInServer(client)
{
    g_bMotherZombie[client] = false;
    g_bWasMotherZombie[client] = false;
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public ZRCInfect_OnClientDisconnect(client)
{
    // Infect a new zombie if the last one leaves.
    if (TeamMgr_IsClientZombie(client))
    {
        // Do nothing during round freeze.
        if (ZRCInfect_RoundFreeze)
        {
            ThrowError("[BUG] Client is zombie during round freeze. This shouldn't happen.");
        }
        
        // If there are no more zombies then pick a new one.
        if (TeamMgr_CountTeam(VTeam_Zombie, true) <= 1)
        {
            new newzombie = ZRCInfect_MotherZombie(1);
            if (newzombie != -1)
            {
                TransMgr_PrintText(newzombie, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC infect disconnect");
            }
        }
    }
}

/**
 * Round has started.
 */
public ZRCInfect_RoundStart()
{
    // Stop timers.
    Util_CloseHandle(g_hMotherInfect);
    Util_CloseHandle(g_hMotherInfectCountdown);
    
    // Fresh round, no zombies.
    g_bZombiesPresent = false;
    
    // Buy time starting.
    ZRCInfect_RoundFreeze = true;
}

/**
 * Pre-round freezetime has finished.
 */
public ZRCInfect_RoundFreezeEnd()
{
    // Stop timers.
    Util_CloseHandle(g_hMotherInfect);
    Util_CloseHandle(g_hMotherInfectCountdown);
    
    // Buy time over.
    ZRCInfect_RoundFreeze = false;
    
    ZRCInfect_StartOutbreakTimer();
}

/**
 * Round has ended.
 * 
 * @param winner    The index of the winning team.
 */
public ZRCInfect_RoundEnd(winner)
{
    // Stop timers.
    Util_CloseHandle(g_hMotherInfect);
    Util_CloseHandle(g_hMotherInfectCountdown);
    
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
            continue;
        
        // Only reset clients that are playing to human team.  Ignore unassigned/spectators.
        if (TeamMgr_IsClientOnPlayerTeam(client))
            TeamMgr_SetClientTeam(client, VTeam_Human);
        
        // Keep track of who was mother zombie this round for next round.
        g_bWasMotherZombie[client] = g_bMotherZombie[client];
        g_bMotherZombie[client] = false;
    }
    
    // No more zombies.
    g_bZombiesPresent = false;
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 * 
 */
public ZRCInfect_PlayerSpawn(client)
{
    CreateTimer(0.1, ZRCInfect_PlayerSpawnPost, client, 0);
}

/**
 * Client has spawned. (post)
 * 
 * @param client    The client index.
 * 
 */
public Action:ZRCInfect_PlayerSpawnPost(Handle:timer, any:client)
{
    // Automatically turn a client into a zombie or human depending on their
    // virtual team.
    if (TeamMgr_IsClientZombie(client))
    {
        if (g_bZombiesPresent)
        {
            ZRCInfect_ClientToZombie(client, 0, false);
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC spawn as zombie");
        }
        else
        {
            ZRCInfect_ClientToZombie(client, 0, true);
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC client to mother zombie");
        }
    }
    
    if (TeamMgr_IsClientHuman(client))
    {
        ZRCInfect_ClientToHuman(client);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC spawn as human");
    }
    
    // Restart outbreak timer if no zombies and not already running (checked in
    // this function).
    ZRCInfect_StartOutbreakTimer();
}

/**
 * Client has been damaged.
 * 
 * @param victim        The index of the hurt client.
 * @param attacker      The index of the attacking client.
 * @param health        How much health the client has after the damage.
 * @param armor         How much armor the client has after the damage.
 * @param weapon        The weapon classname used to hurt the victim. (No weapon_ prefix)
 * @param dmg_health    The amount of health the victim lost.
 * @param dmg_armor     The amount of armor the victim lost.
 * @param hitgroup      The hitgroup index of the victim that was damaged.
 */
public ZRCInfect_PlayerHurt(victim, attacker, health, armor, const String:weapon[], dmg_health, dmg_armor, hitgroup)
{
    // Check if the victim and attacker are valid clients.
    if (!Util_IsClientInGame(victim) || !Util_IsClientInGame(attacker))
        return;
    
    // Check if a zombie is attacking a human.
    if (TeamMgr_IsClientZombie(attacker) && TeamMgr_IsClientHuman(victim))
    {
        //if (StrEqual(weapon, "knife"))
        if (IGame_WeaponIsZombieClaws(weapon))
        {
            ZRCInfect_ClientToZombie(victim, attacker, false);
            TransMgr_PrintText(victim, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC client to zombie");
        }
    }
}

/**
 * Starts the mother zombie selection timer, if no zombies present and not
 * already running.
 */
ZRCInfect_StartOutbreakTimer()
{
    // Check if outbreak timer is already running.
    if (g_hMotherInfect != INVALID_HANDLE)
    {
        return;
    }
    
    // Check if one or more zombies have already spawned.
    if (g_bZombiesPresent)
    {
        return;
    }
    
    // Check if buy time is in progress.
    if (ZRCInfect_RoundFreeze)
    {
        return;
    }
    
    // Get min and max times and pick a random time inbetween.
    new Float:mintime = GetConVarFloat(g_hCvarMZombieMinTime);
    new Float:maxtime = GetConVarFloat(g_hCvarMZombieMaxTime);
    new randomtime = RoundToNearest(GetRandomFloat(mintime, maxtime));
    
    g_hMotherInfect = CreateTimer(float(randomtime), ZRCInfect_OutbreakTimer, false, TIMER_FLAG_NO_MAPCHANGE);
    
    // Start countdown if enabled.
    if (GetConVarBool(g_hCvarMZombieCountdown))
    {
        // Create a pack of data to track countdown.
        new Handle:hCountdownData = CreateDataPack();
        WritePackCell(hCountdownData, 0);
        WritePackCell(hCountdownData, randomtime);
        g_hMotherInfectCountdown = CreateTimer(1.0, ZRCInfect_Countdown, hCountdownData, TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE|TIMER_DATA_HNDL_CLOSE);
        
        // Print first message if the time until infection is below the defined value.
        if (randomtime <= GetConVarInt(g_hCvarMZombieCountdownStart))
        {
            TransMgr_PrintTextAll(false, false, MsgFormat_None, MsgType_Center, g_moduleZRCInfect, false, "ZRC infect countdown", randomtime);
        }
    }
}

/**
 * Turn a human into a zombie.
 * 
 * @param client    The client being infected.
 * @param attacker  The client doing the infecting.
 * @param mzombie   True to make a mother zombie, false for normal zombie.
 */
public ZRCInfect_ClientToZombie(client, attacker, bool:mzombie)
{
    // Temporary bug trap.
    if (ZRCInfect_RoundFreeze)
    {
        ThrowError("Infect during round freeze. This is a bug, unless an admin did it.");
    }
    
    PrintToChatAll("%N infected %N (hp to 1000)", attacker, client);
    SetEntityHealth(client, 1000);
    TeamMgr_SetClientTeam(client, VTeam_Zombie);
    
    // If this is the first zombie of the round then everyone needs to be placed on the right team.
    if (!g_bZombiesPresent)
        TeamMgr_UpdateClientTeams();
    
    g_bZombiesPresent = true;
    
    // Set to true if mzombie is true, but don't set to false if mzombie is false, because they could have been re-infected by other means.
    if (mzombie)
        g_bMotherZombie[client] = true;
    
    // Last thing we do because teams need to be set before this is fired.
    #if defined PROJECT_GAME_CSS
        // Only fire the event if there is an attacker.
        if (attacker != 0)
        {
            // TODO: Make an abstract death notification function in game interface.
            new Handle:hEventPlayerDeath = CreateEvent("player_death", true);
            SetEventInt(hEventPlayerDeath, "userid", GetClientUserId(client));
            SetEventInt(hEventPlayerDeath, "attacker", GetClientUserId(attacker));
            SetEventString(hEventPlayerDeath, "weapon", ZOMBIE_WEAPON_CLASSNAME);
            SetEventBool(hEventPlayerDeath, "headshot", false);
            FireEvent(hEventPlayerDeath);
        }
    #endif
    
    // Forward event to all modules.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell, DataType_Cell};
    new any:eventdata[sizeof(eventdatatypes)][1];
    
    eventdata[0][0] = client;
    eventdata[1][0] = attacker;
    eventdata[2][0] = mzombie;
    
    EventMgr_Forward(g_EvOnClientInfected, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
}

/**
 * Turn a zombie into a human.
 * 
 * @param client    The zombie client to turn human.
 */
public ZRCInfect_ClientToHuman(client)
{
    TeamMgr_SetClientTeam(client, VTeam_Human);
    
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = client;
    
    EventMgr_Forward(g_EvOnClientHuman, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
}

/**
 * Choose a random mother zombie.
 * 
 * @param override_count    If this is set to something > 0 then this is how many mother zombies will be spawned.
 * 
 * @return                  The first mother zombie infected, -1 if none were infected.
 */
ZRCInfect_MotherZombie(override_count = 0)
{
    // Create eligible player list.
    new Handle:adtClients;
    new count = Util_BuildClientList(adtClients, UTILS_FILTER_TEAM | UTILS_FILTER_ALIVE, ZRCInfect_HumanFilter);
    
    // If there are no clients that can become a zombie then stop, wait for more
    // players to connect.
    if (count == 0)
    {
        CloseHandle(adtClients);
        return -1;
    }
    
    // Prune the list further.
    new client;
    new bool:blockconsinfect = GetConVarBool(g_hCvarMZombieBlockConsecutive);
    for (new cindex = 0; cindex < count; cindex++)
    {
        // Don't pick a mother zombie if there is only 1 client left at this point.
        if (count <= 1)
            break;
        
        // Get client stored in array index.
        client = GetArrayCell(adtClients, cindex);
        
        // Prune out clients that were mother zombies last round to prevent double-infection.
        if (blockconsinfect && g_bWasMotherZombie[client])
        {
            // Prune, subtract 1 from count, and backtrack one index.
            RemoveFromArray(adtClients, cindex);
            count--;
            cindex--;
        }
    }
    
    // Determine how many mother zombies to spawn.
    new mzombiecount;
    if (override_count > 0)
    {
        mzombiecount = override_count;
    }
    else
    {
        // Ratio of mother zombies to humans.
        new ratio = GetConVarInt(g_hCvarMZombieRatio);
        
        // Count living clients on the human team.
        new humancount = TeamMgr_CountTeam(VTeam_Human, true);
        
        // Calculate mother zombie count.
        mzombiecount = (ratio > 0) ? RoundToNearest(float(humancount) / float(ratio)) : 1;
        
        // If mothercount is 0, then set to 1.
        if (mzombiecount == 0)
            mzombiecount = 1;
    }
    
    new randindex;
    new randclient;
    new firstinfected = -1;
    for (new cindex = 0; cindex < mzombiecount; cindex++)
    {
        // Should always leave at least one human and stop if there are no more clients left.
        if (TeamMgr_CountTeam(VTeam_Human, true) <= 1 || count <= 0)
            break;
        
        // Get a random client from the array, infect, and remove from list.
        randindex = GetRandomInt(0, count - 1);
        randclient = GetArrayCell(adtClients, randindex);
        ZRCInfect_ClientToZombie(randclient, 0, true);
        TransMgr_PrintText(randclient, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC client to mother zombie");
        RemoveFromArray(adtClients, randindex); count--;
        
        // Store first infected client.
        if (cindex == 0)
            firstinfected = randclient;
    }
    
    CloseHandle(adtClients);
    
    return firstinfected;
}

/**
 * Human filter callback for mother zombie selection.
 *
 * @param client        Client index.
 * @param filters       General filter flags. See UTILS_FILTER_*.
 * @param targetClient  Target client that use the filter. Use 0 if no target
 *                      client.
 *
 * @return              True if client pass filter, false otherwise.
 */
public bool:ZRCInfect_HumanFilter(client, filters, targetClient)
{
    if (!TeamMgr_IsClientHuman(client))
    {
        return false;
    }
    
    return true;
}

/**
 * Timer callback.
 * Choose the mother zombie(s).
 * 
 * @param timer     The timer handle.
 */
public Action:ZRCInfect_OutbreakTimer(Handle:timer)
{
    g_hMotherInfect = INVALID_HANDLE;
    
    // Pick a random mother zombie.
    if (ZRCInfect_MotherZombie() < 0)
    {
        // Not enough players to select a mother zombie. Waiting for more players.
        TransMgr_PrintTextAll(false, false, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC infect waiting for players");
    }
}

/**
 * Timer callback
 * Executed every second until the outbreak.
 * 
 * @param timer             The timer handle.
 * @param hCountdownData    Stores length of the timer and seconds that have passed.
 */
public Action:ZRCInfect_Countdown(Handle:timer, Handle:hCountdownData)
{
    // Read the info from the datapack.
    ResetPack(hCountdownData);
    new counter = ReadPackCell(hCountdownData);
    new total = ReadPackCell(hCountdownData);
    
    counter += 1; // Increment.
    
    // Print if the counter hasn't reached the end.
    if (counter <= total)
    {
        // Print text if the time until infection is below the defined value.
        new timeleft = total - counter;
        if (timeleft <= GetConVarInt(g_hCvarMZombieCountdownStart))
        {
            TransMgr_PrintTextAll(false, false, MsgFormat_None, MsgType_Center, g_moduleZRCInfect, false, "ZRC infect countdown", timeleft);
        }
    }
    
    // Stop if the countdown is finished.
    if (counter >= total)
    {
        g_hMotherInfectCountdown = INVALID_HANDLE;
        return Plugin_Stop;
    }
    
    // Write the new counter value to the datapack.
    ResetPack(hCountdownData);
    WritePackCell(hCountdownData, counter);
    
    return Plugin_Continue;
}

/**
 * Command callback: zr_infect
 * Turn a player into a zombie.
 * 
 * @param client    The client index. 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:ZRCInfect_InfectCommand(client, argc)
{
    // Check if the this core module is disabled, if so then don't do anything with it.
    if (ModuleMgr_IsDisabled(g_moduleZRCInfect))
    {
        return Plugin_Continue;
    }
    
    // Check if the client has permission to use this.
    if (!AccessMgr_HasAccess(client, g_moduleZRCInfect))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // If not enough arguments given, then stop.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command infect syntax");
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
    
    // Check optional parameter.
    // TODO: Teleport player to spawn if enabled (and ztele module available).
    //decl String:strRespawn[4];
    //GetCmdArg(2, strRespawn, sizeof(strRespawn));
    //new bool:bRespawn = bool:StringToInt(strRespawn);
    
    for (new tindex = 0; tindex < result; tindex++)
    {
        // Can't infect a zombie.
        if (TeamMgr_IsClientHuman(targets[tindex]))
        {
            // If a human is infected, cancel the timer that infects a mother zombie.
            if (g_bZombiesPresent)
            {
                ZRCInfect_ClientToZombie(targets[tindex], 0, false);
                TransMgr_PrintText(targets[tindex], MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC client to zombie");
                if (result == 1)
                {
                    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command infect successful", targets[tindex]);
                }
            }
            else
            {
                Util_CloseHandle(g_hMotherInfect);
                ZRCInfect_ClientToZombie(targets[tindex], 0, true);
                TransMgr_PrintText(targets[tindex], MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC client to mother zombie");
                if (result == 1)
                {
                    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command infect mother successful", targets[tindex]);
                }
            }
        }
        else
        {
            if (result == 1)
            {
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command infect unsuccessful", targets[tindex]);
            }
        }
    }
    
    return Plugin_Handled;
}

/**
 * Command callback: zr_human
 * Turn a player into a human.
 * 
 * @param client    The client index. 
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:ZRCInfect_HumanCommand(client, argc)
{
    // Check if the this core module is disabled, if so then don't do anything with it.
    if (ModuleMgr_IsDisabled(g_moduleZRCInfect))
        return Plugin_Continue;
    
    // Check if the client has permission to use this.
    if (!AccessMgr_HasAccess(client, g_moduleZRCInfect))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    // If not enough arguments given, then stop.
    if (argc < 1)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command human syntax");
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
    
    // Check optional parameters.
    // TODO: Teleport to spawn if enabled (and ztele module is available).
    //decl String:strRespawn[4];
    //GetCmdArg(2, strRespawn, sizeof(strRespawn));
    //new bool:bRespawn = bool:StringToInt(strRespawn);
    
    // TODO: Set spawn protection if enabled.
    //decl String:strProtect[4];
    //GetCmdArg(3, strProtect, sizeof(strProtect));
    //new bool:bProtect = bool:StringToInt(strProtect);
    
    for (new tindex = 0; tindex < result; tindex++)
    {
        // Can't infect a zombie.
        if (TeamMgr_IsClientZombie(targets[tindex]))
        {
            ZRCInfect_ClientToHuman(targets[tindex]);
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRC zombie to human");
            if (result == 1)
            {
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command human successful", targets[tindex]);
            }
        }
        else
        {
            if (result == 1)
            {
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ZRC command human unsuccessful", targets[tindex]);
            }
        }
    }
    
    return Plugin_Handled;
}
