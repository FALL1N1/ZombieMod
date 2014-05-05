/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          overlaylib.inc
 *  Type:          Library
 *  Description:   Helps to properly display multiple overlays at once.
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

#if defined _overlaylib_included
 #endinput
#endif
#define _overlaylib_included

#include <sdktools>

/**
 * Stored for each registered overlay.
 */
enum OverlayLib_Info
{
    String:OLibInfo_Path[PLATFORM_MAX_PATH],    /** The path to the overlay file.  (No extension) */
    OLibInfo_Priority                           /** The priority level of the overlay */
}

/**
 * Handle to the global overlay array.
 */
new Handle:g_hOverlays;

/**
 * Storage for per-client data.
 */
new Handle:g_iClientOverlays[MAXPLAYERS + 1];

/**
 * Register an overlay for this library to handle here.
 * The overlay file(s) will automatically be added to the downloads table.
 * 
 * @param path              The path to the overlay .vmt file.  There cannot be a file extension in the path.
 * @param priority          The priority level of the overlay.  0 is top priority.
 *                          If the priority matches any existing ones, then all others will be shifted down one.
 * @param addtodownloads    True to add the registered overlay's files to the downloads table.
 * 
 * @return          The overlay index.
 */
stock OverlayLib_Register(const String:path[], priority, bool:addtodownloads = false)
{
    // Create array if it hasn't been yet.
    if (g_hOverlays == INVALID_HANDLE)
        g_hOverlays = CreateArray(PLATFORM_MAX_PATH + 1);
    
    // Validate the overlay files.
    decl String:vmt[PLATFORM_MAX_PATH], String:vtf[PLATFORM_MAX_PATH];
    Format(vmt, sizeof(vmt), "%s.vmt", path);
    Format(vtf, sizeof(vtf), "%s.vtf", path);
    
    if (addtodownloads)
    {
        if (FileExists(vmt))
            AddFileToDownloadsTable(vmt);
        else
            LogError("[OverlayLib] Missing overlay file: \"%s\"", vmt);
        
        // Don't log an error if this doesn't exist because multiple vmt files can point to one vtf.
        if (FileExists(vtf))
            AddFileToDownloadsTable(vtf);
    }
    
    // Push overlay info into the array, the adt array index is the overlay index.
    new overlayinfo[OverlayLib_Info];
    strcopy(overlayinfo[OLibInfo_Path], sizeof(overlayinfo[OLibInfo_Path]), path);
    overlayinfo[OLibInfo_Priority] = priority;
    
    // Free up the priority level thats about to be set.
    OverlayLib_FreePriorityLevel(priority);
    
    return PushArrayArray(g_hOverlays, overlayinfo[0], sizeof(overlayinfo));
}

/**
 * Destroys all data stored by this library.
 * This should normally be used if the plugin is being unloaded.
 */
stock OverlayLib_Cleanup()
{
    // Destroy all information stored per-client.
    for (new index = 0; index < sizeof(g_iClientOverlays); index++)
    {
        OverlayLib_ClientDestroyInfo(index);
    }
    
    // Destroy all overlay info.
    CloseHandle(g_hOverlays);
}

/**
 * All clients must be initialized before overlays can be assigned to them.
 * This should be called in OnClientPutInServer.
 * 
 * @param client    The client index to initialize.
 */
stock OverlayLib_ClientInit(client)
{
    if (g_iClientOverlays[client] != INVALID_HANDLE)
        CloseHandle(g_iClientOverlays[client]);
    
    g_iClientOverlays[client] = CreateArray();
}

/**
 * When overlay info is no longer needed for a client, destroy it with this.
 * This should be called in OnClientDisconnect to free up memory.
 * 
 * @param client    The client index.
 */
stock OverlayLib_ClientDestroyInfo(client)
{
    if (g_iClientOverlays[client] != INVALID_HANDLE)
        CloseHandle(g_iClientOverlays[client]);
    
    g_iClientOverlays[client] = INVALID_HANDLE;
}

/**
 * Adds an overlay to a client's queue.  The highest priority overlay will be displayed over the rest.
 * Client must be initialized with OverlayLib_ClientInit.
 * 
 * @param client    The client index.
 * @param overlay   The index of the registered overlay.
 */
stock OverlayLib_AddOverlay(client, overlay)
{
    if (FindValueInArray(g_iClientOverlays[client], overlay) != -1)
        return;
    
    if (!OverlayLib_IsValid(overlay))
        ThrowError("[OverlayLib] Invalid overlay index (%d) given to OverlayLib_AddOverlay", overlay);
    
    PushArrayCell(g_iClientOverlays[client], overlay);
}

/**
 * Removes an overlay from a client's queue.
 * Client must be initialized with OverlayLib_ClientInit.
 * 
 * @param client    The client index.
 * @param overlay   The index of the registered overlay.
 */
stock OverlayLib_RemoveOverlay(client, overlay)
{
    new index = FindValueInArray(g_iClientOverlays[client], overlay);
    if (index == -1)
        return;
    
    if (!OverlayLib_IsValid(overlay))
        ThrowError("[OverlayLib] Invalid overlay index (%d) given to OverlayLib_RemoveOverlay", overlay);
    
    RemoveFromArray(g_iClientOverlays[client], index);
}

/**
 * Displays the highest priority overlay in a client's queue.  Clears overlays from screen if queue is empty.
 * 
 * @param client    The client index to update overlays on.
 */
stock OverlayLib_UpdateClient(client)
{
    new count = GetArraySize(g_iClientOverlays[client]);
    if (count == 0)
    {
        OverlayLib_Display(client, -1);
        return;
    }
    
    new overlay;
    new priority;
    new highestpriority = 9999;
    new displayoverlay = -1;
    
    for (new oindex = 0; oindex < count; oindex++)
    {
        overlay = GetArrayCell(g_iClientOverlays[client], oindex);
        priority = OverlayLib_ReadPriority(overlay);
        if (priority < highestpriority)
        {
            highestpriority = priority;
            displayoverlay = overlay;
        }
    }
    
    // Display highest priority overlay.
    OverlayLib_Display(client, displayoverlay);
}

/**
 * Returns how many registered overlays there are.
 * 
 * @return  The number of registered overlays.
 */
stock OverlayLib_OverlayCount()
{
    return GetArraySize(g_hOverlays);
}

/**
 * Returns if an overlay index is valid or not.
 * 
 * @param overlay       The index of the registered overlay.
 * 
 * @return              True if it's valid, false if not.
 */
stock bool:OverlayLib_IsValid(overlay)
{
    return (overlay >= 0 && overlay < GetArraySize(g_hOverlays));
}

/**
 * Returns all information for an overlay.
 * 
 * @param overlay       The index of the registered overlay.
 * @param overlayinfo   The array that contains all overlay info.
 */
stock OverlayLib_ReadOverlayInfo(overlay, overlayinfo[OverlayLib_Info])
{
    GetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
}

/**
 * Sets all information for an overlay.
 * 
 * @param overlay       The index of the registered overlay.
 * @param overlayinfo   The array of info to overwrite current data with.
 */
stock OverlayLib_WriteOverlayInfo(overlay, overlayinfo[OverlayLib_Info])
{
    SetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
}

/**
 * Reads the path registered to an overlay.
 * 
 * @param overlay   The index of the registered overlay.
 * @param path      The string to return the path in.
 * @param maxlen    The max length of the output string.
 * @param materials True to return the path relative to the materials folder, false to return relative to the game folder.
 *                  r_screenoverlay is relative to the materials folder.
 */
stock OverlayLib_ReadPath(overlay, String:path[], maxlen, materials = true)
{
    new overlayinfo[OverlayLib_Info];
    GetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
    
    strcopy(path, maxlen, overlayinfo[OLibInfo_Path]);
    
    // If relative to the materials folder, then strip off materials/ from the path.
    if (materials)
    {
        ReplaceString(path, maxlen, "materials/", "");
        ReplaceString(path, maxlen, "materials\\", "");
    }
}

/**
 * Writes a new path for an overlay.
 * 
 * @param overlay   The index of the registered overlay.
 * @param path      The new path to overwrite old data with.
 */
stock OverlayLib_WritePath(overlay, const String:path[])
{
    new overlayinfo[OverlayLib_Info];
    GetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
    
    strcopy(overlayinfo[OLibInfo_Path], sizeof(overlayinfo[OLibInfo_Path]), path);
    
    SetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
}

/**
 * Reads the priority level of an overlay.
 * 
 * @param overlay   The index of the registered overlay.
 * 
 * @return          The priority level of the overlay.
 */
stock OverlayLib_ReadPriority(overlay)
{
    new overlayinfo[OverlayLib_Info];
    GetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
    
    return overlayinfo[OLibInfo_Priority];
}

/**
 * Writes a new priority for an overlay.
 * Make sure you use OverlayLib_FreePriorityLevel to free up the priority level you're about to write.
 * 
 * @param overlay   The index of the registered overlay.
 * @param path      The new priority level to overwrite old data with.
 */
stock OverlayLib_WritePriority(overlay, priority)
{
    new overlayinfo[OverlayLib_Info];
    GetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
    
    overlayinfo[OLibInfo_Priority] = priority;
    
    SetArrayArray(g_hOverlays, overlay, overlayinfo[0], sizeof(overlayinfo));
}

/**
 * Frees up a priority spot by increments all priority levels that are greaterthan or equal to the priority you seek to free.
 * 
 * @param startpriority The priority level to free.
 */
stock OverlayLib_FreePriorityLevel(priority)
{
    new overlaypriority;
    
    // Loop through all overlays.
    new count = GetArraySize(g_hOverlays);
    for (new overlay = 0; overlay < count; overlay++)
    {
        overlaypriority = OverlayLib_ReadPriority(overlay);
        if (overlaypriority >= priority)
            OverlayLib_WritePriority(overlay, ++overlaypriority);
    }
}

/**
 * Display an overlay to a client.
 * 
 * @param client    The client to display overlay to.
 * @param overlay   The index of the registered overlay to display.  -1 to remove the existing overlay.
 */
stock OverlayLib_Display(client, overlay)
{
    new String:overlaypath[PLATFORM_MAX_PATH] = "\"\"";
    
    if (OverlayLib_IsValid(overlay))
        OverlayLib_ReadPath(overlay, overlaypath, sizeof(overlaypath));
    
    ClientCommand(client, "r_screenoverlay %s", overlaypath);
}
