/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:           game.interface.inc
 *  Type:           Interface
 *  Description:    This interface provides access to abstract mod events and
 *                  functions that are not available in the base Source engine.
 *                  For instance functions like RespawnPlayer or TerminateRound.
 *
 *                  This interface is paired with the zr_core module which is
 *                  responsible for loading the appropriate mod adatper module
 *                  that will implement this interface.
 *
 *                  Events or functions may be faked on mods that doesn't
 *                  natively support it. The gameadapter will create these
 *                  common events, but a mod specific adapter module will be
 *                  responsible for firing these events.
 *
 *  Copyright (C) 2009-2013  Greyscale, Richard Helgeby
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

#if defined _igame_included
 #endinput
#endif
#define _igame_included

/*____________________________________________________________________________*/

/**
 * Supported games.
 */
enum Game
{
    Game_Unknown = -1,
    Game_CSS,
    Game_CSGO,
    Game_TF2
}

/*____________________________________________________________________________*/

/**
 * Virtual teams used internally in ZR.
 */
enum VTeam
{
    VTeam_Invalid = -1,     /** Invalid team. */
    VTeam_Unassigned = 0,   /** Client isn't on a team. */
    VTeam_Spectator,        /** Client is a spectator. */
    VTeam_Zombie,           /** Client is a zombie. */
    VTeam_Human,            /** Client is a human. */
    
    /**
     * Client is a a special spectator that can walk on the map as a regular
     * player, but doesn't affect the game (invisible, no-block, no weapons).
     */
    VTeam_Ghost
}

/*____________________________________________________________________________*/

/**
 * Called when round has started.
 */
// public OnRoundStart()
new ProjectEvent:g_EvRoundStart;

/*____________________________________________________________________________*/

/**
 * Called when pre-round freeze time has finished.
 */
// public OnRoundFreezeEnd()
new ProjectEvent:g_EvRoundFreezeEnd;

/*____________________________________________________________________________*/

/**
 * Called when round has ended.
 *
 * @param winner        Winner team.
 */
// public OnRoundEnd(VTeam:winner)
new ProjectEvent:g_EvRoundEnd;

/*____________________________________________________________________________*/

/**
 * Called when player is about to join a team.
 *
 * @param client        Client index.
 * @param team          Team client is about to join.
 * @param oldTeam       Old team before changing team.
 * @param disconnect    Team change because player disconnects.
 *
 * @return              Plugin_Handled to block broadcast.
 *                      Plugin_Continue to allow broadcast.
 */
// public Action:OnPlayerTeamPre(client, VTeam:team, VTeam:oldTeam, bool:disconnect)
new ProjectEvent:g_EvPlayerTeamPre;

/*____________________________________________________________________________*/

/**
 * Called when player has joined a team.
 *
 * @param client        Client index.
 * @param team          Team client joined.
 * @param oldTeam       Old team before changing team.
 * @param disconnect    Team change because player disconnects.
 */
// public OnPlayerTeam(client, VTeam:team, VTeam:oldTeam, bool:disconnect)
new ProjectEvent:g_EvPlayerTeam;

/*____________________________________________________________________________*/

/**
 * Player has spawned.
 *
 * Note: This event is NOT called when client is spawning in the game (the first
 *       time they see the map, before selecting a team).
 *
 * @param client        Client index.
 */
// public OnPlayerSpawn(client)
new ProjectEvent:g_EvPlayerSpawn;

/*____________________________________________________________________________*/

/**
 * Player has spawned. Delayed post event. Delay defined in
 * PROJECT_SPAWN_POST_DELAY in project_events.inc.
 *
 * Note: This event is NOT called when client is spawning in the game (the first
 *       time they see the map, before selecting a team).
 *
 * @param client        Client index.
 */
// public OnPlayerSpawnPost(client)
new ProjectEvent:g_EvPlayerSpawnPost;

/*____________________________________________________________________________*/

/**
 * Player has been damaged.
 *
 * @param victim        Victim client index.
 * @param attacker      Attacker client index. It may be the world.
 * @param health        Remaining health of victim.
 * @param armor         Remaining armor of victim.
 * @param weapon        Weapon class name used to hurt the victim. No "weapon_"
 *                      prefix.
 * @param dmgHealth     Amount of health the victim lost.
 * @param dmgArmor      Amount of armor the victim lost.
 * @param hitgroup      Index of hit group on the victim that was damaged.
 */
// public OnPlayerHurt(victim, attacker, health, armor, const String:weapon[], dmgHealth, dmgArmor, hitgroup)
new ProjectEvent:g_EvPlayerHurt;

/*____________________________________________________________________________*/

/**
 * Player died.
 *
 * @param victim        Client who died.
 * @param attacker      Attacker client index. It may be the world.
 * @param weapon        Weapon class name used to kill the victim. No "weapon_"
 *                      prefix.
 * @param headshot      Whether the victim died of a headshot.
 */
// public OnPlayerDeath(victim, attacker, const String:weapon[], bool:headshot)
new ProjectEvent:g_EvPlayerDeath;

/**
 * Player jumped.
 *
 * @param client        Client index.
 */
// public OnPlayerJump(client)
new ProjectEvent:g_EvPlayerJump;

/*____________________________________________________________________________*/

/**
 * Player has fired a weapon.
 *
 * @param client        Client who fired weapon.
 * @param weapon        Weapon class name. No "weapon_" prefix.
 */
// public Action:OnPlayerWeaponFire(client, const String:weapon[])
new ProjectEvent:g_EvWeaponFire;

//new ProjectEvent:g_EvWeaponEntityFire;

/*____________________________________________________________________________*/

/*
TODO - Remaining mod specific stuff that must be abstracted:

Cvars. Mod adapter modules should probably handle these. If other modules need
to read values we can provide get-functions in this interface:
* mp_friendlyfire
* mp_restartgame
* mp_autoteambalance
* mp_limitteams
* mp_roundtime

Others:
* team index (solved by teamlib, but we need to move ZR stuff to another place)
* grenades (napalm feature) and knife
* removal of objectives (hostages, rescue zone, and other mod specific objective entities)
* buy command
* weapon slots
* damage flags
* money
* night vision
*/

/**
 * Interface function ID cache.
 */
new Function:g_IGameStringToTeamIndex = INVALID_FUNCTION;
new Function:g_IGameTeamIndexToString = INVALID_FUNCTION;
new Function:g_IGameIsPlayerTeamIndex = INVALID_FUNCTION;
new Function:g_IGameGetDefaultIndexFor = INVALID_FUNCTION;
new Function:g_IGameGetOpposingPlayerTeam = INVALID_FUNCTION;

new Function:g_IGameRespawnPlayer = INVALID_FUNCTION;
new Function:g_IGameSwitchTeam = INVALID_FUNCTION;
new Function:g_IGameTerminateRound = INVALID_FUNCTION;
new Function:g_IGameDropWeapon = INVALID_FUNCTION;
new Function:g_IGameFlashlightIsOn = INVALID_FUNCTION;
new Function:g_IGameFlashlightOn = INVALID_FUNCTION;
new Function:g_IGameFlashlightOff = INVALID_FUNCTION;
new Function:g_IGameWeaponIsZombieClaws = INVALID_FUNCTION;

/*____________________________________________________________________________*/

/**
 * Converts a mod specific team name to a mod specific team index.
 *
 * @param teamName      Mod specific team name to convert.
 *
 * @return              Mod specific team index, or -1 if invalid.
 */
stock IGame_StringToTeamIndex(const String:teamName[])
{
    Interface_ValidateOrFail(g_IGameStringToTeamIndex);
    
    new result;
    
    Call_StartFunction(GetMyHandle(), g_IGameStringToTeamIndex);
    Call_PushString(teamName);
    if (Call_Finish(result) == SP_ERROR_NONE)
    {
        return result;
    }
    
    return -1;
}

/*____________________________________________________________________________*/

/**
 * Converts a mod specific team index to a mod specific team name.
 *
 * @param team          Team index to convert.
 * @param buffer        (Output) Buffer to store mod specific team name in.
 * @param maxlen        Size of buffer.
 *
 * @return              Number of cells written.
 */
stock IGame_TeamIndexToString(teamIndex, String:buffer[], maxlen)
{
    Interface_ValidateOrFail(g_IGameTeamIndexToString);
    
    new result;
    
    Call_StartFunction(GetMyHandle(), g_IGameTeamIndexToString);
    Call_PushCell(teamIndex);
    Call_PushStringEx(buffer, maxlen, SM_PARAM_STRING_UTF8 | SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
    Call_PushCell(maxlen);
    Call_Finish(result);
    
    return result;
}

/*____________________________________________________________________________*/

/**
 * Converts a virtual team name string to a virtual team.
 *
 * @param teamName      Virtual team name to convert.
 *
 * @return              Virtual team, or VTeam_Invalid if invalid.
 */
stock VTeam:IGame_VTeamStringToTeam(const String:teamName[])
{
    if (StrEqual(teamName, "zombie", false))
    {
        return VTeam_Zombie;
    }
    else if (StrEqual(teamName, "human", false))
    {
        return VTeam_Human;
    }
    else if (StrEqual(teamName, "ghost", false))
    {
        return VTeam_Ghost;
    }
    
    return VTeam_Invalid;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the specified team is a team for players.
 *
 * @param team          Team to check.
 *
 * @return              True if team is zombie or human, false otherwise.
 */
stock bool:IGame_IsPlayerTeam(VTeam:team)
{
    if (team == VTeam_Zombie || team == VTeam_Human)
    {
        return true;
    }
    
    return false;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the mod specific team index is a player team (not spectator
 * or unassigned).
 *
 * @param team          Player team index check.
 *
 * @return              True if it's a player team, false otherwise.
 */
stock bool:IGame_IsPlayerTeamIndex(teamIndex)
{
    Interface_ValidateOrFail(g_IGameIsPlayerTeamIndex);
    
    new bool:result;
    
    Call_StartFunction(GetMyHandle(), g_IGameIsPlayerTeamIndex);
    Call_PushCell(teamIndex);
    Call_Finish(result);
    
    return result;
}

/*____________________________________________________________________________*/

/**
 * Gets default mod specific team index for the virtual team.
 *
 * @param team          Virtual team to get default index for.
 *
 * @return              Team index, or -1 on error.
 */
stock IGame_GetDefaultIndexFor(VTeam:team)
{
    Interface_ValidateOrFail(g_IGameGetDefaultIndexFor);
    
    new result;
    
    Call_StartFunction(GetMyHandle(), g_IGameGetDefaultIndexFor);
    Call_PushCell(team);
    if (Call_Finish(result) == SP_ERROR_NONE)
    {
        return result;
    }
    
    return -1;
}

/*____________________________________________________________________________*/

/**
 * Gets the opposing player team index.
 *
 * @param team          Player team index to invert.
 *
 * @return              Opposing team index, -1 on error.
 */
stock IGame_GetOpposingPlayerTeam(teamIndex)
{
    Interface_ValidateOrFail(g_IGameGetOpposingPlayerTeam);
    
    new result;
    
    Call_StartFunction(GetMyHandle(), g_IGameGetOpposingPlayerTeam);
    Call_PushCell(teamIndex);
    if (Call_Finish(result) == SP_ERROR_NONE)
    {
        return result;
    }
    
    return -1;
}

/*____________________________________________________________________________*/

/**
 * Respawns a client.
 *
 * @param client        Player's index.
 */
stock IGame_RespawnPlayer(client)
{
    Interface_ValidateOrFail(g_IGameRespawnPlayer);
    
    Call_StartFunction(GetMyHandle(), g_IGameRespawnPlayer);
    Call_PushCell(client);
    Call_Finish();
}

/*____________________________________________________________________________*/

/**
 * Switches the player's team.
 *
 * @param client        Player's index.
 * @param team          Team index.
 */
stock IGame_SwitchTeam(client, VTeam:team)
{
    Interface_ValidateOrFail(g_IGameSwitchTeam);
    
    Call_StartFunction(GetMyHandle(), g_IGameSwitchTeam);
    Call_PushCell(client);
    Call_PushCell(team);
    Call_Finish();
}

/*____________________________________________________________________________*/

/**
 * Forces round to end.
 *
 * @param winner        The winning team (virtual team). Use VTeam_Invalid to
 *                      terminate the round without a winner (round draw, etc).
 * @param               (Optional) Delay in seconds before terminating round.
 *                      Default is 3 seconds.
 */
stock IGame_TerminateRound(VTeam:winner, Float:delay = 3.0)
{
    Interface_ValidateOrFail(g_IGameTerminateRound);
    
    Call_StartFunction(GetMyHandle(), g_IGameTerminateRound);
    Call_PushCell(winner);
    Call_PushCell(delay);
    Call_Finish();
}

/*____________________________________________________________________________*/

/**
 * Forces player to drop or toss their weapon.
 *
 * @param client        Player's index.
 * @param weaponIndex   Index of weapon to drop.
 * @param toos          True to toss weapon (with velocity), or false to just
 *                      drop weapon.
 */
stock IGame_DropWeapon(client, weaponIndex, bool:toss = false)
{
    Interface_ValidateOrFail(g_IGameDropWeapon);
    
    Call_StartFunction(GetMyHandle(), g_IGameDropWeapon);
    Call_PushCell(client);
    Call_PushCell(weaponIndex);
    Call_PushCell(toss);
    Call_Finish();
}

/*____________________________________________________________________________*/

/**
 * Returns whether the player's flashlight is turned on.
 *
 * Note: This is an optional function. There's no guarantee that it's
 *       implemented in all game mods.
 *
 * @param client        Player's index.
 *
 * @return              True if on, false otherwise.
 */
stock bool:IGame_FlashlightIsOn(client)
{
    Interface_ValidateOrFail(g_IGameFlashlightIsOn);
    
    new bool:result = false;
    
    Call_StartFunction(GetMyHandle(), g_IGameFlashlightIsOn);
    Call_PushCell(client);
    Call_Finish(result);
    
    return result;
}

/*____________________________________________________________________________*/

/**
 * Turns player's flashlight on.
 *
 * Note: This is an optional function. There's no guarantee that it's
 *       implemented in all game mods.
 *
 * @param client        Player's index.
 */
stock IGame_FlashlightOn(client)
{
    Interface_ValidateOrFail(g_IGameFlashlightOn);
    
    Call_StartFunction(GetMyHandle(), g_IGameFlashlightOn);
    Call_PushCell(client);
    Call_Finish();
}

/*____________________________________________________________________________*/

/**
 * Turns player's flashlight off.
 *
 * Note: This is an optional function. There's no guarantee that it's
 *       implemented in all game mods.
 *
 * @param client        Player's index.
 */
stock IGame_FlashlightOff(client)
{
    Interface_ValidateOrFail(g_IGameFlashlightOff);
    
    Call_StartFunction(GetMyHandle(), g_IGameFlashlightOff);
    Call_PushCell(client);
    Call_Finish();
}

/*____________________________________________________________________________*/

/**
 * Returns whether the specified weapon class is used as zombie claws.
 *
 * @param weaponClass       Weapon class name (without "weapon_" prefix).
 */
stock bool:IGame_WeaponIsZombieClaws(const String:weaponClass[])
{
    Interface_ValidateOrFail(g_IGameWeaponIsZombieClaws);
    
    new bool:result;
    
    Call_StartFunction(GetMyHandle(), g_IGameWeaponIsZombieClaws);
    Call_PushString(weaponClass);
    Call_Finish(result);
    
    return result;
}
