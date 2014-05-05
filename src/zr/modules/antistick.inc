/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          antistick.inc
 *  Type:          Module
 *  Description:   Detects collisions between players and unsticks them.
 *
 *  Copyright (C) 2009-2010  Greyscale, Richard Helgeby
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
new Module:g_moduleAntiStick;

/**
 * Register this module.
 */
AntiStick_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Anti-Stick");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "antistick");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Detects collisions between players and unsticks them.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleAntiStick = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleAntiStick, "Event_OnEventsRegister",         "AntiStick_OnEventsRegister");
}

/**
 * Register all events here.
 */
public AntiStick_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleAntiStick, "Hook_ShouldCollide",             "AntiStick_ShouldCollide");
}

/**
 * Plugin is loading.
 */
AntiStick_OnPluginStart()
{
    // Register the module.
    AntiStick_Register();
}

/**
 * Called when 2 entities are asking if they should collide. (touching)
 * 
 * @param entity            Entity asking if it should collide.
 * @param collisiongroup    Collision group flags.  SMLib has them defined in smlib_entities.inc.
 * @param contentsmask      See sdktools_trace for contentmask flags.
 * 
 * @return                  True to allow collision or false for no colliding.
 */
public Action:AntiStick_ShouldCollide(entity, collisiongroup, contentsmask, bool:originalResult)
{
    if (!Client_IsValid(entity, false))
    {
        // Entity is not a player (should never happen). Collide.
        LogMgr_Print(g_moduleAntiStick, LogType_Error, "Bug", "Antistick got a non-player entity (%d). ZR only hooks ShouldCollide on players, so this is a bug.", entity);
        return Plugin_Continue;
    }
    
    new bool:shouldCollide = true;
    
    // Unstick if client is colliding with another client.
    if (Collision_Group_t:collisiongroup == COLLISION_GROUP_PLAYER)
    {
        shouldCollide = !AntiStick_IsClientColliding(entity);
    }
    
    // Convert result to an Action value.
    return shouldCollide ? Plugin_Continue : Plugin_Handled;
}

/**
 * Check if a client is stuck in another client's model box.
 * 
 * @param client    The client to check.
 */
stock bool:AntiStick_IsClientColliding(client)
{
    new Float:vecMins[3], Float:vecMaxs[3], Float:pos[3];
    GetClientMins(client, vecMins);
    GetClientMaxs(client, vecMaxs);
    GetClientAbsOrigin(client, pos);
    
    TR_TraceHullFilter(pos, pos, vecMins, vecMaxs, MASK_SOLID, AntiStick_HullFilter, client);
    return TR_DidHit();
}

public bool:AntiStick_HullFilter(target, mask, any:client)
{
    // Only checking for collisions with other clients.
    return (target != client && Client_IsValid(target, false));
}
