/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          instantchange.inc
 *  Type:          Module include
 *  Description:   Manages instant change of classes.
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
 * Starts instant change timer if allowed.
 *
 * @param client    Client index.
 */
ClassMgr_CheckInstantChange(client)
{
    // Check if instant class change cvar is set.
    new Float:instantChange = GetConVarFloat(g_hCvarClassesChangeTimelimit);
    if (instantChange > 0)
    {
        // Allow instant class change.
        g_ClassAllowInstantChange[client] = true;
        
        // Create timer to disable instant change.
        g_hClassInstantChange[client] = CreateTimer(instantChange, ClassMgr_InstantChange, client, TIMER_FLAG_NO_MAPCHANGE);
    }
    else
    {
        // Make sure instant change is not allowed.
        g_ClassAllowInstantChange[client] = false;
    }
}

/**
 * Stops the instant change timer and resets the flag.
 *
 * @param client        Client index.
 * @param killTimer     Also end the timer if it's running.
 */
ClassMgr_ResetInstantChange(client, bool:killTimer = true)
{
    // Disable instant class change.
    g_ClassAllowInstantChange[client] = false;
    
    // Reset the variable.
    if (killTimer)
    {
        Util_CloseHandle(g_hClassInstantChange[client]);
    }
    else
    {
        g_hClassInstantChange[client] = INVALID_HANDLE;
    }
}

/**
 * Timer callback for disabling instant class change setting on a client.
 */
public Action:ClassMgr_InstantChange(Handle:timer, any:client)
{
    ClassMgr_ResetInstantChange(client, false);
}
