/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          interfaces.inc
 *  Type:          Interface
 *  Description:   Interfacing functions.
 *
 *  Copyright (C) 2009-2010  Greyscale, Richard Helgeby
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

#if defined _interfaces_included
 #endinput
#endif
#define _interfaces_included

// Include all interfaces.
#include "zr/interfaces/game.interface"
#include "zr/interfaces/infection.interface"
#include "zr/interfaces/soundprofiles.interface"

/**
 * Check if an interface function is available.
 */
stock bool:Interface_IsImplemented(Function:func)
{
    return (func != INVALID_FUNCTION);
}

/**
 * Wrapper for implementing a function.
 * 
 * @param funcvar   Variable storing the implementing function.
 * @param func      The function ID that will implement the function. 
 */
stock Interface_Implement(&Function:funcvar, Function:func)
{
    funcvar = func;
}

/**
 * Wrapper for releasing a function.
 * 
 * @param funcvar   Variable storing the implementing function.
 */
stock Interface_Release(&Function:funcvar)
{
    funcvar = INVALID_FUNCTION;
}

/**
 * Validates the specified function. Throws an error if not implemented.
 */
stock Interface_ValidateOrFail(Function:func)
{
    if (func == INVALID_FUNCTION)
    {
        ThrowError("This function is not implemented!");
    }
}
