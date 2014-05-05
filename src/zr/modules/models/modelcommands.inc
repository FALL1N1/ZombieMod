/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          modelcommands.inc
 *  Type:          Module include
 *  Description:   Model command interface (administration / debug).
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

static ModelDB_InspectorBuilt = false;

/*____________________________________________________________________________*/

/**
 * Builds the model database administration commands.
 */
ModelDB_BuildCommands()
{
    if (ModelDB_InspectorBuilt)
    {
        return;
    }
    
    // Prepare the global object inspector, if not ready.
    ObjLib_BuildInspector(PROJECT_CMD_PREFIX);
    
    // Admin commands.
    Project_RegConsoleCmd("model_help", ModelCmd_Help, "Prints a list of model inspector commands.");
    Project_RegConsoleCmd("model_list", ModelCmd_ListObjects, "Prints a list of models.");
    
    ModelDB_InspectorBuilt = true;
}

/*____________________________________________________________________________*/

/**
 * Prints a list of model objects loaded.
 *
 * @param client        Command sender.
 * @param argc          Number of arguments.
 */
public Action:ModelCmd_Help(client, argc)
{
    // TODO: Need a function to insert command prefix.
    Project_ReplyToCommand(client, "Model inspector commands:");
    Project_ReplyToCommand(client, "<cmdprefix>_object_help - List of commands in object inspector. Use the object inspector to inspect model objects.");
    Project_ReplyToCommand(client, "<cmdprefix>_model_help  - Prints this list.");
    Project_ReplyToCommand(client, "<cmdprefix>_model_list  - Prints a list of models.");
    return Plugin_Handled;
}

/*____________________________________________________________________________*/

/**
 * Prints a list of model objects loaded.
 *
 * @param client        Command sender.
 * @param argc          Number of arguments.
 */
public Action:ModelCmd_ListObjects(client, argc)
{
    // Check if client has access.
    if (!AccessMgr_HasAccess(client, g_moduleModelMgr))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No access to command");
        return Plugin_Handled;
    }
    
    new Collection:models = ModelDB_GetCollection();
    new modelCount = ObjLib_GetCollectionSize(models);
    
    new String:id[MODEL_STRING_LEN];
    
    // Loop through each model in the main collection.
    for (new i = 0; i < modelCount; i++)
    {
        new Model:model = Model:ObjLib_GetObjectElementAt(models, i);
        
        Model_GetId(model, id, sizeof(id));
        
        // Print object reference and model ID.
        ReplyToCommand(client, "0x%X\t%s", model, id);
    }
    
    ReplyToCommand(client, "\n%d models.", modelCount);
    
    return Plugin_Handled;
}

/*____________________________________________________________________________*/



// Old code: -------------------------------------------------------------------

/*stock ModelCmd_RegCommands()
{
    Project_RegConsoleCmd("models_list", ModelCmd_List, "Lists all models. Usage: <prefix>_models_list [<client target> <query>]");
    Project_RegConsoleCmd("models_dump", ModelCmd_Dump, "Dumps model data to the console. Usage: <prefix>_models_dump <model|index>");
}*/

/*stock Action:ModelCmd_List(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleModelMgr))
    {
        return Plugin_Continue;
    }
    
    SetGlobalTransTarget(client);
    
    // TODO: Check if client has access to this command.
    if (...)
    {
        
    }
    
    // Check if listing all models.
    if (argc == 0)
    {
        // No parameters. Print syntax.
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ModelMgr list syntax", PROJECT_CMD_PREFIX);
        ReplyToCommand(client, "\n");
        
        // Print a list with all models.
        ModelCmd_PrintList(client);
    }
    else if (argc >= 2)
    {
        // Parse query.
        
        decl String:query[MODEL_MAX_QUERY_LEN];
        query[0] = 0;
        decl String:argbuffer[64 + MODEL_MAX_QUERY_LEN];    // 64 makes space for command name and target parameter.
        argbuffer[0] = 0;
        
        // Get the query.
        GetCmdArgString(argbuffer, sizeof(argbuffer));      // Full argument string (excluding command name).
        new pos = FindCharInString(argbuffer, ' ') + 1;     // Jump after target parameter.
        strcopy(query, sizeof(query), argbuffer[pos]);
        
        GetCmdArg(1, argbuffer, sizeof(argbuffer));
        new target;
        
        // Check if it's a server target.
        if (StrEqual(argbuffer, "@server", false))
        {
            target = 0;
        }
        else
        {
            // It's a name.
            target = FindTarget(client, argbuffer);
        }
        
        // Check if valid target.
        if (target < 0)
        {
            // Invalid target. FindTarget prints the error message.
            return Plugin_Handled;
        }
        
        // Prepare list.
        new Handle:list = CreateArray();
        
        // Run query.
        new errPos = 0;
        new errCode = 0;
        ModelDB_ParseQuery(target, query, ModelParser_List, list, errCode, errPos);
        
        // Check for parse errors.
        if (errCode > 0)
        {
            decl String:errMsg[MODEL_STRING_LEN];
            errMsg[0] = 0;
            decl String:errMarker[MODEL_MAX_QUERY_LEN];
            errMarker[0] = 0;
            
            // Print error message.
            ModelDB_ErrCodeToString(errCode, errMsg, sizeof(errMsg));
            ReplyToCommand(client, "Parse error (error code = %d, position = %d): %s\n", errCode, errPos, errMsg);
            
            // Print query string with marker of error position.
            ReplyToCommand(client, query);
            ModelDB_GetStringMarker(errPos, errMarker, sizeof(errMarker));
            ReplyToCommand(client, errMarker);
            
            return Plugin_Handled;
        }
        
        // Print list.
        ModelCmd_PrintList(client, list);
    }
    else
    {
        // Invalid command. Print syntax.
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ModelMgr list syntax", PROJECT_CMD_PREFIX);
    }
    
    return Plugin_Handled;

}*/

/*stock Action:ModelCmd_Dump(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleModelMgr))
    {
        return Plugin_Continue;
    }
    
    return Plugin_Handled;
}*/


/***************
 *   Helpers   *
 ***************/

/**
 * Prints a model list to the console.
 *
 * @param client    Client or server index.
 * @param list      Optional. List of model indexes. If not specified it will
 *                  print all models.
 */
/*stock ModelCmd_PrintList(client, Handle:list = INVALID_HANDLE)
{
    static const String:fmt[] = "%-12d %-12s %s";
    static const String:fmtHeader[] = "%-12s %-12s %s\n--------------------------------------------------";
    
    new bool:hasList = (list != INVALID_HANDLE);
    new count = hasList ? GetArraySize(list) : ModelCount;
    
    SetGlobalTransTarget(client);
    
    if (count == 0)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "ModelMgr no models found");
        return;
    }
    
    // Print header.
    decl String:strIndex[32];
    decl String:strTeam[32];
    decl String:strName[32];
    Format(strIndex, sizeof(strIndex), "%t", "ZR header index");
    Format(strTeam, sizeof(strTeam), "%t", "ZR header team");
    Format(strName, sizeof(strName), "%t", "ZR header name");
    ReplyToCommand(client, fmtHeader, strIndex, strTeam, strName);
    
    decl String:teamHumans[32];
    decl String:teamZombies[32];
    Format(teamHumans, sizeof(teamHumans), "%t", "ZR team humans");
    Format(teamZombies, sizeof(teamZombies), "%t", "ZR team zombies");
    
    // Loop through each model.
    new model;
    for (new i = 0; i < count; i++)
    {
        model = hasList ? GetArrayCell(list, i) : i;
        
        // Print model info.
        if (ModelData[model][Model_Team] == ModelTeam_Zombies)
        {
            // Zombie.
            ReplyToCommand(client, fmt, model, teamZombies, ModelData[model][Model_Name]);
        }
        else
        {
            // Human.
            ReplyToCommand(client, fmt, model, teamHumans, ModelData[model][Model_Name]);
        }
    }
}*/
