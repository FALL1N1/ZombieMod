/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *  Dependencies: weapons.inc, weprestrict.inc
 *
 *  File:          menus.inc
 *  Type:          Module
 *  Description:   Handles weapons management menu.
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
 * Sends weapon type list to client.
 * 
 * @param client    The client index.
 */
stock WepMenus_Types(client)
{
    // Create menu handle.
    new Handle:hMenu = CreateMenu(WepMenus_TypesHandle);
    
    SetMenuTitle(hMenu,  "%T\n ", "Weapons menu restrict types title", client);
    
    decl String:typename[64];
    for (new wtindex = 0; wtindex < g_iWeaponTypeCount; wtindex++)
    {
        Weapons_ReadWTypeName(wtindex, typename, sizeof(typename));
        AddMenuItem(hMenu, typename, typename);
    }
    
    // If there are no weapons, add an "(Empty)" line.
    if (g_iWeaponTypeCount == 0)
    {
        decl String:empty[64];
        Format(empty, sizeof(empty), "(%T)", "_Empty", client);
        AddMenuItem(hMenu, "empty", empty, ITEMDRAW_DISABLED);
    }
    
    // Set exit back button.
    SetMenuExitBackButton(hMenu, true);
    
    DisplayMenu(hMenu, client, MENU_TIME_FOREVER);
}

/**
 * Called when client selects option in the weapons list menu, and handles it.
 *  
 * @param hMenu     Handle of the menu being used.
 * @param action    The action done on the menu (see menus.inc, enum MenuAction).
 * @param client    The client index.
 * @param slot      The slot index selected (starting from 0).
 */
public WepMenus_TypesHandle(Handle:hMenu, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
        WepMenus_TypeWeapons(client, slot);
    
    // Client closed the menu.
    else if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
            MenuLib_SendMenu(g_hWepRestrictLastMenu[client], client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(hMenu);
}

/**
 * Sends a list of weapons of a certain type in a menu to the client.
 * 
 * @param client    The client index.
 * @param typeindex The array index in g_hWeaponTypes of the weapon type to put in the menu.
 */
new g_WepMenusSelectedIndex[MAXPLAYERS + 1];
stock WepMenus_TypeWeapons(client, typeindex)
{
    // Create menu handle.
    new Handle:hMenu = CreateMenu(WepMenus_TypeWeaponsHandle);
    
    decl String:typename[64];
    Weapons_ReadWTypeName(typeindex, typename, sizeof(typename));
    
    decl String:title[128];
    decl String:restrictall[128];
    decl String:unrestrictall[128];
    
    Format(title, sizeof(title), "%T\n ", "Weapons menu restrict weapon list title", client, typename);
    Format(restrictall, sizeof(restrictall), "%T", "Weapons menu restrict types restrict all", client, typename);
    Format(unrestrictall, sizeof(unrestrictall), "%T\n ", "Weapons menu restrict types unrestrict all", client, typename);
    
    // Draw items as selectable only if not all weapons within the type are restricted or unrestricted.
    SetMenuTitle(hMenu, title);
    
    AddMenuItem(hMenu, "restrictall", restrictall, MenuLib_GetMenuItemDraw(!WepRestrict_AreWeaponsUniform(typeindex, true)));
    AddMenuItem(hMenu, "unrestrictall", unrestrictall, MenuLib_GetMenuItemDraw(!WepRestrict_AreWeaponsUniform(typeindex, false)));
    
    decl String:typeweapon[WEPCONFIG_NAME];
    decl String:display[64 + 2]; // +2 to allow room for the [ ] if its restricted.
    
    // Get an array populated with all weapons of the given type.
    new Handle:hWeapons;
    new count = Weapons_GetWeaponsOfType(typeindex, hWeapons);
    new windex;
    
    for (new weparrayindex = 0; weparrayindex < count; weparrayindex++)
    {
        windex = GetArrayCell(hWeapons, weparrayindex);
        
        // Get name of weapon.
        Weapons_ReadString(windex, WepConfig_Name, typeweapon, sizeof(typeweapon));
        strcopy(display, sizeof(display), typeweapon);
        
        // If the weapon is restricted, then format brackets around it as an indicator.
        if (WepRestrict_IsRestricted(windex))
            Format(display, sizeof(display), "[%s]", typeweapon);
        
        // Add the item to the menu
        AddMenuItem(hMenu, typeweapon, display, MenuLib_GetMenuItemDraw(bool:Weapons_ReadCell(windex, WepConfig_Toggleable)));
    }
    
    CloseHandle(hWeapons);
    
    SetMenuExitBackButton(hMenu, true);
    DisplayMenu(hMenu, client, MENU_TIME_FOREVER);
    
    g_WepMenusSelectedIndex[client] = typeindex;
}

/**
 * Called when client selects option in the weapon group menu, and handles it.
 * 
 * @param hMenu     Handle of the menu being used.
 * @param action    The action done on the menu (see menus.inc, enum MenuAction).
 * @param client    The client index.
 * @param slot      The slot index selected (starting from 0).
 */
public WepMenus_TypeWeaponsHandle(Handle:hMenu, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        // Get name of current weapon type.
        decl String:typename[64];
        Weapons_ReadWTypeName(g_WepMenusSelectedIndex[client], typename, sizeof(typename));
        
        new RestrictQuery:query;
        new bool:single;
        new bool:restrict;
        decl String:returntarget[WEPCONFIG_NAME];
        
        switch(slot)
        {   
            case 0:
            {
                // Restrict all weapons of this type.
                restrict = true;
                query = WepRestrict_Restrict(true, typename, single, returntarget, sizeof(returntarget));
            }
            case 1:
            {
                // Unrestrict all weapons of this type.
                restrict = false;
                query = WepRestrict_Restrict(false, typename, single, returntarget, sizeof(returntarget));
            }
            default:
            {
                // Get weappon name.
                decl String:typeweapon[64];
                GetMenuItem(hMenu, slot, typeweapon, sizeof(typeweapon));
                
                // Get weapon index.
                new windex = Weapons_NameToIndex(typeweapon);
                if (windex == -1)
                {
                    LogMgr_Print(g_moduleWeapons, LogType_Error, "Weapon Restriction Menu", "Invalid weapon (%s) in the restriction menu", typeweapon);
                    return;
                }
                
                // If weapon isn't restricted, then restrict it.
                if (!WepRestrict_IsRestricted(windex))
                {
                    // Restrict this weapon.
                    restrict = true;
                    query = WepRestrict_Restrict(true, typeweapon, single, returntarget, sizeof(returntarget));
                }
                else
                {
                    // Unrestrict this weapon.
                    restrict = false;
                    query = WepRestrict_Restrict(false, typeweapon, single, returntarget, sizeof(returntarget));
                }
            }
        }
        
        // Print query response.
        WepRestrict_PrintQueryResponse((query != Query_Successful) ? client : 0, query, single, restrict, returntarget);
        
        // Resend menu.
        WepMenus_TypeWeapons(client, g_WepMenusSelectedIndex[client]);
    }
    
    // Client closed the menu.
    else if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
            WepMenus_Types(client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(hMenu);
}
