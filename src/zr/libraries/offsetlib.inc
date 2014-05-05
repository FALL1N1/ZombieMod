/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          offsetlib.inc
 *  Type:          Library
 *  Description:   A library of offset wrappers.
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

#if defined _offsetlib_included
 #endinput
#endif
#define _offsetlib_included

/**
 * Gets an client's velocity.
 * 
 * @param client        The client index.
 * @param vecVelocity   The velocity of the client.
 */
stock OffsLib_GetClientVelocity(client, Float:vecVelocity[3])
{
    new offsVelocity = FindSendPropInfo("CBasePlayer", "m_vecVelocity[0]");
    GetEntDataVector(client, offsVelocity, vecVelocity);
}

/**
 * Set an entity's velocity.
 * 
 * @param entity        The entity index.
 * @param vecVelocity   Velocity to apply on the entity
 * @param stack         If modifying velocity, then true will stack new velocity onto the client's
 *                      current velocity, false will reset it.
 */
stock OffsLib_SetEntityVelocity(entity, const Float:vecVelocity[3], bool:stack)
{
    new Float:vecNewVelocity[3];
    new Float:vecCurVelocity[3];
    
    // If we are stacking velocity, then get the entity's current velocity.
    if (stack)
        OffsLib_GetEntityVelocity(entity, vecCurVelocity);
    
    // If we're not stacking then we're just copying the value of vecVelocity to vecNewVelocity.
    AddVectors(vecCurVelocity, vecVelocity, vecNewVelocity);
    
    // Apply velocity on client.
    TeleportEntity(entity, NULL_VECTOR, NULL_VECTOR, vecNewVelocity);
}

/**
 * Get a client's lagged movement value.
 *  
 * @param client    The client index.
 *  
 * @return          LMV value. (1.0 = default, 2.0 = double)
 */
stock Float:OffsLib_GetClientLMV(client)
{
    new offsLMV = FindSendPropInfo("CBasePlayer", "m_flLaggedMovementValue");
    return GetEntDataFloat(client, offsLMV);
}

/**
 * Set a client's lagged movement value.
 *  
 * @param client    The client index.
 * @param value     LMV value. (1.0 = normal, 2.0 = double)
 */
stock OffsLib_SetClientLMV(client, Float:fLMV)
{
    new offsLMV = FindSendPropInfo("CBasePlayer", "m_flLaggedMovementValue");
    SetEntDataFloat(client, offsLMV, fLMV);
}

/**
 * Get a client's default FOV.
 * 
 * @param client    The client index.
 * 
 * @return          The default field of vision of the client.
 */
stock OffsLib_GetClientDFOV(client)
{
    new offsDFOV = FindSendPropInfo("CBasePlayer", "m_iDefaultFOV");
    return GetEntData(client, offsDFOV);
}

/**
 * Set a client's default FOV.
 * 
 * @param client    The client index.
 * @param dfov      The client's new default FOV.
 */
stock OffsLib_SetClientDFOV(client, dfov)
{
    new offsDFOV = FindSendPropInfo("CBasePlayer", "m_iDefaultFOV");
    SetEntData(client, offsDFOV, dfov, 1);
}

/**
 * OffsLib_GetClientWaterLevel (m_nWaterLevel) returns.
 */
#define OFFS_WATERLEVEL_DRY 0
#define OFFS_WATERLEVEL_FEET 1
#define OFFS_WATERLEVEL_HALF 2
#define OFFS_WATERLEVEL_FULL 3

/**
 * Get a client's current water level flag
 * 
 * @param client    The client index.
 * 
 * @return          A water level.  See OFFS_WATERLEVEL_* defines.
 */
stock OffsLib_GetClientWaterLevel(client)
{
    return GetEntProp(client, Prop_Data, "m_nWaterLevel");
}

/**
 * Get a client's frag count.
 * 
 * @param client    The client index.
 * 
 * @return          The number of frags the client has.
 */
stock OffsLib_GetClientFrags(client)
{
    return GetEntProp(client, Prop_Data, "m_iFrags");
}

/**
 * Set a client's frag count.
 * 
 * @param client    The client index.
 * @param frags     The new number of frags to set.
 */
stock OffsLib_SetClientFrags(client, frags)
{
    SetEntProp(client, Prop_Data, "m_iFrags", frags);
}

/**
 * Get a client's death count.
 * 
 * @param client    The client index.
 * 
 * @return          The number of deaths the client has.
 */
stock OffsLib_GetClientDeaths(client)
{
    return GetEntProp(client, Prop_Data, "m_iDeaths");
}

/**
 * Set a client's death count.
 * 
 * @param client    The client index.
 * @param deaths    The new number of deaths to set.
 */
stock OffsLib_SetClientDeaths(client, deaths)
{
    SetEntProp(client, Prop_Data, "m_iDeaths", deaths);
}

/**
 * Gets an entity's color.
 * 
 * @param entity	The entity index.
 * @param color     Output. Entity colors.
 */
stock OffsLib_GetEntityColor(entity, color[4])
{
	static bool:gotconfig = false;
	static String:prop[32];
	
	if (!gotconfig)
	{
		new Handle:gc = LoadGameConfigFile("core.games");
		new bool:exists = GameConfGetKeyValue(gc, "m_clrRender", prop, sizeof(prop));
		CloseHandle(gc);
		
		if (!exists)
		{
			strcopy(prop, sizeof(prop), "m_clrRender");
		}
		
		gotconfig = true;
	}
	
	new offset = GetEntSendPropOffs(entity, prop);
	GetEntDataArray(entity, offset, color, sizeof(color), 1);
}

/**
 * Add a flag to an entity.
 * 
 * @param entity    The entity index.
 * @param flag      The flag to add. (See entity_prop_stocks.inc for flags)
 */
stock OffsLib_AddEntityFlag(entity, flag)
{
    new flags = GetEntityFlags(entity);
    
    // Add the flag if it's not already there.
    if (flag & ~flags)
        flags |= flag;
    
    SetEntProp(entity, Prop_Data, "m_fFlags", flags);
}

/**
 * Remove a flag from an entity.
 * 
 * @param entity    The entity index.
 * @param flag      The flag to remove. (See entity_prop_stocks.inc for flags)
 */
stock OffsLib_RemoveEntityFlag(entity, flag)
{
    new flags = GetEntityFlags(entity);
    
    // Remove the flag if it's there.
    if (flags & flag)
        flags &= ~flag;
    
    SetEntProp(entity, Prop_Data, "m_fFlags", flags);
}

// **********************************************
//              CS:S Offset Stocks
// **********************************************

#if defined PROJECT_GAME_CSS

/**
 * Get the amount of cash a client has.
 * 
 * @param client    The client index.
 * 
 * @return          The amount of cash the client has.
 */
stock OffsLib_GetCash(client)
{
    new offsAccount = FindSendPropInfo("CCSPlayer", "m_iAccount");
    return GetEntData(client, offsAccount);
}

/**
 * Set the amount of cash a client has.
 * 
 * @param client    The client index.
 * @param amount    The amount of cash to set.
 */
stock OffsLib_SetCash(client, amount)
{
    new offsAccount = FindSendPropInfo("CCSPlayer", "m_iAccount");
    SetEntData(client, offsAccount, amount);
}

/**
 * Check if a client owns NVGs.
 *  
 * @param client    The client index.
 * 
 * @return          True if client owns NVGs, false if not.
 */
stock bool:OffsLib_GetHasNVGs(client)
{
    new offsHasNVGs = FindSendPropInfo("CCSPlayer", "m_bHasNightVision");
    return bool:(GetEntData(client, offsHasNVGs, 1) & 1); // only read the LSB because the net prop is 1 bit, not 1 byte.
}

/**
 * Set if a client owns NVGs.
 *  
 * @param client    The client index.
 * @param hasnvgs   True to give NVGs, false to remove.
 */
stock OffsLib_SetHasNVGs(client, bool:hasnvgs)
{
    new offsHasNVGs = FindSendPropInfo("CCSPlayer", "m_bHasNightVision");
    SetEntData(client, offsHasNVGs, value, 1);
}

/**
 * Check if a client's NVGs are on.
 *  
 * @param client    The client index.
 * 
 * @return          True if client's NVGs are on, false if not.
 */
stock bool:OffsLib_GetNVGsOn(client)
{
    new offsNVGsOn = FindSendPropInfo("CCSPlayer", "m_bNightVisionOn");
    return bool:(GetEntData(client, offsNVGsOn, 1) & 1); // only read the LSB because the net prop is 1 bit, not 1 byte.
}

/**
 * Set if a client's NVGs are on.
 * 
 * @param client    The client index.
 * @param enabled   True to turn NVGs on, false to turn them off.
 */
stock OffsLib_SetNVGsOn(client, bool:nvgson)
{
    new offsNVGsOn = FindSendPropInfo("CCSPlayer", "m_bNightVisionOn");
    SetEntData(client, offsNVGsOn, value, 1);
}

/**
 * Gets the client's minimum fade distance.
 *  
 * @param client    The client index.
 * 
 * @return          Minimum fade distance.
 */
stock Float:OffsLib_GetFadeMinDist(client)
{
    new offsFadeMinDist = FindSendPropInfo("CCSPlayer", "m_fadeMinDist");
    return GetEntDataFloat(client, offsFadeMinDist);
}

/**
 * Sets the client's minimum fade distance.
 *  
 * @param client    The client index.
 * @param minDist   Minimum fade distance.
 */
stock Float:OffsLib_SetFadeMinDist(client, Float:minDist)
{
    new offsFadeMinDist = FindSendPropInfo("CCSPlayer", "m_fadeMinDist");
    SetEntDataFloat(client, offsFadeMinDist, minDist, true);
}

/**
 * Gets the client's maximum fade distance.
 *  
 * @param client    The client index.
 * 
 * @return          Maximum fade distance.
 */
stock Float:OffsLib_GetFadeMaxDist(client)
{
    new offsFadeMaxDist = FindSendPropInfo("CCSPlayer", "m_fadeMaxDist");
    return GetEntDataFloat(client, offsFadeMaxDist);
}

/**
 * Sets the client's maximum fade distance.
 *  
 * @param client    The client index.
 * @param maxDist   Maximum fade distance.
 */
stock Float:OffsLib_SetFadeMaxDist(client, Float:maxDist)
{
    new offsFadeMaxDist = FindSendPropInfo("CCSPlayer", "m_fadeMaxDist");
    SetEntDataFloat(client, offsFadeMaxDist, maxDist, true);
}

#endif
