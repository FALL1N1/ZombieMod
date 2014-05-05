/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          zombiereloaded.sp
 *  Type:          Base
 *  Description:   Base file.
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

// Comment out to not require semicolons at the end of each line of code.
#pragma semicolon 1

#include <sourcemod>
#include <smlib>
#include <clientprefs>

#include "zr/project"
#include "zr/base/wrappers"


/******************
 *   Interfaces   *
 ******************/
#include "zr/interfaces/interfaces"


/*****************************
 *   Base project includes   *
 *****************************/
#include "zr/base/versioninfo"
#include "zr/base/accessmanager"
#include "zr/base/translationsmanager"
#include "zr/base/logmanager"     // Translations manager needs to be included before log manager.
#include "zr/base/configmanager"
#include "zr/base/eventmanager"
#include "zr/base/modulemanager"


/***************************
 *   Common Core Modules   *
 ***************************/
#include "zr/modules/common/teammanager"


/***********************
 *   Module includes   *
 ***********************/
#include "zr/modules/gamerules"
#include "zr/modules/gamedata"
#include "zr/modules/zr_core"
#include "zr/modules/mapconfig"
#include "zr/modules/zrc_core/root.zrc"
#include "zr/modules/zriot_core/root.zriot"
#include "zr/modules/sdkhooksadapter"
#include "zr/modules/soundprofiles"

#if defined PROJECT_GAME_CSS
    #include "zr/modules/models/modelmgr"   // Must be included before class manager.  
    // This isn't good.  Why isn't the model attribute its own module?  To avoid dependency on include order just wrap everything in functions and use an interface.
    // This is definitely something we want to avoid at all costs, it goes against the philosophy of ZR's structure.
    // TODO: Yes, this should be made more independent. Maybe with a model managing interface.
#endif

#include "zr/modules/classes/classmanager"  // Model attribute module depends on model manager.
#include "zr/modules/downloader"
#include "zr/modules/zmenu"
#include "zr/modules/zadmin"
#include "zr/modules/antistick"
#include "zr/modules/suicideintercept"
#include "zr/modules/voice"
#include "zr/modules/speed"
#include "zr/modules/respawn"
#include "zr/modules/sfx"
#include "zr/modules/fog"
#include "zr/modules/ztele"
#include "zr/modules/flashlight"
#include "zr/modules/stripobjectives"

// Game-specific modules
#if defined PROJECT_GAME_CSS
    #include "zr/modules/weapons/weapons"
    #include "zr/modules/zspawn"
    #include "zr/modules/napalm"
#endif

// Game adapter modules
#include "zr/modules/cssadapter"

/**
 * Record plugin info.
 */
public Plugin:myinfo =
{
    name = PROJECT_FULLNAME,
    author = PROJECT_AUTHOR,
    description = PROJECT_DESCRIPTION,
    version = PROJECT_VERSION,
    url = PROJECT_URL
};

/**
 * Called before plugin is loaded.
 * 
 * @param myself	Handle to the plugin.
 * @param late		Whether or not the plugin was loaded "late" (after map load).
 * @param error		Error message buffer in case load failed.
 * @param err_max	Maximum number of characters for error message buffer.
 * @return			APLRes_Success for load success, APLRes_Failure or APLRes_SilentFailure otherwise.
 */
public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
    // Let plugin load successfully.
    return APLRes_Success;
}

/**
 * Plugin is loading.
 */
public OnPluginStart()
{
    // Forward event to other project base components.
    
    ModuleMgr_OnPluginStart();
    
    #if defined EVENT_MANAGER
        EventMgr_OnPluginStart();
    #endif
    
    #if defined CONFIG_MANAGER
        ConfigMgr_OnPluginStart();
    #endif
    
    #if defined TRANSLATIONS_MANAGER
        TransMgr_OnPluginStart();
    #else
        Project_LoadExtraTranslations(false); // Call this to load translations if the translations manager isn't included.
    #endif
    
    #if defined LOG_MANAGER
        LogMgr_OnPluginStart();
    #endif
    
    #if defined ACCESS_MANAGER
        AccessMgr_OnPluginStart();
    #endif
    
    #if defined VERSION_INFO
        VersionInfo_OnPluginStart();
    #endif
    
    // Forward the OnPluginStart event to all modules.
    ForwardOnPluginStart();
    
    // All modules should be registered by this point!
    
    #if defined EVENT_MANAGER
        // Forward the OnEventsRegister to all modules.
        EventMgr_Forward(g_EvOnEventsRegister, g_CommonEventData1, 0, 0, g_CommonDataType1);
        
        // Forward the OnEventsReady to all modules.
        EventMgr_Forward(g_EvOnEventsReady, g_CommonEventData1, 0, 0, g_CommonDataType1);
        
        // Forward the OnAllModulesLoaded to all modules.
        EventMgr_Forward(g_EvOnAllModulesLoaded, g_CommonEventData1, 0, 0, g_CommonDataType1);
    #endif
}

/**
 * Plugin is ending.
 */
public OnPluginEnd()
{
    // Unload in reverse order of loading.
    
    #if defined EVENT_MANAGER
        // Forward event to all modules.
        EventMgr_Forward(g_EvOnPluginEnd, g_CommonEventData1, 0, 0, g_CommonDataType1);
    #endif
    
    // Forward event to other project base components.
    
    #if defined VERSION_INFO
        VersionInfo_OnPluginEnd();
    #endif
    
    #if defined ACCESS_MANAGER
        AccessMgr_OnPluginEnd();
    #endif
    
    #if defined LOG_MANAGER
        LogMgr_OnPluginEnd();
    #endif
    
    #if defined TRANSLATIONS_MANAGER
        TransMgr_OnPluginEnd();
    #endif
    
    #if defined CONFIG_MANAGER
        ConfigMgr_OnPluginEnd();
    #endif
    
    #if defined EVENT_MANAGER
        EventMgr_OnPluginEnd();
    #endif
    
    ModuleMgr_OnPluginEnd();
}
