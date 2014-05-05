/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          zrwrappers.inc
 *  Type:          Library
 *  Description:   Function wrappers for ZR.
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

#if defined _zrwrappers_included
 #endinput
#endif
#define _zrwrappers_included

/**
 * Wrapper for SetEntityRenderColor that fires a custom base event when a client's alpha is changed.
 */
stock Wrapper_SetEntRenderClr(entity, r=255, g=255, b=255, a=255)
{
    // This is the wrapped function.
    SetEntityRenderColor(entity, r, g, b, a);
    
    // Don't fire the event if it's not a client changing color.
    if (!Client_IsValid(entity, false))
        return;
    
    // Fire a custom event that tells the plugin the client's color has changed.
    static EventDataTypes:eventdatatypes[] = {DataType_Cell, DataType_Cell};
    new any:eventdata[2][1];
    eventdata[0][0] = entity;
    eventdata[1][0] = a;
    
    EventMgr_Forward(g_EvOnClientAlphaChanged, eventdata, sizeof(eventdata), sizeof(eventdata[]), eventdatatypes);
}
