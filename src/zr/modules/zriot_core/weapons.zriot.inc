/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          weapons.zriot.inc
 *  Type:          Module
 *  Description:   Manages weapons for ZRiot gameplay.
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
new Module:g_moduleZRiotWeapons;

/**
 * Function for outside files to use to return the module's identifier.
 */
stock Module:ZRiotWeps_GetIdentifier() { return g_moduleZRiotWeapons; }

/**
 * Cvar handles.
 */
#if defined PROJECT_GAME_CSS
  new Handle:g_hCvarZRiotWeaponsDrop;
  new Handle:g_hCvarZRiotWeaponsEndEquip;
#endif

/**
 * Array to block any client from picking up weapons.
 */
new bool:g_bZRiotWeaponsBlockWeapon[MAXPLAYERS + 1];

/**
 * Register this module.
 */
ZRiotWeps_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "ZRiot Weapon Control");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "zriotweapons");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Manages weapons for ZRiot gameplay.");
    moduledata[ModuleData_Dependencies][0] = ZRiot_GetIdentifier();
    moduledata[ModuleData_Dependencies][1] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleZRiotWeapons = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_OnEventsRegister",         "ZRiotWeps_OnEventsRegister");
}

/**
 * Register all events here.
 */
public ZRiotWeps_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_OnEventsReady",         "ZRiotWeps_OnEventsReady");
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_OnMyModuleEnable",      "ZRiotWeps_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_OnMyModuleDisable",     "ZRiotWeps_OnMyModuleDisable");
    
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_RoundEnd",                 "ZRiotWeps_RoundEnd");
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_PlayerSpawn",              "ZRiotWeps_PlayerSpawn");
    
    // Register custom events.
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Event_OnClientInfected",         "ZRiotWeps_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleZRiotWeapons, "Hook_WeaponCanUse",              "ZRiotWeps_CanUse");
}

/**
 * All modules and events have been registered by this point.  Event priority can be changed here.
 */
public ZRiotWeps_OnEventsReady()
{
    // Bump up priority for Event_RoundEnd to above infection.zriot module.
    EventMgr_GivePriority("Event_RoundEnd", g_moduleZRiotWeapons, ZRiotInfect_GetIdentifier());
}

/**
 * Plugin is loading.
 */
ZRiotWeps_OnPluginStart()
{
    // Register the module.
    ZRiotWeps_Register();
    
    // Create cvars.
    #if defined PROJECT_GAME_CSS
        g_hCvarZRiotWeaponsDrop =      Project_CreateConVar("weapons_drop", "1", "Force player to drop all weapons when infected.  Disabling this will strip weapons instead.");
        g_hCvarZRiotWeaponsEndEquip =  Project_CreateConVar("weapons_endequip", "1", "Restricts zombies from picking up weapons after the round has ended but before the next round has begun.");
    #endif
    
    #if defined PROJECT_GAME_CSS
        // Listen for buy commands.
        AddCommandListener(ZRiotWeps_BuyListener, "buy");
        AddCommandListener(ZRiotWeps_BuyListener, "autobuy");
        AddCommandListener(ZRiotWeps_BuyListener, "rebuy");
    #endif
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:ZRiotWeps_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    #if defined PROJECT_GAME_CSS
        // Listen for buy commands.
        AddCommandListener(ZRiotWeps_BuyListener, "buy");
        AddCommandListener(ZRiotWeps_BuyListener, "autobuy");
        AddCommandListener(ZRiotWeps_BuyListener, "rebuy");
    #endif
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:ZRiotWeps_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    #if defined PROJECT_GAME_CSS
        // Stop listening for buy commands.
        RemoveCommandListener(ZRiotWeps_BuyListener, "buy");
        RemoveCommandListener(ZRiotWeps_BuyListener, "autobuy");
        RemoveCommandListener(ZRiotWeps_BuyListener, "rebuy");
    #endif
}

/**
 * Client has been infected.
 * 
 * @param client        The infected client.
 * @param attacker      The zombie that infected the client.
 * @param motherzombie  True if the client has been infected as a mother zombie.
 */
public ZRiotWeps_OnClientInfected(client, attacker, bool:motherzombie)
{
    #if defined PROJECT_GAME_CSS
        new bool:weapons[WEPLIB_SLOT_COUNT] = {true, ...};
        weapons[_:Slot_Melee] = false;
        if (GetConVarBool(g_hCvarZRiotWeaponsDrop))
        {
            WepLib_DropAllWeapons(client, weapons);
        }
        else
        {
            WepLib_RemoveAllWeapons(client, weapons);
        }
    #endif
}

/**
 * Round has ended.
 * Note: This needs to be BEFORE the event that sets all clients back to human. 
 * 
 * @param winner    The index of the winning team.
 */
public ZRiotWeps_RoundEnd(winner)
{
    if (!GetConVarBool(g_hCvarZRiotWeaponsEndEquip))
        return;
    
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
            continue;
        
        if (TeamMgr_IsClientZombie(client))
            g_bZRiotWeaponsBlockWeapon[client] = true;
    }
}

/**
 * Client has spawned.
 * 
 * @param client    The client index.
 * 
 */
public ZRiotWeps_PlayerSpawn(client)
{
    if (!Util_IsClientOnTeam(client))
        return;
    
    // Reset block weapons flag.
    g_bZRiotWeaponsBlockWeapon[client] = false;
}

/**
 * Called when a client is trying to pick up a weapon.
 * 
 * @param client    The client index.
 * @param weapon    The weapon index.
 * 
 * @return          Hook action.  See include/core.inc.
 */
public Action:ZRiotWeps_CanUse(client, weapon)
{
    #if defined PROJECT_GAME_CSS
    
    new String:weaponentity[64];
    GetEdictClassname(weapon, weaponentity, sizeof(weaponentity));
    
    // Knife should never be restricted.
    // TODO: This should probably be specified in a config file, with individual
    //       settings for humans and zombies.
    if (StrEqual(weaponentity, "weapon_knife"))
        return Plugin_Continue;
    
    #endif
    
    // The flat override for stopping a client from picking up a weapon.
    if (g_bZRiotWeaponsBlockWeapon[client])
        return Plugin_Handled;
    
    // Only humans can pick up weapons.
    if (!TeamMgr_IsClientHuman(client))
        return Plugin_Handled;
    
    return Plugin_Continue;
}

/**
 * Command listener for the buy, autorebuy, and rebuy commands.
 * Used to block use of this command under certain conditions.
 * 
 * @param client    The client index.
 * @param argc      Argument count.
 */
public Action:ZRiotWeps_BuyListener(client, const String:command[], argc)
{
    // Not a client using the command.
    if (client <= 0)
        return Plugin_Continue;
    
    if (!IsClientInGame(client))
        return Plugin_Continue;
    
    // Only humans can buy weapons.
    if (!TeamMgr_IsClientHuman(client))
    {
        TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Chat, INVALID_MODULE, false, "ZRiot weapons zombie cant use weapon");
        return Plugin_Handled;
    }
    
    return Plugin_Continue;
}
