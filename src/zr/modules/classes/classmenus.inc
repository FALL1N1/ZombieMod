/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          classmenus.inc
 *  Type:          Module include
 *  Description:   Manages class menus.
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

// Note: Global variables are defined in classmanager.inc.

/*
    additional menu option ideas:
        - ability to select which mother zombie to become (if server settings
          (allow it)
    player preference ideas:
        - prefer admin classes in random selections (if player is admin)
*/

/**
 * Temporary list of class indexes while displaying menus.
 */
new Handle:g_ClassMgr_TempClassList[MAXPLAYERS] = {INVALID_HANDLE, ...};

/**
 * Temporary list of class names while displaying menus.
 */
new Handle:g_ClassMgr_TempNameList[MAXPLAYERS] = {INVALID_HANDLE, ...};

/**
 * Builds the main class menu (zclass) for a player.
 *
 * This menu is used to display information about the current and eventually
 * next class being used - in addition to options for selecting human and zombie
 * classes.
 *
 * @param client    The client (index) this menu is built for.
 *
 * @return          Handle to menulib menu or INVALID_HANDLE if failed.
 */
Handle:ClassMgr_CreateMainMenu(client)
{
    new Handle:menu;
    
    decl String:id[ML_DATA_ID];
    decl String:humanName[CLASS_NAME_LEN];
    decl String:zombieName[CLASS_NAME_LEN];
    decl String:currentName[CLASS_NAME_LEN];
    decl String:nextName[CLASS_NAME_LEN];
    id[0] = 0;
    humanName[0] = 0;
    zombieName[0] = 0;
    currentName[0] = 0;
    nextName[0] = 0;
    
    // Generate a unique ID based on client and game time. This isn't used by
    // anything, but menulib requires it.
    new Float:time = GetGameTime();
    Format(id, sizeof(id), "%d %0.2", client, time);
    
    // Build the menu.
    menu = MenuLib_CreateMenu(
            id,                         // id
            INVALID_FUNCTION,           // pre-callback
            INVALID_FUNCTION,           // callback
            "ClassMgr main menu title", // title
            true,                       // translate title
            false,                      // forceback
            true);                      // temp menu
    
    // Check if failed.
    if (menu == INVALID_HANDLE)
    {
        LogMgr_Print(g_moduleClassMgr, LogType_Error, "Main menu", "Failed to create class menu. This is a bug, report to developers.");
        return INVALID_HANDLE;
    }
    
    // Get current and next classes.
    // (Note: Next classes are classes that will be used next time the player
    //        spawn. They are optional and may not be set.)
    new human = g_ClassSelected[client][ClassTeam_Humans];
    new zombie = g_ClassSelected[client][ClassTeam_Zombies];
    new nextHuman = g_ClassNextClass[client][ClassTeam_Humans];
    new nextZombie = g_ClassNextClass[client][ClassTeam_Zombies];
    
    // Get current class names.
    decl String:humanName[CLASS_NAME_LEN];
    decl String:zombieName[CLASS_NAME_LEN]
    humanName[0] = 0;
    zombieName[0] = 0;
    ClsGeneric_GetDisplayName(human, humanName, sizeof(humanName));
    ClsGeneric_GetDisplayName(zombie, zombieName, sizeof(zombieName));
    
    // Add current class info.
    decl String:currentLabel[ML_DATA_LABEL];
    currentLabel[0] = 0;
    Format(currentLabel, sizeof(currentLabel), "%T %s", client, "ClassMgr menu current class", nextClassName);
    MenuLib_AddMenuBtnEx(
            menu,                           // menu handle
            currentLabel,                   // label
            "",                             // info
            false,                          // translate
            ITEMDRAW_RAWLINE,               // style
            INVALID_FUNCTION,               // callback
            BtnNextMenu_None,               // NextMenu
            "");                            // strLinkedMenu
    
    // Get next class name, if any.
    decl String:nextClassName[CLASS_NAME_LEN];
    nextClassName[0] = 0;
    new bool:hasNextClass = bool:ClassMgr_GetNextClassName(client, String:buffer, maxlen);

    // Add next class info.
    if (hasNextClass)
    {
        decl String:nextLabel[ML_DATA_LABEL];
        nextLabel[0] = 0;
        
        Format(nextLabel, sizeof(nextLabel), "%T %s", client, "ClassMgr menu next class", nextClassName);
        
        MenuLib_AddMenuBtnEx(
                menu,                           // menu handle
                nextLabel,                      // label
                "",                             // info
                false,                          // translate
                ITEMDRAW_RAWLINE,               // style
                INVALID_FUNCTION,               // callback
                BtnNextMenu_None,               // NextMenu
                "");                            // strLinkedMenu
    }
    
    // Add space.
    MenuLib_AddMenuBtnEx(
                menu,                           // menu handle
                "\n",                           // label
                "",                             // info
                false,                          // translate
                ITEMDRAW_RAWLINE,               // style
                INVALID_FUNCTION,               // callback
                BtnNextMenu_None,               // NextMenu
                "");                            // strLinkedMenu
    
    // Get whether player can change class, per team.
    new humanStyle = GetConVarBool(g_hCvarClassesHumanSelect) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED;
    new zombieStyle = GetConVarBool(g_hCvarClassesZombieSelect) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED;
    
    // Add zombie button.
    MenuLib_AddMenuBtnEx(
            menu,                           // menu handle
            "ClassMgr menu select zombie",  // label
            "zombie",                       // info
            true,                           // translate
            zombieStyle,                    // style
            ClassMgr_ClassMainHandler,      // callback
            BtnNextMenu_None,               // NextMenu
            "");                            // strLinkedMenu
    
    // Add human button.
    MenuLib_AddMenuBtnEx(
            menu,                           // menu handle
            "ClassMgr menu select human",   // label
            "human",                        // info
            true,                           // translate
            humanStyle,                     // style
            ClassMgr_ClassMainHandler,      // callback
            BtnNextMenu_None,               // NextMenu
            "");                            // strLinkedMenu
    
    /* Layout
    ---------------------------
    Class Selection
    Current class: <class name>
    [Next class: <class name>]
    <\n>
    1. Select Zombie
    2. Select Human
    
    [8. Back]
    0. Exit
    ---------------------------
    */
    
    return menu;
}

Handle:ClassMgr_CreateClassListMenu(client, ClassTeam:team)
{
    // Build a list of classes in the specified team that the client has access
    // to.
    
    g_ClassMgr_TempClassList[client] = CreateArray();
    
    // Setup filter.
    new filter[ClassFilter];
    filter[ClassFilter_IgnoreEnabled] = false;
    filter[ClassFilter_RequireFlags] = 0;
    filter[ClassFilter_DenyFlags] = CLASS_SPECIALFLAGS;
    filter[ClassFilter_Client] = client;
    
    // Allow admin classes if admin.
    if (Auth_IsClientAdmin(client))
    {
        filter[ClassFilter_DenyFlags] -= CLASS_FLAG_ADMIN_ONLY;
    }
    
    // Get class list.
    new resultCount = ClassMgr_GetClasses(g_ClassMgr_TempClassList[client], team, filter);
    
    if (resultCount == 0)
    {
        // No accessible classes. This is a bug because the class validation
        // makes sure there is at least one accessible class for everyone.
        LogMgr_Print(g_moduleClassMgr, LogType_Error, "Class select menu", "No accessible classes found. This is a bug, report to developers.");
        CloseHandle(g_ClassMgr_TempClassList[client]);
        return INVALID_HANDLE;
    }
    
    // Get name list.
    g_ClassMgr_TempNameList[client] = ClsGeneric_GetDisplayNames(g_ClassMgr_TempClassList[client], ClassCache_Modified);
    
    decl String:prompt[ML_DATA_TITLE];
    prompt[0] = 0;
    Format(prompt, sizeof(prompt), "%t", client, (team == ClassTeam_Humans ? "ClassMgr menu select human" : "ClassMgr menu select zombie"));
    
    new Handle:menu = DynMenuLib_CreateMenuEx(
            prompt,                     // prompt
            DynMenu_String,             // type
            DynMenu_List,               // mode
            g_ClassMgr_TempNameList[client],    // entries
            ClassMgr_ClassSelectHandler,// handler
            0,                          // lowerLimit (ignored)
            0,                          // upperLimit (ignored)
            0,                          // smallStep (ignored)
            0,                          // largeStep (ignored)
            INVALID_HANDLE,             // initialValue
            INVALID_HANDLE);            // fromMenu
    
    return menu;
}

public ClassMgr_ClassMainHandler(Handle:menu, client, slot)
{
    // Get team selected in menu.
    // Build (separate function) and display menu.
}

public ClassMgr_ClassSelectHandler(Handle:menu, client, DynMenuAction:action, any:value)
{
    switch(action)
    {
        case DynMenuAction_Select:
        {
            new classIndex = GetArrayCell(g_ClassMgr_TempClassList[client], value);
            
            // Trigger selection event.
            static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell};
            new eventdata[1][2];
            eventdata[0][0] = client;
            eventdata[0][1] = classIndex;
            EventMgr_Forward(g_EvOnClassSelected, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
            
            ClassMgr_DeleteTempLists(client);
        }
        case DynMenuAction_Close:
        {
            DynMenuLib_DeleteMenu(menu);
            ClassMgr_DeleteTempLists(client);
        }
    }
}

stock ClassMgr_DeleteTempLists(client)
{
    Util_CloseHandle(g_ClassMgr_TempClassList[client]);
    Util_CloseHandle(g_ClassMgr_TempNameList[client]);
}
