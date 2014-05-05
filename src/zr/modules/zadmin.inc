/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          zadmin.inc
 *  Type:          Module
 *  Description:   Admin menu for ZR commands.
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
new Module:g_moduleZAdmin;

/**
 * Called when ZAdmin is created.
 * 
 * @param hMenu     Handle to ZAdmin.
 */
new ProjectEvent:g_evOnZAdminCreated;

/**
 * Called when ZAdmin is deleted.
 * 
 * @param hMenu     Handle to ZAdmin.
 */
new ProjectEvent:g_evOnZAdminDeleted;

/**
 * ZAdmin's menu handle.
 */
new Handle:g_hZAdminMenu;

/**
 * ZAdmin's button index in ZMenu.
 */
new g_iZAdminZMenuButton;

/**
 * Register this module.
 */
ZAdmin_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZAdmin");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zadmin");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Admin menu for ZR commands.");
    moduledata[ModuleData_Dependencies][0] = ZMenu_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZAdmin = ModuleMgr_Register(moduledata);
    
    // Create custom events.
    g_evOnZAdminCreated = EventMgr_CreateEvent("Event_OnZAdminCreated");
    g_evOnZAdminDeleted = EventMgr_CreateEvent("Event_OnZAdminDeleted");
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZAdmin, "Event_OnEventsRegister",            "ZAdmin_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZAdmin_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZAdmin, "Event_OnAllModulesLoaded",          "ZAdmin_OnAllModulesLoaded");
    EventMgr_RegisterEvent(g_moduleZAdmin, "Event_OnZMenuCreated",              "ZAdmin_OnZMenuCreated");
    EventMgr_RegisterEvent(g_moduleZAdmin, "Event_OnMyModuleEnable",            "ZAdmin_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleZAdmin, "Event_OnMyModuleDisable",           "ZAdmin_OnMyModuleDisable");
}

/**
 * Plugin is loading.
 */
ZAdmin_OnPluginStart()
{
    // Register the module.
    ZAdmin_Register();
    
    // Register the zmenu command.
    RegConsoleCmd("zadmin", Command_ZAdmin);
}

/**
 * All modules have been registered at this point.
 */
public ZAdmin_OnAllModulesLoaded()
{
    ZAdmin_CreateMenu();
}

/**
 * Called when ZMenu is created.
 * 
 * @param hMenu     Handle to ZMenu.
 */
public ZAdmin_OnZMenuCreated(Handle:hMenu)
{
    ZAdmin_AddButtonToMenu(hMenu);
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ZAdmin_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Enable ZAdmin's button to the menu.
    new Handle:hZMenu = MenuLib_FindMenuById("zmenu");
    MenuLib_BtnWriteCell(hZMenu, g_iZAdminZMenuButton, MenuBtn_Style, ITEMDRAW_DEFAULT);
    
    ZAdmin_CreateMenu();
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ZAdmin_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    // Disable ZAdmin's button to the menu.
    new Handle:hZMenu = MenuLib_FindMenuById("zmenu");
    if (hZMenu != INVALID_HANDLE)
        MenuLib_BtnWriteCell(hZMenu, g_iZAdminZMenuButton, MenuBtn_Style, ITEMDRAW_DISABLED);
    
    ZAdmin_DeleteMenu();
}

ZAdmin_AddButtonToMenu(Handle:hMenu)
{
    // Add ZAdmin's button to the menu if possible.
    decl String:info[16];
    new count = MenuLib_GetMenuBtnCount(hMenu);
    for (new i = 0; i < count; i++)
    {
        MenuLib_BtnReadString(hMenu, i, MenuBtn_Info, info, sizeof(info));
        if (StrEqual(info, "zadmin"))
            return;
    }
    g_iZAdminZMenuButton = MenuLib_AddMenuBtnEx(hMenu, "ZAdmin zmenu button", "zadmin", true, ITEMDRAW_DEFAULT, INVALID_FUNCTION, BtnNextMenu_LinkMenu, "zadmin");
}

ZAdmin_CreateMenu()
{
    ZAdmin_AddButtonToMenu(MenuLib_FindMenuById("zmenu"));
    
    // Register menu with menulib.
    g_hZAdminMenu = MenuLib_CreateMenu("zadmin", INVALID_FUNCTION, INVALID_FUNCTION, "ZAdmin title", true, false, false);
    
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = g_hZAdminMenu;
    EventMgr_Forward(g_evOnZAdminCreated, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
}

ZAdmin_DeleteMenu()
{
    // Forward event to all modules.
    new any:eventdata[1][1];
    eventdata[0][0] = g_hZAdminMenu;
    EventMgr_Forward(g_evOnZAdminDeleted, eventdata, sizeof(eventdata), sizeof(eventdata[]), g_CommonDataType2);
    
    // Delete the entire menu tree.
    MenuLib_DeleteMenu(g_hZAdminMenu);
}

/**
 * Command callback (zmenu)
 * Open up zmenu.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:Command_ZAdmin(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleZAdmin))
        return Plugin_Continue;
    
    if (client == SERVER_INDEX)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Must be player");
        return Plugin_Handled;
    }
    
    MenuLib_DisplayMenu(g_hZAdminMenu, client);
    return Plugin_Handled;
}
