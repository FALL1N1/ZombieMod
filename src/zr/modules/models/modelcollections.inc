/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          modelcollections.inc
 *  Type:          Module include
 *  Description:   Model collection database.
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
 * Maximum number of model collections.
 */
#define MODEL_COLLECTIONS_MAX 64

/**
 * Model collection structure.
 */
enum ModelCollectionAttributes
{
    // General
    String:ModelCollection_Name[MODEL_NAME_LEN],    /** User defined name of the collection. */
    Handle:ModelCollection_Array                    /** Handle to array with model references. */
}

/**
 * Parsed model data.
 */
new ModelCollectionData[MODEL_COLLECTIONS_MAX][ModelAttributes];

/**
 * Stores whether model collections are sucessfully loaded or not.
 */
new bool:ModelCollectionsLoaded;

/**
 * Number of valid model collections.
 */
new ModelCollectionCount;

/**
 * Number of model collections that failed validation. Used during loading.
 */
new ModelCollectionFailedCount;

/**
 * Handle to trie for fast name based lookup.
 */
new Handle:ModelCollectionNameIndex;

/**
 * Loads model collections from file.
 */
bool:ModelDB_LoadCollections()
{
    // Stop if there are no models loaded.
    if (ModelCount == 0)
        return false;
    
    // Get the config path from the gamerules module.
    decl String:configfile[PLATFORM_MAX_PATH];
    GameRules_GetConfigPath(MODELMGR_COLLECTION_FILE, configfile);
    
    if (ConfigMgr_ValidateFile(configfile))
        ConfigMgr_WriteString(g_moduleModelMgr, ModelCollectionCfgIndex, ConfigData_Path, CM_DATA_PATH, configfile);
    else
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Fatal_Module, "Config Validation", "Invalid config file path defined in gamerules: \"%s\".  Disabling module.", configfile);
        return false;
    }
    
    // Prepare trie.
    if (ModelCollectionNameIndex == INVALID_HANDLE)
    {
        ModelCollectionNameIndex = CreateTrie();
    }
    else
    {
        ClearTrie(ModelCollectionNameIndex);
    }
    
    // Reset loaded-state.
    ModelCollectionsLoaded = false;
    
    // Log loading-message.
    LogMgr_Print(g_moduleModelMgr, LogType_Debug, "Config Loading", "Loading model collections from file \"%s\".", configfile);
    
    // Parse model file.
    ConfigMgr_CacheKv(g_moduleModelMgr, ModelCollectionCfgIndex, "ModelDB_LoadCollection");
    
    // Log loaded-message.
    LogMgr_Print(g_moduleModelMgr, LogType_Debug, "Config Loading", "Model collections loaded: %d", ModelCollectionCount);
    
    ModelCollectionsLoaded = true;  // This can be true even if there are no collections (it's optional).
    return true;
}


/**
 * Loads a single model at the current position in the keyvalue parser.
 *
 * @param kvCollections     Keyvalue parser handle.
 * @param sectionIndex      Current section index.
 * @param sectionName       Current section name.
 */
public KvCache:ModelDB_LoadCollection(Handle:kvCollections, sectionIndex, const String:sectionName[])
{
    decl String:buffer[MODEL_STRING_LEN];
    new bool:hasErrors = false;
    
    // Check if maximum number of models is reached.
    if (sectionIndex == MODEL_COLLECTIONS_MAX)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Warning: Maximum number of model collections reached (%d), skipping other collections from \"%s\" (%d).", MODEL_COLLECTIONS_MAX, sectionName, sectionIndex);
        return KvCache_Hault;
    }
    
    // Prepare collection array.
    new Handle:models = ModelCollectionData[ModelCollectionCount][ModelCollection_Array];
    if (models != INVALID_HANDLE)
    {
        ClearArray(models);
    }
    else
    {
        models = CreateArray();
    }
    
    // Get collection name.
    strcopy(ModelCollectionData[ModelCollectionCount][ModelCollection_Name], MODEL_NAME_LEN, sectionName);
    
    // Validate collection name.
    if (StrContains(sectionName, " ") >= 0)
    {
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid model collection name at \"%s\" (%d). Cannot contain spaces.", sectionName, sectionIndex);
        hasErrors = true;
    }
    
    // Loop through collection entries.
    KvGotoFirstSubKey(kvCollections, false);
    do
    {
        // Get collection name.
        KvGetString(kvCollections, "name", buffer, sizeof(buffer));
        
        // Validate name and add to array.
        new model = ModelMgr_GetModelIndex(buffer);
        if (model >= 0)
        {
            PushArrayCell(models, model);
        }
        else
        {
            LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Error: Invalid model name \"%s\" in model collection \"%s\" (%d).", buffer, sectionName, sectionIndex);
            hasErrors = true;
        }
    } while (KvGotoNextKey(kvCollections, false));
    
    // Go one level up (to the collection section).
    KvGoBack(kvCollections);
    
    // Check if there are validation errors.
    if (hasErrors)
    {
        ModelCollectionFailedCount++;
        CloseHandle(models);
        ModelCollectionData[ModelCollectionCount][ModelCollection_Array] = INVALID_HANDLE;
        return KvCache_Ignore;
    }
    
    // Add name to trie index. If this fails there's already a collection
    // with this name.
    if (!SetTrieValue(ModelCollectionNameIndex, sectionName, ModelCollectionCount, false))
    {
        // Name is already in use.
        LogMgr_Print(g_moduleModelMgr, LogType_Error, "Config Validation", "Duplicate model collection name at \"%s\" (%d). Use another section name for this model collection.", sectionName, sectionIndex);
        ModelCollectionFailedCount++;
        return KvCache_Ignore;
    }
    else
    {
        // Model collection is valid.
        
        // Set array.
        ModelCollectionData[ModelCollectionCount][ModelCollection_Array] = models;
        ModelCollectionCount++;
        return KvCache_Continue;
    }
}

/**
 * Returns whether model collections are currently successfully loaded.
 *
 * @return      True if loaded, false otherwise.
 */
bool:ModelDB_AreCollectionsLoaded()
{
    return ModelCollectionsLoaded;
}
