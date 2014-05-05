/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          infection.interface.inc
 *  Type:          Interface
 *  Description:   An interface for common infection-type functions for modules to use.
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

#if defined _iinfection_included
 #endinput
#endif
#define _iinfection_included

/**
 * Interface function ID cache.
 */
new Function:g_IInfectionGetInfectWeapon = INVALID_FUNCTION;
new Function:g_IInfectionAreZombiesPresent = INVALID_FUNCTION;
new Function:g_IInfectionIsClientMZombie = INVALID_FUNCTION;
new Function:g_IInfectionClientToZombie = INVALID_FUNCTION;
new Function:g_IInfectionClientToHuman = INVALID_FUNCTION;

/**
 * Interface functions.
 */

stock IInfection_GetInfectWeapon(String:weapon[], maxlen)
{
    if (g_IInfectionGetInfectWeapon != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_IInfectionGetInfectWeapon);
        Call_PushStringEx(weapon, maxlen, 0, SM_PARAM_COPYBACK);
        Call_PushCell(maxlen);
        Call_Finish();
    }
    else
        ThrowError("This function isn't implemented!");
}

stock bool:IInfection_AreZombiesPresent()
{
    if (g_IInfectionAreZombiesPresent != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_IInfectionAreZombiesPresent);
        new bool:value;
        Call_Finish(value);
        return value;
    }
    else
        ThrowError("This function isn't implemented!");
    
    return false; // Never makes it here anyway.
}

stock bool:IInfection_IsClientMZombie(client)
{
    if (g_IInfectionIsClientMZombie != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_IInfectionIsClientMZombie);
        Call_PushCell(client);
        new bool:value;
        Call_Finish(value);
        return value;
    }
    else
        ThrowError("This function isn't implemented!");
    
    return false;
}

stock IInfection_ClientToZombie(client, attacker = 0, bool:mzombie)
{
    if (g_IInfectionClientToZombie != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_IInfectionClientToZombie);
        Call_PushCell(client);
        Call_PushCell(attacker);
        Call_PushCell(mzombie);
        Call_Finish();
    }
    else
        ThrowError("This function isn't implemented!");
    
}

stock IInfection_ClientToHuman(client)
{
    if (g_IInfectionClientToHuman != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_IInfectionClientToHuman);
        Call_PushCell(client);
        Call_Finish();
    }
    else
        ThrowError("This function isn't implemented!");
    
}
