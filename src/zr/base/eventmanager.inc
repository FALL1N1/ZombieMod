/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          eventmanager.inc
 *  Type:          Base
 *  Description:   Manages project events.
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

// All the event information we need is in this. Update this path to your
// project folder.
#include "zr/project_events"

/**
 * Provides the plugin a way to know if the event manager is included in the project.
 */
#define EVENT_MANAGER

// ---------------
//     Public
// ---------------

// See project.inc

/**
 * Uncomment this to profile every event in your project and log the times to file. (sourcemod/logs/)
 * The profiler is used in EventMgr_Forward.
 * WINDOWS ONLY! 
 */
//#define PROFILE_EVENTS

/**
 * Uncomment this to debug crashes happening somewhere inside an event.
 */
//#define DEBUG_EVENTS

// ---------------
//     Private
// ---------------

#if defined PROFILE_EVENTS
    #include <profiler>
#endif

/**
 * The max number of cells needed for the event manager's allocated index.
 */
#define EM_DATA_CELL_COUNT sizeof(g_EMEvents[])

/**
 * ADT array that stores each created event.
 */
new String:g_EMEvents[PROJECT_MAX_EVENTS][64];

/**
 * The current number of events.
 */
new g_iEMEventCount;

/**
 * Defines the block of data in the module data arrays that contains event function data.
 */
#define EVENT_DATA_FUNCTION g_iEMAllocatedIndexes[0]

/**
 * Array to store the index of the allocated space in the module data arrays for the event manager.
 */
new g_iEMAllocatedIndexes[1];

/**
 * This array stores adt array handles for every event.
 * These arrays will be populated with modules that register the event.
 * This will minimize the work the event forward has to do.
 */
new Handle:g_hEventModuleCache[sizeof(g_EMEvents)];

// **********************************************
//                 Forwards
// **********************************************

/**
 * Plugin is loading.
 */
EventMgr_OnPluginStart()
{
    EventMgr_CreateEvents();
    EventMgr_HookEvents();
    
    // Allocate 1 index for the data we want to store for each module.
    ModuleMgr_Allocate(1, g_iEMAllocatedIndexes);
    
    // Initialize the arrays that will be stored for later iteration.
    for (new eindex = 0; eindex < sizeof(g_hEventModuleCache); eindex++)
        g_hEventModuleCache[eindex] = CreateArray();
}

/**
 * Plugin is ending.
 */
EventMgr_OnPluginEnd()
{
    // Cleanup the event module cache.
    for (new eindex = 0; eindex < sizeof(g_hEventModuleCache); eindex++)
        CloseHandle(g_hEventModuleCache[eindex]);
}

/**
 * A module was just registered.  This is being called before the module has been assigned a module identifier.
 * 
 * @param adtModule The adt array of the module being registered.
 */
stock EventMgr_OnModuleRegister(Handle:adtModule)
{
    // Push an array that has space for the max number of events the project can have as the "Function" type to the module's data array.
    // This is being pushed into our allocated space for event function data.
    new Function:funcProjectEvents[sizeof(g_EMEvents)];
    PushArrayArray(adtModule, funcProjectEvents[0], sizeof(funcProjectEvents));
}

/**
 * Base command is printing a module's info.
 * Print the module data allocated by the event manager.
 * Note: |stock| tag will stop this function from being compiled if the base command is disabled.
 * 
 * @param client    The client index the text is being printed to.
 * @param module    The module to print info for.
 */
stock EventMgr_OnPrintModuleInfo(client, Module:module)
{
    decl String:registeredevents[512];
    registeredevents[0] = 0;
    
    for (new eindex = 0; eindex < sizeof(g_hEventModuleCache); eindex++)
    {
        if (EventMgr_GetModuleCacheIndex(module, ProjectEvent:eindex) == -1)
            continue;
        
        // Format each event onto a string.
        if (registeredevents[0] == 0)
            strcopy(registeredevents, sizeof(registeredevents), g_EMEvents[eindex]);
        else
            Format(registeredevents, sizeof(registeredevents), "%s, %s", registeredevents, g_EMEvents[eindex]);
    }
    
    // If the module has no registered events, then format in the "_None" phrase.
    if (registeredevents[0] == 0)
        Format(registeredevents, sizeof(registeredevents), "%t", "_None");
    
    // Print the module event info.
    PrintToServer("%T", "EventMgr modules info", LANG_SERVER, registeredevents);
}

// **********************************************
//                Public API
// **********************************************

/**
 * Create an event that can be registered by a module.
 * Creating an event that no module is listening for is OK.
 * 
 * @param eventname The name of the event that other modules will need to register.
 * 
 * @return          The event ID if created successfully, INVALID_EVENT otherwise.
 */
stock ProjectEvent:EventMgr_CreateEvent(const String:eventname[])
{
    // Check if this event already exists.
    for (new eindex = 0; eindex < sizeof(g_EMEvents); eindex++)
    {
        if (StrEqual(eventname, g_EMEvents[eindex], false))
            return INVALID_EVENT;
    }
    
    // Add the event to the array.
    strcopy(g_EMEvents[g_iEMEventCount], sizeof(g_EMEvents[]), eventname);
    
    return ProjectEvent:g_iEMEventCount++;
}

/**
 * Registers an event to be forwarded to the module.
 * 
 * @param module        The module to forward event to.
 * @param eventname     The event to forward to the module.
 * @param functionname  The name of the function to forward the event to. (The function needs to be public)
 * 
 * @return              The event ID to the registered event.  INVALID_EVENT if the event doesn't exist.
 */
stock ProjectEvent:EventMgr_RegisterEvent(Module:module, const String:eventname[], const String:functionname[])
{
    decl String:modulefullname[MM_DATA_FULLNAME];
    
    new ProjectEvent:eventid = EventMgr_EventNameToID(eventname);
    if (eventid == INVALID_EVENT)
    {
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        SetFailState("[Event Manager] Invalid event name (%s) in module \"%s\". Check for typos.", eventname, modulefullname);
    }
    
    // Check for duplicates because it might be a copy-paste error that is hard to catch when nothing is reported.
    new count = GetArraySize(g_hEventModuleCache[_:eventid]);
    for (new index = 0; index < count; index++)
    {
        if (module == GetArrayCell(g_hEventModuleCache[_:eventid], index))
        {
            ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
            SetFailState("[Event Manager] Module \"%s\" tried to register the same event (%s) twice.", modulefullname, eventname);
        }
    }
    
    // Find the function ID for the given function name.
    new Function:func = GetFunctionByName(GetMyHandle(), functionname);
    if (func == INVALID_FUNCTION)
    {
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
        SetFailState("[Event Manager] Invalid function (%s) being registered as an event forward by module \"%s\" for event \"%s\"", functionname, modulefullname, g_EMEvents[_:eventid]);
    }
    
    // Enable the event to be forwarded to the module, and set the function ID to the event forward.
    EventMgr_WriteFuncValue(module, eventid, func);
    
    // Add the module to the forward cache.
    PushArrayCell(g_hEventModuleCache[_:eventid], module);
    
    // Return the event ID.
    return eventid;
}

/**
 * Changes event firing priority by moving a module above another in the event fire order.
 * 
 * @param eventid   The event to set priority for.
 * @param module1   The module that should have priority over the other.
 * @param module2   The module that will be second in priority to module1.
 * 
 * @return          True if module1 ends up with priority over module2, false if invalid information is given.
 */
stock bool:EventMgr_GivePriority(const String:eventname[], Module:module1, Module:module2)
{
    // Get the event ID given its name.
    new ProjectEvent:eventid = EventMgr_EventNameToID(eventname);
    if (eventid == INVALID_EVENT)
        return false;
    
    // Find the indexes of the modules in the event cache.
    new mpriority1 = EventMgr_GetModuleCacheIndex(module1, eventid);
    new mpriority2 = EventMgr_GetModuleCacheIndex(module2, eventid);
    if (mpriority1 == -1 || mpriority2 == -1)
        return false;
    
    // Check if module1 has higher priority than module2.
    if (mpriority1 < mpriority2)
        return true;
    
    // Move module1 right above module2.
    new Handle:adtModule = GetArrayCell(g_hEventModuleCache[_:eventid], mpriority1);    //  Copy the handle of the module we're moving
    RemoveFromArray(g_hEventModuleCache[_:eventid], mpriority1);    // Remove from the array.
    ShiftArrayUp(g_hEventModuleCache[_:eventid], mpriority2);   // Free a space right above module2, sliding module2 up 1 index.
    SetArrayCell(g_hEventModuleCache[_:eventid], mpriority2, adtModule);    // Stick the copied handle where module2 used to be.
    
    return true;
}

/**
 * Changes event firing priority by moving a module below another in the event fire order.
 * Inverse of GivePriority. 
 * 
 * @param eventid   The event to set priority for.
 * @param module1   The module that should not have priority over the other.
 * @param module2   The module that module1 will be under in priority.
 * 
 * @return          True if module1 ends up with lower priority than module2, false if invalid information is given.
 */
stock bool:EventMgr_TakePriority(const String:eventname[], Module:module1, Module:module2)
{
    // Get the event ID given its name.
    new ProjectEvent:eventid = EventMgr_EventNameToID(eventname);
    if (eventid == INVALID_EVENT)
        return false;
    
    // Find the indexes of the modules in the event cache.
    new mpriority1 = EventMgr_GetModuleCacheIndex(module1, eventid);
    new mpriority2 = EventMgr_GetModuleCacheIndex(module2, eventid);
    if (mpriority1 == -1 || mpriority2 == -1)
        return false;
    
    // Check if module2 has higher priority than module1.
    if (mpriority1 > mpriority2)
        return true;
    
    // Move module1 right above module2.
    new Handle:adtModule = GetArrayCell(g_hEventModuleCache[_:eventid], mpriority1);    //  Copy the handle of the module we're moving
    RemoveFromArray(g_hEventModuleCache[_:eventid], mpriority1);    // Remove from the array.
    
    // Check if there is an existing slot after module2, if not then just push the handle to the end.
    if (GetArraySize(g_hEventModuleCache[_:eventid]) < mpriority2)
    {
        PushArrayCell(g_hEventModuleCache[_:eventid], adtModule);
    }
    else
    {
        ShiftArrayUp(g_hEventModuleCache[_:eventid], mpriority2);   // Free a space right below module2.
        SetArrayCell(g_hEventModuleCache[_:eventid], mpriority2, adtModule);    // Stick the copied handle one index after module2.
    }
    
    return true;
}

/**
 * Checks if a module has priority over another one for a certain event.
 * 
 * @param eventid   The event to set priority for.
 * @param module1   The module to check if above module2.
 * @param module2   The module being checked to see if module1 is higher than this.
 * 
 * @return          True if module1 has priority over module2, false if not or event isn't registered for one of the modules.
 */
stock bool:EventMgr_HasPriority(ProjectEvent:eventid, Module:module1, Module:module2)
{
    // Find the indexes of the modules in the event cache.
    new mpriority1 = EventMgr_GetModuleCacheIndex(module1, eventid);
    new mpriority2 = EventMgr_GetModuleCacheIndex(module2, eventid);
    if (mpriority1 == -1 || mpriority2 == -1)
        return false;
    
    // Return true if module1 has a lower index.  Lower index = higher priority.
    return (mpriority1 < mpriority2);
}

/**
 * Checks if an event is active for a module.
 * 
 * @param module    The module identifier.
 * @param eventid   The event to check.
 * 
 * @return          True if it's currently active, false if not.
 */
stock bool:EventMgr_IsEventActive(Module:module, ProjectEvent:eventid)
{
    return bool:(EventMgr_GetModuleCacheIndex(module, eventid) > -1);
}

/**
 * Disables an event in a module.
 * If the event is already disabled, nothing will change.
 * 
 * @param module    The module whose event is being disabled.
 * @param eventid   The event to disable in the module.
 */
stock EventMgr_Disable(Module:module, ProjectEvent:eventid)
{
    // Remove the module from the forward cache.
    new index = EventMgr_GetModuleCacheIndex(module, eventid);
    
    // Just stop if the module is already disabled, or non-existant.
    if (index == -1)
        return;
    
    RemoveFromArray(g_hEventModuleCache[eventid], index);
}

/**
 * Enables an event in a module.
 * If the event is already enabled, nothing will change.
 * 
 * @param module    The module whose event is being enabled.
 * @param eventid   The event to enable in the module.
 */
stock EventMgr_Enable(Module:module, ProjectEvent:eventid)
{
    new index = EventMgr_GetModuleCacheIndex(module, eventid);
    
    // Just stop if the module already exists in this event cache.
    if (index != -1)
        return;
    
    // Add the module to the forward cache.
    PushArrayCell(g_hEventModuleCache[eventid], module);
}

/**
 * Sets a new event forward function for the given event.
 * 
 * @param module        The module whose event forward is being set.
 * @param eventid       The event to set the event forward on.
 * @param functionname  The name of the function to forward the event to. (The function needs to be public)
 * 
 * return               The function ID to the event forward.  INVALID_FUNCTION is returned upon failure.
 */
stock Function:EventMgr_SetEventForward(Module:module, ProjectEvent:eventid, const String:functionname[])
{
    // Find the function ID for the given function name.
    new Function:func = GetFunctionByName(GetMyHandle(), functionname);
    
    // Enable the event in the module.
    EventMgr_WriteFuncValue(module, eventid, func);
    
    // Return the function ID.
    return func;
}

/**
 * Forwards an event and a list of data values to all modules who registered it.
 * 
 * @param eventid           The event to forward.  Must be created with CreateEvent.
 * @param eventdata         An array with all data to forward to each module.
 * @param numValues         The number of elements in the first dimension of the eventdata array.
 * @param size              The max size of the arrays/strings if passing them through eventdata.  Otherwise set this to 1.
 * @param eventdatatypes    An array of datatypes for each event value.  See enum EventDataTypes.  This should line up with eventdata.
 * @param modules           An extra filter to narrow down which modules will see the event.  This MUST end with INVALID_MODULE.
 * 
 * @return                  A hook action defined in enum Action.
 */
new Module:g_ForwardToModule;  // The global variable to store the next module to receive the event being forwarded.
stock Action:EventMgr_Forward(ProjectEvent:eventid, any:eventdata[][], numValues, size, EventDataTypes:eventdatatypes[], Module:modules[] = {INVALID_MODULE})
{
    // TODO: This needs to be optimized. This function should not decide whether
    // to forward or not. It should just get a list and forward to modules in
    // that list. The list will be updated when events are registered and
    // modules enabled or disabled.
    
    #if defined PROFILE_EVENTS
        new Handle:profiler = CreateProfiler();
        StartProfiling(profiler);
    #endif
    
    // Validate event.
    if (eventid == INVALID_EVENT)
    {
        ThrowError("Invalid event ID.");
    }
    
    new Function:func;
    new Module:module;
    new Action:result;
    new Action:finalresult = Plugin_Continue;
    
    // Loop through all the modules.
    new count = GetArraySize(g_hEventModuleCache[eventid]);
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        // Get the module identifier from the module cache for this event.
        module = Module:GetArrayCell(g_hEventModuleCache[eventid], moduleindex);
        
        // If the module is disabled, then stop this iteration.
        if (ModuleMgr_IsDisabled(module))
            continue;
        
        // Check if the module should see this event.
        new bool:pass_filter = bool:(modules[0] == INVALID_MODULE);
        for (new filter_module = 0; modules[filter_module] != INVALID_MODULE; filter_module++)
        {
            if (module == modules[filter_module])
            {
                pass_filter = true;
                break;
            }
        }
        
        if (!pass_filter)
            continue;
        
        // Read the function ID.
        func = EventMgr_ReadFuncValue(module, eventid);
        
        // Set the global variable to the next module who will be receiving this event.
        g_ForwardToModule = module;
        
        // Call the event forward.
        Call_StartFunction(GetMyHandle(), func);
        
        // Push each data value into the function parameter list.
        // Loop through each event value.
        for (new x = 0; x < numValues; x++)
        {
            switch (eventdatatypes[x])
            {
                case DataType_Cell:         Call_PushCell(_:eventdata[x][0]);
                case DataType_CellRef:      Call_PushCellRef(_:eventdata[x][0]);
                case DataType_Float:        Call_PushFloat(Float:eventdata[x][0]);
                case DataType_FloatRef:     Call_PushFloatRef(Float:eventdata[x][0]);
                case DataType_Array:        Call_PushArray(eventdata[x], size);
                case DataType_ArrayRef:     Call_PushArrayEx(eventdata[x], size, SM_PARAM_COPYBACK);
                case DataType_String:       Call_PushString(eventdata[x]);
                case DataType_StringRef:    Call_PushStringEx(eventdata[x], size, SM_PARAM_STRING_UTF8|SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
            }
        }
        
        #if defined DEBUG_EVENTS
            decl String:modulefullname[MM_DATA_FULLNAME];
            ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
            LogMessage("EVENT DEBUGGER: Event %s: Module: %s BEFORE CALLING", g_EMEvents[_:eventid], modulefullname);
        #endif
        
        // Call the function.
        Call_Finish(result);
        
        #if defined DEBUG_EVENTS
            LogMessage("EVENT DEBUGGER: Event %s: Module: %s AFTER CALLING", g_EMEvents[_:eventid], modulefullname);
        #endif
        
        if (result == Plugin_Stop)
        {
            finalresult = Plugin_Stop;
            break;
        }
        else if (result == Plugin_Handled)
            finalresult = Plugin_Handled;
    }
    
    #if defined PROFILE_EVENTS
        StopProfiling(profiler);
        LogMessage("PROFILER: Event %s: Forwarded to %d modules, taking %f seconds to do.", g_EMEvents[_:eventid], count, GetProfilerTime(profiler));
        CloseHandle(profiler);
    #endif
    
    return finalresult;
}

/**
 * This function is only valid if being called within a forwarded event in a module!
 * This is needed when more than one module is using the same event. 
 * Returns the module that the current event is being forwarded to.
 * 
 * @return  The module identifier.  
 */
stock Module:EventMgr_GetEventOwner()
{
    return g_ForwardToModule;
}

/**
 * This simply calls a global (or private) forward given an array of data in the same format as module forwarding.
 * It's a convenience function for forwards that are already internal module events.
 * 
 * @param hForward          The handle to the global forward you want to call.
 * @param eventdata         An array with all data to forward to each module.
 * @param numValues         The number of elements in the first dimension of the eventdata array.
 * @param size              The max size of the arrays/strings if passing them through eventdata.  Otherwise set this to 1.
 * @param eventdatatypes    An array of datatypes for each event value.  See enum EventDataTypes.  This should line up with eventdata.
 * 
 * @return                  Returns whatever you defined the forward to return.
 */
public any:EventMgr_ForwardToPlugins(Handle:hForward, any:eventdata[][], numValues, size, EventDataTypes:eventdatatypes[])
{
    new Action:result;
    
    // Call the event forward.
    Call_StartForward(hForward);
    
    // Push each data value into the function parameter list.
    // Loop through each event value.
    for (new x = 0; x < numValues; x++)
    {
        switch (eventdatatypes[x])
        {
            case DataType_Cell:         Call_PushCell(_:eventdata[x][0]);
            case DataType_CellRef:      Call_PushCellRef(_:eventdata[x][0]);
            case DataType_Float:        Call_PushFloat(Float:eventdata[x][0]);
            case DataType_FloatRef:     Call_PushFloatRef(Float:eventdata[x][0]);
            case DataType_Array:        Call_PushArray(eventdata[x], size);
            case DataType_ArrayRef:     Call_PushArrayEx(eventdata[x], size, SM_PARAM_COPYBACK);
            case DataType_String:       Call_PushString(eventdata[x]);
            case DataType_StringRef:    Call_PushStringEx(eventdata[x], size, 0, SM_PARAM_COPYBACK);
        }
    }
    Call_Finish(result);
    return result;
}

// **********************************************
//   Private API (For base project files only)
// **********************************************

/**
 * Returns the event ID for the given event name.
 * 
 * @param eventname The name of the event to find the ID for.
 * 
 * @return          The event ID of the event name given.  INVALID_EVENT if it doesn't exist.
 */
stock ProjectEvent:EventMgr_EventNameToID(const String:eventname[])
{
    for (new eindex = 0; eindex < sizeof(g_EMEvents); eindex++)
    {
        if (StrEqual(eventname, g_EMEvents[eindex], false))
            return ProjectEvent:eindex;
    }
    
    return INVALID_EVENT;
}

/**
 * Returns the index in the event module cache that holds the given module identifier.
 * 
 * @param module    The module identifier that is in the sought index.
 * @param eventid   The event that holds the module cache.
 * 
 * @return          The adt array index that holds the module specified.  -1 if the module isn't in the cache.
 */
stock EventMgr_GetModuleCacheIndex(Module:module, ProjectEvent:eventid)
{
    new count = GetArraySize(g_hEventModuleCache[_:eventid]);
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        if (module == Module:GetArrayCell(g_hEventModuleCache[_:eventid], moduleindex))
            return moduleindex;
    }
    
    return -1;
}

/**
 * Print event fire order to server console for a specific event.
 * 
 * @param eventid   The event to print fire order of.
 */
stock EventMgr_PrintPriority(ProjectEvent:eventid)
{
    decl String:modulefullname[MM_DATA_FULLNAME];
    new count = GetArraySize(g_hEventModuleCache[_:eventid]);
    for (new moduleindex = 0; moduleindex < count; moduleindex++)
    {
        ModuleMgr_ReadString(Module:GetArrayCell(g_hEventModuleCache[_:eventid], moduleindex), ModuleData_FullName, modulefullname, sizeof(modulefullname));
        PrintToServer("Module: %s", modulefullname);
    }
}

/**
 * Event data reader that returns all event function data.
 * 
 * @param module    The module whose event function data to read.
 */
stock EventMgr_ReadAllFuncData(Module:module, Function:eventfuncdata[PROJECT_MAX_EVENTS])
{
    GetArrayArray(ModuleMgr_GetModuleArray(module), EVENT_DATA_FUNCTION, eventfuncdata[0], sizeof(eventfuncdata));
}

/**
 * Reset's all event function data for all events.
 * 
 * @param module    The module whose event function data to reset.
 */
stock EventMgr_ResetFuncData(Module:module)
{
    // Read all the event function data.
    new Function:eventfuncdata[sizeof(g_EMEvents)];
    EventMgr_ReadAllFuncData(module, eventfuncdata);
    
    // Loop through all events.
    for (new eindex = 0; eindex < sizeof(eventfuncdata); eindex++)
    {
        // Reset the function ID to invalid.
        eventfuncdata[eindex] = INVALID_FUNCTION;
    }
    
    // Overwrite the old array with the modified one.
    SetArrayArray(ModuleMgr_GetModuleArray(module), EVENT_DATA_FUNCTION, eventfuncdata[0], sizeof(eventfuncdata));
}

/**
 * Event data reader that returns a single event function value.
 * 
 * @param module    The module whose event function data to read.
 * @param eventid   A registered event id.
 * 
 * @return          The function ID of the event forward in the module.
 */
stock Function:EventMgr_ReadFuncValue(Module:module, ProjectEvent:eventid)
{
    new Function:eventfuncdata[sizeof(g_EMEvents)];
    EventMgr_ReadAllFuncData(module, eventfuncdata);
    
    // Return event's enable value.
    return eventfuncdata[_:eventid];
}

/**
 * Module data writer that writes a new function ID for an event.
 * 
 * @param module    The module whose event function data to write.
 * @param eventid   The event to write new function ID to.
 * @param func      The new function ID to write as the event's forward function.
 */
stock EventMgr_WriteFuncValue(Module:module, ProjectEvent:eventid, Function:func)
{
    // Read all the module data.
    new Function:eventfuncdata[sizeof(g_EMEvents)];
    EventMgr_ReadAllFuncData(module, eventfuncdata);
    
    // Write the new function ID.
    eventfuncdata[_:eventid] = func;
    
    // Overwrite the old array with the modified one.
    SetArrayArray(ModuleMgr_GetModuleArray(module), EVENT_DATA_FUNCTION, eventfuncdata[0], sizeof(eventfuncdata));
}
