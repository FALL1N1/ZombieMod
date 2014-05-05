/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          <file>.inc
 *  Type:          Class attribue module
 *  Description:   <description>
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
 * Template instructions:
 * 
 * All module-specific text has been replaced with text enclosed in <>
 * This is to make preparing this file to become the module you need easier.
 * Simply search-and-replace (usually CTRL+H) each of the tags below and replace it with the module specifics.
 * 
 * Tags:
 * <FuncPrefix>         The prefix of your module's functions.  Usually the module's full name with no whitespaces.
 * <ModuleVarName>      Usually the same as <FuncPrefix>.  Names the global variable that stores this module's identifier.
 * 
 * Remove these instructions after all text replacement operations are completed.
 */

/**
 * This module's identifier.
 */
new Module:g_moduleCls<ModuleVarName>;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:Cls<FuncPrefix>_GetIdentifier() { return g_moduleCls<ModuleVarName>; }

/**
 * Register this module.
 */
Cls<FuncPrefix>_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "");
    moduledata[ModuleData_Dependencies][0] = ClassMgr_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleCls<ModuleVarName> = ModuleMgr_Register(moduledata);
    
    EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnEventsRegister",      "Cls<FuncPrefix>_OnEventsRegister");
    
    // Register attributes in the attribute register.
    //ClsAttribReg_AddAttrib(g_moduleCls<ModuleVarName>, "<attribute>");
    //ClsAttribReg_AddAttrib(g_moduleCls<ModuleVarName>, "<attribute2>");
    
    // Add public API function to function register.
    //new Handle:pluginHandle = GetMyHandle();
    //FuncRegister_AddFunction(GetFunctionByName(pluginHandle, "Cls<FuncPrefix>_<function>"), "Class_<function alias>");
}

/**
 * Plugin is loading.
 */
Cls<FuncPrefix>_OnPluginStart()
{
    // Register the module.
    Cls<FuncPrefix>_Register();
}


/**
 * Register all events here.
 */
public Cls<FuncPrefix>_OnEventsRegister()
{
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnAllPluginsLoaded",      "Cls<FuncPrefix>_OnAllPluginsLoaded");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnPluginEnd",             "Cls<FuncPrefix>_OnPluginEnd");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnAllModulesLoaded",      "Cls<FuncPrefix>_OnAllModulesLoaded");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnModuleEnable",          "Cls<FuncPrefix>_OnModuleEnable");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnMyModuleEnable",        "Cls<FuncPrefix>_OnMyModuleEnable");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnModuleDisable",         "Cls<FuncPrefix>_OnModuleDisable");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnMyModuleDisable",       "Cls<FuncPrefix>_OnMyModuleDisable");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnMapStart",              "Cls<FuncPrefix>_OnMapStart");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnMapEnd",                "Cls<FuncPrefix>_OnMapEnd");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnAutoConfigsBuffered",   "Cls<FuncPrefix>_OnAutoConfigsBuffered");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnConfigsExecuted",       "Cls<FuncPrefix>_OnConfigsExecuted");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnClientPutInServer",     "Cls<FuncPrefix>_OnClientPutInServer");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_OnClientDisconnect",      "Cls<FuncPrefix>_OnClientDisconnect");
    
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_RoundStart",              "Cls<FuncPrefix>_RoundStart");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_RoundFreezeEnd",          "Cls<FuncPrefix>_RoundFreezeEnd");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_RoundEnd",                "Cls<FuncPrefix>_RoundEnd");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_PlayerTeam",              "Cls<FuncPrefix>_PlayerTeam");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_PlayerSpawn",             "Cls<FuncPrefix>_PlayerSpawn");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_PlayerHurt",              "Cls<FuncPrefix>_PlayerHurt");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_PlayerDeath",             "Cls<FuncPrefix>_PlayerDeath");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_PlayerJump",              "Cls<FuncPrefix>_PlayerJump");
    //EventMgr_RegisterEvent(g_moduleCls<ModuleVarName>, "Event_WeaponFire",              "Cls<FuncPrefix>_WeaponFire");
    
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassAttribLoad",           "Cls<FuncPrefix>_OnClassAttribLoad");
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassValidate",             "Cls<FuncPrefix>_OnClassValidate");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassAllLoaded",          "Cls<FuncPrefix>_OnClassAllLoaded");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerPreload",      "Cls<FuncPrefix>_OnClassPlayerPreload");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassApply",              "Cls<FuncPrefix>_OnClassApply");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerInfected",     "Cls<FuncPrefix>_OnClassPlayerInfected");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerHuman",        "Cls<FuncPrefix>_OnClassPlayerHuman");
}

/**
 * A class attribute is loading and ready to be cached.
 *
 * @param classIndex    Class index.
 * @param kv            Handle to keyvalue tree, ready to read attribute value.
 * @param attribute     Name of the current attribute.
 * @param className     Name of the current class (section name in keyvalue tree).
 */
public Cls<FuncPrefix>_OnClassAttribLoad(classIndex, Handle:kv, const String:attribute[], const String:className[])
{
    decl String:buffer[CLASS_STRING_LEN];
    buffer[0] = 0;
    
    if (StrEqual(attribute, "<attribute>", false))
    {
        // TODO: Get value from kv. String, float, cell, etc.
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        
        // TODO: Save in data array at index classIndex.
    }
    /* else if (StrEqual(attribute, "<attribute2>", false))
    {
        // Get other attributes.
    }
    */
}

/**
 * Class manager sent an validation request. Attribute modules do validation on
 * all their attributes, and log errors if any.
 *
 * @param classIndex    Class index.
 * @param kv            Handle to keyvalue tree, ready to read attribute value.
 * @param attribute     Name of the current attribute.
 *
 * @return              Attribute module returns Plugin_Handled on validation error,
 *                      or Plugin_Continue if ok.
 */
public Action:Cls<FuncPrefix>_OnClassValidate(classIndex)
{
    new bool:hasErrors = false;
    
    // TODO: Cache attributes if stored in a enum-array.
    // Example:
    // new classCache[ClassGenericAttributes];
    // classCache = ClassGenericData[classIndex];
    
    decl String:buffer[CLASS_STRING_LEN];
    buffer[0] = 0;
    
    // Get class name.
    decl String:className[CLASS_NAME_LEN];
    strcopy(className, sizeof(className), classCache[ClassAttrib_Name]);
    
    // TODO: Call individual attribute validation functions. Make these youself.
    // Example:
    /*
    if (!ClsGeneric_IsValidTeam(classCache[ClassAttrib_Team]))
    {
        ClassMgr_TeamToString(classCache[ClassAttrib_Team], buffer, sizeof(buffer));
        LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Warning: Invalid team in class \"%s\" (%d): \"%s\"", className, classIndex, buffer);
        hasErrors = true;
    }
    if (!ClsGeneric_IsValidGroup(classCache[ClassAttrib_Group]))
    {
        LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Warning: Invalid group in class \"%s\" (%d): \"%s\"", className, classIndex, classCache[ClassAttrib_Group]);
        hasErrors = true;
    }*/
    
    if (hasErrors)
    {
        return Plugin_Handled;
    }
    
    return Plugin_Continue;
}

/**
 * All classes are loaded now. Attribute modules should now make a copy of their array
 * so the original values can be kept.
 *
 * @param classCount    Number of classes loaded.
 */
public Cls<FuncPrefix>_ClsGeneric_OnClassAllLoaded(classCount)
{
}

/**
 * Preloads player info before player preferences are loaded. The class manger
 * sets initial selected class indexes. Attribute modules may initialize players too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public Cls<FuncPrefix>_OnClassPlayerPreload(client, classIndex)
{
}

/**
 * Loads player info with player preferences (from cookies). The class manger
 * sets new selected class indexes according to player preferences. Attribute modules
 * may initialize players with their preferences too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public Cls<FuncPrefix>_OnClassPlayerLoad(client, classIndex)
{
}

/**
 * Class attributes are applied. Attribute modules should apply their own attributes
 * on the player now.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public Cls<FuncPrefix>_OnClassApply(client, classIndex)
{
}

/**
 * A player was infected. This event is fired AFTER class attributes are applied.
 * Attribute modules should use this event and not the infection module's event when
 * initializing zombie specific features.
 *
 * @param client        Client index.
 * @param attacker      Attacker client index.
 * @param motherZombie  Specifies whether the attacker was a mother zombie.
 */
public Cls<FuncPrefix>_OnClassPlayerInfect(client, attacker, bool:motherZombie)
{
}

/**
 * A player was turned back into a human. This event is fired AFTER class attributes
 * are applied. Attribute modules should use this event and not the infection module's
 * event when initializing human specific features.
 *
 * @param client        Client index.
 */
public Cls<FuncPrefix>_OnClassPlayerHuman(client)
{
}

/**
 * All plugins have loaded.
 */
public Cls<FuncPrefix>_OnAllPluginsLoaded()
{
}

/**
 * Plugin is ending.
 */
public Cls<FuncPrefix>_OnPluginEnd()
{
}


/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public Cls<FuncPrefix>_OnEventsReady()
{
}

/**
 * All modules have been registered.
 */
public Cls<FuncPrefix>_OnAllModulesLoaded()
{
}

/**
 * A module has been enabled.
 * 
 * @return      Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:Cls<FuncPrefix>_OnModuleEnable(Module:module)
{
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @return      Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:Cls<FuncPrefix>_OnMyModuleEnable()
{
}

/**
 * A module has been disabled.
 * 
 * @return      Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:Cls<FuncPrefix>_OnModuleDisable(Module:module)
{
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @return      Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:Cls<FuncPrefix>_OnMyModuleDisable()
{
}

/**
 * The map has started.
 */
public Cls<FuncPrefix>_OnMapStart()
{
}

/**
 * The map has ended.
 */
public Cls<FuncPrefix>_OnMapEnd()
{
}

/**
 * This is called before OnConfigsExecuted but any time after OnMapStart.
 * Per-map settings should be set here. 
 */
public Cls<FuncPrefix>_OnAutoConfigsBuffered()
{
}

/**
 * All convars are set, cvar-dependent code should use this.
 */
public Cls<FuncPrefix>_OnConfigsExecuted()
{
}

/**
 * Called when a registered config file (by this module) is manually.
 */
public Cls<FuncPrefix>_OnConfigReload(configindex)
{
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public Cls<FuncPrefix>_OnClientPutInServer(client)
{
}

/**
 * Client is disconnecting from the server.
 * 
 * @param client    The client index.
 */
public Cls<FuncPrefix>_OnClientDisconnect(client)
{
}

/**
 * Round has started.
 */
public Cls<FuncPrefix>_RoundStart()
{
}

/**
 * Pre-round freezetime has finished.
 */
public Cls<FuncPrefix>_RoundFreezeEnd()
{
}

/**
 * Round has ended.
 */
public Cls<FuncPrefix>_RoundEnd()
{
}

/**
 * Client has joined a team.
 * 
 * @param client        The client index.
 * @param team          The client's new team.
 * @param oldteam       The team the client is switching from.
 * @param disconnect    True if the client is disconnecting, false if switching to a team.
 */
public Cls<FuncPrefix>_PlayerTeam(client, team, oldteam, bool:disconnect)
{
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 */
public Cls<FuncPrefix>_PlayerSpawn(client)
{
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
public Cls<FuncPrefix>_PlayerHurt(victim, attacker, health, armor, const String:weapon[], dmg_health, dmg_armor, hitgroup)
{
}

/**
 * Client has been killed.
 * 
 * @param victim    The index of the killed client.
 * @param attacker  The killer of the victim.
 * @param weapon    The weapon classname used to kill the victim. (No weapon_ prefix)
 * @param headshot  True if the death was by headshot, false if not.
 */
public Cls<FuncPrefix>_PlayerDeath(victim, attacker, const String:weapon[], bool:headshot)
{
}

/**
 * Client has jumped.
 * 
 * @param client        The client index.
 */
public Cls<FuncPrefix>_PlayerJump(client)
{
}

/**
 * Client has fired a weapon.
 * 
 * @param client        The client index.
 * @param weapon        The weapon classname fired. (No weapon_ prefix)
 */
public Cls<FuncPrefix>_WeaponFire(client, const String:weapon[])
{
}
