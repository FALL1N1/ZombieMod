/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          speed.inc
 *  Type:          Module
 *  Description:   Handles setting client speed.
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
new Module:g_moduleSpeed;

/**
 * This value will be added to the player's current speed if it is non-zero.
 */
new Float:g_flSpeedModifier[MAXPLAYERS + 1];

/**
 * Register this module.
 */
Speed_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Speed");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "speed");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Test speed");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleSpeed = ModuleMgr_Register(moduledata);
    
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleSpeed, "Event_OnClientPutInServer",      "Speed_OnClientPutInServer");
    EventMgr_RegisterEvent(g_moduleSpeed, "Event_OnPlayerRunCmd",           "Speed_OnPlayerRunCmd");
    EventMgr_RegisterEvent(g_moduleSpeed, "Hook_PreThinkPost",              "Speed_PreThinkPost");
}

/**
 * Plugin is loading.
 */
Speed_OnPluginStart()
{
    // Register the module.
    Speed_Register();
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public Speed_OnClientPutInServer(client)
{
    // Initialize to 0.
    g_flSpeedModifier[client] = 0.0;
}

public Speed_PreThinkPost(client)
{
    // Don't waste time if the modifier is 0.
    if (g_flSpeedModifier[client] == 0)
        return;
    
    if (!IsPlayerAlive(client))
        return;
    
    // Note: Default is around 200.0 - 250.0.
    
    new Float:curspeed = GetEntPropFloat(client, Prop_Data, "m_flMaxspeed");
    SetEntPropFloat(client, Prop_Data, "m_flMaxspeed", curspeed + g_flSpeedModifier[client]);
}

public Action:Speed_OnPlayerRunCmd(client, &buttons, &impulse, Float:vel[3], Float:angles[3], &weapon)
{
    if (!IsPlayerAlive(client))
        return Plugin_Continue;
    
    // Speed is clamped by the client AND server in cvars.
    // The server cvars are easy to take care of.  The client ones require this function to bypass.
    
    // x-axis speed.
    if (vel[0] < 0.0)
        vel[0] = -5000.0;
    
    else if (vel[0] > 0.0)
        vel[0] = 5000.0;
    
    // y-axis speed.
    if (vel[1] < 0.0)
        vel[1] = -5000.0;
    
    else if (vel[1] > 0.0)
        vel[1] = 5000.0;
    
    return Plugin_Continue; 
}

/**
 * Getters/Setters
 */
 
stock Float:GetSpeedModifier(client)
{
    return g_flSpeedBoost[client];
}

stock SetSpeedModifier(client, Float:value)
{
    g_flSpeedBoost[client] = value;
}
