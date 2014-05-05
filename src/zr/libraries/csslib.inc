/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:           csslib.inc
 *  Type:           Library
 *  Description:    Utilities for Counter-Strike: Source.
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

#if defined _csslib_included
 #endinput
#endif
#define _csslib_included

#include <sdktools>

new Handle:CSSLib_GameConfig = INVALID_HANDLE;
new Handle:CSSLib_CallFlashlightIsOn = INVALID_HANDLE;
new Handle:CSSLib_CallFlashlightTurnOn = INVALID_HANDLE;
new Handle:CSSLib_CallFlashlightTurnOff = INVALID_HANDLE;
new bool:CSSLib_SDKCallsPrepared = false;

/*____________________________________________________________________________*/

/**
 * Prepares SDK calls.
 */
stock CSSLib_SetupSDKCalls()
{
    if (CSSLib_GameConfig == INVALID_HANDLE)
    {
        // Load game config file.
        CSSLib_GameConfig = LoadGameConfigFile("csslib");
        
        // Check if failed.
        if (CSSLib_GameConfig == INVALID_HANDLE)
        {
            SetFailState("Failed to load csslib game config file from the \"gamedata\" directory.");
            return;
        }
    }
    
    // Prepare SDK calls.
    
    if (CSSLib_CallFlashlightIsOn == INVALID_HANDLE)
    {
        StartPrepSDKCall(SDKCall_Player);
        PrepSDKCall_SetFromConf(CSSLib_GameConfig, SDKConf_Virtual, "FlashlightIsOn");
        PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
        CSSLib_CallFlashlightIsOn = EndPrepSDKCall();
        
        CSSLib_ValidateCallOrFail(CSSLib_CallFlashlightIsOn, "CCSPlayer", "FlashlightIsOn");
    }
    if (CSSLib_CallFlashlightTurnOn == INVALID_HANDLE)
    {
        StartPrepSDKCall(SDKCall_Player);
        PrepSDKCall_SetFromConf(CSSLib_GameConfig, SDKConf_Virtual, "FlashlightTurnOn");
        CSSLib_CallFlashlightTurnOn = EndPrepSDKCall();
        
        CSSLib_ValidateCallOrFail(CSSLib_CallFlashlightTurnOn, "CCSPlayer", "FlashlightTurnOn");
    }
    if (CSSLib_CallFlashlightTurnOff == INVALID_HANDLE)
    {
        StartPrepSDKCall(SDKCall_Player);
        PrepSDKCall_SetFromConf(CSSLib_GameConfig, SDKConf_Virtual, "FlashlightTurnOff");
        CSSLib_CallFlashlightTurnOff = EndPrepSDKCall();
        
        CSSLib_ValidateCallOrFail(CSSLib_CallFlashlightTurnOff, "CCSPlayer", "FlashlightTurnOff");
    }
    
    CSSLib_SDKCallsPrepared = true;
}

/*____________________________________________________________________________*/

/**
 * Converts a CS:S specific team name to a CS:S team index.
 *
 * @param teamName      CS:S team name to convert.
 *
 * @return              CS:S team index, or -1 if invalid.
 */
stock CSSLib_StringToTeam(const String:teamName[])
{
    if (StrEqual(teamName, "t", false)
        || StrEqual(teamName, "terrorists", false))
    {
        return CS_TEAM_T;
    }
    else if (StrEqual(teamName, "ct", false)
        || StrEqual(teamName, "counter-terrorists", false))
    {
        return CS_TEAM_CT;
    }
    else if (StrEqual(teamName, "spec", false)
        || StrEqual(teamName, "spectators", false))
    {
        return CS_TEAM_SPECTATOR;
    }
    
    return -1;
}

/*____________________________________________________________________________*/

/**
 * Converts a CS:S team index to a CS:S team name.
 *
 * @param team          Team index to convert.
 * @param buffer        (Output) Buffer to store CS:S team name in.
 * @param maxlen        Size of buffer.
 *
 * @return              Number of cells written.
 */
CSSLib_TeamToString(teamIndex, String:buffer[], maxlen)
{
    switch (teamIndex)
    {
        case CS_TEAM_NONE:
        {
            return strcopy(buffer, maxlen, "Unassigned");
        }
        case CS_TEAM_SPECTATOR:
        {
            return strcopy(buffer, maxlen, "Spectators");
        }
        case CS_TEAM_T:
        {
            return strcopy(buffer, maxlen, "Terrorists");
        }
        case CS_TEAM_CT:
        {
            return strcopy(buffer, maxlen, "Counter-Terrorists");
        }
    }
    
    return 0;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the CS:S team index is a player team (not spectator or
 * unassigned).
 *
 * @param team          Player team index check.
 *
 * @return              True if it's a player team, false otherwise.
 */
stock bool:CSSLib_IsPlayerTeam(teamIndex)
{
    switch (teamIndex)
    {
        case CS_TEAM_NONE:
        {
            return false;
        }
        case CS_TEAM_SPECTATOR:
        {
            return false;
        }
        case CS_TEAM_T:
        {
            return true;
        }
        case CS_TEAM_CT:
        {
            return true;
        }
    }
    
    return false;
}

/*____________________________________________________________________________*/

/**
 * Gets the opposing player team index. Converts T to CT and CT to T.
 *
 * @param team          Player team index to invert.
 *
 * @return              Opposing team index, -1 on error.
 */
stock CSSLib_GetOpposingPlayerTeam(teamIndex)
{
    if (teamIndex == CS_TEAM_T)
    {
        return CS_TEAM_CT;
    }
    else if (teamIndex == CS_TEAM_CT)
    {
        return CS_TEAM_T;
    }
    
    return -1;
}

/*____________________________________________________________________________*/

/**
 * Takes a winning team index and returns its corresponding round end reason.
 * Ex: Takes index '2' and returns the Terrorists_Win round end reason.
 * 
 * @param teamIndex     The team index that won the round.
 * 
 * @return              The round end reason. CSRoundEnd_GameStart if invalid
 *                      team index.
 */
stock CSRoundEndReason:CSSLib_TeamToReason(teamIndex)
{
    if (teamIndex == CS_TEAM_T)
    {
        return CSRoundEnd_TerroristWin;
    }
    else if (teamIndex == CS_TEAM_CT)
    {
        return CSRoundEnd_CTWin;
    }
    
    return CSRoundEnd_GameStart;
}

/*____________________________________________________________________________*/

/**
 * Returns whether the player's flashlight is turned on.
 *
 * @param client        Player's index.
 *
 * @return              True if on, false otherwise.
 */
stock bool:CSSLib_FlashlightIsOn(client)
{
    CSSLib_CheckPreparedOrFail();
    
    return bool:SDKCall(CSSLib_CallFlashlightIsOn, client);
}

/*____________________________________________________________________________*/

/**
 * Turns player's flashlight on.
 *
 * @param client        Player's index.
 */
stock CSSLib_FlashlightOn(client)
{
    CSSLib_CheckPreparedOrFail();
    
    SDKCall(CSSLib_CallFlashlightTurnOn, client);
}

/*____________________________________________________________________________*/

/**
 * Turns player's flashlight off.
 *
 * @param client        Player's index.
 */
stock CSSLib_FlashlightOff(client)
{
    CSSLib_CheckPreparedOrFail();
    
    SDKCall(CSSLib_CallFlashlightTurnOff, client);
}

/*____________________________________________________________________________*/

/**
 * Validates a SDK call handle. Stops the plugin if not found.
 */
static stock CSSLib_ValidateCallOrFail(Handle:call, const String:className[], const String:callName[])
{
    if (call == INVALID_HANDLE)
    {
        SetFailState("Game function \"%s::%s\" was not found.", className, callName);
    }
}

/*____________________________________________________________________________*/

/**
 * Throws an error if SDK calls aren't prepared.
 */
static stock CSSLib_CheckPreparedOrFail()
{
    if (!CSSLib_SDKCallsPrepared)
    {
        ThrowError("CS:S SDK Calls not prepared.");
    }
}
