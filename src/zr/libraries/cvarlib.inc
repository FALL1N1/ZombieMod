/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          cvarlib.inc
 *  Type:          Library
 *  Description:   Cvar-related API.
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

#if defined _cvarlib_included
 #endinput
#endif
#define _cvarlib_included

/**************
 *   Public   *
 **************/

/**
 * Sets a cvar to the given value and then locks it.
 * 
 * @param cvar      The cvar to lock.
 * @param value     The value to lock cvar as.
 */
stock CvarLib_LockCvar(const String:cvar[], const String:value[])
{
    new Handle:hCvar = FindConVar(cvar);
    
    // Set new value and lock it.
    SetConVarString(hCvar, value);
    HookConVarChange(hCvar, CvarLib_CvarLock);
}

/**
 * Unlocks a locked cvar.
 * 
 * @param cvar      The cvar to lock.
 *
 * @error           Invalid or corrupt handle.
 */
stock CvarLib_UnlockCvar(const String:cvar[])
{
    UnhookConVarChange(FindConVar(cvar), CvarLib_CvarLock);
}

/***************
 *   Private   *
 ***************/

/**
 * Called when a console variable's value is changed.
 *
 * Note:    This is a recursive call; when the user change the cvar, and
 *          when this code reset it to the old value. The isChanging tells
 *          whether it's the user that's changing or not, so the recursion is
 *          stopped.
 * 
 * @param convar		Handle to the convar that was changed.
 * @param oldValue		String containing the value of the convar before it was changed.
 * @param newValue		String containing the new value of the convar.
 */
public CvarLib_CvarLock(Handle:convar, const String:oldValue[], const String:newValue[])
{
    UnhookConVarChange(convar, CvarLib_CvarLock);
    SetConVarString(convar, oldValue);
    HookConVarChange(convar, CvarLib_CvarLock);
}
