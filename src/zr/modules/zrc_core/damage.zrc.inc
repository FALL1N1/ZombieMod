/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          damage.zrc.inc
 *  Type:          Module
 *  Description:   Handles damage rules for ZRClassic.
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

#include "zr/libraries/damagelib"

/**
 * This module's identifier.
 */
new Module:g_moduleZRCDmg;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRCDmg_GetIdentifier() { return g_moduleZRCDmg; }

/**
 * Cvar handles.
 */
#if defined PROJECT_GAME_CSS || defined PROJECT_GAME_TF2
  new Handle:g_hCvarMPFF;
#endif

/**
 * Register this module.
 */
ZRCDmg_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRC Damage Control");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zrcdamage");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Handles damage rules for ZRClassic gameplay.");
    moduledata[ModuleData_Dependencies][0] = ZRC_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZRCDmg = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZRCDmg, "Event_OnEventsRegister",            "ZRCDmg_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZRCDmg_OnEventsRegister()
{
    // Register all the events needed for this module.
    
    
    // Register custom events.
    EventMgr_RegisterEvent(g_moduleZRCDmg, "Hook_TraceAttack",                  "ZRCDmg_TraceAttack");
    EventMgr_RegisterEvent(g_moduleZRCDmg, "Hook_OnTakeDamage",                 "ZRCDmg_OnTakeDamage");
}

/**
 * Plugin is loading.
 */
ZRCDmg_OnPluginStart()
{
    // Register the module.
    ZRCDmg_Register();
    
    // Cache game cvar handles for later.
    #if defined PROJECT_GAME_CSS || defined PROJECT_GAME_TF2
        g_hCvarMPFF = FindConVar("mp_friendlyfire");
    #endif
}

/**
 * Stop humans on opposite teams from killing each other.
 * 
 * @param victim        The client being attacked.
 * @param attacker      The client attacking the victim.
 * @param damage        The amount of damage inflicted.
 * @param damagetype    A bit indicating type of damage that was being inflicted.
 * ...
 * 
 * @return              Hook action.  See include/core.inc.
 */
public Action:ZRCDmg_TraceAttack(victim, &attacker, &Float:damage, &damagetype)
{
    // Check if the victim and attacker are valid clients.
    if (!Util_IsClientInGame(victim) || !Util_IsClientInGame(attacker))
        return Plugin_Continue;
    
    #if defined PROJECT_GAME_CSS || defined PROJECT_GAME_TF2
    
    // Block clients on the same virtual team from attacking eachother, if
    // friendly fire is disabled.
    // Ex: Terrorist human will not be able to shoot Counter-Terrorist human.
    if (!GetConVarBool(g_hCvarMPFF))
    {
        if (TeamMgr_GetClientTeam(victim) == TeamMgr_GetClientTeam(attacker))
        {
            return Plugin_Handled;
        }
    }
    #endif
    
    return Plugin_Continue;
}

/**
 * Stop damage if zombie is infecting a human.
 * 
 * @param victim        The client being attacked.
 * @param attacker      The client attacking the victim.
 * @param damage        The amount of damage inflicted.
 * @param damagetype    A bit indicating type of damage that was being inflicted.
 * 
 * @return              Hook action.  See include/core.inc.
 */
public Action:ZRCDmg_OnTakeDamage(victim, &attacker, &Float:damage, &damagetype)
{
    // Check if the victim and attacker are valid clients.
    if (!Util_IsClientInGame(victim) || !Util_IsClientInGame(attacker))
        return Plugin_Continue;
    
    // Check if a zombie is attacking a human.
    if (TeamMgr_IsClientZombie(attacker) && TeamMgr_IsClientHuman(victim))
    {
        // Knifing is included in DMG_BULLET in CS:S.
        // They couldn't just use DMG_SLASH because that would make sense.
        if (damagetype & UTILS_DMG_BULLET)
        {
            SetEntityHealth(victim, GetClientHealth(victim) + RoundToNearest(damage));
        }
    }
    
    return Plugin_Continue;
}
