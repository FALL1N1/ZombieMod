/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          gamedata.inc
 *  Type:          Module
 *  Description:   Handles virtual offsets and signatures.
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
 * General module defines.
 */
#define GAMEDATA_FILE "plugin.zombiereloaded"

/**
 * This module's identifier.
 */
//new Module:g_moduleGameData;

/**
 * Gamedata config handle.
 */
//new Handle:g_hGameConfig;

/**
 * Register this module.
 */
stock GameData_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "GameData");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "gamedata");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Preps virtual offsets and signatures.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    /*g_moduleGameData = */
    ModuleMgr_Register(moduledata);
    
    // Register all the events needed for this module.
    
}

/**
 * Plugin is loading.
 */
GameData_OnPluginStart()
{
    // ZR doesn't need it's own gamedata file at the moment.
    
    /*
    // Register the module.
    GameData_Register();
    
    // Load game config file.
    g_hGameConfig = LoadGameConfigFile(GAMEDATA_FILE);
    
    // If gamedata file can't be loaded, then stop the plugin.
    if (g_hGameConfig == INVALID_HANDLE)
    {
        SetFailState("Can't load game config file (%s) from the \"gamedata\" directory.", GAMEDATA_FILE);
    }
    */
}
