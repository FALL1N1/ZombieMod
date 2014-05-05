/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          genericattributes.inc
 *  Type:          Class attribue module
 *  Description:   Stores the generic class attributes. This is a required module, without it classes won't work.
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

#include "zr/libraries/authlib"
#include "zr/libraries/utilities"

/**
 * This module's identifier.
 */
static Module:g_moduleClsGeneric;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ClsGeneric_GetIdentifier() { return g_moduleClsGeneric; }

/**
 * Data structure for generic class data (the class manager's own attributes).
 */
enum ClassGenericAttributes
{
    bool:ClassAttrib_Enabled,                       /** Specifies whether the class is enabled or not. */
    ClassTeam:ClassAttrib_Team,                     /** Which team the class belongs to. */
    
    bool:ClassAttrib_TeamDefault,                   /** Specifies whether it's the default class in its team. */
    bool:ClassAttrib_IsAdminClass,                  /** Specifies whether the class is a admins-only class or not. */
    bool:ClassAttrib_IsMotherZombie,                /** Specifies whether the class is mother zombie class or not (only valid in zombie team). */
    
    ClsGeneric_AuthModes:ClassAttrib_AuthMode,      /** Authorization mode. Restricts the class according to this setting, groups and flags. */
    String:ClassAttrib_Flags[CLASS_STRING_LEN],     /** List of SourceMod flag names for authorization. Separated by comma (,) */
    String:ClassAttrib_Groups[CLASS_STRING_LEN],    /** List of SourceMod groups for authorization. Separated by comma (,). */
    
    String:ClassAttrib_Name[CLASS_NAME_LEN],        /** Unique name used to identify the class (section name in keyvalue file). */
    String:ClassAttrib_DisplayName[CLASS_NAME_LEN],     /** Name displayed in the class menu. */
    String:ClassAttrib_Description[CLASS_STRING_LEN]    /** Description displayed in the class menu. */
}

/**
 * Original class data cache.
 */
static ClassGenericData[CLASS_MAX][ClassGenericAttributes];

/**
 * Writable class data cache.
 */
static ClassGenericData2[CLASS_MAX][ClassGenericAttributes];

/**
 * Buffer to store whether the class name has been saved.
 */
static bool:ClassGotName[CLASS_MAX];

/**
 * Player class data cache.
 */
static ClassPlayerData[MAXPLAYERS + 1][ClassGenericAttributes];


/**
 * Register this module.
 */
ClsGeneric_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Generic class attributes");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "genericattributes");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Stores generic class attributes. This is a required module, without it classes won't work.");
    moduledata[ModuleData_Dependencies][0] = ClassMgr_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleClsGeneric = ModuleMgr_Register(moduledata);
    
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnEventsRegister",      "ClsGeneric_OnEventsRegister");
    
    // Add attributes to the attribute register.
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "enabled");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "team");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "team_default");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "is_admin_class");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "is_mother_zombie");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "auth_mode");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "flags");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "groups");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "display_name");
    ClassAttribReg_AddAttrib(g_moduleClsGeneric, "description");
}

/**
 * Plugin is loading.
 */
ClsGeneric_OnPluginStart()
{
    // Register the module.
    ClsGeneric_Register();
}

/**
 * Register all events here.
 */
public ClsGeneric_OnEventsRegister()
{
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnAllPluginsLoaded",      "ClsGeneric_OnAllPluginsLoaded");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnPluginEnd",             "ClsGeneric_OnPluginEnd");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnAllModulesLoaded",      "ClsGeneric_OnAllModulesLoaded");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnModuleEnable",          "ClsGeneric_OnModuleEnable");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnMyModuleEnable",        "ClsGeneric_OnMyModuleEnable");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnModuleDisable",         "ClsGeneric_OnModuleDisable");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnMyModuleDisable",       "ClsGeneric_OnMyModuleDisable");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnMapStart",              "ClsGeneric_OnMapStart");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnMapEnd",                "ClsGeneric_OnMapEnd");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnAutoConfigsBuffered",   "ClsGeneric_OnAutoCfgsBuffered");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnConfigsExecuted",       "ClsGeneric_OnConfigsExecuted");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClientPutInServer",     "ClsGeneric_ClientPutInServer");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClientDisconnect",      "ClsGeneric_OnClientDisconnect");
    
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_RoundStart",              "ClsGeneric_RoundStart");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_RoundFreezeEnd",          "ClsGeneric_RoundFreezeEnd");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_RoundEnd",                "ClsGeneric_RoundEnd");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_PlayerTeam",              "ClsGeneric_PlayerTeam");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_PlayerSpawn",             "ClsGeneric_PlayerSpawn");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_PlayerHurt",              "ClsGeneric_PlayerHurt");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_PlayerDeath",             "ClsGeneric_PlayerDeath");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_PlayerJump",              "ClsGeneric_PlayerJump");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_WeaponFire",              "ClsGeneric_WeaponFire");
    
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassClear",                "ClsGeneric_OnClassClear");
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassAttribLoad",           "ClsGeneric_OnClassAttribLoad");
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassValidate",             "ClsGeneric_OnClassValidate");
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassAllLoaded",            "ClsGeneric_OnClassAllLoaded");
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerPreload",        "ClsGeneric_OnClassPlayerPreload");
    EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerLoad",           "ClsGeneric_OnClassPlayerLoad");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassApply",              "ClsGeneric_OnClassApply");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerInfect",       "ClsGeneric_OnClassPlayerInfect");
    //EventMgr_RegisterEvent(g_moduleClsGeneric, "Event_OnClassPlayerHuman",        "ClsGeneric_OnClassPlayerHuman");
}

/**
 * Class data clear request.
 *
 * @param classIndex    Class index to clear, or -1 to clear all classes.
 */
public ClsGeneric_OnClassClear(classIndex)
{
    new start = 0;
    new end = CLASS_MAX;
    
    if (classIndex >= 0)
    {
        // Clear specified class only.
        start = classIndex;
        end = classIndex + 1;
    }
    
    for (new i = start; i < end; i ++)
    {
        // Clear original cache.
        ClsGeneric_ClearClass(i);
    }
}

/**
 * A class attribute is loading and ready to be cached.
 *
 * @param classIndex    Class index.
 * @param kv            Handle to keyvalue tree, ready to read attribute value.
 * @param attribute     Name of the current attribute.
 * @param className     Name of the current class (section name in keyvalue tree).
 */
public ClsGeneric_OnClassAttribLoad(classIndex, Handle:kv, const String:attribute[], const String:className[])
{
    // Save class name if not saved yet.
    if (!ClassGotName[classIndex])
    {
        strcopy(ClassGenericData[classIndex][ClassAttrib_Name], CLASS_NAME_LEN, className);
        ClassGotName[classIndex] = true;
    }
    
    decl String:value[CLASS_STRING_LEN];
    value[0] = 0;
    KvGetString(kv, attribute, value, sizeof(value));
    
    if (StrEqual(attribute, "enabled", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassGenericData[classIndex][ClassAttrib_Enabled] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, value) == PhraseToBool_True);
    }
    else if (StrEqual(attribute, "team", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassGenericData[classIndex][ClassAttrib_Team] = ClassMgr_StringToTeam(value);
    }
    else if (StrEqual(attribute, "team_default", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassGenericData[classIndex][ClassAttrib_TeamDefault] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, value) == PhraseToBool_True);
    }
    else if (StrEqual(attribute, "is_admin_class", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassGenericData[classIndex][ClassAttrib_IsAdminClass] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, value) == PhraseToBool_True);
    }
    else if (StrEqual(attribute, "is_mother_zombie", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassGenericData[classIndex][ClassAttrib_IsMotherZombie] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, value) == PhraseToBool_True);
    }
    else if (StrEqual(attribute, "auth_mode", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassGenericData[classIndex][ClassAttrib_AuthMode] = ClsGeneric_StringToAuthMode(value);
    }
    else if (StrEqual(attribute, "groups", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        strcopy(ClassGenericData[classIndex][ClassAttrib_Groups], CLASS_STRING_LEN, value);
    }
    else if (StrEqual(attribute, "flags", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        strcopy(ClassGenericData[classIndex][ClassAttrib_Flags], CLASS_STRING_LEN, value);
    }
    else if (StrEqual(attribute, "display_name", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        strcopy(ClassGenericData[classIndex][ClassAttrib_DisplayName], CLASS_NAME_LEN, value);
    }
    else if (StrEqual(attribute, "description", false))
    {
        //KvGetString(kv, attribute, buffer, sizeof(buffer));
        strcopy(ClassGenericData[classIndex][ClassAttrib_Description], CLASS_NAME_LEN, value);
    }
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
public Action:ClsGeneric_OnClassValidate(classIndex)
{
    new bool:hasErrors = false;
    
    // Cache attributes.
    new classCache[ClassGenericAttributes];
    classCache = ClassGenericData[classIndex];
    
    decl String:buffer[CLASS_STRING_LEN];
    buffer[0] = 0;
    
    // Get class name.
    decl String:className[CLASS_NAME_LEN];
    strcopy(className, sizeof(className), classCache[ClassAttrib_Name]);
    
    if (!ClsGeneric_IsValidTeam(classCache[ClassAttrib_Team]))
    {
        ClassMgr_LogAttribErrGeneric(g_moduleClsGeneric, "team", className, classIndex);
        hasErrors = true;
    }
    if (!ClsGeneric_IsValidAuthMode(classCache[ClassAttrib_AuthMode]))
    {
        ClassMgr_LogAttribErrGeneric(g_moduleClsGeneric, "auth_mode", className, classIndex);
        hasErrors = true;
    }
    if (!ClsGeneric_IsValidFlagList(classCache[ClassAttrib_Flags], CLASS_LIST_SEPARATOR))
    {
        ClassMgr_LogAttribErrString(g_moduleClsGeneric, "flags", className, classIndex, classCache[ClassAttrib_Flags]);
        hasErrors = true;
    }
    if (!ClsGeneric_IsValidGroupList(classCache[ClassAttrib_Groups], CLASS_LIST_SEPARATOR))
    {
        ClassMgr_LogAttribErrString(g_moduleClsGeneric, "groups", className, classIndex, classCache[ClassAttrib_Groups]);
        hasErrors = true;
    }
    if (!ClsGeneric_IsValidDisplayName(classCache[ClassAttrib_DisplayName]))
    {
        ClassMgr_LogAttribErrString(g_moduleClsGeneric, "display_name", className, classIndex, classCache[ClassAttrib_DisplayName]);
        hasErrors = true;
    }
    
    // Check if name is empty.
    if (strlen(className) == 0)
    {
        LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Warning: Invalid class name on class at index %d. Class name cannot be empty.", buffer, classIndex);
        hasErrors = true;
    }
    else
    {
        // Check for reserved keywords in class names.
        if (ClassMgr_IsReservedKeyword(className))
        {
            LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Warning: Cannot use \"%s\" as class name on class at index %d. This is a reserved word.", buffer, classIndex);
            hasErrors = true;
        }
        
        // Check if name is numeric.
        if (String_IsNumeric(className))
        {
            LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Warning: Cannot use \"%s\" as class name on class at index %d. Class name cannot be a number.", buffer, classIndex);
            hasErrors = true;
        }
        
        // Check for duplicate class names.
        new dummy;
        buffer[0] = 0;
        String_ToLower(className, buffer, sizeof(buffer));  // Class names are case insensitive.
        if (GetTrieValue(g_hClassNameIndex, buffer, dummy))
        {
            // Class name already in use.
            LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Warning: Class name \"%s\" in class at index %d is already in use.", buffer, classIndex);
            hasErrors = true;
        }
        else
        {
            // Add class to name index.
            SetTrieValue(g_hClassNameIndex, buffer, classIndex);
        }
    }
    
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
public ClsGeneric_OnClassAllLoaded(classCount)
{
    for (new classIndex = 0; classIndex < classCount; classIndex++)
    {
        ClsGeneric_LoadWritableCache(classIndex);
    }
}

/**
 * Preloads player info before player preferences are loaded. The class manger
 * sets initial selected class indexes. Attribute modules may initialize players too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public ClsGeneric_OnClassPlayerPreload(client, classIndex)
{
    ClsGeneric_LoadPlayerCache(client, classIndex);
}

/**
 * Loads player info with player preferences (from cookies). The class manger
 * sets new selected class indexes according to player preferences. Attribute modules
 * may initialize players with their preferences too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public ClsGeneric_OnClassPlayerLoad(client, classIndex)
{
    ClsGeneric_LoadPlayerCache(client, classIndex);
}

/**
 * Enables the specified class in the writable cache.
 *
 * @param index         Class index.
 */
public ClsGeneric_EnableClass(index)
{
    ClassGenericData2[index][ClassAttrib_Enabled] = true;
}

/**
 * Makes a copy of original class data into the writable cahce.
 *
 * @param classIndex    Class to load.
 */
static ClsGeneric_LoadWritableCache(classIndex)
{
    ClassGenericData2[classIndex][ClassAttrib_Enabled] = ClassGenericData[classIndex][ClassAttrib_Enabled];
    ClassGenericData2[classIndex][ClassAttrib_Team] = ClassGenericData[classIndex][ClassAttrib_Team];
    ClassGenericData2[classIndex][ClassAttrib_TeamDefault] = ClassGenericData[classIndex][ClassAttrib_TeamDefault];
    ClassGenericData2[classIndex][ClassAttrib_IsAdminClass] = ClassGenericData[classIndex][ClassAttrib_IsAdminClass];
    ClassGenericData2[classIndex][ClassAttrib_IsMotherZombie] = ClassGenericData[classIndex][ClassAttrib_IsMotherZombie];
    ClassGenericData2[classIndex][ClassAttrib_AuthMode] = ClassGenericData[classIndex][ClassAttrib_AuthMode];
    strcopy(ClassGenericData2[classIndex][ClassAttrib_Flags], CLASS_STRING_LEN, ClassGenericData[classIndex][ClassAttrib_Flags]);
    strcopy(ClassGenericData2[classIndex][ClassAttrib_Groups], CLASS_STRING_LEN, ClassGenericData[classIndex][ClassAttrib_Groups]);
    strcopy(ClassGenericData2[classIndex][ClassAttrib_Name], CLASS_NAME_LEN, ClassGenericData[classIndex][ClassAttrib_Name]);
    strcopy(ClassGenericData2[classIndex][ClassAttrib_DisplayName], CLASS_NAME_LEN, ClassGenericData[classIndex][ClassAttrib_DisplayName]);
    strcopy(ClassGenericData2[classIndex][ClassAttrib_Description], CLASS_STRING_LEN, ClassGenericData[classIndex][ClassAttrib_Description]);
}

/**
 * Loads class attributes into the client's cache.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 * @param cache         Optional. Cache to read from. Player cache is ignored.
 */
static ClsGeneric_LoadPlayerCache(client, classIndex, ClassCacheType:cache = ClassCache_Modified)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            ClassPlayerData[client][ClassAttrib_Enabled] = ClassGenericData[classIndex][ClassAttrib_Enabled];
            ClassPlayerData[client][ClassAttrib_Team] = ClassGenericData[classIndex][ClassAttrib_Team];
            ClassPlayerData[client][ClassAttrib_TeamDefault] = ClassGenericData[classIndex][ClassAttrib_TeamDefault];
            ClassPlayerData[client][ClassAttrib_IsAdminClass] = ClassGenericData[classIndex][ClassAttrib_IsAdminClass];
            ClassPlayerData[client][ClassAttrib_IsMotherZombie] = ClassGenericData[classIndex][ClassAttrib_IsMotherZombie];
            ClassPlayerData[client][ClassAttrib_AuthMode] = ClassGenericData[classIndex][ClassAttrib_AuthMode];
            strcopy(ClassPlayerData[client][ClassAttrib_Flags], CLASS_STRING_LEN, ClassGenericData[classIndex][ClassAttrib_Flags]);
            strcopy(ClassPlayerData[client][ClassAttrib_Groups], CLASS_STRING_LEN, ClassGenericData[classIndex][ClassAttrib_Groups]);
            strcopy(ClassPlayerData[client][ClassAttrib_Name], CLASS_NAME_LEN, ClassGenericData[classIndex][ClassAttrib_Name]);
            strcopy(ClassPlayerData[client][ClassAttrib_DisplayName], CLASS_NAME_LEN, ClassGenericData[classIndex][ClassAttrib_DisplayName]);
            strcopy(ClassPlayerData[client][ClassAttrib_Description], CLASS_STRING_LEN, ClassGenericData[classIndex][ClassAttrib_Description]);
        }
        case ClassCache_Modified:
        {
            ClassPlayerData[client][ClassAttrib_Enabled] = ClassGenericData2[classIndex][ClassAttrib_Enabled];
            ClassPlayerData[client][ClassAttrib_Team] = ClassGenericData2[classIndex][ClassAttrib_Team];
            ClassPlayerData[client][ClassAttrib_TeamDefault] = ClassGenericData2[classIndex][ClassAttrib_TeamDefault];
            ClassPlayerData[client][ClassAttrib_IsAdminClass] = ClassGenericData2[classIndex][ClassAttrib_IsAdminClass];
            ClassPlayerData[client][ClassAttrib_IsMotherZombie] = ClassGenericData2[classIndex][ClassAttrib_IsMotherZombie];
            ClassPlayerData[client][ClassAttrib_AuthMode] = ClassGenericData2[classIndex][ClassAttrib_AuthMode];
            strcopy(ClassPlayerData[client][ClassAttrib_Flags], CLASS_STRING_LEN, ClassGenericData2[classIndex][ClassAttrib_Flags]);
            strcopy(ClassPlayerData[client][ClassAttrib_Groups], CLASS_STRING_LEN, ClassGenericData2[classIndex][ClassAttrib_Groups]);
            strcopy(ClassPlayerData[client][ClassAttrib_Name], CLASS_NAME_LEN, ClassGenericData2[classIndex][ClassAttrib_Name]);
            strcopy(ClassPlayerData[client][ClassAttrib_DisplayName], CLASS_NAME_LEN, ClassGenericData2[classIndex][ClassAttrib_DisplayName]);
            strcopy(ClassPlayerData[client][ClassAttrib_Description], CLASS_STRING_LEN, ClassGenericData2[classIndex][ClassAttrib_Description]);
        }
    }
}

/**
 * Clears the specified class.
 *
 * @param classIndex    Class index.
 * @param cache         Optional. Specifies what class cache to clear. Player
 *                      cache is ignored.
 */
static ClsGeneric_ClearClass(classIndex, ClassCacheType:cache = ClassCache_Original)
{
    decl attributes[ClassGenericAttributes];
    attributes[ClassAttrib_Enabled] = false;
    attributes[ClassAttrib_Team] = ClassTeam_Invalid;
    attributes[ClassAttrib_TeamDefault] = false;
    attributes[ClassAttrib_IsAdminClass] = false;
    attributes[ClassAttrib_IsMotherZombie] = false;
    attributes[ClassAttrib_AuthMode] = ClsGeneric_InvalidMode;
    attributes[ClassAttrib_Flags][0] = 0;
    attributes[ClassAttrib_Groups][0] = 0;
    attributes[ClassAttrib_Name][0] = 0;
    attributes[ClassAttrib_DisplayName][0] = 0;
    attributes[ClassAttrib_Description][0] = 0;
    
    ClassGotName[classIndex] = false;
    
    switch (cache)
    {
        case ClassCache_Original:
        {
            ClassGenericData[classIndex] = attributes;
        }
        case ClassCache_Modified:
        {
            ClassGenericData2[classIndex] = attributes;
        }
    }
}


/***************************
 *    GET/SET FUNCTIONS    *
 ***************************/

/**
 * Gets the group list from the specified class.
 *
 * @param index         Class index or client index.
 * @param buffer        Destination string buffer.
 * @param maxlen        Size of buffer.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Number of cells written.
 */
public ClsGeneric_GetGroups(classIndex, String:buffer[], maxlen, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return strcopy(buffer, maxlen, ClassGenericData[classIndex][ClassAttrib_Groups]);
        }
        case ClassCache_Modified:
        {
            return strcopy(buffer, maxlen, ClassGenericData2[classIndex][ClassAttrib_Groups]);
        }
        case ClassCache_Player:
        {
            return strcopy(buffer, maxlen, ClassPlayerData[classIndex][ClassAttrib_Groups]);
        }
    }
    
    return 0;
}

/**
 * Gets the flag list from the specified class.
 *
 * @param index         Class index or client index.
 * @param buffer        Destination string buffer.
 * @param maxlen        Size of buffer.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Number of cells written.
 */
public ClsGeneric_GetFlags(classIndex, String:buffer[], maxlen, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return strcopy(buffer, maxlen, ClassGenericData[classIndex][ClassAttrib_Flags]);
        }
        case ClassCache_Modified:
        {
            return strcopy(buffer, maxlen, ClassGenericData2[classIndex][ClassAttrib_Flags]);
        }
        case ClassCache_Player:
        {
            return strcopy(buffer, maxlen, ClassPlayerData[classIndex][ClassAttrib_Flags]);
        }
    }
    
    return 0;
}

/**
 * Gets the name of the specified class.
 *
 * @param index         Class index or client index.
 * @param buffer        Destination string buffer.
 * @param maxlen        Size of buffer.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Number of cells written.
 */
public ClsGeneric_GetName(classIndex, String:buffer[], maxlen, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original, ClassCache_Modified:
        {
            return strcopy(buffer, maxlen, ClassGenericData[classIndex][ClassAttrib_Name]);
        }
        case ClassCache_Player:
        {
            return strcopy(buffer, maxlen, ClassPlayerData[classIndex][ClassAttrib_Name]);
        }
    }
    
    return 0;
}

/**
 * Gets the display name of the specified class.
 *
 * @param index         Class index or client index.
 * @param buffer        Destination string buffer.
 * @param maxlen        Size of buffer.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Number of cells written.
 */
public ClsGeneric_GetDisplayName(classIndex, String:buffer[], maxlen, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return strcopy(buffer, maxlen, ClassGenericData[classIndex][ClassAttrib_DisplayName]);
        }
        case ClassCache_Modified:
        {
            return strcopy(buffer, maxlen, ClassGenericData2[classIndex][ClassAttrib_DisplayName]);
        }
        case ClassCache_Player:
        {
            return strcopy(buffer, maxlen, ClassPlayerData[classIndex][ClassAttrib_DisplayName]);
        }
    }
    
    return 0;
}

/**
 * Gets the display name from several class indexes in a source ADT array.
 *
 * @param sourceList    ADT array with class indexes.
 * @param cache         Specifies what class cache to read from, except
 *                      ClassCache_Player.
 *
 * @return              Handle to array with display names. Must be closed when
 *                      no longer used.
 */
public Handle:ClsGeneric_GetDisplayNames(Handle:sourceList, ClassCacheType:cache)
{
    if (cache == ClassCache_Player)
    {
        ThrowError("ClassCache_Player can't be by ClsGeneric_GetDisplayNames.");
        return INVALID_HANDLE;
    }

    new Handle:result = CreateArray();
    new size = GetArraySize(sourceList);
    
    // Loop through source list.
    for (new i = 0; i < size; i++)
    {
        new classIndex = GetArrayCell(sourceList, i);
        
        decl String:className[CLASS_NAME_LEN];
        className[0] = 0;
        
        ClsGeneric_GetDisplayName(classIndex, className, sizeof(className), cache);
        PushArrayString(result, className);
    }
    
    return result;
}

/**
 * Gets the description of the specified class.
 *
 * @param index         Class index or client index.
 * @param buffer        Destination string buffer.
 * @param maxlen        Size of buffer.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Number of cells written.
 */
public ClsGeneric_GetDescription(classIndex, String:buffer[], maxlen, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return strcopy(buffer, maxlen, ClassGenericData[classIndex][ClassAttrib_Description]);
        }
        case ClassCache_Modified:
        {
            return strcopy(buffer, maxlen, ClassGenericData2[classIndex][ClassAttrib_Description]);
        }
        case ClassCache_Player:
        {
            return strcopy(buffer, maxlen, ClassPlayerData[classIndex][ClassAttrib_Description]);
        }
    }
    
    return 0;
}

/**
 * Returns whether the specified class is enabled or not.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              True if enabled, false otherwise.
 */
public bool:ClsGeneric_IsEnabled(index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return ClassGenericData[index][ClassAttrib_Enabled];
        }
        case ClassCache_Modified:
        {
            return ClassGenericData2[index][ClassAttrib_Enabled];
        }
        case ClassCache_Player:
        {
            return ClassPlayerData[index][ClassAttrib_Enabled];
        }
    }
    
    return false;
}

/**
 * Sets whether a class is enabled or not.
 *
 * @param index         Class index.
 * @param enabled       Enabled state.
 */
public ClsGeneric_SetEnabled(index, bool:enabled)
{
    ClassGenericData2[index][ClassAttrib_Enabled] = enabled;
}

/**
 * Gets the team from the specified class.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Class team or ClassTeamNew_Invalid on error.
 */
public ClassTeam:ClsGeneric_GetTeam(index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original, ClassCache_Modified: return ClassGenericData[index][ClassAttrib_Team];
        case ClassCache_Player: return ClassPlayerData[index][ClassAttrib_Team];
    }
    
    return ClassTeam_Invalid;
}

/**
 * Returns whether the class is on the specified team.
 *
 * @param team          Team to compare.
 * @param index         Class index or client index (client's class).
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 */
public bool:ClsGeneric_TeamEqual(ClassTeam:team, index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original, ClassCache_Modified: return ClassGenericData[index][ClassAttrib_Team] == team;
        case ClassCache_Player: return ClassPlayerData[index][ClassAttrib_Team] == team;
    }
    return false;
}

/**
 * Gets the team default flag from the specified class.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Team default flag, or false on error.
 */
public bool:ClsGeneric_GetTeamDefault(index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
             return ClassGenericData[index][ClassAttrib_TeamDefault];
        }
        case ClassCache_Modified:
        {
             return ClassGenericData2[index][ClassAttrib_TeamDefault];
        }
        case ClassCache_Player:
        {
            return ClassPlayerData[index][ClassAttrib_TeamDefault];
        }
    }
    
    return false;
}

/**
 * Gets the is admin flag from the specified class.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Is admin flag, or false on error.
 */
public bool:ClsGeneric_GetIsAdminClass(index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return ClassGenericData[index][ClassAttrib_IsAdminClass];
        }
        case ClassCache_Modified:
        {
            return ClassGenericData2[index][ClassAttrib_IsAdminClass];
        } 
        case ClassCache_Player:
        {
            return ClassPlayerData[index][ClassAttrib_IsAdminClass];
        }
    }
    
    return false;
}

/**
 * Gets the is mother zombie flag from the specified class.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Is mother zombie flag, or false on error.
 */
public bool:ClsGeneric_GetIsMotherZombie(index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return ClassGenericData[index][ClassAttrib_IsMotherZombie];
        }
        case ClassCache_Modified:
        {
            return ClassGenericData2[index][ClassAttrib_IsMotherZombie];
        }
        case ClassCache_Player:
        {
            return ClassPlayerData[index][ClassAttrib_IsMotherZombie];
        }
    }
    
    return false;
}

/**
 * Gets settings as flags from the specified class.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Class flags.
 */
public ClsGeneric_GetClassFlags(index, ClassCacheType:cache)
{
    new flags = 0;
    
    if (ClsGeneric_GetIsAdminClass(index, cache))
    {
        flags += CLASS_FLAG_ADMIN_ONLY;
    }
    if (ClsGeneric_GetIsMotherZombie(index, cache))
    {
        flags += CLASS_FLAG_MOTHER_ZOMBIE;
    }
    
    return flags;
}

/**
 * Gets the authorization mode from the specified class.
 *
 * @param index         Class index or client index.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Class authorization mode or ClsGeneric_InvalidMode on error.
 */
public ClsGeneric_AuthModes:ClsGeneric_GetAuthMode(index, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return ClassGenericData[index][ClassAttrib_AuthMode];
        }
        case ClassCache_Modified:
        {
            return ClassGenericData2[index][ClassAttrib_AuthMode];
        }
        case ClassCache_Player:
        {
            return ClassPlayerData[index][ClassAttrib_AuthMode];
        }
    }
    
    return ClsGeneric_InvalidMode;
}

/**
 * Converts a string to a authorization mode.
 *
 * @param mode      Mode string to convert.
 *
 * @return          Authorization blending mode, or ClsGeneric_InvalidMode on error.
 */
ClsGeneric_AuthModes:ClsGeneric_StringToAuthMode(const String:mode[])
{
    if (strlen(mode) > 0)
    {
        if (StrEqual(mode, "disabled", false))
        {
            return ClsGeneric_Disabled;
        }
        else if (StrEqual(mode, "flag", false))
        {
            return ClsGeneric_Flag;
        }
        else if (StrEqual(mode, "group", false))
        {
            return ClsGeneric_Group;
        }
        else if (StrEqual(mode, "either", false))
        {
            return ClsGeneric_Either;
        }
        else if (StrEqual(mode, "both", false))
        {
            return ClsGeneric_Both;
        }
        else if (StrEqual(mode, "all", false))
        {
            return ClsGeneric_All;
        }
    }
    
    return ClsGeneric_InvalidMode;
}


/******************************
 *    VALIDATION FUNCTIONS    *
 ******************************/

/**
 * Validates team.
 *
 * @param team          Team to validate.
 *
 * @return              True if valid, false otherwise.
 */
static bool:ClsGeneric_IsValidTeam(ClassTeam:team)
{
    if (team == ClassTeam_Zombies ||
        team == ClassTeam_Humans)
    {
        return true;
    }
    
    return false;
}

/**
 * Validates authorization mode.
 *
 * @param mode          Mode to validate.
 *
 * @return              True if valid, false otherwise.
 */
static bool:ClsGeneric_IsValidAuthMode(ClsGeneric_AuthModes:mode)
{
    if (mode != ClsGeneric_InvalidMode)
    {
        return true;
    }
    
    return false;
}

/**
 * Validates a group name.
 *
 * @param group         Group name to validate.
 *
 * @return              True if valid, false otherwise.
 */
static stock bool:ClsGeneric_IsValidGroup(const String:group[])
{
    // Check if the group exist in SourceMod's admin cache.
    if (FindAdmGroup(group) != INVALID_GROUP_ID)
    {
        return true;
    }
    
    return false;
}

/**
 * Validates a group list and log invalid names.
 *
 * @param groupList     List of groups to validate, separated by the separator.
 * @param separator     Separator string to separate list entries.
 *
 * @return              True if all are valid, false otherwise.
 */
static bool:ClsGeneric_IsValidGroupList(const String:groupList[], const String:separator[])
{
    // No groups is also valid.
    if (strlen(groupList) == 0)
    {
        return true;
    }
    
    decl String:groups[AUTHLIB_MAX_GROUPS][AUTHLIB_GROUP_LEN];
    new count = ExplodeString(groupList, separator, groups, AUTHLIB_MAX_GROUPS, AUTHLIB_GROUP_LEN);
    new bool:passed = true;
    
    // Loop through groups in the list.
    for (new group = 0; group < count; group++)
    {
        // Validate group.
        if (FindAdmGroup(groups[group]) == INVALID_GROUP_ID)
        {
            LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Error: Invalid group name: \"%s\"", groups[group]);
            passed = false;
        }
    }
    
    return passed;
}

/**
 * Validates a flag name.
 *
 * @param flag          Flag name to validate.
 *
 * @return              True if valid, false otherwise.
 */
static bool:ClsGeneric_IsValidFlag(const String:flag[])
{
    new AdminFlag:dummy;
    if (FindFlagByName(flag, dummy))
    {
        return true;
    }
    
    return false;
}

/**
 * Validates a flag list and log invalid names.
 *
 * @param flagList      List of flags to validate, separated by the separator.
 * @param separator     Separator string to separate list entries.
 *
 * @return              True if all are valid, false otherwise.
 */
static bool:ClsGeneric_IsValidFlagList(const String:flagList[], const String:separator[])
{
    // No flags is also valid.
    if (strlen(flagList) == 0)
    {
        return true;
    }
    
    decl String:flags[AdminFlags_TOTAL][AUTHLIB_FLAG_LEN];
    new count = ExplodeString(flagList, separator, flags, AdminFlags_TOTAL, AUTHLIB_FLAG_LEN);
    new bool:passed = true;
    
    // Loop through groups in the list.
    for (new flag = 0; flag < count; flag++)
    {
        // Validate group.
        if (!ClsGeneric_IsValidFlag(flags[flag]))
        {
            LogMgr_Print(g_moduleClsGeneric, LogType_Error, "Config Validation", "Error: Invalid flag name: \"%s\"", flags[flag]);
            passed = false;
        }
    }
    
    return passed;
}

/**
 * Validates display name.
 *
 * @param displayName   Name to validate.
 *
 * @return              True if valid, false otherwise.
 */
static bool:ClsGeneric_IsValidDisplayName(const String:displayName[])
{
    if (strlen(displayName) > 0)
    {
        return true;
    }
    
    return false;
}
