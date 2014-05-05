/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          nextindexes.inc
 *  Type:          Module include
 *  Description:   Manages next class indexes.
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
 * Resets the next class indexes for the specified client.
 *
 * @param client    Client index.
 */
ClassMgr_ResetNextIndexes(client)
{
    g_ClassNextClass[client][ClassTeam_Zombies] = -1;
    g_ClassNextClass[client][ClassTeam_Humans] = -1;
}

/**
 * Restores next class indexes on a player, if available.
 * Note: Does not apply attributes. The classes are only marked as selected.
 *
 * @param client        The client index.
 * @param excludeTeam   Optional. Do not restore the specified team.
 */
ClassMgr_RestoreNextIndexes(client, ClassTeam:excludeTeam = ClassTeam_Invalid)
{
    // Get next class indexes.
    new zombie = g_ClassNextClass[client][ClassTeam_Zombies];
    new human = g_ClassNextClass[client][ClassTeam_Humans];
    
    // TODO: This code is redundant and can be improved by looping through teams.
    
    // Check if the zombie team should be excluded.
    if (excludeTeam != ClassTeam_Zombies)
    {
        // Validate zombie class index.
        if (ClassMgr_IsValidIndex(zombie))
        {
            // Mark next zombie class as selected.
            g_ClassSelected[client][ClassTeam_Zombies] = zombie;
        }
        
        // Reset index.
        g_ClassNextClass[client][ClassTeam_Zombies] = -1;
    }
    
    // Check if the human team should be excluded.
    if (excludeTeam != ClassTeam_Humans)
    {
        // Validate human class index.
        if (ClassMgr_IsValidIndex(human))
        {
            // Mark next zombie class as selected.
            g_ClassSelected[client][ClassTeam_Humans] = human;
        }
        
        // Reset index.
        g_ClassNextClass[client][ClassTeam_Humans] = -1;
    }
}
