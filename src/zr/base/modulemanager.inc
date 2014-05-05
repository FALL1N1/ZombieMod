/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          modulemanager.inc
 *  Type:          Base
 *  Description:   Manages project modules.
 *
 *  Copyright (C) 2009-2011  Greyscale
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

// ---------------
//     Public
// ---------------

// See project.inc

// ---------------
//     Private
// ---------------

/**
 * Dummy array used as a way to count the cells required to store module data.
 */
stock g_DummyModuleData[ModuleData];

/**
 * This is a macro that takes a module and returns the handle to it's data array.
 */
#define MM_HANDLE_FROM_ID(%1) GetArrayCell(g_adtModuleList, _:%1)

/**
 * Defines the block of data in the module data arrays that contains module-defined data.
 */
#define MODULE_DATA g_iMMAllocatedIndexes[0]

/**
 * Base array that contains all module data array handles.
 */
new Handle:g_adtModuleList;

/**
 * Global variable that indicates the next module data array index available.
 */
new g_iModuleArrayNextIndex;

/**
 * Array to store the index of the allocated space in the module data arrays for the module manager.
 */
new g_iMMAllocatedIndexes[1];

/**
 * The max number of cells needed per block of data in the module data array.
 */
new g_iMMMaxBlockSize;

// **********************************************
//                 Forwards
// **********************************************


/**
 * Plugin is loading.
 */
ModuleMgr_OnPluginStart()
{
    // Create the adt array used to store module data array handles.
    if (g_adtModuleList == INVALID_HANDLE)
        g_adtModuleList = CreateArray();
    
    // The next open index is 0, the first one.
    g_iModuleArrayNextIndex = 0;
    
    // Allocate 1 index for the data we want to store for each module.
    ModuleMgr_Allocate(1, g_iMMAllocatedIndexes);
    
    // Initialize the max block size to the number of cells needed for the module manager.
    g_iMMMaxBlockSize = sizeof(g_DummyModuleData);
    
    // Now check each of the other base project files if they need more cells, and set the max to that if true.
    
    #if defined EVENT_MANAGER
        if (EM_DATA_CELL_COUNT > g_iMMMaxBlockSize)
            g_iMMMaxBlockSize = EM_DATA_CELL_COUNT;
    #endif
    
    #if defined CONFIG_MANAGER
        if (sizeof(g_DummyConfigData) > g_iMMMaxBlockSize)
            g_iMMMaxBlockSize = sizeof(g_DummyConfigData);
    #endif
    
    #if defined LOG_MANAGER
        if (LM_DATA_CELL_COUNT > g_iMMMaxBlockSize)
            g_iMMMaxBlockSize = LM_DATA_CELL_COUNT;
    #endif
    
    #if defined PROJECT_BASE_CMD
        // Register default base cmd sub-command tree.
        BaseCmd_Init();
    #endif
}

/**
 * Plugin is ending.
 */
ModuleMgr_OnPluginEnd()
{
    // Loop through all the modules, and kill them.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        ModuleMgr_Unregister(Module:moduleindex);
    }
    
    #if defined PROJECT_BASE_CMD
        // Destroy base command data.
        BaseCmd_Destroy();
    #endif
}

// **********************************************
//                Public API
// **********************************************

/**
 * Registers a new module with the project base.
 * 
 * @param moduledata    Array populated with the module's data.  See enum ModuleData.
 * 
 * @return              A unique module identifier.  INVALID_MODULE if registration failed.
 * Note: The IDs used in the code starts from 0, while client interaction starts from 1.
 * So "module ID" starts from 1, while "module identifier" starts from 0.  Modules shouldn't have to worry about this.
 */
stock Module:ModuleMgr_Register(moduledata[ModuleData])
{
    // Validate the module data before storing the data.
    if (!ModuleMgr_Validate(moduledata))
        return INVALID_MODULE;
    
    // This is the array that will hold all the actual module data.
    new Handle:adtModule = CreateArray(g_iMMMaxBlockSize);
    
    // Push all the given module data to the new array.
    // This is being pushed into our allocated space for module data.
    PushArrayArray(adtModule, moduledata[0]);
    
    // Forward event to other base project files.
    
    #if defined EVENT_MANAGER
        EventMgr_OnModuleRegister(adtModule);
    #endif
    
    #if defined CONFIG_MANAGER
        ConfigMgr_OnModuleRegister(adtModule);
    #endif
    
    #if defined LOG_MANAGER
        LogMgr_OnModuleRegister(adtModule);
    #endif
    
    // Store the handle in the global module list.
    new module = PushArrayCell(g_adtModuleList, adtModule);
    
    #if defined ACCESS_MANAGER
        AccessMgr_OnModuleRegistered(Module:module);  // Don't need to store data in the module, just read module data.
    #endif
    
    // Return the module identifier.
    return Module:module;
}

/**
 * Returns if a module identifier is valid or not.
 * 
 * @param module    The module identifier to check validity of.
 * 
 * @return          True if the module identifier is valid, false if not.
 */
stock bool:ModuleMgr_IsModuleValid(Module:module)
{
    return (_:module > -1 && _:module < MODULE_COUNT);
}

/**
 * Disables a registered module.
 * 
 * @param module    The module to disable.
 * 
 * @return          See enum ModuleToggleQuery.
 */
stock ModuleToggleQuery:ModuleMgr_Disable(Module:module)
{
    if (ModuleMgr_IsDisabled(module))
        return ToggleQuery_Stopped;
    
    #if defined EVENT_MANAGER
        // Forward event to all modules.
        new any:eventdata[1][1];
        eventdata[0][0] = module;
        
        new Action:result1 = EventMgr_Forward(g_EvOnModuleDisable, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
        new Action:result2 = Plugin_Continue;
        
        // If a module stops this module from disabling, then don't forward this event.
        new any:eventdata2[2][512];
        if (result1 == Plugin_Continue)
        {
            static EventDataTypes:eventdatatypes[] = {DataType_StringRef, DataType_Cell};
            eventdata2[1][0] = sizeof(eventdata2[]);
            
            // Send this event only to the module being disabled.
            new Module:modules[2] = {INVALID_MODULE, ...};
            modules[0] = module;
            
            result2 = EventMgr_Forward(g_EvOnMyModuleDisable, eventdata2, sizeof(eventdata2), sizeof(eventdata2[]), eventdatatypes, modules);
        }
        
        if (result1 == Plugin_Handled || result2 == Plugin_Handled)
        {
            // Check if a refusal message was set.
            if (eventdata2[0][0])
            {
                PrintToServer(eventdata2[0]);
                return ToggleQuery_RefuseWithMsg;
            }
            return ToggleQuery_Refuse;
        }
    #endif
    
    // Set the ModuleData_Disabled data to 'true'.
    ModuleMgr_WriteCell(module, ModuleData_Disabled, true);
    
    new bool:disableddependents = false;
    new Module:dependents[MM_DATA_DEPENDENCIES];
    new count = ModuleMgr_FindDependentModules(module, dependents);
    for (new dindex = 0; dindex < count; dindex++)
    {
        // Disable dependent modules.
        if (!ModuleMgr_IsDisabled(dependents[dindex]))
        {
            ModuleMgr_Disable(dependents[dindex]);
            disableddependents = true;
        }
    }
    
    return disableddependents ? ToggleQuery_Dependent : ToggleQuery_Successful;
}

/**
 * Enables a registered module.
 * 
 * @param module    The module to enable.
 * 
 * @return          See enum ModuleToggleQuery.
 */
stock ModuleToggleQuery:ModuleMgr_Enable(Module:module)
{
    if (!ModuleMgr_IsDisabled(module))
        return ToggleQuery_Stopped;
    
    new Module:dependencies[MM_DATA_DEPENDENCIES];
    ModuleMgr_ReadArray(module, ModuleData_Dependencies, dependencies, sizeof(dependencies));
    
    // Validate that the module's dependencies are registered/enabled.
    for (new dindex = 0; dindex < sizeof(dependencies); dindex++)
    {
        if (dependencies[dindex] == INVALID_MODULE)
            break;
        
        // Check if this module is set to be enabled while its dependency is disabled.
        if (ModuleMgr_IsDisabled(dependencies[dindex]))
            return ToggleQuery_Dependent;
    }
    
    // Enable the module BEFORE the event is fired, so the module can see when the OnMyModuleEnable event fires.
    ModuleMgr_WriteCell(module, ModuleData_Disabled, false);
    
    #if defined EVENT_MANAGER
        // Forward event to all modules.
        new any:eventdata[1][1];
        eventdata[0][0] = module;
        
        new Action:result1 = EventMgr_Forward(g_EvOnModuleEnable, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
        new Action:result2 = Plugin_Continue;
        
        // If a module stops this module from enabling, then don't forward this event.
        new any:eventdata2[2][512];
        if (result1 == Plugin_Continue)
        {
            static EventDataTypes:eventdatatypes[] = {DataType_StringRef, DataType_Cell};
            eventdata2[1][0] = sizeof(eventdata2[]);
            
            // Send this event only to the module being enabled.
            new Module:modules[2] = {INVALID_MODULE, ...};
            modules[0] = module;
            
            result2 = EventMgr_Forward(g_EvOnMyModuleEnable, eventdata2, sizeof(eventdata2), sizeof(eventdata2[]), eventdatatypes, modules);
        }
        
        // Undo the enabling if it refuses.
        if (result1 == Plugin_Handled || result2 == Plugin_Handled)
        {
            ModuleMgr_WriteCell(module, ModuleData_Disabled, true);
            
            // Check if a refusal message was set.
            if (eventdata2[0][0])
            {
                PrintToServer(eventdata2[0]);
                return ToggleQuery_RefuseWithMsg;
            }
            return ToggleQuery_Refuse;
        }
    #endif
    
    return ToggleQuery_Successful;
}

/**
 * Returns if a module is disabled.
 * 
 * @param module    The module to check.
 * 
 * @return          True if the module is disabled, false if enabled.
 */
stock bool:ModuleMgr_IsDisabled(Module:module)
{
    // Read the value in ModuleData_Disabled and return the cell as a bool.
    return bool:ModuleMgr_ReadCell(module, ModuleData_Disabled);
}

/**
 * Finds any modules matching the given information.
 * 
 * @param data      The data to match the value with.
 * @param value     The value of the data that must match.
 * @param modules   Output array containing a list of modules that match. (optional)
 * @param matched   The number of modules that matched. (optional)
 * 
 * @return          The first module whose data value matches the given value.  INVALID_MODULE if no modules matched.
 */
stock Module:ModuleMgr_Find(ModuleData:data, any:value, Module:modules[] = {INVALID_MODULE}, &matched = 0)
{
    new matchcount;
    new Module:module;
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        // If the data matches, then add the module identifier to the output array.
        if (ModuleMgr_ReadCell(module, data) == value)
        {
            modules[matchcount] = module;
            matchcount++;
        }
    }
    
    // Return the first module that matched.
    if (matchcount > 0)
    {
        return modules[0];
    }
    
    // No modules matched.
    return INVALID_MODULE;
}

/**
 * Finds any modules matching the given information.
 * 
 * @param data          The data to match the string with. (See enum ModuleData)
 * @param value         The string value of the data that must match.
 * @param casesensitive True if the strings must also match case.
 * @param modules       Output array containing a list of module identifiers that match. (optional)
 * @param matched       The number of modules that matched. (optional)
 * 
 * @return              The first module whose data value matches the given value.  INVALID_MODULE if no modules matched.
 */
stock Module:ModuleMgr_FindByString(ModuleData:data, const String:value[], bool:casesensitive = false, Module:modules[] = {INVALID_MODULE}, &matched = 0)
{
    decl String:datastring[MODULE_DATA_LONGEST_STRING];
    new matchcount;
    new Module:module;
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        // If the string matches, then add the module identifier to the output array.
        ModuleMgr_ReadString(module, data, datastring, sizeof(datastring));
        if (StrEqual(datastring, value, casesensitive))
        {
            modules[matchcount] = module;
            matchcount++;
        }
    }
    
    // Return the first module that matched.
    if (matchcount > 0)
    {
        return modules[0];
    }
    
    // No modules matched.
    return INVALID_MODULE;
}

/**
 * Module data reader that returns all available module data.
 * Modules can use this for communicating with other modules.
 * 
 * @param module        The module whose data to read.
 * @param moduledata    Output array for all module data.  See enum ModuleData.
 */
stock ModuleMgr_ReadAll(Module:module, moduledata[ModuleData])
{
    GetArrayArray(MM_HANDLE_FROM_ID(module), MODULE_DATA, moduledata[0], sizeof(moduledata));
}

/**
 * Module data reader for any data type except strings.
 * Modules can use this for communicating with other modules.
 * 
 * @param module    The module whose cell data to read.
 * @param data      The data to get the value of.  See enum ModuleData.
 * 
 * @return          The value of the desired module data.
 */
stock ModuleMgr_ReadCell(Module:module, ModuleData:data)
{
    new moduledata[ModuleData];
    ModuleMgr_ReadAll(module, moduledata);
    
    // Return the value.
    return _:moduledata[data];
}

/**
 * Module data reader for any arrays.
 * Modules can use this for communicating with other modules.
 * 
 * @param module    The module whose array to read.
 * @param data      The data to get the value of.  See enum ModuleData.
 * @param buffer    Output array for the data read.
 * @param numValues The length of the array.
 */
stock ModuleMgr_ReadArray(Module:module, ModuleData:data, any:buffer[], numValues)
{
    new moduledata[ModuleData];
    ModuleMgr_ReadAll(module, moduledata);
    
    // This is necessary because using 'data' directly in moduledata causes compiler errors
    // because it can't know that 'data' is going to be an array.
    switch(data)
    {
        case ModuleData_Dependencies:
        {
            // Copy array contents to buffer.
            for (new index = 0; index < numValues; index++)
                buffer[index] = moduledata[ModuleData_Dependencies][index];
        }
    }
}

/**
 * Module data reader for any string typed values.
 * Modules can use this for communicating with other modules. 
 * 
 * @param module    The module whose string data to read.
 * @param data      The data to get the value of.  See enum ModuleData.
 * @param output    Output variable for the data read.
 * @param maxlen    The max length of the output string.
 */
stock ModuleMgr_ReadString(Module:module, ModuleData:data, String:output[], maxlen)
{
    new moduledata[ModuleData];
    ModuleMgr_ReadAll(module, moduledata);
    
    // Copy full name to output
    strcopy(output, maxlen, String:moduledata[data]);
}

/**
 * Returns a list of modules that depend on the given module.
 * 
 * @param module        The module to find dependencies for.
 * @param dependencies  A list of modules that depend on the given module.
 * 
 * @return              The number of modules that depend on the given module.
 */
stock ModuleMgr_FindDependentModules(Module:module, Module:dependencies[MM_DATA_DEPENDENCIES])
{
    new dindex = 0;
    new Module:_dependencies[MM_DATA_DEPENDENCIES];
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        ModuleMgr_ReadArray(Module:moduleindex, ModuleData_Dependencies, _dependencies, sizeof(_dependencies));
        for (new _dindex = 0; _dindex < sizeof(_dependencies); _dindex++)
        {
            if (_dependencies[_dindex] == INVALID_MODULE)
                break;
            
            if (dindex >= MM_DATA_DEPENDENCIES)
            {
                decl String:modulefullname[MM_DATA_FULLNAME];
                ModuleMgr_ReadString(Module:moduleindex, ModuleData_FullName, modulefullname, sizeof(modulefullname));
                SetFailState("More than %d modules depend on module \"%s\".  Define \"MM_DATA_DEPENDENCIES\" must be increased.", MM_DATA_DEPENDENCIES, modulefullname);
            }
            
            if (_dependencies[_dindex] == module)
            {
                dependencies[dindex++] = Module:moduleindex;
                break;
            }
        }
    }
    
    return dindex;
}

// **********************************************
//   Private API (For base project files only)
// **********************************************

/**
 * Unregistered a registered module.  Call this only when the plugin is ending.
 * 
 * @param module        The module identifer of the module to unregister.
 * 
 * @return              True if unregistered successfully, false if module doesn't exist.
 */
stock bool:ModuleMgr_Unregister(Module:module)
{
    if (!ModuleMgr_IsModuleValid(module))
        return false;
    
    // Get the module's array handle.
    new Handle:adtModule = MM_HANDLE_FROM_ID(module);
    if (adtModule == INVALID_HANDLE)
        return false;
    
    // Destroy all the data stored by every component, and then set the space in the global array to INVALID_HANDLE.
    CloseHandle(adtModule);
    SetArrayCell(g_adtModuleList, _:module, INVALID_HANDLE);
    
    return true;
}

/**
 * Validates all values in a ModuleData array, and prints errors for each invalid value.
 * 
 * @param moduledata    Array of module data to validate.
 * @param moduleflags   0 if all values were valid.  Non-zero means there is a bitstring of module data flags that failed.
 * 
 * @return              True if validation was successful, false if there were invalid values.
 */
stock bool:ModuleMgr_Validate(moduledata[ModuleData], &moduleflags = 0)
{
    moduleflags = 0;
    
    // ModuleData_Disabled can't be invalid.
    // ModuleData_Hidden can't be invalid.
    
    // Validate ModuleData_FullName.
    if (!moduledata[ModuleData_FullName][0])
    {
        moduleflags |= MODULE_DATA_FULLNAME;
        LogError("[Module Manager] [Registration Failed] Module needs a full name.");
    }
    
    // Validate ModuleData_ShortName.
    if (!moduledata[ModuleData_ShortName][0])
    {
        moduleflags |= MODULE_DATA_SHORTNAME;
        LogError("[Module Manager] [Registration Failed] Module \"%s\" needs a (unique) short name.", moduledata[ModuleData_FullName]);
    }
    else
    {
        // Shortname must be unique.
        new Module:clashmodule = ModuleMgr_FindByString(ModuleData_ShortName, moduledata[ModuleData_ShortName]);
        if (clashmodule != INVALID_MODULE)
        {
            decl String:clashmodulefullname[MM_DATA_FULLNAME];
            ModuleMgr_ReadString(clashmodule, ModuleData_FullName, clashmodulefullname, sizeof(clashmodulefullname));
            
            moduleflags |= MODULE_DATA_SHORTNAME;
            LogError("[Module Manager] [Registration Failed] Module \"%s\" short name (%s) clashes with that of module \"%s\"", moduledata[ModuleData_FullName], moduledata[ModuleData_ShortName], clashmodulefullname);
        }
    }
    
    // ModuleData_Description can't be invalid.
    
    decl String:dependencyfullname[32];
    
    // Validate that the module's dependencies are registered/enabled.
    for (new dindex = 0; dindex < sizeof(moduledata[ModuleData_Dependencies]); dindex++)
    {
        if (moduledata[ModuleData_Dependencies][dindex] == INVALID_MODULE)
            break;
        
        // Check if this module is set to be enabled while its dependency is disabled.
        if (!moduledata[ModuleData_Disabled] && ModuleMgr_IsDisabled(moduledata[ModuleData_Dependencies][dindex]))
        {
            moduledata[ModuleData_Disabled] = true;
            
            ModuleMgr_ReadString(moduledata[ModuleData_Dependencies][dindex], ModuleData_FullName, dependencyfullname, sizeof(dependencyfullname));
            LogMessage("[Module Manager] Warning: Module \"%s\" was disabled because its dependency (%s) is disabled.", moduledata[ModuleData_FullName], dependencyfullname);
        }
    }
    
    // Return false if the moduleflags is non-zero.
    return (moduleflags == 0);
}

/**
 * Finds any modules matching the given module ID or shortname.
 * This should only be used if input it coming from a client.  Otherwise use ModuleMgr_FindByString.
 * 
 * @param value         The module ID to match to a module. (This will be matched first if both are inputed)
 * @param shortname     The short name to match to a module.
 * 
 * @return              The module whose module index or shortname matched. (in that order)  INVALID_MODULE if no modules matched.
 */
stock Module:ModuleMgr_FindByID(moduleID = 0, const String:moduleshortname[] = "")
{
    decl String:matchshortname[MM_DATA_SHORTNAME];
    new Module:module;
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        // If the module is hidden, then ignore it.
        if (bool:ModuleMgr_ReadCell(module, ModuleData_Hidden))
            continue;
        
        // Compare the module ID to each module.
        if (moduleID == MODULE_TO_ID(module))
            return module;
        
        // Compare the short name to each module.
        ModuleMgr_ReadString(module, ModuleData_ShortName, matchshortname, sizeof(matchshortname));
        if (StrEqual(moduleshortname, matchshortname, false))
            return module;
    }
    
    // No modules matched.
    return INVALID_MODULE;
}

/**
 * Reserve space in the module data array.
 * 
 * @param count     The number of indexes per module that's needed.
 * @param indexes   The indexes within the array that have been allocated to you.  (number of elements = param 'count')
 */
stock ModuleMgr_Allocate(count, indexes[])
{
    // While the count is above 0, allocate each index until the count has been reduced to 0.
    new index = 0;
    while (count > 0)
    {
        count--;
        indexes[index] = g_iModuleArrayNextIndex;
        g_iModuleArrayNextIndex++;
        index++;
    }
}

/**
 * Module data writer that overwrites all data for a module with the given data.
 * 
 * @param module        The module whose data to write.
 * @param moduledata    New data to replace the old data.  See enum ModuleData.
 */
stock ModuleMgr_WriteAll(Module:module, moduledata[ModuleData])
{
    SetArrayArray(MM_HANDLE_FROM_ID(module), MODULE_DATA, moduledata[0], sizeof(moduledata));
}

/**
 * Module data writer that writes a specified non-string data value.
 * 
 * @param module    The module whose cell data to write.
 * @param data      Data to write new value to.  See enum ModuleData.
 * @param value     Any cell value to write as the new data.
 */
stock ModuleMgr_WriteCell(Module:module, ModuleData:data, any:value)
{
    // Read all the module data.
    new moduledata[ModuleData];
    ModuleMgr_ReadAll(module, moduledata);
    
    // Change the value of the specified module data.
    moduledata[data] = value;
    
    // Overwrite the old array with the modified one.
    ModuleMgr_WriteAll(module, moduledata);
}

/**
 * Module data writer that writes an array of data values.
 * 
 * @param module    The module whose array data to write.
 * @param data      Data to write new array to.  See enum ModuleData.
 * @param values    The array of values to write.
 * @param numValues The size of the array.
 */
stock ModuleMgr_WriteArray(Module:module, ModuleData:data, any:values[], numValues)
{
    // Read all the module data.
    new moduledata[ModuleData];
    ModuleMgr_ReadAll(module, moduledata);
    
    // This is necessary because using 'data' directly in moduledata causes compiler errors
    // because it can't know that 'data' is going to be an array.
    switch(data)
    {
        case ModuleData_Dependencies:
        {
            // Copy array contents to buffer.
            for (new index = 0; index < numValues; index++)
                moduledata[ModuleData_Dependencies][index] = values[index];
        }
    }
    
    // Overwrite the old array with the modified one.
    ModuleMgr_WriteAll(module, moduledata);
}

/**
 * Module data writer that writes a specified string data value.
 * 
 * @param module    The module whose string data to write.
 * @param data      Data to write new string to.  See enum ModuleData.
 * @param maxlen    The max length of the data value.  See enum ModuleData.
 * @param value     A string to write as the new data value.
 */
stock ModuleMgr_WriteString(Module:module, ModuleData:data, maxlen, const String:value[])
{
    // Read all the module data.
    new moduledata[ModuleData];
    ModuleMgr_ReadAll(module, moduledata);
    
    // Change the value of the specified module data.
    strcopy(String:moduledata[data], maxlen, value);
    
    // Overwrite the old array with the modified one.
    ModuleMgr_WriteAll(module, moduledata);
}

/**
 * Wrapper for macro MM_HANDLE_FROM_ID so other base components can use it.
 * 
 * @param module    The module identifier.
 */
stock Handle:ModuleMgr_GetModuleArray(Module:module)
{
    return MM_HANDLE_FROM_ID(module);
}

/**
 * Returns a module's ID given it's array handle.
 * 
 * @param adtModule The module's array handle to get the module identifier for.
 * 
 * @return          The module's module identifier, INVALID_MODULE if no module matched the handle.
 */
stock Module:ModuleMgr_GetModule(Handle:adtModule)
{
    new Module:module;
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        // If the handle's match, then return the current index, which is the module identifier.
        if (adtModule == MM_HANDLE_FROM_ID(module))
            return module;
    }
    
    return INVALID_MODULE;
}

/**
 * Allows other modules to get the handle of the main array to manipulate data.
 * 
 * @return  The handle to the array with all module data handles.
 */
stock Handle:ModuleMgr_GetList()
{
    return g_adtModuleList;
}

// **********************************************
//                Base Command
// **********************************************

#if defined PROJECT_BASE_CMD

/**
 * Base arrays that hold all sub-commands and their sub-commands.
 */
new Handle:g_hServerSubCmds;
new Handle:g_hClientSubCmds;

// **********************************************
//              Base Command API
// **********************************************

/**
 * Initialize the base command data containers.
 */
stock BaseCmd_Init()
{
    // Create base command.
    #if defined PROJECT_BASE_CMD
        // Register the base command of the project module system.
        RegConsoleCmd(PROJECT_BASE_CMD, ModuleMgr_BaseCommand, "The plugins base command to handle modules.");
    #endif
    
    g_hServerSubCmds = CreateKeyValues("basecmd_server");
    g_hClientSubCmds = CreateKeyValues("basecmd_client");
    
    // Register default sub-commands.
    BaseCmd_RegisterDefs();
}

/**
 * Destroy the base command data.
 */
stock BaseCmd_Destroy()
{
    CloseHandle(g_hServerSubCmds);
    CloseHandle(g_hClientSubCmds);
}

/**
 * Registers all default sub-commands.
 */
stock BaseCmd_RegisterDefs()
{
    // Create directions.
    new String:g_dirBlank[1][16] = {""};
    new String:dirModules[1][16] = {"modules"};
    
    // Server commands.
    BaseCmd_Register(CommandFor_Server, g_dirBlank, 0, "credits", "BaseCmd_Credits");
    BaseCmd_Register(CommandFor_Server, g_dirBlank, 0, "dumpcmdtree", "BaseCmd_DumpCmdTree");
    BaseCmd_Register(CommandFor_Server, g_dirBlank, 0, "modules");
    BaseCmd_Register(CommandFor_Server, g_dirBlank, 0, "version", "BaseCmd_Version");
    
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "enable",      "BaseCmd_Enable");
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "disable",     "BaseCmd_Disable");
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "disable_all", "BaseCmd_DisableAll");
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "info",        "BaseCmd_Info");
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "list",        "BaseCmd_List");
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "refresh",     "BaseCmd_Refresh");
    BaseCmd_Register(CommandFor_Server, dirModules, sizeof(dirModules), "reload",      "BaseCmd_Reload");
    
    // Client commands.
    BaseCmd_Register(CommandFor_Client, g_dirBlank, 0, "credits", "BaseCmd_Credits");
    BaseCmd_Register(CommandFor_Client, g_dirBlank, 0, "modules", "BaseCmd_List");
    BaseCmd_Register(CommandFor_Client, g_dirBlank, 0, "version", "BaseCmd_Version");
}

/**
 * Register a sub-command under the base command.
 * 
 * Note: If given non-existant directions they will be automatically created.
 * 
 * @param who           For the server or client.
 * @param directions    The path to the sub-command being registered.
 *                      Ex: "modules" would be the path to "list."  Blank if top-level sub-command.
 * @param dircount      The number of elements (with values) in 'directions.'
 * @param subcommand    The sub-command to register.  (Max length is 16 characters)
 * @param callback      The function called when the sub-command is used.
 */
stock BaseCmd_Register(CommandFor:who, const String:directions[][16], dircount, const String:subcommand[], const String:callback[] = "")
{
    // Get the handle of the kv we are going to modify.
    new Handle:kv = (who == CommandFor_Server) ? g_hServerSubCmds : g_hClientSubCmds;
    KvRewind(kv);
    
    // This will take us to the depth we want to be at to create the new sub-command.
    for (new dirindex = 0; dirindex < dircount; dirindex++)
    {
        // Check if the index is blank.
        if (directions[dirindex][0] == 0)
            break;
        
        KvJumpToKey(kv, directions[dirindex], true);
    }
    
    // Create the sub-command.
    KvJumpToKey(kv, subcommand, true);
    
    // Now store its data.
    if (callback[0] != 0)
        KvSetString(kv, "__callback", callback);
}

/**
 * Unregister a registered sub-command under the base command.
 * 
 * @param who           For the server or client.
 * @param directions    The path to the sub-command being unregistered.
 *                      Ex: "modules" would be the path to "list."  Blank if top-level sub-command.
 * @param dircount      The number of elements (with values) in 'directions.'
 * @param subcommand    The sub-command to unregister.
 * 
 * @return              True if the sub-command was unregistered, false if it didn't exist.
 */
stock bool:BaseCmd_Unregister(CommandFor:who, const String:directions[][16], dircount, const String:subcommand[], const String:callback[] = "")
{
    // Get the handle of the kv we are going to modify.
    new Handle:kv = (who == CommandFor_Server) ? g_hServerSubCmds : g_hClientSubCmds;
    KvRewind(kv);
    
    // This will take us to the depth we want to be at to create the new sub-command.
    for (new dirindex = 0; dirindex < dircount; dirindex++)
    {
        // Check if the index is blank.
        if (directions[dirindex][0] == 0)
            break;
        
        if (!KvJumpToKey(kv, directions[dirindex]))
            return false;
    }
    
    return KvDeleteKey(kv, subcommand);
}

/**
 * Returns the handle to the respective KV.
 * 
 * @param who   See BASECMD_* defines.
 * 
 * @return      The KV array handle depending on 'who' it's for.
 */
stock Handle:BaseCmd_GetKV(CommandFor:who)
{
    return (who == CommandFor_Server) ? g_hServerSubCmds : g_hClientSubCmds;
}

/**
 * Command callback: <basecommand>  See define PROJECT_BASE_CMD
 * Root command for plugin and module management.
 * 
 * @param client    The client index.  Or SERVER_INDEX if coming from the server.
 * @param argc      The number of arguments that the client sent with the command.
 */
public Action:ModuleMgr_BaseCommand(client, argc)
{
    // Derive 'who' from the client using the command.
    new CommandFor:who = (client > 0 && client <= MaxClients) ? CommandFor_Client : CommandFor_Server;
    
    // Set to "server" if the server is using the command, "client" if client is.
    new String:strWho[8];
    strWho = (who == CommandFor_Server) ? "server" : "client";
    
    // Get all arguments.
    new String:strArgs[argc + 1][64];
    for (new arg = 0; arg <= argc; arg++)
        GetCmdArg(arg, strArgs[arg], 64);
    
    // Build directions from the arguments.
    
    new Handle:kv = BaseCmd_GetKV(who);
    KvRewind(kv);
    
    decl String:callback[33];
    for (new arg = 1; arg <= argc; arg++)
    {
        // Invalid arguments given.
        if (!KvJumpToKey(kv, strArgs[arg]))
        {
            argc = arg - 1;     // Pretend the invalid argument(s) never existed..
            break;
        }
        
        // If the value of the "__callback" key is returned, then call the function in it.
        KvGetString(kv, "__callback", callback, sizeof(callback));
        if (callback[0] != 0)
        {
            // Note, we can initialize a string in this loop because it's going to be broken.
            new Handle:hLeftovers = CreateArray(32);
            for (new _arg = arg + 1; _arg <= argc; _arg++)
                PushArrayString(hLeftovers, strArgs[_arg]);
            
            // Call sub-command's callback function.
            Call_StartFunction(GetMyHandle(), GetFunctionByName(GetMyHandle(), callback));
            
            Call_PushCell(client);
            Call_PushCell(hLeftovers);
            Call_PushCell(argc - arg);
            
            Call_Finish();
            
            CloseHandle(hLeftovers);
            return Plugin_Handled;
        }
    }
    
    // Not enough arguments, or invalid argument found if this is reached.
    
    decl String:subcommand[16], String:phraseformat[64];
    
    // Print the title of the sub-commands about to be printed.
    if (StrEqual(strArgs[argc], PROJECT_BASE_CMD, false))
    {
        Format(phraseformat, sizeof(phraseformat), "BaseCmd %s syntax", strWho);
        ReplyToCommand(client, "%T", phraseformat, client, PROJECT_FULLNAME, PROJECT_BASE_CMD);
    }
    else
    {
        Format(phraseformat, sizeof(phraseformat), "BaseCmd %s %s syntax", strWho, strArgs[argc]);
        ReplyToCommand(client, "%T", phraseformat, client, PROJECT_FULLNAME);
    }
    
    // Print all of the valid sub-commands.
    KvGotoFirstSubKey(kv);
    do
    {
        KvGetSectionName(kv, subcommand, sizeof(subcommand));
        
        if (StrEqual(strArgs[argc], PROJECT_BASE_CMD, false))
            Format(phraseformat, sizeof(phraseformat), "BaseCmd %s %s description", strWho, subcommand);
        else
            Format(phraseformat, sizeof(phraseformat), "BaseCmd %s %s %s description", strWho, strArgs[argc], subcommand);
        
        ReplyToCommand(client, "    %17s- %T", subcommand, phraseformat, client);
    } while (KvGotoNextKey(kv));
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return Plugin_Handled;
}

// **********************************************
//           Base Command Callbacks
// **********************************************

/**
 * Base sub-command: "credits"
 * Prints credit string to server or client.
 * 
 * @param client    The client using the command.
 */
public BaseCmd_Credits(client)
{
    // Print credits.
    ReplyToCommand(client, PROJECT_CREDITS);
}

/**
 * Base sub-command: "dumpcmdtree"
 * Dumps the entire tree of sub-commands to the specified file.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public BaseCmd_DumpCmdTree(client, Handle:hParams, argc)
{
    // Check if there are enough arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "BaseCmd dumpcmdtree syntax", LANG_SERVER, PROJECT_BASE_CMD);
        return;
    }
    
    decl String:path[PLATFORM_MAX_PATH];
    GetArrayString(hParams, 0, path, sizeof(path));
    
    new Handle:kv = BaseCmd_GetKV(CommandFor_Server);
    KvRewind(kv);
    KeyValuesToFile(kv, path);
}

/**
 * Base sub-command: "version"
 * Prints plugin version info to server or client.
 * 
 * @param client    The client using the command.
 */
public BaseCmd_Version(client)
{
    #if defined VERSION_INFO
        VersionPrint(client);
    #else
        Project_PrintToServer("%T", "Missing base component", LANG_SERVER, "versioninfo.inc");
    #endif
}

/**
 * Base sub-command: "modules enable <moduleID>"
 * Enables a module.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public BaseCmd_Enable(client, Handle:hParams, argc)
{
    // Check if there are enough arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "ModuleMgr modules sub-command syntax", LANG_SERVER, PROJECT_BASE_CMD, "modules", "enable");
        return;
    }
    
    decl String:strModuleID[MM_DATA_SHORTNAME];
    GetArrayString(hParams, 0, strModuleID, sizeof(strModuleID));
    
    new Module:module = ModuleMgr_FindByID(StringToInt(strModuleID), strModuleID);
    if (module == INVALID_MODULE)
    {
        Project_PrintToServer("%T", "ModuleMgr module invalid", LANG_SERVER, strModuleID);
        return;
    }
    
    new ModuleToggleQuery:result = ModuleMgr_Enable(module);
    
    decl String:modulefullname[MM_DATA_FULLNAME];
    ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    
    switch (result)
    {
        case ToggleQuery_Successful:    Project_PrintToServer("%T", "ModuleMgr modules enable", LANG_SERVER, modulefullname);
        case ToggleQuery_Stopped:       Project_PrintToServer("%T", "ModuleMgr modules enable stopped", LANG_SERVER, modulefullname);
        case ToggleQuery_Refuse:        Project_PrintToServer("%T", "ModuleMgr modules enable refuse", LANG_SERVER, modulefullname);
        case ToggleQuery_Dependent:     Project_PrintToServer("%T", "ModuleMgr modules enable dependent", LANG_SERVER, modulefullname);
    }
}

/**
 * Base sub-command: "modules disable <moduleID>"
 * Disables a module.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public BaseCmd_Disable(client, Handle:hParams, argc)
{
    // Check if there are enough arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "ModuleMgr modules sub-command syntax", LANG_SERVER, PROJECT_BASE_CMD, "modules", "disable");
        return;
    }
    
    decl String:strModuleID[MM_DATA_SHORTNAME];
    GetArrayString(hParams, 0, strModuleID, sizeof(strModuleID));
    
    new Module:module = ModuleMgr_FindByID(StringToInt(strModuleID), strModuleID);
    if (module == INVALID_MODULE)
    {
        Project_PrintToServer("%T", "ModuleMgr module invalid", LANG_SERVER, strModuleID);
        return;
    }
    
    new ModuleToggleQuery:result = ModuleMgr_Disable(module);
    
    decl String:modulefullname[MM_DATA_FULLNAME];
    ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    
    switch (result)
    {
        case ToggleQuery_Successful:    Project_PrintToServer("%T", "ModuleMgr modules disable", LANG_SERVER, modulefullname);
        case ToggleQuery_Stopped:       Project_PrintToServer("%T", "ModuleMgr modules disable stopped", LANG_SERVER, modulefullname);
        case ToggleQuery_Refuse:        Project_PrintToServer("%T", "ModuleMgr modules disable refuse", LANG_SERVER, modulefullname);
        case ToggleQuery_Dependent:     Project_PrintToServer("%T", "ModuleMgr modules disable dependent", LANG_SERVER, modulefullname);
    }
}

/**
 * Base sub-command: "modules disable_all"
 * Disables all modules.
 * 
 * @param client    The client using the command.
 */
public BaseCmd_DisableAll(client)
{
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
        ModuleMgr_Disable(Module:moduleindex);
    
    Project_PrintToServer("%T", "ModuleMgr modules disable_all", LANG_SERVER);
}

/**
 * Base sub-command: "modules info <moduleID>"
 * Prints information about a module.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public BaseCmd_Info(client, Handle:hParams, argc)
{
    // Check if there are enough arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "ModuleMgr modules sub-command syntax", LANG_SERVER, PROJECT_BASE_CMD, "modules", "disable");
        return;
    }
    
    decl String:strModuleID[MM_DATA_SHORTNAME];
    GetArrayString(hParams, 0, strModuleID, sizeof(strModuleID));
    
    new Module:module = ModuleMgr_FindByID(StringToInt(strModuleID), strModuleID);
    if (module == INVALID_MODULE)
    {
        Project_PrintToServer("%T", "ModuleMgr module invalid", LANG_SERVER, strModuleID);
        return;
    }
    
    decl String:modulefullname[MM_DATA_FULLNAME];
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    decl String:moduledesc[MM_DATA_DESCRIPTION];
    new bool:bModuleDisabled;
    decl String:strModuleEnabled[16];
    
    // Get the module's short name to match to the given partial name.
    ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    ModuleMgr_ReadString(module, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
    ModuleMgr_ReadString(module, ModuleData_Description, moduledesc, sizeof(moduledesc));
    bModuleDisabled = bool:ModuleMgr_ReadCell(module, ModuleData_Disabled);
    
    #if defined TRANSLATIONS_MANAGER
        TransMgr_BoolToPhrase(client, !bModuleDisabled, BoolPhrase_YesNo, strModuleEnabled, sizeof(strModuleEnabled));
    #else
        IntToString(_:!bModuleDisabled, strModuleEnabled, sizeof(strModuleEnabled));
    #endif
    
    // Print all the data stored in the module manager itself.
    PrintToServer("%T", "ModuleMgr modules info", LANG_SERVER, modulefullname, moduleshortname, moduledesc, strModuleEnabled);
    
    // Forward event to other project base components.
    #if defined EVENT_MANAGER
        EventMgr_OnPrintModuleInfo(client, module);
    #endif
    
    #if defined CONFIG_MANAGER
        ConfigMgr_OnPrintModuleInfo(client, module);
    #endif
    
    #if defined TRANSLATIONS_MANAGER
        TransMgr_OnPrintModuleInfo(client, module);
    #endif
    
    #if defined LOG_MANAGER
        LogMgr_OnPrintModuleInfo(client, module);
    #endif
}

/**
 * Base sub-command: "modules list <moduleID>"
 * Show registered modules.
 * 
 * @param client    The client using the command.
 */
public BaseCmd_List(client)
{
    decl String:modulefullname[MM_DATA_FULLNAME];
    decl String:moduleshortname[MM_DATA_SHORTNAME];
    decl String:moduledescription[MM_DATA_DESCRIPTION];
    new bool:bModuleDisabled;
    decl String:strModuleEnabled[16];
    decl String:line[256];
    
    new Module:module;
    new moduleID;
    
    // Loop through all the modules.
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        // Get the module's module ID for client interaction.
        moduleID = MODULE_TO_ID(module);
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        ModuleMgr_ReadString(module, ModuleData_ShortName, moduleshortname, sizeof(moduleshortname));
        bModuleDisabled = bool:ModuleMgr_ReadCell(module, ModuleData_Disabled);
        
        #if defined TRANSLATIONS_MANAGER
            TransMgr_BoolToPhrase(client, !bModuleDisabled, BoolPhrase_EnabledDisabled, strModuleEnabled, sizeof(strModuleEnabled), true);
        #else
            IntToString(_:!bModuleDisabled, strModuleEnabled, sizeof(strModuleEnabled));
        #endif
        
        // Server and client see different things.
        if (client == SERVER_INDEX)
        {
            // If the module is hidden then don't list it.
            if (bool:ModuleMgr_ReadCell(module, ModuleData_Hidden))
                Format(line, sizeof(line), "%T", "ModuleMgr modules list hidden", client);
            
            else
                Format(line, sizeof(line), "%T", "ModuleMgr modules list", client, modulefullname, strModuleEnabled, moduleshortname);
            Format(line, sizeof(line), "  %02d %s", moduleID, line);
        }
        else
        {
            // Don't show hidden modules to clients.
            if (bool:ModuleMgr_ReadCell(module, ModuleData_Hidden))
                continue;
            
            // Don't show disabled modules to clients.
            if (bModuleDisabled)
                continue;
            
            ModuleMgr_ReadString(module, ModuleData_Description, moduledescription, sizeof(moduledescription));
            Format(line, sizeof(line), "  %T", "ModuleMgr modules list client", client, modulefullname, moduledescription);
        }
        ReplyToCommand(client, line);
    }
}

/**
 * Base sub-command: "modules refresh <moduleID>"
 * Reloads/refreshes all registered modules.
 * 
 * @param client    The client using the command.
 */
public BaseCmd_Refresh(client)
{
    // Loop through all the modules.
    new Module:module;
    new count = MODULE_COUNT;
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Read moduleindex as a Module type.
        module = Module:moduleindex;
        
        ModuleMgr_Disable(module);
        ModuleMgr_Enable(module);
    }
    
    Project_PrintToServer("%T", "ModuleMgr modules refresh", LANG_SERVER);
}

/**
 * Base sub-command: "modules reload <moduleID>"
 * Reloads a module.
 * 
 * @param client    The client using the command.
 * @param hParams   An ADT array with all the leftover arguments in the command-string.  Don't close this!
 * @param argc      The number of leftover arguments. (GetArraySize(hParams))
 */
public BaseCmd_Reload(client, Handle:hParams, argc)
{
    // Check if there are enough arguments.
    if (argc < 1)
    {
        Project_PrintToServer("%T", "ModuleMgr modules sub-command syntax", LANG_SERVER, PROJECT_BASE_CMD, "modules", "reload");
        return;
    }
    
    decl String:strModuleID[MM_DATA_SHORTNAME];
    GetArrayString(hParams, 0, strModuleID, sizeof(strModuleID));
    
    new Module:module = ModuleMgr_FindByID(StringToInt(strModuleID), strModuleID);
    if (module == INVALID_MODULE)
    {
        Project_PrintToServer("%T", "ModuleMgr module invalid", LANG_SERVER, strModuleID);
        return;
    }
    
    ModuleMgr_Disable(module);
    ModuleMgr_Enable(module);
    
    // Verify that the module didn't refuse to load.
    if (ModuleMgr_IsDisabled(module))
        return;
    
    decl String:modulefullname[MM_DATA_FULLNAME];
    ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    Project_PrintToServer("%T", "ModuleMgr modules reload", LANG_SERVER, modulefullname);
}
#endif
