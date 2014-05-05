/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          flashlight.inc
 *  Type:          Module
 *  Description:   Blocks flashlight on certain players.
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
 * Cvar handles.
 */
new Handle:g_hCvarFlashlightBlockZombies;
new Handle:g_hCvarFlashlightBlockHumans;

/**
 * Impulse number for flashlight.
 */
#define FLASHLIGHT_IMPULSE 100

/**
 * This module's identifier.
 */
new Module:g_moduleFlashlight;

/**
 * Register this module.
 */
Flashlight_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Flashlight Blocker");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "flashlightblock");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Blocks flashlight on certain players.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleFlashlight = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleFlashlight, "Event_OnEventsRegister",        "Flashlight_OnEventsRegister");
}

/**
 * Register all events here.
 */
public Flashlight_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleFlashlight, "Event_OnPlayerRunCmd",          "Flashlight_OnPlayerRunCmd");
    EventMgr_RegisterEvent(g_moduleFlashlight, "Event_OnClientInfected",        "Flashlight_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleFlashlight, "Event_OnClientHuman",           "Flashlight_OnClientHuman");
}

/**
 * Plugin is loading.
 */
Flashlight_OnPluginStart()
{
    // Register the module.
    Flashlight_Register();
    
    // Create cvars.
    g_hCvarFlashlightBlockZombies = Project_CreateConVar("flashlight_block_zombies", "0", "Block zombies from using their flashlight.");
    g_hCvarFlashlightBlockHumans = Project_CreateConVar("flashlight_block_humans", "0", "Block humans from using their flashlight.");
}

/**
 * Called when a clients movement buttons are being processed
 * 
 * @param client    Index of the client.
 * @param buttons   Copyback buffer containing the current commands (as bitflags - see entity_prop_stocks.inc).
 * @param impulse   Copyback buffer containing the current impulse command.
 * @param vel       Players desired velocity.
 * @param angles    Players desired view angles.
 * @param weapon    Entity index of the new weapon if player switches weapon, 0 otherwise.
 * 
 * @return 			Plugin_Handled to block the commands from being processed, Plugin_Continue otherwise.
 */
public Action:Flashlight_OnPlayerRunCmd(client, &buttons, &impulse, Float:vel[3], Float:angles[3], &weapon)
{
    // If the client isn't using their flashlight, then we don't care.
    if (impulse != FLASHLIGHT_IMPULSE)
        return Plugin_Continue;
    
    if ( (GetConVarBool(g_hCvarFlashlightBlockZombies) && TeamMgr_IsClientZombie(client)) || (GetConVarBool(g_hCvarFlashlightBlockHumans) && TeamMgr_IsClientHuman(client)) )
        impulse = 0;
    
    return Plugin_Continue;
}

/**
 * Client has been infected.
 * 
 * @param client        The infected client.
 * @param attacker      The zombie that infected the client.
 * @param mzombie       True if the client has been infected as a mother zombie.
 */
public Flashlight_OnClientInfected(client, attacker, bool:mzombie)
{
    // Turn flashlight off if blocked for this client.
    if (GetConVarBool(g_hCvarFlashlightBlockZombies))
    {
        if (IGame_FlashlightIsOn(client))
            IGame_FlashlightOff(client);
    }
}

/**
 * Client has turned back to human.
 * 
 * @param client        The client that became human.
 */
public Flashlight_OnClientHuman(client)
{
    // Turn flashlight off if blocked for this client.
    if (GetConVarBool(g_hCvarFlashlightBlockHumans))
    {
        if (IGame_FlashlightIsOn(client))
            IGame_FlashlightOff(client);
    }
}
