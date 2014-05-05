/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          modelfilter.inc
 *  Type:          Module include
 *  Description:   Model filtering tools for the model db.
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


/**
 * @section Filtering flags.
 */
#define MODEL_FILTER_RANDOM         (1<<0)  /** Pick a random model if more than one. */
#define MODEL_FILTER_IGNORE_AUTH    (1<<1)  /** Ignore model restrictions. */
#define MODEL_FILTER_ZOMBIE         (1<<2)  /** Only zomibe models. */
#define MODEL_FILTER_HUMAN          (1<<3)  /** Only human models. */
#define MODEL_FILTER_BOTH_TEAMS     (1<<4)  /** Include models from both teams. */
#define MODEL_FILTER_PUBLIC         (1<<5)  /** Only public models (no restrictions). */
#define MODEL_FILTER_ADMIN          (1<<6)  /** Only admin models. */
#define MODEL_FILTER_MOTHER_ZOMB    (1<<7)  /** Only mother zombie models. */

#define MODEL_FILTER_MAX            ((1<<9) - 1)    /** Max value, all flags enabled. */
/**
 * @endsection
 */

/*____________________________________________________________________________*/

/**
 * Gets all models according to the specified filter.
 *
 * @param filters   Filtering flags. See MODEL_FILTER_* constants.
 *                  MODEL_FILTER_RANDOM is ignored.
 * @param client    Optional. Client to use for authorization. Default is -1.
 *                  The server will always get access.
 * @param models    Optional. Custom list of source model objects. If not
 *                  specified the model database will be used as source.
 *
 * @return          Handle to result list. Must be closed when no longer used.
 */
public Handle:ModelDB_GetModels(filters, client /*= -1*/, Handle:models /*= INVALID_HANDLE*/)
{
    ModelDB_ValidateFilter(filters);
    
    // Get source list.
    new Handle:modelList = ModelDB_GetSourceList(models);
    new modelCount = GetArraySize(modelList);
    
    new Handle:resultList = CreateArray();
    
    // Check if there are no models.
    if (modelCount == 0)
    {
        // No models. Just return an empty list.
        return resultList;
    }
    
    // Loop through each model and test filters.
    for (new i = 0; i < modelCount; i++)
    {
        new Model:model = GetArrayCell(modelList, i); 
        if (ModelDB_FilterTest(client, filters, model))
        {
            // Model passed filter.
            PushArrayCell(resultList, model);
        }
    }
    
    return resultList;
}

/*____________________________________________________________________________*/

/**
 * Gets a model according to the specified filter.
 *
 * @param filters   Filtering flags. See MODEL_FILTER_* constants.
 *                  MODEL_FILTER_RANDOM is ignored.
 * @param client    Optional. Client to use for authorization. Default is -1.
 *                  The server will always get access.
 * @param models    Optional. Custom list of source model objects. If not
 *                  specified the model database will be used as source.
 *
 * @return          Model object if found, otherwise INVALID_MODEL.
 */
public Model:ModelDB_GetModel(filters, client /*= -1*/, Handle:models /*= INVALID_HANDLE*/)
{
    new Handle:resultList = Handle:ModelDB_GetModels(filters, client, models);
    new modelCount = GetArraySize(resultList);
    
    new Model:model = INVALID_MODEL;
    
    // Check if there are no models.
    if (modelCount == 0)
    {
        CloseHandle(resultList);
        return INVALID_MODEL;
    }
    
    if (filters & MODEL_FILTER_RANDOM)
    {
        // Pick random model.
        new randIndex = Math_GetRandomInt(0, modelCount - 1);
        model = Model:GetArrayCell(resultList, randIndex);
    }
    else
    {
        // Pick first model.
        model = Model:GetArrayCell(resultList, 0);
    }
    
    CloseHandle(resultList);
    return model;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the specified model passes a filter.
 *
 * @param client    Client that will use the model. Used for authorization.
 *                  The server will always get access.
 * @param filters   Filtering flags.
 * @param model     Model to test.
 *
 * @return          True if passed, false otherwise.
 */
public bool:ModelDB_FilterTest(client, filters, Model:model)
{
    // Cache relevant attributes for readability.
    new VTeam:team = Model_GetTeam(model);
    new bool:motherZombiesOnly = Model_GetMotherZombiesOnly(model);
    new bool:adminsOnly = Model_GetAdminsOnly(model);
    new bool:isHidden = Model_IsHidden(model);
    new ModelAuthMode:authMode = Model_GetAuthMode(model);
    
    // Team filters.
    if (!(filters & MODEL_FILTER_BOTH_TEAMS))
    {
        // Both teams flag is not set. Check individual team flags.
        
        // Zombie team.
        if (filters & MODEL_FILTER_ZOMBIE
            && team != VTeam_Zombie)
        {
            // Not a zombie model.
            return false;
        }
        
        // Human team.
        if (filters & MODEL_FILTER_HUMAN
            && team != VTeam_Human)
        {
            // Not a human model.
            return false;
        }
    }
    
    // Public model.
    if (filters & MODEL_FILTER_PUBLIC
        && (isHidden || authMode == ModelAuth_Disabled))
    {
        // Not public (hidden or with auth mode enabled).
        return false;
    }
    
    // Admins only.
    if (filters & MODEL_FILTER_ADMIN
        && !adminsOnly)
    {
        // Not an admin model.
        return false;
    }
    
    // Mother zombies only.
    if (filters & MODEL_FILTER_MOTHER_ZOMB
        && !motherZombiesOnly)
    {
        // Not a mother zombie model.
        return false;
    }
    
    // Authorization. Check if not ignored, and not authorized. Server always
    // get access (for listings in console).
    if (client >= 0
        && client != SERVER_INDEX
        && filters &~ MODEL_FILTER_IGNORE_AUTH
        && !ModelDB_HasAccess(client, model))
    {
        // Not authorized.
        return false;
    }
    
    return true;
}

/*____________________________________________________________________________*/

/**
 * Gets the specified list or main model collection.
 */
Handle:ModelDB_GetSourceList(Handle:models = INVALID_HANDLE)
{
    if (models != INVALID_HANDLE)
    {
        return models;
    }
    else
    {
        return ModelDB_GetCollectionElements();
    }
}

/*____________________________________________________________________________*/

/**
 * Throws an error if the specified model filter has invalid values.
 */
ModelDB_ValidateFilter(modelFilter)
{
    if (modelFilter < 0)
    {
        ThrowError("Filter value out of bounds: %d. Too low, must be zero or positive.", modelFilter);
    }
    
    if (modelFilter > MODEL_FILTER_MAX)
    {
        ThrowError("Filter value out of bounds: %d. Too high, maximum is %d.", modelFilter, MODEL_FILTER_MAX);
    }
    
    new bool:auth = (modelFilter & MODEL_FILTER_IGNORE_AUTH) == 0;
    new bool:zombie = (modelFilter & MODEL_FILTER_ZOMBIE) > 0;
    new bool:human = (modelFilter & MODEL_FILTER_HUMAN) > 0;
    new bool:bothTeams = (modelFilter & MODEL_FILTER_BOTH_TEAMS) > 0;
    new bool:publicModel = (modelFilter & MODEL_FILTER_PUBLIC) > 0;
    new bool:adminModel = (modelFilter & MODEL_FILTER_ADMIN) > 0;
    new bool:motherZombie = (modelFilter & MODEL_FILTER_MOTHER_ZOMB) > 0;
    
    new bool:singleTeam = false;
    if (zombie || human)
    {
        singleTeam = true;
    }
    
    if (zombie && human)
    {
        ThrowError("Conflicting flags: MODEL_FILTER_ZOMBIE and MODEL_FILTER_HUMAN.");
    }
    
    if (bothTeams && (singleTeam || motherZombie))
    {
        ThrowError("Conflicting flags: MODEL_FILTER_BOTH_TEAMS cannot be enabled when a team-specific flag is enabled.");
    }
    
    if (auth && adminModel && publicModel)
    {
        ThrowError("Conflicting flags: MODEL_FILTER_ADMIN and MODEL_FILTER_PUBLIC when authorization is enabled.");
    }
}

/*____________________________________________________________________________*/

/****************************
 *   Conversion functions   *
 ****************************/

/**
 * Converts the filter name into a filter value.
 *
 * @param filterName    Name to convert.
 *
 * @return              Filter value, or -1 if failed.
 */
ModelDB_StringToFilter(const String:filterName[])
{
    if (strlen(filterName) == 0)
    {
        return -1;
    }
    else if (StrEqual(filterName, "random", false))
    {
        return MODEL_FILTER_RANDOM;
    }
    else if (StrEqual(filterName, "no_auth", false))
    {
        return MODEL_FILTER_IGNORE_AUTH;
    }
    else if (StrEqual(filterName, "zombie", false))
    {
        return MODEL_FILTER_ZOMBIE;
    }
    else if (StrEqual(filterName, "human", false))
    {
        return MODEL_FILTER_HUMAN;
    }
    else if (StrEqual(filterName, "both_teams", false))
    {
        return MODEL_FILTER_BOTH_TEAMS;
    }
    else if (StrEqual(filterName, "public", false))
    {
        return MODEL_FILTER_PUBLIC;
    }
    else if (StrEqual(filterName, "admin", false))
    {
        return MODEL_FILTER_ADMIN;
    }
    else if (StrEqual(filterName, "mother_zombie", false))
    {
        return MODEL_FILTER_MOTHER_ZOMB;
    }
    
    return -1;
}
