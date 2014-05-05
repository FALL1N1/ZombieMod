/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          fog.inc
 *  Type:          Module
 *  Description:   Controls fog on maps.
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
 * This module's identifier.
 */
new Module:g_moduleFog;

/**
 * Cvar handles.
 */
new Handle:g_hCvarFog;
new Handle:g_hCvarFogOverride;
new Handle:g_hCvarFogColor1;
new Handle:g_hCvarFogColor2;
new Handle:g_hCvarFogBlend;
new Handle:g_hCvarFogDir;
new Handle:g_hCvarFogStartDist;
new Handle:g_hCvarFogEndDist;
new Handle:g_hCvarFogDensity;

/**
 * Stores if a fog controller already exists on the map.
 */
new bool:g_bFogOnMap;

/**
 * The map's current env_fog_controller and sky_camera entity indexes.
 */
new g_iFogControllerIndex;
new g_iFogSkyCamera;

/**
 * Tracks when a fog cvar has been changed.
 */
new bool:g_bFogSettingsChanged;

/**
 * Register this module.
 */
Fog_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Fog Control");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "fog");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Controls fog on maps.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleFog = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleFog, "Event_OnEventsRegister",               "Fog_OnEventsRegister");
}

/**
 * Register all events here.
 */
public Fog_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleFog, "Event_OnMyModuleEnable",               "Fog_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleFog, "Event_OnMyModuleDisable",              "Fog_OnMyModuleDisable");
    EventMgr_RegisterEvent(g_moduleFog, "Event_OnConfigsExecuted",              "Fog_OnConfigsExecuted");
    
    EventMgr_RegisterEvent(g_moduleFog, "Event_RoundStart",                     "Fog_RoundStart");
}

/**
 * Plugin is loading.
 */
Fog_OnPluginStart()
{
    // Register the module.
    Fog_Register();
    
    // Create cvars.
    g_hCvarFog =            Project_CreateConVar("fog", "0", "Enable fog manipulation.");
    g_hCvarFogOverride =    Project_CreateConVar("fog_override", "0", "Enabling this will override any existing fog.  If this is 1 and <prefix>_fog is 0, fog will be stripped from all maps.");
    g_hCvarFogColor1 =      Project_CreateConVar("fog_color1", "200 200 200", "The primary RGB color of the fog.");
    g_hCvarFogColor2 =      Project_CreateConVar("fog_color2", "200 200 200", "The secondary RGB color of the fog [Dependency: <prefix>_fog_blend_colors]");
    g_hCvarFogBlend =       Project_CreateConVar("fog_blend", "0", "Enables blending between the two defined fog colors.");
    g_hCvarFogDir =         Project_CreateConVar("fog_dir", "0 90 0", "A direction vector pointing from the primary fog color to the secondary. [Dependency: <prefix>_fog_use_angles]");
    g_hCvarFogStartDist =   Project_CreateConVar("fog_start_dist", "10", "Distance, in game units, from a player to begin rendering fog.");
    g_hCvarFogEndDist =     Project_CreateConVar("fog_end_dist", "500", "Distance, in game units, from a player to stop rendering fog.");
    g_hCvarFogDensity =     Project_CreateConVar("fog_density", "1.0", "Scales the density of the fog.  [Range: 0-1]");
    
    // Hook cvar changes to track when to update the fog.
    Fog_HookCvars();
}

/**
 * Hooks all fog cvars.
 */
static stock Fog_HookCvars()
{
    HookConVarChange(g_hCvarFog, Fog_CvarHook);
    HookConVarChange(g_hCvarFogOverride, Fog_CvarHook);
    HookConVarChange(g_hCvarFogColor1, Fog_CvarHook);
    HookConVarChange(g_hCvarFogColor2, Fog_CvarHook);
    HookConVarChange(g_hCvarFogBlend, Fog_CvarHook);
    HookConVarChange(g_hCvarFogDir, Fog_CvarHook);
    HookConVarChange(g_hCvarFogStartDist, Fog_CvarHook);
    HookConVarChange(g_hCvarFogEndDist, Fog_CvarHook);
    HookConVarChange(g_hCvarFogDensity, Fog_CvarHook);
}

/**
 * Unhooks all fog cvars.
 */
static stock Fog_UnhookCvars()
{
    // Unhook cvar changes to track when to update the fog.
    UnhookConVarChange(g_hCvarFog, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogOverride, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogColor1, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogColor2, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogBlend, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogDir, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogStartDist, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogEndDist, Fog_CvarHook);
    UnhookConVarChange(g_hCvarFogDensity, Fog_CvarHook);
}

/**
 * Called when a fog cvar is changed.
 */
public Fog_CvarHook(Handle:convar, String:oldValue[], String:newValue[])
{
    g_bFogSettingsChanged = true;
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:Fog_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    // Initialize fog by forwarding this to OnConfigsExecuted.
    Fog_OnConfigsExecuted();
    
    // Hook cvar changes to track when to update the fog.
    Fog_HookCvars();
    
    g_bFogSettingsChanged = true;
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:Fog_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    // No longer need these hooks.
    Fog_UnhookCvars();
    
    // Remove fog if created by this.
    if (GetConVarBool(g_hCvarFogOverride) && g_iFogControllerIndex != -1)
    {
        // Return to its state of existence before this module did anything.
        if (g_bFogOnMap)
            Fog_Enable1(g_iFogControllerIndex, false);
        else
            AcceptEntityInput(g_iFogControllerIndex, "Kill");
        
        Fog_Enable2(g_iFogSkyCamera, false);
    }
}

/**
 * All convars are set, cvar-dependent code should use this.
 */
public Fog_OnConfigsExecuted()
{
    // Look for an existing fog controller on the map, if it doesn't exist, then create it.
    g_iFogControllerIndex = FindEntityByClassname(-1, "env_fog_controller");
    if (g_iFogControllerIndex == -1) // If there is no controller, create one.
    {
        g_iFogControllerIndex = CreateEntityByName("env_fog_controller");
        DispatchSpawn(g_iFogControllerIndex);
        g_bFogOnMap = false;
    }
    else
        g_bFogOnMap = true;
    
    // Now we need a sky_camera to mirror the fog settings to.
    g_iFogSkyCamera = FindEntityByClassname(-1, "sky_camera");
    if (g_iFogSkyCamera == -1) // If there is no sky_camera, create one.
    {
        g_iFogSkyCamera = CreateEntityByName("sky_camera");
        DispatchSpawn(g_iFogControllerIndex);
    }
    
    Fog_Update();
}

/**
 * Round has started.
 */
public Fog_RoundStart()
{
    // Update fog settings if any cvars changed.
    if (g_bFogSettingsChanged)
    {
        Fog_Update();
        g_bFogSettingsChanged = false;
    }
}

/**
 * Private stocks.
 */

/**
 * Enable or disable the fog entity based on the current map and configs.
 */
static stock Fog_Update()
{
    // If there is no fog on the map or the override is enabled then enable or disable the fog according to the fog enable/disable cvar.
    if (!g_bFogOnMap || GetConVarBool(g_hCvarFogOverride))
    {
        // Update the properties set by cvar.
        Fog_UpdateSettings();
        
        // Enable/disable it.
        Fog_Enable1(g_iFogControllerIndex, false); // Fog must be turned off then on to show property changes.
        Fog_Enable1(g_iFogControllerIndex, GetConVarBool(g_hCvarFog));
        Fog_Enable2(g_iFogSkyCamera, GetConVarBool(g_hCvarFog));
    }
}

/**
 * Read all cvars and update the fog controller values.
 */
static stock Fog_UpdateSettings()
{
    decl String:color1[16], String:color2[16], String:dir[16];
    new bool:blend;
    new Float:startdist, Float:enddist, Float:density;
    
    // Get all values.
    GetConVarString(g_hCvarFogColor1, color1, sizeof(color1));
    GetConVarString(g_hCvarFogColor2, color2, sizeof(color2));
    GetConVarString(g_hCvarFogDir, dir, sizeof(dir));
    blend = GetConVarBool(g_hCvarFogBlend);
    startdist = GetConVarFloat(g_hCvarFogStartDist);
    enddist = GetConVarFloat(g_hCvarFogEndDist);
    density = GetConVarFloat(g_hCvarFogDensity);
    
    // Update all fog properties.
    Fog_SetColor1(g_iFogControllerIndex, color1); Fog_SetColor1(g_iFogSkyCamera, color1);
    Fog_SetColor2(g_iFogControllerIndex, color2); Fog_SetColor2(g_iFogSkyCamera, color2);
    Fog_SetBlend(g_iFogControllerIndex, blend); Fog_SetBlend(g_iFogSkyCamera, blend);
    Fog_SetDir(g_iFogControllerIndex, dir); Fog_SetDir(g_iFogSkyCamera, dir);
    Fog_SetStartDist(g_iFogControllerIndex, startdist); Fog_SetStartDist(g_iFogSkyCamera, startdist);
    Fog_SetEndDist(g_iFogControllerIndex, enddist); Fog_SetEndDist(g_iFogSkyCamera, enddist);
    Fog_SetDensity(g_iFogControllerIndex, density); Fog_SetDensity(g_iFogSkyCamera, density);
}

/**
 * Fog manipulator stocks.
 */

/**
 * Enable/disable env_fog_controller entity.
 * 
 * @param fog       env_fog_controller entity index.
 * @param enable    True/false.
 */
static stock Fog_Enable1(fog, bool:enable)
{
    if (enable)
        AcceptEntityInput(fog, "TurnOn");
    else
        AcceptEntityInput(fog, "TurnOff");
}

/**
 * Enable/disable sky_camera entity.
 * 
 * @param fog       env_fog_controller entity index.
 * @param enable    True/false.
 */
static stock Fog_Enable2(fog, bool:enable)
{
    decl String:strEnable[2];
    IntToString(enable, strEnable, sizeof(strEnable));
    DispatchKeyValue(fog, "fogenable", strEnable);
}

/**
 * Set fog's primary color.
 * 
 * @param fog       env_fog_controller entity index.
 * @param color     The rgb color of the fog.
 */
static stock Fog_SetColor1(fog, const String:color[])
{
    DispatchKeyValue(fog, "fogcolor", color);
}

/**
 * Set fog's secondary color.
 * 
 * @param fog       env_fog_controller entity index.
 * @param color     The rgb color of the fog.
 */
static stock Fog_SetColor2(fog, const String:color[])
{
    DispatchKeyValue(fog, "fogcolor2", color);
}

/**
 * Enable fog blending.
 * 
 * @param fog       env_fog_controller entity index.
 * @param blending  Enable/disable.
 */
static stock Fog_SetBlend(fog, bool:blend)
{
    decl String:strBlend[2];
    IntToString(blend, strBlend, sizeof(strBlend));
    DispatchKeyValue(fog, "fogblend", strBlend);
}

/**
 * Set the blending direction if use_angles is 0.
 * 
 * @param fog       env_fog_controller entity index.
 * @param dir       The direction vector pointing from the primary color to the secondary.
 */
static stock Fog_SetDir(fog, const String:dir[])
{
    DispatchKeyValue(fog, "fogdir", dir);
}

/**
 * Set fog's start distance.
 * 
 * @param fog           env_fog_controller entity index.
 * @param startdist     The start distance of the fog.
 */
static stock Fog_SetStartDist(fog, Float:startdist)
{
    DispatchKeyValueFloat(fog, "fogstart", startdist);
}

/**
 * Set fog's end distance.
 * 
 * @param fog           env_fog_controller entity index.
 * @param enddist       The end distance of the fog.
 */
static stock Fog_SetEndDist(fog, Float:enddist)
{
    DispatchKeyValueFloat(fog, "fogend", enddist);
}

/**
 * Set fog's density.
 * 
 * @param fog       env_fog_controller entity index.
 * @param density   The density of the fog.
 */
static stock Fog_SetDensity(fog, Float:density)
{
    DispatchKeyValueFloat(fog, "fogmaxdensity", density);
}
