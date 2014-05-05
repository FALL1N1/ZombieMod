/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          soundprofiles.interface.inc
 *  Type:          Interface
 *  Description:   An interface for modules to get data from the sound profiles module.
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

#if defined _isndprofs_included
 #endinput
#endif
#define _isndprofs_included

/**
 * Interface function ID cache.
 */
new Function:g_ISndProfsGetNumProfiles = INVALID_FUNCTION;
new Function:g_ISndProfsGetProfileSound = INVALID_FUNCTION;
new Function:g_ISndProfsGetRandProfileSound = INVALID_FUNCTION;

/**
 * Interface functions.
 */

stock ISndProfs_GetNumProfiles()
{
    if (g_ISndProfsGetNumProfiles != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_ISndProfsGetNumProfiles);
        new result;
        Call_Finish(result);
        return result;
    }
    else
        ThrowError("This function isn't implemented!");
    
    return 0;
}

stock bool:ISndProfs_GetProfileSound(const String:profile[], index, String:sound[], maxlen)
{
    if (g_ISndProfsGetProfileSound != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_ISndProfsGetProfileSound);
        Call_PushString(profile);
        Call_PushCell(index);
        Call_PushString(sound);
        Call_PushCell(maxlen);
        new bool:result;
        Call_Finish(result);
        return result;
    }
    else
        ThrowError("This function isn't implemented!");
    
    return false;
}

stock bool:ISndProfs_GetRandProfileSound(const String:profile[], String:sound[], maxlen)
{
    if (g_ISndProfsGetRandProfileSound != INVALID_FUNCTION)
    {
        Call_StartFunction(GetMyHandle(), g_ISndProfsGetRandProfileSound);
        Call_PushString(profile);
        Call_PushString(sound);
        Call_PushCell(maxlen);
        new bool:result;
        Call_Finish(result);
        return result;
    }
    else
        ThrowError("This function isn't implemented!");
    
    return false;
}
