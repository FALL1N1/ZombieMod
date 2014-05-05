/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          weaponlib.inc
 *  Type:          Library
 *  Description:   Weapon-related API.
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

#if defined _weaponlib_included
 #endinput
#endif
#define _weaponlib_included

// This entire file is CS:S-specific.
#if defined PROJECT_GAME_CSS

#include <sdktools>

// Include libraries
#include "zr/libraries/utilities"

/**
 * The number of slots for weapons to be stored in.  (CS:S)
 */
#define WEPLIB_SLOT_COUNT 5

/**
 * Weapon slots. (CS:S)
 */
enum WepLib_Slots
{
    Slot_Invalid        = -1,   /** Invalid weapon (slot). */
    Slot_Primary        = 0,    /** Primary weapon slot. */
    Slot_Secondary      = 1,    /** Secondary weapon slot. */
    Slot_Melee          = 2,    /** Melee (knife) weapon slot. */
    Slot_Projectile     = 3,    /** Projectile (grenades, flashbangs, etc) weapon slot. */
    Slot_Extras         = 4,    /** Explosive (c4) weapon slot, NVGs. */
}

/**
 * Defines for grenade counts for use in WepLib_Set/GetAmmoEx. (CS:S)
 */
#define AMMO_INDEX_HE 11
#define AMMO_INDEX_FLASH 12
#define AMMO_INDEX_SMOKE 13

/**
 * Checks if a client has a specific weapon.
 * 
 * @param client    The client index.
 * @param weapon    The weapon classname.
 * 
 * @return          True if the client has the weapon, false if not.
 */
stock bool:WepLib_HasWeapon(client, const String:weapon[])
{
    // x = slot index
    new weaponindex;
    decl String:classname[64];
    for (new weaponslot = 0; weaponslot < WEPLIB_SLOT_COUNT; weaponslot++)
    {
        weaponindex = GetPlayerWeaponSlot(client, weaponslot);
        
        // If slot is empty, then stop.
        if (weaponindex == -1)
            continue;
        
        // If the weapon's classname matches, then return true.
        GetEdictClassname(weaponindex, classname, sizeof(classname));
        if (StrEqual(weapon, classname, false))
            return true;
    }
    
    return false;
}

/**
 * Return an array that contains all client's weapon indexes.
 * Note: This only returns the first grenade in the 4th slot.
 * 
 * @param client    The client index.
 * @param weapons   The weapon index array.  -1 if no weapon in slot.
 */
stock WepLib_GetWeapons(client, weapons[WepLib_Slots])
{
    for (new weaponslot = 0; weaponslot < WEPLIB_SLOT_COUNT; weaponslot++)
    {
        weapons[WepLib_Slots:weaponslot] = GetPlayerWeaponSlot(client, weaponslot);
    }
}

/**
 * Returns weapon index of the client's deployed weapon.
 * 
 * @param client    The client index.
 *  
 * @return          The weapon index of the deployed weapon.  -1 if no weapon is deployed.
 */
stock WepLib_GetDeployedWeaponIndex(client)
{
    // Return the client's active weapon.
    return GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
}

/**
 * Returns slot of client's deployed weapon.
 * 
 * @param client    The client index.
 *  
 * @return          The slot number of deployed weapon.
 */
stock WepLib_Slots:Weplib_GetDeployedWeaponSlot(client)
{
    // Get client's deployed weapon.
    new deployedweapon = WepLib_GetDeployedWeaponIndex(client);
    
    // If client has no deployed weapon, then stop.
    if (deployedweapon == -1)
        return Slot_Invalid;
    
    for (new weaponslot = 0; weaponslot < WEPLIB_SLOT_COUNT; weaponslot++)
    {
        // If the weapon indexes match, then return it.
        if (weapons[weaponslot] == deployedweapon)
            return WepLib_Slots:weaponslot;
    }
    
    return Slot_Invalid;
}

/**
 * Refresh a weapon by taking it and giving it back.
 * Sometimes changing models on a client screws up the weapon attachments/angles. 
 * 
 * @param client        The client index.
 * @param slot          The weapon slot to refresh. (see enum WepLib_Slots)
 */
stock WepLib_RefreshWeapon(client, WepLib_Slots:slot)
{
    new weaponindex = GetPlayerWeaponSlot(client, _:slot);
    if (weaponindex == -1)
        return;
    
    // Get the classname of the weapon to re-give.
    decl String:entityname[64];
    GetEdictClassname(weaponindex, entityname, sizeof(entityname));
    
    // Refresh weapon.
    RemovePlayerItem(client, weaponindex);
    GivePlayerItem(client, entityname);
}

/**
 * Refresh all weapons by taking them and giving them back.
 * 
 * @param client        The client index.
 * @param weapons       An array of boolean values for each weapon slot.  True means remove, false means ignore. 
 */
stock WepLib_RefreshAllWeapons(client, bool:weapons[WEPLIB_SLOT_COUNT] = {true, ...})
{
    new weaponindex;
    for (new weaponslot = 0; weaponslot < WEPLIB_SLOT_COUNT; weaponslot++)
    {
        // Check if caller wants to ignore this slot.
        if (!weapons[weaponslot])
            continue;
        
        weaponindex = GetPlayerWeaponSlot(client, weaponslot);
        if (weaponindex == -1)
            continue;
        
        WepLib_RefreshWeapon(client, WepLib_Slots:weaponindex);
    }
}

/**
 * Remove all weapons.
 * 
 * @param client        The client index.
 * @param weapons       An array of boolean values for each weapon slot.  True means remove, false means ignore.
 *  
 */
stock WepLib_RemoveAllWeapons(client, bool:weapons[WEPLIB_SLOT_COUNT] = {true, ...})
{
    new weaponindex;
    for (new weaponslot = 0; weaponslot < WEPLIB_SLOT_COUNT; weaponslot++)
    {
        // Check if caller wants to ignore this slot.
        if (!weapons[weaponslot])
            continue;
        
        weaponindex = GetPlayerWeaponSlot(client, weaponslot);
        if (weaponindex != -1)
        {
            Util_RemovePlayerItem(client, weaponindex);
        }
    }
    
    // Remove left-over projectiles.
    WepLib_GrenadeStripAll(client);
}

/**
 * Force a client to drop all weapons.
 * 
 * @param client        The client index.
 * @param weapons       An array of boolean values for each weapon slot.  True means remove, false means ignore.
 */
stock WepLib_DropAllWeapons(client, bool:weapons[WEPLIB_SLOT_COUNT] = {true, ...})
{
    new weaponindex;
    for (new weaponslot = 0; weaponslot < WEPLIB_SLOT_COUNT; weaponslot++)
    {
        // Check if caller wants to ignore this slot.
        if (!weapons[weaponslot])
            continue;
        
        weaponindex = GetPlayerWeaponSlot(client, weaponslot);
        if (weaponindex != -1)
        {
            CS_DropWeapon(client, weaponindex, true);
        }
    }
    
    // Drop left-over projectiles.
    WepLib_GrenadeDropAll(client);
}

/**
 * Checks if a client is in a buyzone.
 * Game: CS:S
 * 
 * @param client    The client index.
 */
stock bool:WepLib_InBuyZone(client)
{
    new iBuyZoneOffset = FindSendPropInfo("CCSPlayer", "m_bInBuyZone");
    return bool:GetEntData(client, iBuyZoneOffset);
}

// **********************************************
//            Weapon Ammo Stocks
// **********************************************

/**
 * Get clip/reserve ammo on a weapon.
 * 
 * @param weapon    The weapon index.
 * @param clip      True gets clip ammo, false gets reserve.
 */
stock WepLib_GetAmmo(weapon, bool:clip)
{
    // Initialize a variable with the offset name for the given clip.
    new String:offset[16];
    if (clip)
        offset = "m_iClip1";
    else
        offset = "m_iClip2";
    
    // Return ammo value.
    return GetEntProp(weapon, Prop_Data, offset);
}

/**
 * Get the amount of ammo in a weapon given the client and an array index for the weapon.
 * 
 * @param client    The client index.
 * @param index     The array index of the game's m_iAmmo offset to get.
 */
stock WepLib_GetAmmoEx(client, index)
{
    new offsAmmo = FindSendPropInfo("CBasePlayer", "m_iAmmo");
    return GetEntData(client, offsAmmo + (index * 4));
}

/**
 * Returns an 32-element array that contains the ammo counts for each array index for m_iAmmo on a client.
 * 
 * @param client            The client index.
 * @param grenadecounts     The output array.
 * @param startindex        Only copies the data from this index on.
 * @param endindex          Stops copying data once this index is reached.
 */
stock WepLib_GetAmmoArray(client, weapons[32], startindex = 0, endindex = 31)
{
    new offsAmmo = FindSendPropInfo("CBasePlayer", "m_iAmmo");
    for (new ammoindex = startindex; ammoindex < endindex + 1; ammoindex++)
        weapons[ammoindex] = GetEntData(client, offsAmmo + (ammoindex * 4));
}

/**
 * Set clip/reserve ammo on a weapon.
 * 
 * @param weapon    The weapon index.
 * @param clip      True sets clip ammo, false sets reserve.
 * @param value     The amount of ammo to set to.
 * @param add       True to add to current value, false to overwrite.
 */
stock WepLib_SetAmmo(weapon, bool:clip, value, bool:add = false)
{
    // Initialize a variable with the offset name for the given clip.
    new String:offset[16];
    if (clip)
        offset = "m_iClip1";
    else
        offset = "m_iClip2";
    
    // Handle adding or setting.
    new ammovalue = 0;
    if (add)
        ammovalue = WepLib_GetAmmo(weapon, clip);
    
    // Set new ammo.
    SetEntProp(weapon, Prop_Data, offset, ammovalue + value);
}

/**
 * Set the amount of ammo in a weapon given the client and an array index for the weapon.
 * 
 * @param client    The client index.
 * @param index     The array index of the game's m_iAmmo offset to set.
 * @param value     The amount of ammo to set to.
 * @param add       True to add to current value, false to overwrite.
 */
stock WepLib_SetAmmoEx(client, index, value, bool:add = false)
{
    new offsAmmo = FindSendPropInfo("CBasePlayer", "m_iAmmo");
    
    // Handle adding or setting.
    new ammovalue = 0;
    if (add)
        ammovalue = WeaponAmmoGetGrenadeCount(client, type);
    
    SetEntData(client, offsAmmo + (index * 4), ammovalue + value, 4, true);
}

// **********************************************
//              Grenade Stocks
// **********************************************

/**
 * Used to explicitly remove projectiles from a client.
 * 
 * @param client    The client index.
 */
stock WepLib_GrenadeStripAll(client)
{
    // While GetPlayerWeaponSlot returns a valid projectile, remove it and then test again.
    new grenade = GetPlayerWeaponSlot(client, _:Slot_Projectile);
    while (grenade != -1)
    {
        Util_RemovePlayerItem(client, grenade);
        grenade = GetPlayerWeaponSlot(client, _:Slot_Projectile);
    }
}

/**
 * Used to explicitly force a client to drop all projectiles.
 * 
 * @param client    The client index.
 */
stock WepLib_GrenadeDropAll(client)
{
    // While GetPlayerWeaponSlot returns a valid projectile, remove it and then test again.
    new grenade = GetPlayerWeaponSlot(client, _:Slot_Projectile);
    while (grenade != -1)
    {
        CS_DropWeapon(client, grenade, true);
        grenade = GetPlayerWeaponSlot(client, _:Slot_Projectile);
    }
}

/**
 * Take a grenade entity and return the index used in WepLib_SetAmmoEx
 * 
 * @param grenadeent    The entity name of the grenade.
 * 
 * @return              The ammo index associated with the grenade.  -1 if invalid grenade entity.
 */
stock WepLib_GrenadeEntToIndex(const String:grenadeent[])
{
    if (StrEqual(grenadeent, "weapon_hegrenade"))
        return AMMO_INDEX_HE;
    
    else if (StrEqual(grenadeent, "weapon_flashbang"))
        return AMMO_INDEX_FLASH;
    
    else if (StrEqual(grenadeent, "weapon_smokegrenade"))
        return AMMO_INDEX_SMOKE;
    
    return -1;
}

/**
 * Take the index used in WepLib_SetAmmoEx and return a grenade entity.
 * 
 * @param index         The ammo index associated with the grenade.
 * @param grenadeent    The entity name of the grenade.  "" if not found.
 * @param maxlen        The max length of the output string. 
 */
stock WepLib_IndexToGrenadeEnt(index, String:grenadeent[], maxlen)
{
    switch(index)
    {
        case AMMO_INDEX_HE:     strcopy(grenadeent, maxlen, "weapon_hegrenade");
        case AMMO_INDEX_FLASH:  strcopy(grenadeent, maxlen, "weapon_flashbang");
        case AMMO_INDEX_SMOKE:  strcopy(grenadeent, maxlen, "weapon_smokegrenade");
        default:                strcopy(grenadeent, maxlen, "");
    }
}

#endif
