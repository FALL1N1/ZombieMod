/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          model.inc
 *  Type:          Class attribue module
 *  Description:   Stores model attributes.
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
#include "zr/libraries/zrwrappers"

/**
 * This module's identifier.
 */
new Module:g_moduleClsModel;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ClsModel_GetIdentifier() { return g_moduleClsModel; }

/**
 * The alpha value was changed on a player.
 *
 * @param client        Client index.
 * @param alpha         Alpha value.
 * OnClassAlphaChanhed(client, alpha)
 */
new ProjectEvent:g_EvOnClassAlphaChanged;

/**
 * Health alpha blending modes.
 */
enum ClassAlphaModes
{
    ClassAlphaFade_Invalid = -1,
    ClassAlphaFade_None,            /** No health alpha blending. */
    ClassAlphaFade_Switch,          /** Switch between max (initial) and min depending on health level. */
    ClassAlphaFade_SwitchOnce,      /** Switch from max to min once when below specified health level. */
    ClassAlphaFade_Linear,          /** Linear fading synced with health. */
    ClassAlphaFade_Exp              /** Exponential fading synced with health. */
}

/**
 * Data structure for model attributes.
 */
enum ClassModelAttributes
{
    String:ClassAttrib_Model[255],                  /** Model query string. This will result in a model index that's used in CurrentModel below. */
    ClassAttrib_CurrentModel,                       /** Current model index. May be negative. If so, the query will be executed again. */
    ClassAlphaModes:ClassAttrib_AlphaFadeMode,      /** Alpha blending mode. */
    bool:ClassAttrib_AlphaInvert,                   /** Invert alpha blending (transparent when close, and opaque on distance). */
    ClassAttrib_AlphaHealth,                        /** Health level (switch mode) or minimum health level (fading modes). */
    ClassAttrib_AlphaMin,                           /** Minimum alpha value. */
    ClassAttrib_AlphaMax,                           /** Maximum alpha value. */
    ClassAttrib_AlphaMinDist,                       /** Minimum fade distance. */
    ClassAttrib_AlphaMaxDist,                       /** Maximum fade distance. */   
}

/**
 * Original class data cache (read-only after loading is done).
 */
new ClassModelData[CLASS_MAX][ClassModelAttributes];

/**
 * Second class data cache (writable).
 */
new ClassModelData2[CLASS_MAX][ClassModelAttributes];

/**
 * Player class data cache.
 */
new ClassModelPlayerData[MAXPLAYERS + 1][ClassModelAttributes];

/**
 * Original model path cache.
 */
new String:ClassModelOriginalPath[MAXPLAYERS + 1][PLATFORM_MAX_PATH];

/**
 * Stores whether the alpha have been switched on a player (switch mode blending).
 */
new bool:ClassModelAlphaSwitched[MAXPLAYERS + 1];

/**
 * @section Attribute limit values. Used when validating.
 */
#define CLASS_ALPHA_MIN             0
#define CLASS_ALPHA_MAX             255
#define CLASS_ALPHA_HEALTH_MIN      0
#define CLASS_ALPHA_HEALTH_MAX      30000
#define CLASS_ALPHA_DIST_MIN        0
#define CLASS_ALPHA_DIST_MAX        32767
/**
 * @endsection
 */

/**
 * Function cache for API functions that this module use.
 */
new Function:ClsModel_GetHealthFunc;

/**
 * Register this module.
 */
ClsModel_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Model class attributes");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "model");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Stores model attributes.");
    moduledata[ModuleData_Dependencies][0] = ClassMgr_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = ModelMgr_GetIdentifier();
    //moduledata[ModuleData_Dependencies][2] = ClsHealth_GetIdentifier();   // Depends on health module because of alpha.
    moduledata[ModuleData_Dependencies][2] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleClsModel = ModuleMgr_Register(moduledata);
    
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnEventsRegister",      "ClsModel_OnEventsRegister");
    
    g_EvOnClassAlphaChanged = EventMgr_CreateEvent("Event_OnClassAlphaChanged");
    
    // Register attributes in the attribute register.
    ClassAttribReg_AddAttrib(g_moduleClsModel, "model");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_mode");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_invert");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_health");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_min");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_max");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_min_dist");
    ClassAttribReg_AddAttrib(g_moduleClsModel, "alpha_max_dist");
    
    // Chache api function IDs that this module use.
    ClsModel_GetHealthFunc = FuncLib_IsFunctionAvailable("ClsHealth_GetHealth");
}

/**
 * Plugin is loading.
 */
ClsModel_OnPluginStart()
{
    // Register the module.
    ClsModel_Register();
}


/**
 * Register all events here.
 */
public ClsModel_OnEventsRegister()
{
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnAllPluginsLoaded",      "ClsModel_OnAllPluginsLoaded");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnPluginEnd",             "ClsModel_OnPluginEnd");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnAllModulesLoaded",        "ClsModel_OnAllModulesLoaded");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnEventsReady",             "ClsModel_OnEventsReady");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnModuleEnable",          "ClsModel_OnModuleEnable");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnMyModuleEnable",        "ClsModel_OnMyModuleEnable");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnModuleDisable",         "ClsModel_OnModuleDisable");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnMyModuleDisable",       "ClsModel_OnMyModuleDisable");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnMapStart",              "ClsModel_OnMapStart");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnMapEnd",                "ClsModel_OnMapEnd");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnAutoConfigsBuffered",   "ClsModel_OnAutoConfigsBuffered");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnConfigsExecuted",       "ClsModel_OnConfigsExecuted");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClientPutInServer",     "ClsModel_OnClientPutInServer");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClientDisconnect",      "ClsModel_OnClientDisconnect");
    
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_RoundStart",              "ClsModel_RoundStart");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_RoundFreezeEnd",          "ClsModel_RoundFreezeEnd");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_RoundEnd",                "ClsModel_RoundEnd");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_PlayerTeam",              "ClsModel_PlayerTeam");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_PlayerSpawn",             "ClsModel_PlayerSpawn");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_PlayerHurt",                "ClsModel_PlayerHurt");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_PlayerDeath",             "ClsModel_PlayerDeath");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_PlayerJump",              "ClsModel_PlayerJump");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_WeaponFire",              "ClsModel_WeaponFire");
    
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassClear",              "ClsModel_OnClassClear");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassAttribLoad",         "ClsModel_OnClassAttribLoad");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassValidate",           "ClsModel_OnClassValidate");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassAllLoaded",          "ClsModel_OnClassAllLoaded");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassPlayerLoad",         "ClsModel_OnClassPlayerLoad");
    EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassPlayerPreload",      "ClsModel_OnClassPlayerPreload");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassApply",              "ClsModel_OnClassApply");     // Do not apply yet.
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassPlayerInfected",   "ClsModel_OnClassPlayerInfected");
    //EventMgr_RegisterEvent(g_moduleClsModel, "Event_OnClassPlayerHuman",      "ClsModel_OnClassPlayerHuman");
}

/**
 * Class data clear request.
 *
 * @param classIndex    Class index to clear, or -1 to clear all classes.
 */
public ClsModel_OnClassClear(classIndex)
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
        ClsModel_ClearClass(i);
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
public ClsModel_OnClassAttribLoad(classIndex, Handle:kv, const String:attribute[], const String:className[])
{
    decl String:buffer[CLASS_STRING_LEN];
    buffer[0] = 0;
    
    if (StrEqual(attribute, "model", false))
    {
        KvGetString(kv, attribute, buffer, sizeof(buffer));
        strcopy(ClassModelData[classIndex][ClassAttrib_Model], CLASS_NAME_LEN, buffer);
    }
    else if (StrEqual(attribute, "alpha_mode", false))
    {
        KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassModelData[classIndex][ClassAttrib_AlphaFadeMode] = ClsModel_StringToMode(buffer);
    }
    else if (StrEqual(attribute, "alpha_invert", false))
    {
        KvGetString(kv, attribute, buffer, sizeof(buffer));
        ClassModelData[classIndex][ClassAttrib_AlphaInvert] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, buffer) == PhraseToBool_True);
    }
    else if (StrEqual(attribute, "alpha_min", false))
    {
        ClassModelData[classIndex][ClassAttrib_AlphaMin] = KvGetNum(kv, attribute);
    }
    else if (StrEqual(attribute, "alpha_max", false))
    {
        ClassModelData[classIndex][ClassAttrib_AlphaMax] = KvGetNum(kv, attribute);
    }
    else if (StrEqual(attribute, "alpha_min_dist", false))
    {
        ClassModelData[classIndex][ClassAttrib_AlphaMinDist] = KvGetNum(kv, attribute);
    }
    else if (StrEqual(attribute, "alpha_max_dist", false))
    {
        ClassModelData[classIndex][ClassAttrib_AlphaMaxDist] = KvGetNum(kv, attribute);
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
public Action:ClsModel_OnClassValidate(classIndex)
{
    new bool:hasErrors = false;
    
    // Cache attributes.
    new classCache[ClassModelAttributes];
    classCache = ClassModelData[classIndex];
    
    decl String:buffer[CLASS_STRING_LEN];
    buffer[0] = 0;
    
    // Get class name.
    decl String:className[CLASS_NAME_LEN];
    className[0] = 0;
    ClsGeneric_GetName(classIndex, className, sizeof(className), ClassCache_Original);
    
    strcopy(buffer, sizeof(buffer), classCache[ClassAttrib_Model]);
    new errCode;
    new errPos;
    if (!ClsModel_IsValidModel(buffer, errCode, errPos))
    {
        ClassMgr_LogAttribErrString(g_moduleClsModel, "model", className, classIndex, buffer);
        
        // Get parse error message.
        decl String:errMsg[64];
        errMsg[0] = 0;
        ModelDB_ErrCodeToString(errCode, errMsg, sizeof(errMsg));
        LogMgr_Print(g_moduleClsModel, LogType_Error, "Config Validation", "Model parse error (%d) at column %d: %s", errCode, errPos, errMsg);
        
        // Print marker.
        decl String:errMarker[MODEL_MAX_QUERY_LEN];
        errMarker[0] = 0;
        ModelDB_GetStringMarker(errPos, errMarker, sizeof(errMarker));
        LogMgr_Print(g_moduleClsModel, LogType_Error, "Config Validation", "%s", buffer);
        LogMgr_Print(g_moduleClsModel, LogType_Error, "Config Validation", "%s", errMarker);
        
        hasErrors = true;
    }
    if (!ClsModel_IsValidAlphaMode(classCache[ClassAttrib_AlphaFadeMode]))
    {
        ClassMgr_LogAttribErrGeneric(g_moduleClsModel, "alpha_mode", className, classIndex);
        hasErrors = true;
    }
    if (!ClsModel_IsValidAlphaHealth(classCache[ClassAttrib_AlphaHealth]))
    {
        ClassMgr_LogAttribErrCell(g_moduleClsModel, "alpha_health", className, classIndex, classCache[ClassAttrib_AlphaHealth]);
        hasErrors = true;
    }
    if (!ClsModel_IsValidAlpha(classCache[ClassAttrib_AlphaMin]))
    {
        ClassMgr_LogAttribErrCell(g_moduleClsModel, "alpha_min", className, classIndex, classCache[ClassAttrib_AlphaMin]);
        hasErrors = true;
    }
    if (!ClsModel_IsValidAlpha(classCache[ClassAttrib_AlphaMax]))
    {
        ClassMgr_LogAttribErrCell(g_moduleClsModel, "alpha_max", className, classIndex, classCache[ClassAttrib_AlphaMax]);
        hasErrors = true;
    }
    if (!ClsModel_IsValidAlphaDist(classCache[ClassAttrib_AlphaMinDist]))
    {
        ClassMgr_LogAttribErrCell(g_moduleClsModel, "alpha_min_dist", className, classIndex, classCache[ClassAttrib_AlphaMinDist]);
        hasErrors = true;
    }
    if (!ClsModel_IsValidAlphaDist(classCache[ClassAttrib_AlphaMaxDist]))
    {
        ClassMgr_LogAttribErrCell(g_moduleClsModel, "alpha_max_dist", className, classIndex, classCache[ClassAttrib_AlphaMaxDist]);
        hasErrors = true;
    }
    
    // Swap min/max alpha values if needed.
    // Note: Do not swap distance values, because inversion is allowed.
    if (classCache[ClassAttrib_AlphaMin] > classCache[ClassAttrib_AlphaMax])
    {
        new temp = classCache[ClassAttrib_AlphaMin];
        classCache[ClassAttrib_AlphaMin] = classCache[ClassAttrib_AlphaMax];
        classCache[ClassAttrib_AlphaMax] = temp;
        LogMgr_Print(g_moduleClsModel, LogType_Error, "Config Validation", "Warning: Mismatched alpha_min and alpha_max attributes in class \"%s\" (%d). Values have been automatically swapped.", className, classIndex);
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
public ClsModel_OnClassAllLoaded(classCount)
{
    // Check if no classes are loaded.
    if (classCount == 0)
    {
        return;
    }
    
    // Copy class data into second array.
    for (new classIndex = 0; classIndex < classCount; classIndex++)
    {
        strcopy(ClassModelData2[classIndex][ClassAttrib_Model], CLASS_NAME_LEN, ClassModelData[classIndex][ClassAttrib_Model]);
        ClassModelData2[classIndex][ClassAttrib_AlphaFadeMode] = ClassModelData[classIndex][ClassAttrib_AlphaFadeMode];
        ClassModelData2[classIndex][ClassAttrib_AlphaInvert] = ClassModelData[classIndex][ClassAttrib_AlphaInvert];
        ClassModelData2[classIndex][ClassAttrib_AlphaHealth] = ClassModelData[classIndex][ClassAttrib_AlphaHealth];
        ClassModelData2[classIndex][ClassAttrib_AlphaMin] = ClassModelData[classIndex][ClassAttrib_AlphaMin];
        ClassModelData2[classIndex][ClassAttrib_AlphaMax] = ClassModelData[classIndex][ClassAttrib_AlphaMax];
        ClassModelData2[classIndex][ClassAttrib_AlphaMinDist] = ClassModelData[classIndex][ClassAttrib_AlphaMinDist];
        ClassModelData2[classIndex][ClassAttrib_AlphaMaxDist] = ClassModelData[classIndex][ClassAttrib_AlphaMaxDist];
    }
}

/**
 * Preloads player info before player preferences are loaded. The class manger
 * sets initial selected class indexes. Attribute modules may initialize players too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public ClsModel_OnClassPlayerPreload(client, classIndex)
{
    // Forward event.
    ClsModel_OnClassPlayerLoad(client, classIndex);
}

/**
 * Loads player info with player preferences (from cookies). The class manger
 * sets new selected class indexes according to player preferences. Attribute modules
 * may initialize players with their preferences too.
 *
 * @param client        Client index.
 * @param classIndex    Class index.
 */
public ClsModel_OnClassPlayerLoad(client, classIndex)
{
    // Load attributes from the specified class into the player's cache.
    strcopy(ClassModelPlayerData[client][ClassAttrib_Model], CLASS_NAME_LEN, ClassModelData2[classIndex][ClassAttrib_Model]);
    ClassModelPlayerData[client][ClassAttrib_AlphaFadeMode] = ClassModelData2[classIndex][ClassAttrib_AlphaFadeMode];
    ClassModelPlayerData[client][ClassAttrib_AlphaInvert] = ClassModelData2[classIndex][ClassAttrib_AlphaInvert];
    ClassModelPlayerData[client][ClassAttrib_AlphaHealth] = ClassModelData2[classIndex][ClassAttrib_AlphaHealth];
    ClassModelPlayerData[client][ClassAttrib_AlphaMin] = ClassModelData2[classIndex][ClassAttrib_AlphaMin];
    ClassModelPlayerData[client][ClassAttrib_AlphaMax] = ClassModelData2[classIndex][ClassAttrib_AlphaMax];
    ClassModelPlayerData[client][ClassAttrib_AlphaMinDist] = ClassModelData2[classIndex][ClassAttrib_AlphaMinDist];
    ClassModelPlayerData[client][ClassAttrib_AlphaMaxDist] = ClassModelData2[classIndex][ClassAttrib_AlphaMaxDist];
}

/**
 * Class attributes are applied. Attribute modules should apply their own attributes
 * on the player now.
 *
 * @param client        Client index.
 */
public ClsModel_OnClassApply(client)
{
    // TODO: Apply model and alpha.
    
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public ClsModel_OnEventsReady()
{
    // Models must be loaded before classes.
    EventMgr_GivePriority("Event_OnMapStart", ModelMgr_GetIdentifier(), ClassMgr_GetIdentifier());
}

/**
 * All modules have been registered.
 */
public ClsModel_OnAllModulesLoaded()
{
}

/**
 * The module that hooked this event callback has been enabled.
 */
public ClsModel_OnMyModuleEnable()
{
    // TODO: Set model and alpha on all players, but only if classes are loaded.
}

/**
 * The module that hooked this event callback has been disabled.
 */
public ClsModel_OnMyModuleDisable()
{
    // TODO: Reset model and alpha to CS:S default on all players.
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 */
public ClsModel_PlayerSpawn(client)
{
    // Cache original model.
    GetClientModel(client, ClassModelOriginalPath[client], PLATFORM_MAX_PATH);
    
    // Reset alpha switched setting.
    ClassModelAlphaSwitched[client] = false;
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
public ClsModel_PlayerHurt(victim, attacker, health, armor, const String:weapon[], dmg_health, dmg_armor, hitgroup)
{
    // Update alpha.
    ClsModel_UpdateAlpha(victim);
}


/************************
 *    INTERNAL LOGIC    *
 ************************/

/**
 * Clears the specified class.
 *
 * @param classIndex    Class index.
 * @param cache         Optional. Specifies what class cache to clear. Player
 *                      cache is ignored.
 */
ClsModel_ClearClass(classIndex, ClassCacheType:cache = ClassCache_Original)
{
    decl attributes[ClassModelAttributes];
    attributes[ClassAttrib_CurrentModel] = -1;
    attributes[ClassAttrib_Model][0] = 0;
    attributes[ClassAttrib_AlphaFadeMode] = ClassAlphaFade_None;
    attributes[ClassAttrib_AlphaInvert] = false;
    attributes[ClassAttrib_AlphaHealth] = 0;
    attributes[ClassAttrib_AlphaMin] = 0;
    attributes[ClassAttrib_AlphaMax] = 0;
    attributes[ClassAttrib_AlphaMinDist] = 0;
    attributes[ClassAttrib_AlphaMaxDist] = 0;
    
    switch (cache)
    {
        case ClassCache_Original:
        {
            ClassModelData[classIndex] = attributes;
        }
        case ClassCache_Modified:
        {
            ClassModelData2[classIndex] = attributes;
        }
    }
}

/**
 * Changes the model on a player.
 *
 * @param client        The client index.
 * @param classIndex    The class to read from.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 * @return  True on success, false otherwise.
 */
ClsModel_ApplyModel(client, classIndex, ClassCacheType:cache = ClassCache_Player)
{
    // TODO: Update code to use new modules and libraries.
    
    /*new bool:isAttributePreset = false;
    new index;
    new ModelTeam:team;
    new access;
    new model;
    
    decl String:model[CLASS_STRING_LEN];
    model[0] = 0;
    
    // Get correct index according to cache type.
    if (cache == ClassCache_Player)
    {
        index = client;
    }
    else
    {
        index = classIndex;
    }
    
    // Get the model path from the specified cache.
    ClsModel_GetModel(index, model, sizeof(model), cache);
    
    // Get model team setting from the specified cache.
    team = ModelsTeamIdToTeam(ClassGetTeamID(index, cachetype));
    
    // Check if the user specified a pre-defined model setting. If so, setup
    // model filter settings.
    if (StrEqual(modelpath, "random", false))
    {
        // Set access filter flags.
        access = MODEL_ACCESS_PUBLIC | MODEL_ACCESS_ADMINS;
        
        // Specify client for including admin models if client is admin.
        index = client;
        
        isAttributePreset = true;
    }
    else if (StrEqual(modelpath, "random_public", false))
    {
        access = MODEL_ACCESS_PUBLIC;
        index = -1;
        isAttributePreset = true;
    }
    else if (StrEqual(modelpath, "random_hidden", false))
    {
        access = MODEL_ACCESS_HIDDEN;
        index = -1;
        isAttributePreset = true;
    }
    else if (StrEqual(modelpath, "random_admin", false))
    {
        access = MODEL_ACCESS_ADMINS;
        index = -1;
        isAttributePreset = true;
    }
    else if (StrEqual(modelpath, "random_mother_zombie", false))
    {
        access = MODEL_ACCESS_MOTHER_ZOMBIES;
        index = -1;
        isAttributePreset = true;
    }
    else if (StrEqual(modelpath, "default", false))
    {
        // Get current model.
        GetClientModel(client, modelpath, sizeof(modelpath));
        
        // Restore original model if not already set.
        if (!StrEqual(ClassOriginalPlayerModel[client], modelpath))
        {
            strcopy(modelpath, sizeof(modelpath), ClassOriginalPlayerModel[client]);
        }
        else
        {
            // Wanted model is already set, don't change.
            return true;
        }
    }
    else if (StrEqual(modelpath, "no_change", false))
    {
        // Do nothing.
        return true;
    }
    
    // Check if model setting is a attribute preset.
    if (isAttributePreset)
    {
        // Get model based on filter settings set up earlier.
        model = ModelsGetRandomModel(index, team, access);
        
        // Check if found.
        if (model >= 0)
        {
            // Get model path.
            ModelsGetFullPath(model, modelpath, sizeof(modelpath));
        }
        else
        {
            // Couldn't find any models based on filter. Fall back to a random
            // public model. Then get its path.
            model = ModelsGetRandomModel(-1, team, MODEL_ACCESS_PUBLIC);
            ModelsGetFullPath(model, modelpath, sizeof(modelpath));
        }
    }
    
    SetEntityModel(client, modelpath);*/
}

/**
 * Updates the alpha value on a player.
 */
ClsModel_UpdateAlpha(client)
{
    // Validate dependencies.
    if (ClsModel_GetHealthFunc == INVALID_FUNCTION)
    {
        // Health data not available. Do nothing.
        return;
    }
    
    new currentHealth = GetClientHealth(client);
    
    Call_StartFunction(GetMyHandle(), ClsModel_GetHealthFunc);
    Call_PushCell(client);
    Call_PushCell(_:ClassCache_Player);
    new maxHealth = Call_Finish(maxHealth);
    
    // Cache attributes.
    decl attributes[ClassModelAttributes];
    attributes = ClassModelPlayerData[client];
    
    new alphaHealth = attributes[ClassAttrib_AlphaHealth];
    new min = attributes[ClassAttrib_AlphaMin];
    new max = attributes[ClassAttrib_AlphaMax];
    new bool:invert = attributes[ClassAttrib_AlphaInvert];
    
    switch (attributes[ClassAttrib_AlphaFadeMode])
    {
        case ClassAlphaFade_Invalid, ClassAlphaFade_None:
        {
            // No health alpha blending.
            return;
        }
        case ClassAlphaFade_Switch:
        {
            // Switch between max (initial) and min depending on health level.
            new alpha;
            if (currentHealth < alphaHealth)
            {
                alpha = invert ? max : min;
                ClsModel_SetAlpha(client, alpha);
            }
            else
            {
                alpha = invert ? min : max;
                ClsModel_SetAlpha(client, alpha);
            }
        }
        case ClassAlphaFade_SwitchOnce:
        {
            // Switch from max to min once when below specified health level.
            if (!ClassModelAlphaSwitched[client] && currentHealth < alphaHealth)
            {
                new alpha = invert ? max : min;
                ClassModelAlphaSwitched[client] = true;
                ClsModel_SetAlpha(client, alpha);
            }
        }
        case ClassAlphaFade_Linear, ClassAlphaFade_Exp:
        {
            // Linear or exponential fading synced with health.
            
            // Only update alpha when health is within the range.
            if (alphaHealth <= currentHealth <= maxHealth)
            {
                new Float:alpha;
                new Float:alphaRange = float(max) - float(min);
                new Float:healthRange = float(maxHealth) - float(alphaHealth);
                
                // Get fade offset value.
                if (attributes[ClassAttrib_AlphaFadeMode] == ClassAlphaFade_Linear)
                {
                    alpha = ClsModel_FadeLinear(healthRange, alphaRange, float(currentHealth));
                }
                else
                {
                    alpha = ClsModel_FadeExp(healthRange, alphaRange, float(currentHealth));
                }
                
                // Invert ofsset.
                alpha = healthRange - alpha;
                
                // Push offset up to match min health offset.
                alpha += alphaHealth;
                
                // Apply alpha.
                ClsModel_SetAlpha(client, RoundToNearest(alpha));
            }
        }
    }
}

/**
 * Returns the (y) value of a point in a linear fading process (from max to zero).
 *
 * @param fadeRange     Fade range (x range).
 * @param valueRange    Range of values to fade (y range).
 * @param point         A value witin the range (x value).
 */
Float:ClsModel_FadeLinear(Float:fadeRange, Float:valueRange, Float:point)
{
    return point * (valueRange / fadeRange);
}

/**
 * Returns the (y) value of a point in a exponential fading process (from max to zero).
 *
 * @param fadeRange     Fade range (x range).
 * @param valueRange    Range of values to fade (y range).
 * @param point         A value witin the fade range (x value).
 */
Float:ClsModel_FadeExp(Float:fadeRange, Float:valueRange, Float:point)
{
    /* Formula info:
     * - A graph shaped like 1/X
     * - Hits Y-axis at Y=valueRange when X is 0
     * - Hits X-axis when X is ~fadeRange
     */
    
    // Credit to Scone for this formula. It may go below the x-axis at the end,
    // but not much, so we cap it.
    // http://forums.alliedmods.net/showthread.php?t=135065
    
    new Float:value = valueRange * ((1 / (point / fadeRange + 0.05) - 1) / 19);
    
    // The formula is not perfect at end x-values, so make sure the value
    // is always positive.
    if (value < 0)
    {
        value = 0.0;
    }
    
    return value;
}

/**
 * Sets a client's alpha value.
 * 
 * @param client    The client index.
 * @param alpha     Alpha value to set. (0-255)
 */
ClsModel_SetAlpha(client, alpha)
{
    // Turn rendermode on, on the client.
    SetEntityRenderMode(client, RENDER_TRANSALPHA);
    
    // Set alpha value on the client.
    Wrapper_SetEntRenderClr(client, 255, 255, 255, alpha);
    
    // Trigger alpha changed event.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell};
    decl any:eventdata[sizeof(eventdatatypes)][1];
    eventdata[0][0] = client;
    eventdata[1][0] = alpha;
    EventMgr_Forward(g_EvOnClassAlphaChanged, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
}

/**
 * Converts a string to a alpha blending mode.
 *
 * @param mode      Mode string to convert.
 *
 * @return          Alpha blending mode, or ClassAlphaFade_Invalid on error.
 */
ClassAlphaModes:ClsModel_StringToMode(const String:mode[])
{
    if (strlen(mode) > 0)
    {
        if (StrEqual(mode, "none", false))
        {
            return ClassAlphaFade_None;
        }
        else if (StrEqual(mode, "switch", false))
        {
            return ClassAlphaFade_Switch;
        }
        else if (StrEqual(mode, "switch_once", false))
        {
            return ClassAlphaFade_SwitchOnce;
        }
        else if (StrEqual(mode, "linear", false))
        {
            return ClassAlphaFade_Linear;
        }
        else if (StrEqual(mode, "exp", false))
        {
            return ClassAlphaFade_Exp;
        }
    }
    
    return ClassAlphaFade_Invalid;
}


/******************************
 *    VALIDATION FUNCTIONS    *
 ******************************/

/**
 * Validates model.
 *
 * Note: Consider this a little expensive for long queries.
 *
 * @param model         Model to validate.
 * @param errCode       Output. Model query parser error code.
 *                      (See MODEL_ERR_* defines in models/modelparser.inc.)
 * @param errPos        Output. Position in query string where it
 *                      failed.
 *
 * @return              True if valid, false otherwise.
 */
public bool:ClsModel_IsValidModel(const String:model[], &errCode, &errPos)
{
    // Skip validation while refactoring model manager.
    return true;
    
    /*errCode = 0;
    errPos = 0;
    
    // Parse model query.
    ModelDB_ParseQuery(0, model, ModelParser_First, INVALID_HANDLE, errCode, errPos);
    
    // Check if query is valid.
    if (errCode == 0)
    {
        return true;
    }
    
    return false;
    */
}

/**
 * Validates alpha mode.
 *
 * @pram mode           Alpha blending mode to validate.
 *
 * @return              True if valid, false otherwise.
 */
public bool:ClsModel_IsValidAlphaMode(ClassAlphaModes:mode)
{
    if (mode != ClassAlphaFade_Invalid)
    {
        return true;
    }
    
    return false;
}

/**
 * Validates alpha value.
 *
 * @pram alpha          Alpha value to validate.
 *
 * @return              True if valid, false otherwise.
 */
public bool:ClsModel_IsValidAlpha(alpha)
{
    if (CLASS_ALPHA_MIN <= alpha <= CLASS_ALPHA_MAX)
    {
        return true;
    }
    
    return false;
}

/**
 * Validates alpha health value.
 *
 * @pram health         Alpha health value to validate.
 *
 * @return              True if valid, false otherwise.
 */
public bool:ClsModel_IsValidAlphaHealth(health)
{
    if (CLASS_ALPHA_HEALTH_MIN <= health <= CLASS_ALPHA_HEALTH_MAX)
    {
        return true;
    }
    
    return false;
}

/**
 * Validates alpha distance value.
 *
 * @pram dist           Distance value to validate.
 *
 * @return              True if valid, false otherwise.
 */
public bool:ClsModel_IsValidAlphaDist(dist)
{
    if (CLASS_ALPHA_DIST_MIN <= dist <= CLASS_ALPHA_DIST_MAX)
    {
        return true;
    }
    
    return false;
}

/***************************
 *    GET/SET FUNCTIONS    *
 ***************************/

/**
 * Gets the model query string from the specified class.
 *
 * @param index         Class index or client index.
 * @param buffer        Destination string buffer.
 * @param maxlen        Size of buffer.
 * @param cache         Specifies what class cache to read from. If player
 *                      cache is used 'index' will be used as a client index.
 *
 * @return              Number of cells written.
 */
public ClsModel_GetModelQuery(classIndex, String:buffer[], maxlen, ClassCacheType:cache)
{
    switch (cache)
    {
        case ClassCache_Original:
        {
            return strcopy(buffer, maxlen, ClassModelData[classIndex][ClassAttrib_Model]);
        }
        case ClassCache_Modified:
        {
            return strcopy(buffer, maxlen, ClassModelData2[classIndex][ClassAttrib_Model]);
        }
        case ClassCache_Player:
        {
            return strcopy(buffer, maxlen, ClassModelData2[classIndex][ClassAttrib_Model]);
        }
    }
    
    return 0;
}
