/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          modelmgr.inc
 *  Type:          Module
 *  Description:   Manages models and model collections.
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
new Module:g_moduleModelMgr;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ModelMgr_GetIdentifier() { return g_moduleModelMgr; }

/**
 * The names of the models config files.
 */
#define MODELMGR_MODELS_FILE       "models.txt"
#define MODELMGR_COLLECTION_FILE   "modelcollections.txt"

/**
 * Default sizes.
 */
#define MODEL_NAME_LEN     64
#define MODEL_STRING_LEN   255

// Obsolete team enum. Keeping this while migrating to VTeam.
enum ModelTeam
{
    ModelTeam_Invalid = -1,
    ModelTeam_Zombies,
    ModelTeam_Humans
}

/**
 * Config indexes.
 */
new ModelCfgIndex;
new ModelCollectionCfgIndex;

// Library dependencies.
#include "zr/libraries/objectlib"

// Module components.
#include "zr/modules/models/model"
#include "zr/modules/models/modeldb"
//#include "zr/modules/models/modelcollections"
#include "zr/modules/models/modelfilter"
//#include "zr/modules/models/modelparser"
#include "zr/modules/models/modelcommands"


/**
 * Register this module.
 */
ModelMgr_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Model manager");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "modelmgr");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manages models and model collections.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleModelMgr = ModuleMgr_Register(moduledata);
    
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnEventsRegister",      "ModelMgr_OnEventsRegister");
    
    // Register config file that this module will use.
    ConfigMgr_Register(g_moduleModelMgr, "ModelMgr_OnConfigReload", "", ModelCfgIndex);
    ConfigMgr_Register(g_moduleModelMgr, "ModelMgr_OnConfigReload", "", ModelCollectionCfgIndex);
    
    // Register commands.
    ModelDB_BuildCommands();
}

/**
 * Plugin is loading.
 */
ModelMgr_OnPluginStart()
{
    // Register the module.
    ModelMgr_Register();
    
    // Create tries.
    //ModelCollectionNameIndex = CreateTrie();
}

/**
 * Register all events here.
 */
public ModelMgr_OnEventsRegister()
{
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnAllPluginsLoaded",      "ModelMgr_OnAllPluginsLoaded");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnPluginEnd",             "ModelMgr_OnPluginEnd");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnAllModulesLoaded",      "ModelMgr_OnAllModulesLoaded");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnModuleEnable",          "ModelMgr_OnModuleEnable");
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnMyModuleEnable",          "ModelMgr_OnMyModuleEnable");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnModuleDisable",         "ModelMgr_OnModuleDisable");
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnMyModuleDisable",         "ModelMgr_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnMapStart",                "ModelMgr_OnMapStart");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnMapEnd",                "ModelMgr_OnMapEnd");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnAutoConfigsBuffered",   "ModelMgr_OnAutoConfigsBuffered");
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnConfigsExecuted",         "ModelMgr_OnConfigsExecuted");
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnClientPutInServer",       "ModelMgr_OnClientPutInServer");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnClientDisconnect",      "ModelMgr_OnClientDisconnect");
    EventMgr_RegisterEvent(g_moduleModelMgr, "Event_OnClientPostAdminCheck",    "ModelMgr_OnClientPostAdminCheck");
    
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_RoundStart",              "ModelMgr_RoundStart");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_RoundFreezeEnd",          "ModelMgr_RoundFreezeEnd");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_RoundEnd",                "ModelMgr_RoundEnd");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_PlayerTeam",              "ModelMgr_PlayerTeam");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_PlayerSpawn",             "ModelMgr_PlayerSpawn");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_PlayerHurt",              "ModelMgr_PlayerHurt");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_PlayerDeath",             "ModelMgr_PlayerDeath");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_PlayerJump",              "ModelMgr_PlayerJump");
    //EventMgr_RegisterEvent(g_moduleModelMgr, "Event_WeaponFire",              "ModelMgr_WeaponFire");
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public ModelMgr_OnEventsReady()
{
}

/**
 * The model manager was enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ModelMgr_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // TODO: Don't let the module load unless the configs cached successfully.
    
    // Load models if not already loaded.
    if (!ModelDB_IsLoaded() && ModelDB_Load() != ModelDBResult_Loaded)
    {
        Format(refusalmsg, maxlen, "%T", "ModelMgr models refuse enable", LANG_SERVER);
        return Plugin_Handled;
    }
    
    // Load model collections if not already loaded.
    /*if (!ModelDB_AreCollectionsLoaded() && !ModelDB_LoadCollections())
    {
        Format(refusalmsg, maxlen, "%T", "ModelMgr model collections refuse enable", LANG_SERVER);
        return Plugin_Handled;
    }*/
    
    return Plugin_Continue;
}

/**
 * The model manager is being disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ModelMgr_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    // Unload.
    ModelDB_Unload();
}

/**
 * The map has started.
 */
public ModelMgr_OnMapStart()
{
    //ModelDB_Load();
    //ModelDB_LoadCollections();
}

/**
 * All convars are set, load the model database.
 */
public ModelMgr_OnConfigsExecuted()
{
    ModelDB_Load();
}

/**
 * Called when a registered config file (by this module) is manually reloaded.
 */
public ModelMgr_OnConfigReload(configindex)
{
    ModelDB_Reload();
    //ModelDB_LoadCollections();
}

public ModelMgr_OnClientPutInServer(client)
{
    // Clear model access cache for this player in case something tries to
    // authorize a model before the cache is updated.
    //ModelMgr_ClearPlayerAccess(client);
}

/**
 * Client is authorized.
 *
 * @param client    Client index.
 */
public ModelMgr_OnClientPostAdminCheck(client)
{
    // Update model access cache for this player.
    //ModelMgr_UpdatePlayerAccess(client);
}


/******************
 *   Public API   *
 ******************/

/**
 * Gets the number of valid models loaded.
 */
/*public ModelMgr_GetModelCount()
{
    return ModelCount;
}*/

/**
 * Updates the entire model access cache for every connected player.
 * Useful when reloading a config.
 */
/*public ModelMgr_UpdateAccessCache()
{
    for (new client = 1; client < MaxClients + 1; client++)
    {
        if (IsClientConnected(client) && IsClientAuthorized(client))
        {
            ModelMgr_UpdatePlayerAccess(client);
        }
    }
}*/

/**
 * Updates the model access cache for a player.
 *
 * @param client    Player to cache.
 */
/*public ModelMgr_UpdatePlayerAccess(client)
{
    // Loop through every model and update authorized-flags.
    for (new model = 0; model < ModelCount; model++)
    {
        ModelHasAccess[client][model] = ModelDB_IsPlayerAuthorized(client, model);
    }
}*/

/**
 * Removes access to all models for a player.
 *
 * @param client    Player to clear.
 */
/*public ModelMgr_ClearPlayerAccess(client)
{
    for (new model = 0; model < ModelCount; model++)
    {
        ModelHasAccess[client][model] = false;
    }
}*/

/**
 * Returns whether a player has access to a model. Reads a cached value.
 *
 * @param client    Player index.
 * @param model     Model index.
 *
 * @return          True if the player has acces, false otherwise.
 */
/*public bool:ModelMgr_HasAccess(client, model)
{
    return ModelHasAccess[client][model];
}*/

/**
 * Returns the total number of models.
 *
 * @param enabledOnly   Only count enabled models.
 *
 * @return              Number of models.
 */
/*public bool:ModelMgr_GetModelCount(bool:enabledOnly = false)
{
    new modelCount = ObjLib_GetCollectionSize(ModelDB_Models);
    
    if (enabledOnly)
    {
        new enabledCount = 0;
        
        // Loop through collection and count enabled models.
        for (new i = 0; i < modelCount; i++)
        {
            new Model:model = ObjLib_GetObjectElementAt(ModelDB_Models, i);
            if (Model_IsEnabled(model))
            {
                enabledCount++;
            }
        }
    }
    else
    {
        return modelCount;
    }
}*/

/**
 * Returns whether the specified model index is valid or not.
 *
 * @param model     Model index to validate.
 *
 * @return          True if valid, false otherwise.
 */
/*public bool:ModelMgr_IsValidIndex(model)
{
    //new modelCount = ModelMgr_GetModelCount(false);
    return model >= 0 && (model < ModelCount < MODELS_MAX);
}*/

/**
 * Gets the index of a model.
 *
 * @param modelID       ID that identifies the model.
 *
 * @return              Model index if found, -1 otherwise.
 */
/*public ModelMgr_GetModelIndex(const String:modelID[])
{
    // Disabled while refactoring.
    return -1;
    
    new model = -1;
    
    // Lookup the model name in the trie index.
    if (!GetTrieValue(ModelDB_ModelIndexMap, modelID, model))
    {
        return -1;
    }
    
    return model;
}*/

/**
 * Gets the index of a model collection.
 *
 * @param collectionName    Name that identifies the model collection.
 *
 * @return                  Model collection index if found, -1 otherwise.
 */
/*public ModelMgr_GetCollectionIndex(const String:collectionName[])
{
    new collection = -1;
    
    // Lookup the collection name in the trie index.
    if (!GetTrieValue(ModelCollectionNameIndex, collectionName, collection))
    {
        return -1;
    }
    
    return collection;
}*/

/*
 * Gets the full model file path for the specified model.
 *
 * @param model     Model index.
 * @param file      Destination string buffer.
 * @param maxlen    Size of destination buffer.
 *
 * @return          Number of cells written.
 */
/*public ModelMgr_GetModelFile(model, String:file[], maxlen)
{
    new count;
    
    // Validate index.
    if (!ModelMgr_IsValidIndex(model))
    {
        return 0;
    }
    
    // Build model path.
    count = strcopy(file, maxlen, ModelData[model][Model_Path]);
    
    // Append file name.
    count += StrCat(file, maxlen, ModelData[model][Model_FileName]);
    
    // Append file extension.
    count += StrCat(file, maxlen, ".mdl");
    
    return count;
}*/

/**
 * Parses a model query string for a client.
 *
 * @param client    Client index.
 * @param query     Model query string.
 * @param mode      Parser mode.
 * @param list      Optional. List to put models in if mode is ModelParser_List.
 *
 * @return          -2 if it's a predefined option (mode).
 *                  If in list mode: Number of models added to the list.
 *                  Otherwise model index if found, or -1 if no models found.
 */
/*public ModelMgr_ParseQuery(client, const String:query[], ModelParserMode:mode, Handle:list)
{
    // Wrap internal parser.
    return ModelDB_ParseQuery(client, query, mode, list);
}*/

/**
 * Returns whether the specified word is reserved.
 *
 * @param word      Word to check.
 *
 * @return          True if reserved, false otherwise.
 */
public bool:ModelMgr_IsReservedWord(const String:word[])
{
    // Filter words.
    if (ModelDB_StringToFilter(word) != -1)
    {
        return true;
    }
    
    // Auth modes.
    if (ModelDB_StringToAuthMode(word) != ModelAuth_Invalid)
    {
        return true;
    }
    
    // Teams.
    if (ModelDB_StringToTeam(word) != ModelTeam_Invalid)
    {
        return true;
    }
    
    // Parser modes.
    /*if (ModelDB_StringToParserMode(word) != ModelParser_Invalid)
    {
        return true;
    }*/
    
    return false;
}
