/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          root.zrc.inc
 *  Type:          Module
 *  Description:   Root module for the ZR core.  All ZR core modules depend on this.
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
#include "zr/libraries/cvarlib"
#include "zr/libraries/utilities"
#include "zr/libraries/weaponlib"

/**
 * This module's identifier.
 */
new Module:g_moduleZRCRoot;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRC_GetIdentifier() { return g_moduleZRCRoot; }

/**
 * Register this module.
 */
ZRC_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRC Root");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zrc_root");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Top of the chain module for ZRClassic's set of core modules.  This can not be enabled/disabled by anything other than game rules.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    moduledata[ModuleData_Root] = true;
    
    // Send this array of data to the module manager.
    g_moduleZRCRoot = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZRCRoot, "Event_OnEventsRegister",           "ZRC_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZRC_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZRCRoot, "Event_OnMyModuleEnable",           "ZRC_OnMyModuleEnable");
}

// Include sub-modules.
#include "zr/modules/zrc_core/damage.zrc.inc"
#include "zr/modules/zrc_core/gameplay.zrc.inc"
#include "zr/modules/zrc_core/infection.zrc.inc"
#include "zr/modules/zrc_core/teammanager.zrc.inc"
#include "zr/modules/zrc_core/weapons.zrc.inc"

/**
 * Plugin is loading.
 */
ZRC_OnPluginStart()
{
    // Register the module.
    ZRC_Register();
    
    // Forward event to sub-modules.
    ZRCDmg_OnPluginStart();
    ZRCGPlay_OnPluginStart();
    ZRCInfect_OnPluginStart();
    ZRCTM_OnPluginStart();
    ZRCWeps_OnPluginStart();
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ZRC_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Enable core ZR modules.
    new Module:dependentmodules[MM_DATA_DEPENDENCIES];
    new count = ModuleMgr_FindDependentModules(g_moduleZRCRoot, dependentmodules);
    for (new dmoduleindex = 0; dmoduleindex < count; dmoduleindex++)
        ModuleMgr_Enable(dependentmodules[dmoduleindex]);
    
    return Plugin_Continue;
}
