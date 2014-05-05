/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          modeldb.inc
 *  Type:          Module include
 *  Description:   Model database.
 *
 *  Copyright (C) 2009-2014  Greyscale, Richard Helgeby
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

/*____________________________________________________________________________*/

/**
 * Whether models are loaded.
 */
static bool:ModelDB_Loaded = false;

/*____________________________________________________________________________*/

/**
 * Main model collection.
 */
static Collection:ModelDB_Models = INVALID_COLLECTION;

/*____________________________________________________________________________*/

/**
 * Mapping of model IDs to model objects. ADT Trie.
 */
static Handle:ModelDB_ModelIDMap = INVALID_HANDLE;

/*____________________________________________________________________________*/

/**
 * Mapping of model ID to model index. ADT Trie.
 *
 * Note: Indexes should only be used temporarily. They may change if the model
 *       database is reloaded or the map is changed.
 */
static Handle:ModelDB_ModelIndexMap = INVALID_HANDLE;

/*____________________________________________________________________________*/

/**
 * Loading results.
 */
enum ModelDBLoadResult
{
    ModelDBResult_Loaded,
    ModelDBResult_AlreadyLoaded,
    ModelDBResult_InvalidPath,
    ModelDBResult_NoModels,
    ModelDBResult_FatalError
}

/*____________________________________________________________________________*/

/**
 * Loads models from file.
 *
 * @return      Loading result.
 */
ModelDBLoadResult:ModelDB_Load()
{
    if (ModelDB_Loaded)
    {
        // Models are already loaded. They must be unloaded before reloading.
        // TODO: We could support appending models from another file.
        return ModelDBResult_AlreadyLoaded;
    }
    
    // Get the model config path from the gamerules module. It will figure out
    // which model config file to use according to map configs and cvars.
    decl String:configPath[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(MODELMGR_MODELS_FILE, configPath);
    
    // Validate.
    if (ConfigMgr_ValidateFile(configPath))
    {
        ConfigMgr_WriteString(g_moduleModelMgr, ModelCfgIndex, ConfigData_Path, CM_DATA_PATH, configPath);
    }
    else
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\". Disabling module.", configPath);
        return ModelDBResult_InvalidPath;
    }
    
    // Append sourcemod path.
    new String:fullPath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, fullPath, sizeof(fullPath), configPath);
    
    PrintToServer("Loading models from file: %s", fullPath);
    
    // Prepare types if not built.
    Model_BuildType();
    Model_BuildMainRootType();
    
    // Load and prepare the KeyValue tree.
    new Handle:kv = CreateKeyValues("zombiereloaded");
    FileToKeyValues(kv, fullPath);
    KvRewind(kv);
    
    // Prepare parser context.
    new Object:parseContext = ObjLib_GetParseContext(
            "ModelDB",              // name
            Model_MainRootType,     // rootType
            true,                   // ignoreEmptySections
            true,                   // continueOnError
            ModelDB_ErrorHandler);  // errorHandler
    
    // Parse models. Implies validation. Using list mode until object mode is
    // ready.
    new Handle:list = ObjLib_ParseInListMode(kv, parseContext);
    
    // The collection object is stored in the list. 
    if (GetArraySize(list) == 0)
    {
        // No models found.
        ObjLib_DeleteObject(parseContext);
        CloseHandle(kv);
        CloseHandle(list);
        return ModelDBResult_NoModels;
    }
    
    // Extract model collection from the list.
    ModelDB_Models = Collection:GetArrayCell(list, 0);
    
    // Validate collection object.
    if (ModelDB_Models == INVALID_COLLECTION)
    {
        // Fatal error. Error when parsing model collection sub section.
        ObjLib_DeleteObject(parseContext);
        CloseHandle(kv);
        CloseHandle(list);
        return ModelDBResult_FatalError;
    }
    
    // Do post processing of models (additional validation, precaching, etc).
    ModelDB_PostProcess(ModelDB_Models);
    
    // Cleanup.
    ObjLib_DeleteObject(parseContext);
    CloseHandle(kv);
    CloseHandle(list);
    
    ModelDB_Loaded = true;
    return ModelDBResult_Loaded;
}

/*____________________________________________________________________________*/

/**
 * Unloads the model database.
 */
stock ModelDB_Unload()
{
    // Check if already unloaded.
    if (!ModelDB_IsLoaded())
    {
        return;
    }
    
    ModelDB_DeleteMappings();
    
    // Delete main collection and models in it.
    ModelDB_DeleteMainCollection();
    
    // Delete types.
    Model_DeleteType();
    Model_DeleteMainRootType();
    
    ModelDB_Loaded = false;
}

/*____________________________________________________________________________*/

/**
 * Reloads the model database.
 */
stock ModelDB_Reload()
{
    // Unload and load.
    ModelDB_Unload();
    ModelDB_Load();
}

/*____________________________________________________________________________*/

/**
 * Returns whether the main model database is loaded and validated.
 */
bool:ModelDB_IsLoaded()
{
    return ModelDB_Loaded;
}

/*____________________________________________________________________________*/

/**
 * Returns the main model collection object.
 */
Collection:ModelDB_GetCollection()
{
    return ModelDB_Models;
}

/*____________________________________________________________________________*/

/**
 * Gets the model collection elements.
 *
 * @return      ADT array with model objects.
 */
Handle:ModelDB_GetCollectionElements()
{
    return ObjLib_CollectionGetElements(ModelDB_Models);
}

/*____________________________________________________________________________*/

/**
 * Returns number of model objects in the main model collection.
 */
ModelDB_Size()
{
    new Handle:elements = ModelDB_GetCollectionElements();
    return GetArraySize(elements);
}

/*____________________________________________________________________________*/

/**
 * Executes post processing on all enabled models.
 *
 * @param modelCollection       Collection of models to do post processing on.
 */
ModelDB_PostProcess(Collection:modelCollection)
{
    // Loop through models and do post processing for each model.
    new modelCount = ObjLib_GetCollectionSize(modelCollection);
    for (new i = 0; i < modelCount; i++)
    {
        new Model:model = Model:ObjLib_GetObjectElementAt(modelCollection, i);
        
        // Only process model if enabled.
        if (Model_IsEnabled(model))
        {
            // Do post processing on model.
            ModelDB_PostProcessModel(model, i);
        }
    }
}

/*____________________________________________________________________________*/

/**
 * Executes post processing on the specified model.
 *
 * @param model         Model to do post processing on.
 * @param modelIndex    Index value to map model to.
 */
ModelDB_PostProcessModel(Model:model, modelIndex)
{
    // Add model files to downloads table.
    new downloadCount = ModelDB_AddToDownloadsTable(model);
    if (downloadCount == 0)
    {
        // No model files were found. Disable model.
        Model_SetEnabled(model, false);
        
        // Get ID.
        new String:id[MODEL_STRING_LEN];
        Model_GetId(model, id, sizeof(id));
        
        // Log a warning.
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Warning: Couldn't find any model files for model with ID \"%s\". Disabling model.", id);
    }
    
    // Precache model.
    ModelDB_PrecacheModel(model);
    
    // Add model to ID and index mappings.
    ModelDB_AddToMappings(model, modelIndex);
}

/*____________________________________________________________________________*/

/**
 * Deletes all models in the specified collection.
 *
 * Note: The collection and its elements are not removed.
 *
 * @param modelCollection       Collection of models to delete.
 */
ModelDB_DeleteModels(Collection:modelCollection)
{
    // Loop through models and do post processing for each model.
    new modelCount = ObjLib_GetCollectionSize(modelCollection);
    for (new i = 0; i < modelCount; i++)
    {
        new Model:model = Model:ObjLib_GetObjectElementAt(modelCollection, i);
        
        ObjLib_DeleteObject(Object:model);
    }
}

/*____________________________________________________________________________*/

/**
 * Deletes the main model collection and the models in it.
 */
ModelDB_DeleteMainCollection()
{
    ModelDB_DeleteModels(ModelDB_Models);
    ObjLib_DeleteCollection(ModelDB_Models);
}

/*____________________________________________________________________________*/

/**
 * Adds the model's files to the downloads table. Includes all files with the
 * same file name (but different extensions) in the specified directory path.
 *
 * Note: This function does not add the material files the model is using.
 *
 * @param model         Model object.
 *
 * @return              Number of files added to downloads table.
 */
ModelDB_AddToDownloadsTable(Model:model)
{
    new String:buffer[PLATFORM_MAX_PATH];
    new String:path[PLATFORM_MAX_PATH];
    new String:fileName[PLATFORM_MAX_PATH];
    new String:directory[PLATFORM_MAX_PATH];
    
    // Get model path.
    Model_GetPath(model, path, sizeof(path));
    
    // Split path and filename. Loop backwards and locate the first directory
    // separator.
    // TODO: Move this to a separate helper function.
    new pathLen = strlen(path);
    for (new pos = pathLen; pos >= 0; pos--)
    {
        // Check for directory separator, supports both Windows and *nix
        // systems.
        if (path[pos] == '/' || path[pos] == '\\')
        {
            // File name starts after this position.
            
            // Extract path up to and inclusive this position. Path separator
            // will be included at the end. Add one to leave space for null
            // terminator.
            strcopy(directory, pos + 1, path);
            
            // Extract filename after this position. Add one to leave space for
            // null terminator.
            strcopy(fileName, sizeof(fileName), path[pos + 1]);
            
            break;
        }
    }
    
    // Open directory with model files.
    // TODO: Will this cause an error with the built in zombie model in CS: GO?
    //       We might need to detect if the model is a stock model or not.
    new Handle:dir = OpenDirectory(directory);
    
    // Check if failed.
    if (dir == INVALID_HANDLE)
    {
        // Get model id so the server admin can identify the model.
        new String:modelId[MODEL_STRING_LEN];
        Model_GetId(model, modelId, sizeof(modelId));
        
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error opening directory \"%s\" for model with id \"%s\".", path, modelId);
        return 0;
    }
    
    // File counter for the current model.
    new downloadCount = 0;
    
    new FileType:type;
    new String:file[64];
    new String:fileShort[64];
    
    // Search for model files with the specified name and add them to
    // downloads table.
    while (ReadDirEntry(dir, file, sizeof(file), type))
    {
        // Skip if entry isn't a file.
        if (type != FileType_File)
        {
            continue;
        }
        
        // Find break point index in the string to get model name. Add one to
        // make space for null terminator.
        // TODO: Search backwards, in case a model has multiple dots in its file
        //       name.
        new breakpoint = FindCharInString(file, '.') + 1;
        strcopy(fileShort, breakpoint, file);
        
        // If this file doesn't match model name, then skip it.
        if (!StrEqual(fileName, fileShort, false))
        {
            continue;
        }
        
        // Format a full path string.
        strcopy(buffer, sizeof(buffer), path);
        Format(buffer, sizeof(buffer), "%s%s", buffer, file);
        
        AddFileToDownloadsTable(buffer);
        downloadCount++;
    }
    
    CloseHandle(dir);
    return downloadCount;
}

/*____________________________________________________________________________*/

/**
 * Precaches the specified model.
 *
 * @param model     Model object.
 *
 * @return          Model index, or 0 on error.
 */
ModelDB_PrecacheModel(Model:model)
{
    new String:path[PLATFORM_MAX_PATH];
    Model_GetPath(model, path, sizeof(path));
    
    new modelIndex = PrecacheModel(path);
    if (modelIndex == 0)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Loading", "Failed to precache model: %s", path);
        return 0;
    }
    
    LogMgr_Print(g_moduleModelMgr, LogType_Debug, "Config Loading", "Precaced model %s (index: %d)", path, modelIndex);
    
    return modelIndex;
}

/*____________________________________________________________________________*/

/**
 * Builds the model maps if not built.
 */
ModelDB_BuildMappings()
{
    if (ModelDB_ModelIDMap == INVALID_HANDLE)
    {
        ModelDB_ModelIDMap = CreateTrie();
    }
    if (ModelDB_ModelIndexMap == INVALID_HANDLE)
    {
        ModelDB_ModelIndexMap = CreateTrie();
    }
}

/*____________________________________________________________________________*/

/**
 * Adds the specified model to the maps.
 *
 * Note: If a model has a duplicate ID it will be disabled, and a warning will
 *       be logged.
 *
 * @param model         Model to add to maps.
 * @param modelIndex    Index value to map model to.
 */
ModelDB_AddToMappings(Model:model, modelIndex)
{
    // Make sure maps are built.
    ModelDB_BuildMappings();
    
    // Get model id.
    new String:id[MODEL_STRING_LEN];
    Model_GetId(model, id, sizeof(id));
    
    // Add model to index map.
    SetTrieValue(ModelDB_ModelIndexMap, id, modelIndex);
    
    // Check if it already exists in the map.
    new dummyBuffer;
    if (GetTrieValue(ModelDB_ModelIDMap, id, dummyBuffer))
    {
        // Duplicate ID detected. Disable this model.
        Model_SetEnabled(model, false);
        
        // Log a warning.
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Duplicate model ID detected: \"%s\". You have at least two model entries with the same ID. IDs must be unique, so use another ID on the other model(s). Disabled model until this is fixed.", id);
        
        // Return early and skip ID mapping of this model.
        return;
    }
    
    // Add model to ID map.
    SetTrieValue(ModelDB_ModelIDMap, id, model);
}

/*____________________________________________________________________________*/

ModelDB_DeleteMappings()
{
    // Close and reset handles of mappings.
    
    if (ModelDB_ModelIDMap != INVALID_HANDLE)
    {
        Util_CloseHandle(ModelDB_ModelIDMap);
    }
    if (ModelDB_ModelIndexMap != INVALID_HANDLE)
    {
        Util_CloseHandle(ModelDB_ModelIndexMap);
    }
}

/*____________________________________________________________________________*/

/**
 * Returns whether a client has access to the specified model.
 *
 * @param client    Client index.
 * @param model     Model to test.
 *
 * @return          True if client has access, false otherwise.
 */
bool:ModelDB_HasAccess(client, Model:model)
{
    // TODO: Implement autorization cache. Update when loading models and when
    //       a client is connected.
    return true;
}

/*____________________________________________________________________________*/

/**
 * General error callback.
 *
 * The return value may be used to decide what to do next. In most cases it's
 * irrelevant and the value is ignored. But the keyvalue parser is using the
 * return value to abort or continue parsing.
 *
 * It would be considered a good habit to return a proper value even if it's
 * ignored in some cases.
 *
 * @param typeDescriptor    Related type descriptor.
 * @param errorType         Type of error.
 * @param message           Error message.
 * @param object            Related object, if available.
 * @param parseContext      Parser context.
 *
 * @return                  What to do next.
 *                          * Plugin_Handled - Error is handled and further
 *                          processing should be aborted.
 *                          * Plugin_Continue - Continue processing if possible.
 *                          This is useful to let the KeyValue parser continue
 *                          parsing remaining keys.
 *                          * Other values are treated as if Plugin_Handled was
 *                          returned, parsing will be aborted.
 */
public Action:ModelDB_ErrorHandler(ObjectType:typeDescriptor, ObjLibError:errorType, const String:message[], Object:object, Object:parseContext)
{
    // Build path to current location.
    /*new String:path[PLATFORM_MAX_PATH];
    ObjLib_KvBuildPath(parseContext, path, sizeof(path));*/
    
    switch (errorType)
    {
        case ObjLibError_InvalidKey:
        {
            LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config validation", "%s  See the manual for valid key names.", message);
        }
        case ObjLibError_TypeMismatch:
        {
            // Bug. Should not happen in this module.
            ThrowError("[BUG] Key type mismatch. This is a bug in Zombie:Reloaded.");
        }
        case ObjLibError_NullDataKey:
        {
            // Bug. Should not happen in this module.
            ThrowError("[BUG] Attempted to read null key. This is a bug in Zombie:Reloaded.");
        }
        case ObjLibError_KeyExist:
        {
            // Bug. ModelDB doesn't create keys, unless the object is mutable,
            // which is also a bug.
            ThrowError("[BUG] Attempted to create key that already exists. This is a bug in Zombie:Reloaded.");
        }
        case ObjLibError_Immutable:
        {
            // Bug. Attempted to modify a type descriptor, ModelDB doesn't do
            // this.
            ThrowError("[BUG] Object is immutable. This is a bug in Zombie:Reloaded.");
        }
        case ObjLibError_ValidationError, ObjLibError_KvUnexpectedKey,
             ObjLibError_KvUnexpectedSection, ObjLibError_KvInvalidSection,
             ObjLibError_KvInvalidConstraint, ObjLibError_KvTypeMismatch,
             ObjLibError_KvCollectionTypeMismatch:
        {
            LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config validation", "%s  See the manual for valid values.", message);
        }
        default:
        {
            // Bug. Unexpected error type.
            ThrowError("[BUG] Unexpected error type: %d  This is a bug in Zombie:Reloaded.", errorType);
        }
    }
    
    // Continue parsing model attributes to check for other invalid values.
    return Plugin_Continue;
}

/*____________________________________________________________________________*/



// Old code: -------------------------------------------------------------------

/**
 * Maximum number of models.
 */
#define MODELS_MAX 64

/**
 * Separator for entries in lists (flags and groups).
 * Note: MUST be just one character. Otherwise parser must be updated too.
 */
#define MODEL_ENTRY_SEPARATOR " "

/**
 * Maximum number of entries in a model query string.
 */
#define MODEL_MAX_ENTRIES 16

/**
 * Model settings structure.
 */
//enum ModelAttributes
//{
//    // General
//    String:Model_DisplayName[MODEL_NAME_LEN],   /** User defined name to be displayed in menus. */
//    String:Model_Name[MODEL_NAME_LEN],          /** Unique name to identify the model. */
//    String:Model_FileName[MODEL_NAME_LEN],      /** File name of model (no file extension). */
//    String:Model_Path[PLATFORM_MAX_PATH],       /** Path to model files. */
//    ModelTeam:Model_Team,                       /** Which team the model belongs to. */
//    
//    // Restrictions
//    bool:Model_MotherZombiesOnly,               /** Only allow mother zombies to use the model. */
//    bool:Model_AdminsOnly,                      /** Only allow admins to use the model. */
//    bool:Model_IsHidden,                        /** Exclude model from random selections. */
//    
//    // Advanced authorization
//    ModelAuthMode:Model_AuthMode,               /** Model authorization mode. */
//    String:Model_Flags[MODEL_STRING_LEN],       /** Flag name whitelist. Separated by comma (,). */
//    String:Model_Groups[MODEL_STRING_LEN]       /** Group name whitelist. Separated by comma (,). */
//}

/**
 * Parsed model data.
 */
//new ModelData[MODELS_MAX][ModelAttributes];

/**
 * Stores whether models are sucessfully loaded or not.
 */
//new bool:ModelsLoaded;

/**
 * Number of valid models.
 */
//new ModelCount;

/**
 * Number of models that failed validation. Used during loading.
 */
//new ModelFailedCount;

/**
 * Handle to trie for fast name based lookup.
 */
//new Handle:ModelNameIndex;

/**
 * Cache that tells whether a player has access to a certain model.
 */
//new bool:ModelHasAccess[MAXPLAYERS + 1][MODELS_MAX];


/**
 * Loads models from file.
 */
/*stock bool:ModelDB_LoadModels()
{
    // Get the config path from the gamerules module.
    decl String:configfile[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(MODELMGR_MODELS_FILE, configfile);
    
    if (ConfigMgr_ValidateFile(configfile))
        ConfigMgr_WriteString(g_moduleModelMgr, ModelCfgIndex, ConfigData_Path, CM_DATA_PATH, configfile);
    else
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\".  Disabling module.", configfile);
        return false;
    }
    
    // Prepare trie.
    if (ModelNameIndex == INVALID_HANDLE)
    {
        ModelNameIndex = CreateTrie();
    }
    else
    {
        ClearTrie(ModelNameIndex);
    }
    
    // Reset loaded-state.
    ModelsLoaded = false;
    
    // Log loading-message.
    LogMgr_Print(g_moduleModelMgr, LogType_Debug, "Config Loading", "Loading models from file \"%s\".", configfile);
    
    // Parse model file.
    ConfigMgr_CacheKv(g_moduleModelMgr, ModelCfgIndex, "ModelDB_LoadModel");
    
    // Check if there are no models.
    if (ModelCount == 0)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: No valid models found in model config file: %s", configfile);
        return false;
    }
    
    // Precache models.
    //ModelDB_PrecacheModels();
    
    // Log loaded-message.
    LogMgr_Print(g_moduleModelMgr, LogType_Debug, "Config Loading", "Models loaded: %d", ModelCount);
    
    ModelsLoaded = true;
    return true;
}*/

/**
 * Loads a single model at the current position in the keyvalue parser.
 *
 * @param kvModels          Keyvalue parser handle.
 * @param sectionIndex      Current section index.
 * @param sectionName       Current section name.
 */
/*stock KvCache:ModelDB_LoadModel(Handle:kvModels, sectionIndex, const String:sectionName[])
{
    decl String:buffer[MODEL_STRING_LEN];
    decl String:path[PLATFORM_MAX_PATH];
    decl String:fileName[MODEL_NAME_LEN];
    new bool:hasErrors = false;
    new temp;
    
    // Check if maximum number of models is reached.
    if (sectionIndex == MODELS_MAX)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Loading", "Warning: Maximum number of models reached (%d), skipping other models from \"%s\" (%d).", MODELS_MAX, sectionName, sectionIndex);
        return KvCache_Hault;
    }
    
    
    // Load attributes
    // ---------------
    
    strcopy(ModelData[ModelCount][Model_Name], MODEL_NAME_LEN, sectionName);
    
    KvGetString(kvModels, "displayname", buffer, sizeof(buffer));
    strcopy(ModelData[ModelCount][Model_DisplayName], MODEL_NAME_LEN, buffer);
    
    KvGetString(kvModels, "filename", fileName, sizeof(fileName));
    strcopy(ModelData[ModelCount][Model_FileName], MODEL_NAME_LEN, fileName);
    
    KvGetString(kvModels, "path", path, sizeof(path));
    strcopy(ModelData[ModelCount][Model_Path], PLATFORM_MAX_PATH, path);
    
    KvGetString(kvModels, "team", buffer, sizeof(buffer));
    ModelData[ModelCount][Model_Team] = ModelDB_StringToTeam(buffer);
    
    KvGetString(kvModels, "mother_zombies_only", buffer, sizeof(buffer));
    ModelData[ModelCount][Model_MotherZombiesOnly] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, buffer) == PhraseToBool_True);
    
    KvGetString(kvModels, "admins_only", buffer, sizeof(buffer));
    ModelData[ModelCount][Model_AdminsOnly] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, buffer) == PhraseToBool_True);
    
    KvGetString(kvModels, "is_hidden", buffer, sizeof(buffer));
    ModelData[ModelCount][Model_IsHidden] = (TransMgr_PhraseToBoolEx(BoolPhrase_YesNo, buffer) == PhraseToBool_True);
    
    KvGetString(kvModels, "auth_mode", buffer, sizeof(buffer));
    ModelData[ModelCount][Model_AuthMode] = ModelDB_StringToAuthMode(buffer);
    
    KvGetString(kvModels, "flags", buffer, sizeof(buffer));
    strcopy(ModelData[ModelCount][Model_Flags], MODEL_STRING_LEN, buffer);
    
    KvGetString(kvModels, "groups", buffer, sizeof(buffer));
    strcopy(ModelData[ModelCount][Model_Groups], MODEL_STRING_LEN, buffer);
    
    
    // Validate attributes
    // -------------------
    
    // Validate name.
    if (StrContains(sectionName, " ") >= 0)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid model name at \"%s\" (%d). Cannot contain spaces.", sectionName, sectionIndex);
        hasErrors = true;
    }
    
    // Build path and check if model file exist.
    strcopy(buffer, sizeof(buffer), path);
    StrCat(buffer, sizeof(buffer), ModelData[ModelCount][Model_FileName]);
    StrCat(buffer, sizeof(buffer), ".mdl");
    if (!FileExists(buffer))
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid model \"path\" or \"filename\" value at \"%s\" (%d). File not found: \"%s\".", sectionName, sectionIndex, buffer);
        hasErrors = true;
    }
    
    // Validate team.
    if (ModelData[ModelCount][Model_Team] == ModelTeam_Invalid)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid model \"team\" value at \"%s\" (%d).", sectionName, sectionIndex);
        hasErrors = true;
    }
    
    // Validate auth mode.
    if (ModelData[ModelCount][Model_AuthMode] == ModelAuth_Invalid)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid model \"auth_mode\" value at \"%s\" (%d).", sectionName, sectionIndex);
        hasErrors = true;
    }
    
    // Validate flags.
    temp = Auth_ValidateFlags(ModelData[ModelCount][Model_Flags], MODEL_ENTRY_SEPARATOR);
    if (temp >= 0)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid \"flags\" value (flag no. %d) at \"%s\" (%d).", temp + 1, sectionName, sectionIndex);
        hasErrors = true;
    }
    
    // Validate groups.
    temp = Auth_ValidateGroups(ModelData[ModelCount][Model_Groups], MODEL_ENTRY_SEPARATOR);
    if (temp >= 0)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid \"groups\" value (group no. %d) at \"%s\" (%d).", temp + 1, sectionName, sectionIndex);
        hasErrors = true;
    }
    
    // Check if there are validation errors.
    if (hasErrors)
    {
        ModelFailedCount++;
        return KvCache_Ignore;
    }
    
    
    // Add model files to downloads table
    // ----------------------------------
    
    // Open directory with model files.
    new Handle:dir = OpenDirectory(path);
    
    // Check if failed.
    if (dir == INVALID_HANDLE)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error opening directory \"%s\" for model \"%s\" (%d).", path, sectionName, sectionIndex);
        return KvCache_Ignore;
    }
    
    // File counter for the current model.
    new downloadCount = 0;
    
    new FileType:type;
    decl String:file[64];
    decl String:fileShort[64];
    
    // Search for model files with the specified name and add them to
    // downloads table.
    while (ReadDirEntry(dir, file, sizeof(file), type))
    {
        // Skip if entry isn't a file.
        if (type != FileType_File)
        {
            continue;
        }
        
        // Find break point index in the string to get model name.
        // Add one to make space for null terminator.
        new breakpoint = FindCharInString(file, '.') + 1;
        strcopy(fileShort, breakpoint, file);
        
        // If this file doesn't match model name, then skip it.
        if (!StrEqual(fileName, fileShort, false))
        {
            continue;
        }
        
        // Format a full path string.
        strcopy(buffer, sizeof(buffer), path);
        Format(buffer, sizeof(buffer), "%s%s", buffer, file);
        
        AddFileToDownloadsTable(buffer);
        downloadCount++;
    }
    
    CloseHandle(dir);
    
    // Check if no model files were found.
    if (!downloadCount)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Couldn't find any model files for \"%s\" (%d). Check \"filename\" and \"path\".", sectionName, sectionIndex);
        ModelFailedCount++;
        return KvCache_Ignore;
    }
    
    // Add name to trie index. If this fails there's already a model
    // with this name.
    if (!SetTrieValue(ModelNameIndex, sectionName, ModelCount, false))
    {
        // Name is already in use.
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Duplicate model name at \"%s\" (%d). Use another section name for this model.", sectionName, sectionIndex);
        ModelFailedCount++;
        return KvCache_Ignore;
    }
    else
    {
        // Model is valid.
        ModelCount++;
        return KvCache_Continue;
    }
}*/

/**
 * Returns whether a player has access to a model or not.
 *
 * Note: This function is expensive. Use ModelMgr_HasAccess to read from a
 *       cached result.
 *
 * @param client    Player index.
 * @param model     Model index.
 *
 * @return          True if the player has access, false otherwise.
 */
/*bool:ModelDB_IsPlayerAuthorized(client, model)
{
    decl String:flagList[MODEL_STRING_LEN];
    decl String:groupList[MODEL_STRING_LEN];
    
    // Get authorization values.
    new ModelAuthMode:authMode = ModelData[model][Model_AuthMode];
    strcopy(flagList, sizeof(flagList), ModelData[model][Model_Flags]);
    strcopy(groupList, sizeof(groupList), ModelData[model][Model_Groups]);
    
    switch (authMode)
    {
        case ModelAuth_Disabled:    // No authorization.
        {
            return true;
        }
        case ModelAuth_Flag:        // Require flag.
        {
            return Auth_ClientHasFlags(client, flagList, MODEL_ENTRY_SEPARATOR);
        }
        case ModelAuth_Group:       // Require group.
        {
            return Auth_IsClientInGroups(client, groupList, MODEL_ENTRY_SEPARATOR);
        }
        case ModelAuth_Either:      // Require flag or group.
        {
            return Auth_IsClientAuthorized(client, groupList, flagList, MODEL_ENTRY_SEPARATOR, Auth_Either);
        }
        case ModelAuth_Both:        // Require flag and group.
        {
            return Auth_IsClientAuthorized(client, groupList, flagList, MODEL_ENTRY_SEPARATOR, Auth_Both);
        }
        case ModelAuth_All:         // Require all flags and all groups.
        {
            return Auth_IsClientAuthorized(client, groupList, flagList, MODEL_ENTRY_SEPARATOR, Auth_All);
        }
    }
    
    // Invalid auth mode (should never happen because model is validated).
    return false;
}*/

/**
 * Returns whether models are currently successfully loaded.
 *
 * @return      True if loaded, false otherwise.
 */
/*bool:ModelDB_AreModelsLoaded()
{
    return ModelsLoaded;
}*/


/***************************
 *  Conversion functions   *
 ***************************/

/**
 * Converts a string to a team value.
 *
 * @param team  String to convert.
 * @return      Team value, or ModelTeam_Invalid on error.
 */
stock ModelTeam:ModelDB_StringToTeam(const String:team[])
{
    if (StrEqual(team, "zombies", false))
    {
        return ModelTeam_Zombies;
    }
    else if (StrEqual(team, "humans", false))
    {
        return ModelTeam_Humans;
    }
    
    return ModelTeam_Invalid;
}

/**
 * Converts a string to a model authorization mode.
 *
 * @param authMode  String to convert.
 *
 * @return          Model authorization mode, or ModelAuth_Invalid on error.
 */
ModelAuthMode:ModelDB_StringToAuthMode(const String:authMode[])
{
    if (strlen(authMode) == 0)
    {
        return ModelAuth_Invalid;
    }
    else if (StrEqual(authMode, "disabled", false))
    {
        return ModelAuth_Disabled;  /* No authorization. */
    }
    else if (StrEqual(authMode, "flag", false))
    {
        return ModelAuth_Flag;      /* Require flag. */
    }
    else if (StrEqual(authMode, "group", false))
    {
        return ModelAuth_Group;     /* Require group. */
    }
    else if (StrEqual(authMode, "either", false))
    {
        return ModelAuth_Either;    /* Require flag or group. */
    }
    else if (StrEqual(authMode, "both", false))
    {
        return ModelAuth_Both;      /* Require flag and group. */
    }
    else if (StrEqual(authMode, "all", false))
    {
        return ModelAuth_All;       /* Require all flags and all groups. */
    }
    
    return ModelAuth_Invalid;       /* Invalid authorization mode. */
}
