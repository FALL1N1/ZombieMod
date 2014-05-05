/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *  Dependencies: weapons.inc
 *
 *  File:          zmarket.inc
 *  Type:          Module
 *  Description:   A customizable menu of weapons that clients can build weapon setups with.
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
new Module:g_moduleZMarket;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZMarket_GetIdentifier() { return g_moduleZMarket; }

/**
 * Cvar handles.
 */
new Handle:g_hCvarZMarketBuyzone;
new Handle:g_hCvarZMarketRebuy;
new Handle:g_hCvarZMarketAutoRebuy;
new Handle:g_hCvarZMarketMultiWepFormat;

/**
 * Token to separate each weapon in a client's weapon setup (define value basically obsolete)
 */
#define ZMARKET_WEAPON_COOKIE_TOKEN "|"

/**
 * The maximum number of weapons you can have per slot.
 */
#define ZMARKET_MAX_WEAPONS_PER_SLOT 16

/**
 * Cookie handles.
 */
new Handle:g_hZMarketAutoRebuyCookie = INVALID_HANDLE;
new Handle:g_hZMarketWeaponSetupCookie[WEPLIB_SLOT_COUNT];

/**
 * Register this module.
 */
ZMarket_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZMarket");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zmarket");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "A customizable menu of weapons that clients can build weapon setups with.");
    moduledata[ModuleData_Dependencies][0] = Weapons_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZMarket = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZMarket, "Event_OnEventsRegister",       "ZMarket_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZMarket_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZMarket, "Event_OnPluginEnd",            "ZMarket_OnPluginEnd");
    EventMgr_RegisterEvent(g_moduleZMarket, "Event_OnClientPutInServer",    "ZMarket_OnClientPutInServer");
    EventMgr_RegisterEvent(g_moduleZMarket, "Event_OnClientCookiesCached",  "ZMarket_OnClientCookiesCached");
    EventMgr_RegisterEvent(g_moduleZMarket, "Event_PlayerSpawn",            "ZMarket_PlayerSpawn");
}

/**
 * Plugin is loading.
 */
ZMarket_OnPluginStart()
{
    // Register the module.
    ZMarket_Register();
    
    // Create cvars.
    g_hCvarZMarketBuyzone =         Project_CreateConVar("zmarket_buyzone",         "1",        "Requires player to be inside a buyzone to use ZMarket.");
    g_hCvarZMarketRebuy =           Project_CreateConVar("zmarket_rebuy",           "1",        "Allow players to rebuy their weapon setup.");
    g_hCvarZMarketAutoRebuy =       Project_CreateConVar("zmarket_autorebuy",       "1",        "Allow players to automatically rebuy their weapon setup on spawn. [Dependency: zmarket_rebuy]");
    g_hCvarZMarketMultiWepFormat =  Project_CreateConVar("zmarket_multiwep_format", " (##)",    "The text that indicates the number of weapons in a single slot in the ZMarket menu. ['##' = The item count]");
    
    // Create commands.
    RegConsoleCmd("zmarket", Command_ZMarket, "Accesses ZMarket menu.");
    
    // Initialize cookies.
    g_hZMarketAutoRebuyCookie = RegClientCookie("zr_zmarket_autorebuy", "", CookieAccess_Protected);
    
    decl String:rebuycookiename[32];
    for (new wepslotindex = 0; wepslotindex < WEPLIB_SLOT_COUNT; wepslotindex++)
    {
        // Format cookie name and description.
        Format(rebuycookiename, sizeof(rebuycookiename), "zr_zmarket_wepsetup_%d", wepslotindex);
        g_hZMarketWeaponSetupCookie[wepslotindex] = RegClientCookie(rebuycookiename, "", CookieAccess_Protected);
    }
}

/**
 * Plugin is ending.
 */
public ZMarket_OnPluginEnd()
{
    // Destroy the cookie data.
    CloseHandle(g_hZMarketAutoRebuyCookie);
    for (new wepslotindex = 0; wepslotindex < WEPLIB_SLOT_COUNT; wepslotindex++)
        CloseHandle(g_hZMarketWeaponSetupCookie[wepslotindex]);
}

/**
 * Client has joined the server.
 * 
 * @param client    The client index.
 */
public ZMarket_OnClientPutInServer(client)
{
    ZMarket_ResetBuyCount(client);
}

/**
 * A client's cookies have been cached from the database.
 * 
 * @param client    The client index.
 */
public ZMarket_OnClientCookiesCached(client)
{
    // Check if the cookie is initialized for this client.
    if (!CookieLib_IsInitialized(client, g_hZMarketAutoRebuyCookie))
    {
        // By default, auto-rebuy is off for first-time players.
        CookieLib_SetBool(client, g_hZMarketAutoRebuyCookie, false);
    }
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 */
public ZMarket_PlayerSpawn(client)
{
    // Execute the event 0.1 seconds later.
    CreateTimer(0.1, ZMarket_PostPlayerSpawn, client);
}

public Action:ZMarket_PostPlayerSpawn(Handle:timer, any:client)
{
    if (!IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Continue;
    
    // Reset buy counts for client.
    ZMarket_ResetBuyCount(client);
    
    // If auto-rebuy is disabled, then ensure it is also disabled on the client as well.
    if (!GetConVarBool(g_hCvarZMarketAutoRebuy))
    {
        CookieLib_SetBool(client, g_hZMarketAutoRebuyCookie, false);
    }
    
    if (!TeamMgr_IsClientHuman(client))
        return Plugin_Continue;
    
    // If auto-rebuy is enabled, then force client to rebuy weapons.
    if (CookieLib_GetBool(client, g_hZMarketAutoRebuyCookie))
    {
        ZMarket_RebuySavedSetup(client, true);
    }
    
    return Plugin_Continue;
}

// **********************************************
//             Buy Count Stocks
// **********************************************

/**
 * Reset the buy count for a client.
 * 
 * @param client    The client index.
 */
stock ZMarket_ResetBuyCount(client)
{
    // Loop through all weapons and initialize the buy count to 0.
    for (new windex = 0; windex < g_iWeaponCount; windex++)
        Weapons_WriteCell(windex, WepConfig_BuyCount, 0, client);
}

/**
 * Set a client's buy count on a weapon.
 * 
 * @param client    The client index.
 * @param windex    The weapon cache index.
 * @param value     The amount of times the client has bought the weapon.
 */
stock ZMarket_SetBuyCount(client, windex, value, bool:add = false)
{
    // Handle adding or setting.
    new buylimit = 0;
    if (add)
        buylimit = ZMarket_GetBuyCount(client, windex);
    
    Weapons_WriteCell(windex, WepConfig_BuyCount, buylimit + value, client);
}

/**
 * Get the buys left for a client of a weapon.
 * 
 * @param client    The client index.
 * @param windex    The weapon cache index.
 * 
 * @return          The number of times the client has bought the weapon in their current life.
 */
stock ZMarket_GetBuyCount(client, windex)
{
    return Weapons_ReadCell(windex, WepConfig_BuyCount, client);
}

// **********************************************
//                ZMarket Menus
// **********************************************

/**
 * Sends main ZMarket menu to client.
 * 
 * @param client    The client index.
 * 
 * @return          True if the menu was sent successfully, false if not.
 */
stock bool:ZMarket_MenuMain(client)
{
    // If the module isn't enabled, then stop.
    if (ModuleMgr_IsDisabled(g_moduleZMarket))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "Feature is disabled");
        return false;
    }
    
    // Create menu handle.
    new Handle:menu_zmarket_main = CreateMenu(ZMarket_MenuMainHandle);
    
    decl String:title[128];
    decl String:rebuysavedsetup[128];
    decl String:buyweapons[128];
    decl String:savecurrentsetup[128];
    decl String:viewsavedsetup[128];
    decl String:autorebuy[128];
    
    // Get auto-rebuy setting.
    decl String:autorebuysetting[64];
    TransMgr_BoolToPhrase(client, CookieLib_GetBool(client, g_hZMarketAutoRebuyCookie), BoolPhrase_OnOff, autorebuysetting, sizeof(autorebuysetting));
    
    // Format menu options.
    Format(title, sizeof(title), "%T\n ", "Weapons menu zmarket main title", client);
    Format(rebuysavedsetup, sizeof(rebuysavedsetup), "%T", "Weapons menu zmarket main rebuy saved setup", client);
    Format(buyweapons, sizeof(buyweapons), "%T", "Weapons menu zmarket main buy weapons", client);
    Format(savecurrentsetup, sizeof(savecurrentsetup), "%T", "Weapons menu zmarket main save current setup", client);
    Format(viewsavedsetup, sizeof(viewsavedsetup), "%T", "Weapons menu zmarket main view weapon setup", client);
    Format(autorebuy, sizeof(autorebuy), "%T", "Weapons menu zmarket main auto-rebuy", client, autorebuysetting);
    
    // Get cvar values.
    new bool:zmarketrebuy = GetConVarBool(g_hCvarZMarketRebuy);
    new bool:zmarketautorebuy = GetConVarBool(g_hCvarZMarketAutoRebuy);
    
    // Determine if the option will be selectable or not.
    new itemdraw = MenuLib_GetMenuItemDraw(zmarketrebuy || zmarketautorebuy);
    
    // Add formatted options to menu.
    SetMenuTitle(menu_zmarket_main, title);
    AddMenuItem(menu_zmarket_main, "Rebuy Saved Setup", rebuysavedsetup, itemdraw);
    AddMenuItem(menu_zmarket_main, "Buy Weapons", buyweapons);
    AddMenuItem(menu_zmarket_main, "Save Current Setup", savecurrentsetup, itemdraw);
    AddMenuItem(menu_zmarket_main, "View Weapon Setup", viewsavedsetup, itemdraw);
    AddMenuItem(menu_zmarket_main, "Auto-Rebuy", autorebuy, MenuLib_GetMenuItemDraw(zmarketautorebuy));
    
    // Set exit back button.
    SetMenuExitBackButton(menu_zmarket_main, true);
    DisplayMenu(menu_zmarket_main, client, MENU_TIME_FOREVER);
    return true;
}

/**
 * Called when client selects option in the main ZMarket menu, and handles it.
 * 
 * @param menu_zmarket_main    Handle of the menu being used.
 * @param action                The action done on the menu (see menus.inc, enum MenuAction).
 * @param client                The client index.
 * @param slot                  The slot index selected (starting from 0).
 */
public ZMarket_MenuMainHandle(Handle:menu_zmarket_main, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        switch(slot)
        {
            // Buy Saved Setup
            case 0:
            {
                ZMarket_RebuySavedSetup(client);
                ZMarket_MenuMain(client);
            }
            // Buy Weapons
            case 1:     ZMarket_MenuTypes(client);
            
            // Save Current Setup
            case 2:
            {
                ZMarket_SaveCurrentSetup(client);
                ZMarket_MenuMain(client);
            }
            // View Weapon Setup
            case 3:     ZMarket_MenuWepSetup(client);
            
            // Auto-rebuy
            case 4:
            {
                ZMarket_ToggleAutoRebuy(client);
                ZMarket_MenuMain(client);
            }
        }
    }
    // Client closed the menu.
    else if (action == MenuAction_Cancel)
    {
        // TODO: ZMenu
        // Client hit "Back" button.
        //if (slot == MenuCancel_ExitBack)
            //ZMenuMain(client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(menu_zmarket_main);
}


/**
 * Sends weapon type list to client.
 *  
 * @param client    The client index.
 */
stock ZMarket_MenuTypes(client)
{
    // Create menu handle.
    new Handle:menu_zmarket_types = CreateMenu(ZMarket_MenuTypesHandle);
    
    SetMenuTitle(menu_zmarket_types, "%T\n ", "Weapons menu zmarket types title", client);
    
    decl String:typename[64];
    for (new wtindex = 0; wtindex < g_iWeaponTypeCount; wtindex++)
    {
        Weapons_ReadWTypeName(wtindex, typename, sizeof(typename));
        AddMenuItem(menu_zmarket_types, typename, typename);
    }
    
    // If there are no weapons, add an "(Empty)" line.
    if (g_iWeaponTypeCount == 0)
    {
        decl String:empty[64];
        Format(empty, sizeof(empty), "(%T)", "_Empty", client);
        AddMenuItem(menu_zmarket_types, "empty", empty, ITEMDRAW_DISABLED);
    }
    
    SetMenuExitBackButton(menu_zmarket_types, true);
    DisplayMenu(menu_zmarket_types, client, MENU_TIME_FOREVER);
}

/**
 * Called when client selects option in the weapons list menu, and handles it.
 *  
 * @param menu_zmarket_types    Handle of the menu being used.
 * @param action                The action done on the menu (see menus.inc, enum MenuAction).
 * @param client                The client index.
 * @param slot                  The slot index selected (starting from 0).
 */
public ZMarket_MenuTypesHandle(Handle:menu_zmarket_types, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
        ZMarket_MenuTypeWeapons(client, slot);
    
    // Client closed the menu.
    else if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
            ZMarket_MenuMain(client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(menu_zmarket_types);
}

/**
 * Sends a list of weapons of a certain type in a menu to the client.
 * 
 * @param client    The client index.
 * @param typeindex The array index in g_hWeaponTypes of the weapon type to put in the menu. 
 */
new g_ZMarketMenuSelectedIndex[MAXPLAYERS + 1];
stock ZMarket_MenuTypeWeapons(client, typeindex)
{
    // Create menu handle.
    new Handle:menu_zmarket_typeweapons = CreateMenu(ZMarket_MenuTypeWeaponsHandle);
    
    // Get name of current weapon type.
    decl String:typename[64];
    Weapons_ReadWTypeName(typeindex, typename, sizeof(typename));
    
    decl String:typeweaponentity[64];
    decl String:display[128];
    
    new windex;
    decl String:strWIndex[4];
    decl String:typeweapon[WEPCONFIG_NAME];
    decl String:typeweapondisplay[128];
    new bool:hasweapon;
    new WepLib_Slots:slot;
    new bool:restricted;
    new buylimit, buycount, buysleft;
    new itemprice;
    
    // Get an array populated with all weapons of the given type.
    new Handle:hWeapons;
    new count = Weapons_GetWeaponsOfType(typeindex, hWeapons);
    for (new weparrayindex = 0; weparrayindex < count; weparrayindex++)
    {
        windex = GetArrayCell(hWeapons, weparrayindex);

        // Get needed data on the weapon.
        Weapons_ReadString(windex, WepConfig_Name, typeweapon, sizeof(typeweapon));
        Weapons_ReadString(windex, WepConfig_Entity, typeweaponentity, sizeof(typeweaponentity));
        hasweapon = WepLib_HasWeapon(client, typeweaponentity);
        slot = WepLib_Slots:Weapons_ReadCell(windex, WepConfig_Slot);
        restricted = WepRestrict_IsRestricted(windex);
        buylimit = Weapons_ReadCell(windex, WepConfig_BuyLimit);
        buycount = ZMarket_GetBuyCount(client, windex);
        buysleft = buylimit - buycount;
        
        // Do appropriate formatting for the type of item client is buying.
        #if defined PROJECT_GAME_CSS
        if (!hasweapon || slot == Slot_Projectile)
        #else
        if (!hasweapon)
        #endif
        {
            itemprice = Weapons_ReadCell(windex, WepConfig_Price);
        }
        else
        {
            itemprice = Weapons_ReadCell(windex, WepConfig_AmmoPrice);
            Format(typeweapondisplay, sizeof(typeweapondisplay), "%s (%T)", typeweapon, "Weapons menu zmarket types weapon ammo", client);
        }
        
        // If the itemprice is invalid, then set to known integer to be later replaced.
        if (itemprice < 0)
            itemprice = -1;
        
        // If weapon is restricted then format "[]" around it.
        strcopy(typeweapondisplay, sizeof(typeweapondisplay), typeweapon);
        if (restricted)
            Format(typeweapondisplay, sizeof(typeweapondisplay), "[%s]", typeweapon);
        
        // Fix this if it's negative, it shouldn't be.
        if (buysleft < 0)
            buysleft = 0;
        
        // If the buy limit is disabled for the weapon, then set as known integer to be later replaced.
        if (buylimit <= 0)
            buysleft = -1;
        
        // Format price onto the menu entry.
        Format(display, sizeof(display), "%T", "Weapons menu zmarket types weapon info", client, typeweapondisplay, itemprice);
        
        // Format on "Buys Left" if the weapon has a buy limit.
        if (buylimit > 0)
            Format(display, sizeof(display), "%s%T", display, "Weapons menu zmarket types weapon buys left", client, buysleft);
        
        // Remove the known invalid number from the string, and replace with N/A, and remove currency symbol.
        ReplaceString(display, sizeof(display), "$-1", "N/A");
        ReplaceString(display, sizeof(display), "-1", "N/A");
        
        IntToString(windex, strWIndex, sizeof(strWIndex));
        
        // Disable option if it isn't toggleable.
        new itemdraw = MenuLib_GetMenuItemDraw((itemprice > -1) && !restricted && (buylimit <= 0 || buysleft > 0));
        AddMenuItem(menu_zmarket_typeweapons, strWIndex, display, itemdraw);
    }
    
    SetMenuTitle(menu_zmarket_typeweapons, "%T\n ", "Weapons menu zmarket types weapon type title", client, typename);
    CloseHandle(hWeapons);
    
    SetMenuExitBackButton(menu_zmarket_typeweapons, true);
    DisplayMenu(menu_zmarket_typeweapons, client, MENU_TIME_FOREVER);
    
    g_ZMarketMenuSelectedIndex[client] = typeindex;
}

/**
 * Called when client selects option in the type weapons menu, and handles it.
 *  
 * @param menu_zmarket_typeweapons  Handle of the menu being used.
 * @param action                    The action done on the menu (see menus.inc, enum MenuAction).
 * @param client                    The client index.
 * @param slot                      The slot index selected (starting from 0).
 */
public ZMarket_MenuTypeWeaponsHandle(Handle:menu_zmarket_typeweapons, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        decl String:typeweaponindex[64];
        GetMenuItem(menu_zmarket_typeweapons, slot, typeweaponindex, sizeof(typeweaponindex));
        
        // Equip weapon on client.
        ZMarket_Equip(client, StringToInt(typeweaponindex));
        
        // Open types menu.
        ZMarket_MenuTypes(client);
    }
    // Client closed the menu.
    if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
            ZMarket_MenuTypes(client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(menu_zmarket_typeweapons);
}

/**
 * A string array that is used to reference each weapon slot by name, given the index.
 */
new String:g_strTransSlotNames[WEPLIB_SLOT_COUNT][16] = {
    "primary",
    "secondary",
    "melee",
    "projectile",
    "extras"
};

/**
 * Sends list of weapons in a client's weapon setup to the client.
 * 
 * @param client    The client index.
 */
stock bool:ZMarket_MenuWepSetup(client)
{
    // Create menu handle.
    new Handle:menu_zmarket_weaponsetup = CreateMenu(ZMarket_MenuWepSetupHandle);
    
    new String:weaponsetup[WEPLIB_SLOT_COUNT][64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    CookieLib_GetArray(client, g_hZMarketWeaponSetupCookie, weaponsetup, sizeof(weaponsetup), sizeof(weaponsetup[]));
    
    // Get the empty translation.
    decl String:none[64];
    Format(none, sizeof(none), "%T", "_None", client);
    
    // Count and format.
    new weaponcount[WepLib_Slots];
    for (new wepslotindex = 0; wepslotindex < sizeof(weaponcount); wepslotindex++)
    {
        // Count how many weapons are in the slot before inserting the "empty" phrase.
        weaponcount[wepslotindex] = ShopListLib_Count(weaponsetup[wepslotindex], "", ZMARKET_WEAPON_COOKIE_TOKEN);
        
        // If the client doesn't have any weapons in this slot, then set the weapon to the "none" phrase.
        if (weaponcount[wepslotindex] == 0)
            strcopy(weaponsetup[wepslotindex], sizeof(weaponsetup[]), none);
    }
    
    SetMenuTitle(menu_zmarket_weaponsetup, "%T\n ", "Weapons menu zmarket weapon setup title", client);
    
    decl String:clearall[128];
    decl String:weaponslot[WEPLIB_SLOT_COUNT][128];
    decl String:phraseformat[64];
    decl String:shoppinglist[128];
    decl String:info[4];
    
    Format(clearall, sizeof(clearall), "%T\n ", "Weapons menu zmarket weapon setup clear all", client);
    AddMenuItem(menu_zmarket_weaponsetup, "-1", clearall, MenuLib_GetMenuItemDraw(!StrEqual(weaponsetup[Slot_Primary], none) || !StrEqual(weaponsetup[Slot_Secondary], none) || !StrEqual(weaponsetup[Slot_Melee], none) || !StrEqual(weaponsetup[Slot_Projectile], none) || !StrEqual(weaponsetup[Slot_Extras], none)));
    
    decl String:multiwepformat[32];
    GetConVarString(g_hCvarZMarketMultiWepFormat, multiwepformat, sizeof(multiwepformat));
    
    // Format each weapon slot's menu line.
    for (new wepslotindex = 0; wepslotindex < WEPLIB_SLOT_COUNT; wepslotindex++)
    {
        // Format the weapon slot's menu line.
        Format(phraseformat, sizeof(phraseformat), "Weapons menu zmarket weapon setup %s", g_strTransSlotNames[wepslotindex]);
        if (weaponcount[wepslotindex] <= 1)
        {
            Format(weaponslot[wepslotindex], sizeof(weaponslot[]), "%T %s", phraseformat, client, weaponsetup[wepslotindex]);
        }
        else
        {
            ShopListLib_Construct(weaponsetup[wepslotindex], shoppinglist, ZMARKET_MAX_WEAPONS_PER_SLOT * 64, ZMARKET_WEAPON_COOKIE_TOKEN, "\n \t", multiwepformat);
            Format(weaponslot[wepslotindex], sizeof(weaponslot[]), "%T \n \t%s", phraseformat, client, shoppinglist);
        }
        
        IntToString(wepslotindex, info, sizeof(info));
        AddMenuItem(menu_zmarket_weaponsetup, info, weaponslot[wepslotindex], MenuLib_GetMenuItemDraw(!StrEqual(weaponsetup[wepslotindex], none)));
    }
    
    SetMenuExitBackButton(menu_zmarket_weaponsetup, true);
    DisplayMenu(menu_zmarket_weaponsetup, client, MENU_TIME_FOREVER);
}

/**
 * Called when client selects option in the weapon setup menu, and handles it.
 * 
 * @param menu_zmarket_weaponsetup  Handle of the menu being used.
 * @param action                    The action done on the menu (see menus.inc, enum MenuAction).
 * @param client                    The client index.
 * @param slot                      The slot index selected (starting from 0).
 */
public ZMarket_MenuWepSetupHandle(Handle:menu_zmarket_weaponsetup, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        // Convert the menu slot chosen by the client to a weapon slot datatype.
        decl String:info[4];
        GetMenuItem(menu_zmarket_weaponsetup, slot, info, sizeof(info));
        new WepLib_Slots:weaponslot = WepLib_Slots:StringToInt(info);
        
        // This means the "Clear All" button was pressed.
        if (weaponslot == Slot_Invalid)
        {
            new String:values[WEPLIB_SLOT_COUNT][1];
            CookieLib_SetArray(client, g_hZMarketWeaponSetupCookie, values, sizeof(values));
            
            // Re-send menu.
            ZMarket_MenuWepSetup(client);
            return;
        }
        
        // Get the list of weapons stored under this weapon slot.
        decl String:weaponsetup[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
        GetClientCookie(client, g_hZMarketWeaponSetupCookie[weaponslot], weaponsetup, sizeof(weaponsetup));
        
        // Count how many weapons are in the slot.
        new weaponcount = ShopListLib_Count(weaponsetup, "", ZMARKET_WEAPON_COOKIE_TOKEN);
        
        // If there's only 1 weapon in the slot then just clear it.
        if (weaponcount <= 1)
        {
            // Clear rebuy slot.
            SetClientCookie(client, g_hZMarketWeaponSetupCookie[weaponslot], "");
            ZMarket_MenuWepSetup(client);
        }
        else
        {
            // Open a submenu to handle multiple weapons in a slot.
            ZMarket_MenuSubWepSetup(client, weaponslot);
        }
    }
    // Client closed the menu.
    if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
            ZMarket_MenuMain(client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(menu_zmarket_weaponsetup);
}

/**
 * Sends list of weapons in a client's weapon setup of a certain slot to the client.
 * 
 * @param client        The client index.
 * @param weaponslot    Weapons in this slot will be printed.
 */
stock ZMarket_MenuSubWepSetup(client, WepLib_Slots:weaponslot)
{
    // Create menu handle.
    new Handle:menu_zmarket_subweaponsetup = CreateMenu(ZMarket_MenuSubWepSetupHandle);
    
    SetMenuTitle(menu_zmarket_subweaponsetup, "%T\n ", "Weapons menu zmarket sub weapon setup title", client);
    
    decl String:info[64 + 5];  // +5 chars to allow for the serialization formatting.
    
    // Get the weapons to show in the menu.
    decl String:weaponlist[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    GetClientCookie(client, g_hZMarketWeaponSetupCookie[_:weaponslot], weaponlist, sizeof(weaponlist));
    
    // Explode the shopping list to add as individual menu items.
    new String:weapons[ZMARKET_MAX_WEAPONS_PER_SLOT][64];
    ExplodeString(weaponlist, ZMARKET_WEAPON_COOKIE_TOKEN, weapons, sizeof(weapons), sizeof(weapons[]));
    
    // Add weapons to the menu.
    for (new listindex = 0; listindex < sizeof(weapons); listindex++)
    {
        // If there are no more weapons, then break the loop here.
        if (weapons[listindex][0] == 0)
            break;
        
        Format(info, sizeof(info), "%d|%s", _:weaponslot, weapons[listindex]); // don't separate with "_" because a weapon name could contain it, "|" is less likely.
        AddMenuItem(menu_zmarket_subweaponsetup, info, weapons[listindex]);
    }
    
    SetMenuExitBackButton(menu_zmarket_subweaponsetup, true);
    DisplayMenu(menu_zmarket_subweaponsetup, client, MENU_TIME_FOREVER);
}

/**
 * Called when client selects option in the view weapon setup menu, and handles it.
 *  
 * @param menu_zmarket_subweaponsetup   Handle of the menu being used.
 * @param action                        The action done on the menu (see menus.inc, enum MenuAction).
 * @param client                        The client index.
 * @param slot                          The slot index selected (starting from 0).
 */
public ZMarket_MenuSubWepSetupHandle(Handle:menu_zmarket_subweaponsetup, MenuAction:action, client, slot)
{
    // Client selected an option.
    if (action == MenuAction_Select)
    {
        // Get the slot and weapon selected.
        decl String:info[64 + 5];
        GetMenuItem(menu_zmarket_subweaponsetup, slot, info, sizeof(info));
        
        // Separate slot and weapon in an array.
        new String:weaponinfo[2][64];
        ExplodeString(info, "|", weaponinfo, sizeof(weaponinfo), sizeof(weaponinfo[]));
        new WepLib_Slots:weaponslot = WepLib_Slots:StringToInt(weaponinfo[0]);
        
        // weaponinfo[0] = weaponslot = slot
        // weaponinfo[1] = weapon
        
        // Remove from cookie.
        decl String:weapons[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
        GetClientCookie(client, g_hZMarketWeaponSetupCookie[_:weaponslot], weapons, sizeof(weapons));
        ShopListLib_Remove(weapons, sizeof(weapons), weaponinfo[1], ZMARKET_WEAPON_COOKIE_TOKEN);
        SetClientCookie(client, g_hZMarketWeaponSetupCookie[_:weaponslot], weapons);
        
        // Resend this menu if there is more to remove, resend weapon setup menu if all is removed.
        if (ShopListLib_Count(weapons, "", ZMARKET_WEAPON_COOKIE_TOKEN) > 0)
            ZMarket_MenuSubWepSetup(client, weaponslot);
        else
            ZMarket_MenuWepSetup(client);
    }
    
    // Client closed the menu.
    if (action == MenuAction_Cancel)
    {
        // Client hit "Back" button.
        if (slot == MenuCancel_ExitBack)
            ZMarket_MenuWepSetup(client);
    }
    // Client hit "Exit" button.
    else if (action == MenuAction_End)
        CloseHandle(menu_zmarket_subweaponsetup);
}

/**
 * Equip a weapon on a client from ZMarket.
 * 
 * @param client    The client index.
 * @param windex    The weapon cache index.
 * @param rebuy     True if the client is rebuying weapons.
 * @param autorebuy True if the client is auto-rebuying weapons.
 * 
 * @return          True if the weapons/ammo were equipped, false if not.  
 */
stock bool:ZMarket_Equip(client, windex, bool:rebuy = false, bool:autorebuy = false)
{
    // Check if an invalid index was given.
    if (windex == -1)
    {
        ZMarket_CleanWeaponSetup(client);
        return false;
    }
    
    new WepLib_Slots:slot = WepLib_Slots:Weapons_ReadCell(windex, WepConfig_Slot);
    
    #if defined PROJECT_GAME_CSS
        new bool:set = !(slot == Slot_Projectile);
    #else
        new bool:set = true;
    #endif
    
    // Check if the client is alive.
    if (!IsPlayerAlive(client))
    {
        ZMarket_AddWeaponToSlot(client, slot, windex, set);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket updated weapon setup");
    }
    
    decl String:weaponentity[64];
    Weapons_ReadString(windex, WepConfig_Entity, weaponentity, sizeof(weaponentity));
    
    // Check if the client is not a human or not in a buyzone.
    if (!TeamMgr_IsClientHuman(client) || (!autorebuy && GetConVarBool(g_hCvarZMarketBuyzone) && !WepLib_InBuyZone(client)))
    {
        // Update cookie with new weapon.
        if (slot != Slot_Projectile || !ZMarket_IsGrenadeCountMaxedEx(client, weaponentity))
            ZMarket_AddWeaponToSlot(client, slot, windex, set);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket updated weapon setup");
        return false;
    }
    
    // Get the appropriate price of the item being bought.
    new bool:hasweapon = WepLib_HasWeapon(client, weaponentity);
    
    // Special case for nvgs in CS:S.
    #if defined PROJECT_GAME_CSS
        if (StrEqual(weaponentity, "item_nvgs", false))
            hasweapon = OffsLib_GetHasNVGs(client);
    #endif
    
    #if defined PROJECT_GAME_CSS
    new itemprice = (hasweapon && slot != Slot_Projectile) ? Weapons_ReadCell(windex, WepConfig_AmmoPrice) : Weapons_ReadCell(windex, WepConfig_Price);
    #else
    new itemprice = hasweapon ? Weapons_ReadCell(windex, WepConfig_AmmoPrice) : Weapons_ReadCell(windex, WepConfig_Price);
    #endif
    
    // Don't let the price be below 0.
    if (itemprice < 0)
        itemprice = 0;
    
    
    decl String:weaponname[WEPCONFIG_NAME];
    Weapons_ReadString(windex, WepConfig_Name, weaponname, sizeof(weaponname));
    
    // Check if the weapon is restricted.
    if (WepRestrict_IsRestricted(windex))
    {
        // If rebuy is attempting to rebuy a restricted weapon, then remove it from the weapon setup.
        if (rebuy)
        {
            ZMarket_RemoveWeaponFromSlot(client, slot, weaponname, true);
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket weapon setup restricted", weaponname);
        }
        else
        {
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapon is restricted", weaponname);
        }
        
        return false;
    }
    
    // Get the buy count information for this weapon.
    new buylimit = Weapons_ReadCell(windex, WepConfig_BuyLimit);
    new buycount = ZMarket_GetBuyCount(client, windex);
    new buysleft = buylimit - buycount;
    if (buylimit > 0 && buysleft <= 0)
    {
        if (!rebuy)
            TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket buylimit exceeded", weaponname, buylimit);
        return false;
    }
    
    #if defined PROJECT_GAME_CSS
    if (slot == Slot_Projectile)
    {
        // Check if the grenade ammo index returned is valid.
        new grenadeindex = WepLib_GrenadeEntToIndex(weaponentity);
        if (grenadeindex == -1)
        {
            LogMgr_Print(g_moduleZMarket, LogType_Error, "Grenades", "Client \"%L\" attempted to buy weapon entity \"%s\" marked as a projectile. Check your weapon config.", client, weaponentity);
            return false;
        }
        
        new grenadecount = WepLib_GetAmmoEx(client, grenadeindex);
        new grenadelimit = ZMarket_GetGrenadeLimit(grenadeindex);
        
        // If client is at, or exceeds the grenade limit, then stop.
        if (grenadecount >= grenadelimit)
        {
            // Client can't carry any more of this type of grenade.
            if (!rebuy)
            {
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket grenade max", grenadelimit);
            }
            
            return false;
        }
    }
    #endif
    
    // Check cash flow.
    new cash = OffsLib_GetCash(client);
    if (cash < itemprice)
    {
        // Update cookie with new weapon.
        if (slot != Slot_Projectile || !ZMarket_IsGrenadeCountMaxedEx(client, weaponentity))
            ZMarket_AddWeaponToSlot(client, slot, windex, set);
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket updated weapon setup");
        return false;
    }
    
    // Get a list of the client's current weapons.
    new weapons[WepLib_Slots];
    WepLib_GetWeapons(client, weapons);
        
    // Check if client is buying the weapon or ammo for it.
    #if defined PROJECT_GAME_CSS
    if (!hasweapon || slot == Slot_Projectile)
    #else
    if (!hasweapon)
    #endif
    {
        #if defined PROJECT_GAME_CSS
        if (slot != Slot_Projectile)
        {
            // If there is already a weapon in the slot, then force client to drop it.
            if (weapons[slot] != -1)
                 CS_DropWeapon(client, weapons[slot], true);
        }
        #else
        // If there is already a weapon in the slot, then force client to drop it.
        if (weapons[slot] != -1)
            // CS_DropWeapon when not compiling for CS:S?
            //SDKToolsLib_CSWeaponDrop(client, weapons[slot]);
            LogMgr_Print(g_moduleZMarket, LogType_Debug, "ZMarket_Equip", "Drop weapon not implemented!");
        #endif
        
        // Give client the weapon.
        GivePlayerItem(client, weaponentity);
        ZMarket_SetBuyCount(client, windex, 1, true);
        
        // Take cash money.
        OffsLib_SetCash(client, cash - itemprice);
    
        // Check if the client's weapon setup already has the max number of grenades that they can hold if a grenade is being equipped.
        if (slot != Slot_Projectile || !ZMarket_IsGrenadeCountMaxedEx(client, weaponentity))
        {
            // Add the weapon to their setup.
            ZMarket_AddWeaponToSlot(client, slot, windex, set);
        }
        
        // If client isn't rebuying the weapon, then tell them the weapon has been bought.
        if (!rebuy)
        {
            // Tell client they bought a weapon.
            if (slot == Slot_Primary || slot == Slot_Secondary)
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket buy with ammo", weaponname);
            else
                TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket buy no ammo", weaponname);
        }
    }
    else if (!rebuy)
    {
        // Get ammo type and stop if it's invalid.
        decl String:ammocvar[64];
        Weapons_ReadString(windex, WepConfig_AmmoCvar, ammocvar, sizeof(ammocvar));
        if (ammocvar[0] == 0)
            return false;
        
        // Read the value of the ammo cvar.  Assume it's valid, because it's already been validated on config cache.
        new ammoamt = GetConVarInt(FindConVar(ammocvar));
        WepLib_SetAmmo(windex, false, ammoamt);
        
        // Take cash money.
        OffsLib_SetCash(client, cash - itemprice);
    }
    
    return true;
}

/**
 * Saves a client's current weapon setup in the rebuy cache.
 * 
 * @param client    The client index.
 *  
 * @return          True if the setup was saved successfully, false if not.
 */
stock bool:ZMarket_SaveCurrentSetup(client)
{
    if (!IsPlayerAlive(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Must be alive");
        return false;
    }
    
    if (TeamMgr_GetClientTeam(client) != VTeam_Human)
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Must be human");
        return false;
    }
    
    decl String:weaponentity[64];
    decl String:weaponname[WEPCONFIG_NAME];
    new windex;
    
    new weapons[WepLib_Slots];
    WepLib_GetWeapons(client, weapons);
    for (new wepslotindex = 0; wepslotindex < sizeof(weapons); wepslotindex++)
    {
        SetClientCookie(client, g_hZMarketWeaponSetupCookie[wepslotindex], "");
        
        #if defined PROJECT_GAME_CSS
        if (weapons[wepslotindex] != -1 && WepLib_Slots:wepslotindex != Slot_Projectile)
        #else
        if (weapons[wepslotindex] != -1)
        #endif
        {
            GetEdictClassname(weapons[wepslotindex], weaponentity, sizeof(weaponentity));
            windex = Weapons_ClsNameToDisplay(weaponentity, weaponname, sizeof(weaponname));
            
            // Check that this weapon is configured.
            if (windex != -1)
                ZMarket_AddWeaponToSlot(client, WepLib_Slots:wepslotindex, windex, true); // limit to 1 per slot here unless the game someday allows more
        }
    }
    
    #if defined PROJECT_GAME_CSS
    
    decl String:grenadeent[64];
    decl String:grenades[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    
    new grenadecount[32];
    WepLib_GetAmmoArray(client, grenadecount, AMMO_INDEX_HE, AMMO_INDEX_SMOKE);
    for (new ammoindex = AMMO_INDEX_HE; ammoindex <= AMMO_INDEX_SMOKE; ammoindex++)
    {
        WepLib_IndexToGrenadeEnt(ammoindex, grenadeent, sizeof(grenadeent));
        windex = Weapons_ClsNameToDisplay(grenadeent, weaponname, sizeof(weaponname));
        
        // Server is missing a grenade config in their weapons config file, so skip.
        if (windex == -1)
            continue;
        
        for (new i = 0; i < grenadecount[ammoindex]; i++)
            ShopListLib_Add(grenades, sizeof(grenades), weaponname, ZMARKET_WEAPON_COOKIE_TOKEN);
    }
    
    new projectileindex = _:Slot_Projectile;
    SetClientCookie(client, g_hZMarketWeaponSetupCookie[projectileindex], grenades);
    
    // Add NVGs to the weapon setup if the client has them.
    if (OffsLib_GetHasNVGs(client))
    {
        windex = Weapons_ClsNameToDisplay("item_nvgs", weaponname, sizeof(weaponname));
        ZMarket_AddWeaponToSlot(client, Slot_Extras, windex, false);
    }
    
    #endif
    
    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket save current weapon setup");
    return true;
}

/**
 * Add a weapon to the weapon setup cookie given the slot and value.
 * 
 * @param client    The client index.
 * @param slot      The weapon slot to set value to.
 * @param windex    The weapon cache index.
 * @param set       True to overwrite the old value with the new weapon, false to ADD the new weapon to the existing ones.
 */
stock ZMarket_AddWeaponToSlot(client, WepLib_Slots:slot, windex, bool:set)
{
    decl String:weaponname[WEPCONFIG_NAME];
    Weapons_ReadString(windex, WepConfig_Name, weaponname, sizeof(weaponname));
    
    decl String:slotsetup[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    GetClientCookie(client, g_hZMarketWeaponSetupCookie[_:slot], slotsetup, sizeof(slotsetup));
    
    if (set)
        SetClientCookie(client, g_hZMarketWeaponSetupCookie[_:slot], weaponname);
    else
    {
        // Count how many weapons are in the slot.
        new weaponcount = ShopListLib_Count(slotsetup, "", ZMARKET_WEAPON_COOKIE_TOKEN);
        if (weaponcount < ZMARKET_MAX_WEAPONS_PER_SLOT)
        {
            ShopListLib_Add(slotsetup, sizeof(slotsetup), weaponname, ZMARKET_WEAPON_COOKIE_TOKEN);
            SetClientCookie(client, g_hZMarketWeaponSetupCookie[_:slot], slotsetup);
        }
        else
        {
            LogMgr_Print(g_moduleZMarket, LogType_Error, "Limit Exceeded", "Weapon setup for client %N is at the max limit of %d weapons for slot %d.", client, ZMARKET_MAX_WEAPONS_PER_SLOT, _:slot);
        }
    }
}

/**
 * Remove a weapon from the weapon setup cookie given the slot and weapon.
 *
 * @param client    The client index.
 * @param slot      The weapon slot to remove weapon from.
 * @param value     The value (weaponname) of the slot.
 * @param all       True to remove all occurences, false to remove one from the list. 
 * 
 * @return          True if the weapon was found and removed, false if no weapon was found.
 */
stock bool:ZMarket_RemoveWeaponFromSlot(client, WepLib_Slots:slot, const String:value[], bool:all = false)
{
    decl String:slotsetup[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    GetClientCookie(client, g_hZMarketWeaponSetupCookie[_:slot], slotsetup, sizeof(slotsetup));
    
    new bool:success;
    if (all)
    {
        // Removes all occurences of a single weapon in the rebuy cookie
        while (StrContains(slotsetup, value, false) > -1)
        {
            ShopListLib_Remove(slotsetup, sizeof(slotsetup), value, ZMARKET_WEAPON_COOKIE_TOKEN);
            success = true;
        }
    }
    else
    {
        success = ShopListLib_Remove(slotsetup, sizeof(slotsetup), value, ZMARKET_WEAPON_COOKIE_TOKEN);
    }
    
    SetClientCookie(client, g_hZMarketWeaponSetupCookie[_:slot], slotsetup);
    return success;
}

/**
 * Removes a weapon from a client's setup. (broader version of ZMarket_RemoveWeaponFromSlot)
 * 
 * @param client    The client index.
 */
stock ZMarket_RemoveWeaponFromSetup(client, const String:weapon[])
{
    for (new wepslotindex = 0; wepslotindex < WEPLIB_SLOT_COUNT; wepslotindex++)
        ZMarket_RemoveWeaponFromSlot(client, WepLib_Slots:wepslotindex, weapon, true);
}

/**
 * Iterates each weapon in a client's setup and strips out invalid weapons.
 * 
 * @param client    The client index.
 */
stock ZMarket_CleanWeaponSetup(client)
{
    new String:weaponsetup[WEPLIB_SLOT_COUNT][64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    CookieLib_GetArray(client, g_hZMarketWeaponSetupCookie, weaponsetup, sizeof(weaponsetup), sizeof(weaponsetup[]));
    
    new String:slotweapons[ZMARKET_MAX_WEAPONS_PER_SLOT][64];
    for (new wepslotindex = 0; wepslotindex < sizeof(weaponsetup); wepslotindex++)
    {
        if (weaponsetup[wepslotindex][0] == 0)
            continue;
        
        ExplodeString(weaponsetup[wepslotindex], ZMARKET_WEAPON_COOKIE_TOKEN, slotweapons, sizeof(slotweapons), sizeof(slotweapons[]));
        for (new _wepslotindex = 0; _wepslotindex < ZMARKET_MAX_WEAPONS_PER_SLOT; _wepslotindex++)
        {
            // If no more weapons are in the slot, then stop.
            if (slotweapons[_wepslotindex][0] == 0)
                break;
            
            // Check if the weapon is invalid.
            if (Weapons_NameToIndex(slotweapons[_wepslotindex]) == -1)
            {
                ZMarket_RemoveWeaponFromSetup(client, slotweapons[_wepslotindex]);
                LogMgr_Print(g_moduleZMarket, LogType_Normal, "Weapon Setup Cleanup", "Found and removed a non-existant weapon (%s) in the weapon setup of %L.", slotweapons[_wepslotindex], client);
            }
        }
    }
    
    return false;
}

/**
 * Force a client to rebuy their weapons.
 * 
 * @param client    The client index.
 * @param autorebuy True if this is being called for a client auto-rebuying the weapons, false if not.
 */
stock ZMarket_RebuySavedSetup(client, bool:autorebuy = false)
{
    if (!IsPlayerAlive(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Must be alive");
        return;
    }
    
    if (!TeamMgr_IsClientHuman(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Must be human");
        return;
    }
    
    if (!autorebuy && GetConVarBool(g_hCvarZMarketBuyzone) && !WepLib_InBuyZone(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket buyzone");
        return;
    }
    
    new String:weaponsetup[WEPLIB_SLOT_COUNT][64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    CookieLib_GetArray(client, g_hZMarketWeaponSetupCookie, weaponsetup, sizeof(weaponsetup), sizeof(weaponsetup[]));
    
    new String:slotweapons[ZMARKET_MAX_WEAPONS_PER_SLOT][64];
    for (new wepslotindex = 0; wepslotindex < sizeof(weaponsetup); wepslotindex++)
    {
        if (weaponsetup[wepslotindex][0] == 0)
            continue;
        
        ExplodeString(weaponsetup[wepslotindex], ZMARKET_WEAPON_COOKIE_TOKEN, slotweapons, sizeof(slotweapons), sizeof(slotweapons[]));
        for (new _wepslotindex = 0; _wepslotindex < ZMARKET_MAX_WEAPONS_PER_SLOT; _wepslotindex++)
        {
            // If no more weapons are in the slot, then stop.
            if (slotweapons[_wepslotindex][0] == 0)
                break;
            
            ZMarket_Equip(client, Weapons_NameToIndex(slotweapons[_wepslotindex]), true, autorebuy);
        }
    }
}

/**
 * Toggle auto-rebuy on a client.
 * 
 * @param client    The client index.
 */
stock ZMarket_ToggleAutoRebuy(client)
{
    // If auto-rebuy is disabled, then stop.
    if (!GetConVarBool(g_hCvarZMarketAutoRebuy))
        return;
    
    // Toggle the cookie.
    new bool:autorebuystate = CookieLib_GetBool(client, g_hZMarketAutoRebuyCookie);
    CookieLib_SetBool(client, g_hZMarketAutoRebuyCookie, !autorebuystate);
    
    if (autorebuystate)     TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket auto-rebuy toggle off");
    else                    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, g_moduleZMarket, false, "Weapons zmarket auto-rebuy toggle on");
}

/**
 * Command callback: zmarket
 * Open the ZMarket menu.
 * 
 * @param client    The client index.
 * @param argc      The number of arguments that the server sent with the command.
 */
public Action:Command_ZMarket(client, argc)
{
    // Check if the module is disabled.
    if (ModuleMgr_IsDisabled(g_moduleZMarket))
        return Plugin_Continue;
    
    // If client is console, then stop and tell them this feature is for players only.
    if (client > SERVER_INDEX)
        ZMarket_MenuMain(client);
    else
        TransMgr_PrintText(SERVER_INDEX, MsgFormat_Plugin, MsgType_Server, g_moduleZMarket, false, "Must be player");
    
    // This stops the "Unknown command" message in client's console.
    return Plugin_Handled;
}

#if defined PROJECT_GAME_CSS

/**
 * Return the max number of grenades of this type that the client can hold.
 * 
 * @param index The grenade ammo index used in WepLib_SetAmmoEx.
 * 
 * @return      The max number of grenades allowed.  -1 if invalid index was given.
 */
stock ZMarket_GetGrenadeLimit(index)
{
    switch(index)
    {
        case AMMO_INDEX_HE:     return GPBridge_IsLoaded() ? GPBridge_Intercept(): 1;
        case AMMO_INDEX_FLASH:  return 2;
        case AMMO_INDEX_SMOKE:  return 1;
    }
    
    return -1;
}

/**
 * Checks if a client has the max number of a type of grenade that they can hold.
 * 
 * @param client    The client index.
 * @param index     The ammo index associated with the grenade.
 * 
 * @return          True if the client can't hold any more of the grenade, false if they can.
 */
stock bool:ZMarket_IsGrenadeCountMaxed(client, index)
{
    return (WepLib_GetAmmoEx(client, index) >= ZMarket_GetGrenadeLimit(index));
}

/**
 * Checks if a client's weapon setup can hold any more of a certain grenade.
 * Like ZMarket_IsGrenadeCountMaxed except it checks the client's weapon setup, not the equipped weapons.
 * 
 * @param client        The client index.
 * @param weaponentity  The classname of the grenade to count.
 */
stock bool:ZMarket_IsGrenadeCountMaxedEx(client, const String:weaponentity[])
{
    // Check if the client's weapon setup already has the max number of grenades that they can hold if a grenade is being equipped.
    decl String:slotsetup[64 * ZMARKET_MAX_WEAPONS_PER_SLOT];
    new WepLib_Slots:slot = Slot_Projectile;
    GetClientCookie(client, g_hZMarketWeaponSetupCookie[_:slot], slotsetup, sizeof(slotsetup));
    
    decl String:weaponname[WEPCONFIG_NAME];
    Weapons_ClsNameToDisplay(weaponentity, weaponname, sizeof(weaponname));
    
    return (ShopListLib_Count(slotsetup, weaponname, ZMARKET_WEAPON_COOKIE_TOKEN) >= ZMarket_GetGrenadeLimit(WepLib_GrenadeEntToIndex(weaponentity)));
}

#endif