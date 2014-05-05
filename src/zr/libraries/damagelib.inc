/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          damagelib.inc
 *  Type:          Library
 *  Description:   Damage-related API.
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

#if defined _damagelib_included
 #endinput
#endif
#define _damagelib_included

/**
 * @section Counter Strike: Source specific damage flags.
 */
#if defined PROJECT_GAME_CSS
    #define UTILS_DMG_FALL        (DMG_FALL)      // Client was damaged by falling.
    #define UTILS_DMG_BLAST       (DMG_BLAST)     // Client was damaged by explosion.
    #define UTILS_DMG_BURN        (DMG_DIRECT)    // Client was damaged by fire.
    #define UTILS_DMG_BULLET      (DMG_NEVERGIB)  // Client was shot or knifed.
    #define UTILS_DMG_HEADSHOT    (1 << 30)       // Client was shot in the head.
#else
    // undefined.
    #define UTILS_DMG_FALL        0         // Client was damaged by falling.
    #define UTILS_DMG_BLAST       0         // Client was damaged by explosion.
    #define UTILS_DMG_BURN        0         // Client was damaged by fire.
    #define UTILS_DMG_BULLET      0         // Client was shot or knifed.
    #define UTILS_DMG_HEADSHOT    0         // Client was shot in the head.
#endif

/**
 * TODO: Get other games
 */

/**
 * Deal damage to a client.
 * Credits to PimpinJuice
 * 
 * @param victim    Client to do damage to.
 * @param damage    Amount of damage.
 * @param attacker  Who the damage is coming from.  -1 for no attacker.
 * @param dmgflags  The type of damage to deal.
 * @param weapon    Specific damage from a certain weapon
 * 
 * @noreturn
 */
stock DealDamage(victim, damage, attacker = -1, dmgflags = DMG_GENERIC, const String:weapon[] = "")
{
	new String:strDmg[8], String:strDmgFlags[16];
	IntToString(damage, strDmg, sizeof(strDmg));
	IntToString(dmgflags, strDmgFlags, sizeof(strDmgFlags));
	
	new hurtent = CreateEntityByName("point_hurt");
	if(hurtent)
	{
		DispatchKeyValue(victim, "targetname", "dmgtarget");
		DispatchKeyValue(hurtent, "DamageTarget", "dmgtarget");
		DispatchKeyValue(hurtent, "Damage", strDmg);
		DispatchKeyValue(hurtent, "DamageType", strDmgFlags);
		if(weapon[0])
		{
			DispatchKeyValue(hurtent, "classname", weapon);
		}
		DispatchSpawn(hurtent);
		
		AcceptEntityInput(hurtent, "Hurt", (attacker > 0) ? attacker : -1);
		DispatchKeyValue(victim, "targetname", "nodmg");
		
		// clean up
		RemoveEdict(hurtent);
	}
}
