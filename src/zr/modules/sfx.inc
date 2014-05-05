/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          sfx.inc
 *  Type:          Module
 *  Description:   Special effects!
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
new Module:g_moduleSFX;

/**
 * Cvar handles.
 */
new Handle:g_hCvarSFXInfectExplosion;
new Handle:g_hCvarSFXInfectFireball;
new Handle:g_hCvarSFXInfectSmoke;
new Handle:g_hCvarSFXInfectSparks;
new Handle:g_hCvarSFXInfectShake;
new Handle:g_hCvarSFXInfectShakeAmp;
new Handle:g_hCvarSFXInfectShakeFreq;
new Handle:g_hCvarSFXInfectShakeDur;

/**
 * @section env_explosion spawn flags.
 */
#define EXP_NODAMAGE               1
#define EXP_REPEATABLE             2
#define EXP_NOFIREBALL             4
#define EXP_NOSMOKE                8
#define EXP_NODECAL               16
#define EXP_NOSPARKS              32
#define EXP_NOSOUND               64
#define EXP_RANDOMORIENTATION    128
#define EXP_NOFIREBALLSMOKE      256
#define EXP_NOPARTICLES          512
#define EXP_NODLIGHTS           1024
#define EXP_NOCLAMPMIN          2048
#define EXP_NOCLAMPMAX          4096
/**
 * @endsection
 */

/**
 * Register this module.
 */
SFX_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Special Effects");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "sfx");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Special effects!");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleSFX = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleSFX, "Event_OnEventsRegister",               "SFX_OnEventsRegister");
}

/**
 * Register all events here.
 */
public SFX_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleSFX, "Event_OnClientInfected",               "SFX_OnClientInfected");
    EventMgr_RegisterEvent(g_moduleSFX, "Event_OnClientHuman",                  "SFX_OnClientHuman");
}

/**
 * Plugin is loading.
 */
SFX_OnPluginStart()
{
    // Register the module.
    SFX_Register();
    
    // Create cvars.
    g_hCvarSFXInfectExplosion = Project_CreateConVar("sfx_infect_explosion", "1", "Create a harmless explosion on infection.");
    g_hCvarSFXInfectFireball = Project_CreateConVar("sfx_infect_fireball", "1", "Create fireball in the explosion. [Dependency: <prefix>_infect_explosion]");
    g_hCvarSFXInfectSmoke = Project_CreateConVar("sfx_infect_smoke", "1", "Create smoke in the explosion. [Dependency: <prefix>_infect_explosion]");
    g_hCvarSFXInfectSparks = Project_CreateConVar("sfx_infect_sparks", "1", "Emit sparks from the explosion. [Dependency: <prefix>_infect_explosion]");
    g_hCvarSFXInfectShake = Project_CreateConVar("sfx_infect_shake", "1", "Shake the infected player's screen.");
    g_hCvarSFXInfectShakeAmp = Project_CreateConVar("sfx_infect_shake_amp", "15.0", "The amplitude, or intensity, of the shake.");
    g_hCvarSFXInfectShakeFreq = Project_CreateConVar("sfx_infect_shake_frequency", "1.0", "The frequency, or speed, of the shake.");
    g_hCvarSFXInfectShakeDur = Project_CreateConVar("sfx_infect_shake_duration", "5.0", "The duration, or timespan, of the shake.");
}

/**
 * Client has been infected.
 * 
 * @param client        The infected client.
 * @param attacker      The zombie that infected the client.
 * @param mzombie       True if the client has been infected as a mother zombie.
 */
public SFX_OnClientInfected(client, attacker, bool:mzombie)
{
    decl Float:pos[3];
    GetClientAbsOrigin(client, pos);
    pos[2] += 40;
    
    // Prep explosion.
    if (GetConVarBool(g_hCvarSFXInfectExplosion))
    {
        new flags = EXP_NODAMAGE|EXP_NODECAL;
        if (!GetConVarBool(g_hCvarSFXInfectFireball))   flags |= EXP_NOFIREBALL;
        if (!GetConVarBool(g_hCvarSFXInfectSmoke))      flags |= EXP_NOSMOKE;
        if (!GetConVarBool(g_hCvarSFXInfectSparks))     flags |= EXP_NOSPARKS;
        SFX_Explosion(pos, flags);
    }
    
    // Shake client.
    if (GetConVarBool(g_hCvarSFXInfectShake))
        SFX_Shake(client, GetConVarFloat(g_hCvarSFXInfectShakeAmp), GetConVarFloat(g_hCvarSFXInfectShakeFreq), GetConVarFloat(g_hCvarSFXInfectShakeDur));
}

/**
 * Client has turned back to human.
 * 
 * @param client        The client that became human.
 */
public SFX_OnClientHuman(client)
{
}

/**
 * Create an explosion effect at the specified origin.
 * 
 * @param origin    The (x, y, z) coordinate of the explosion.
 * @param flags     The flags to set on the explosion.
 */
static stock SFX_Explosion(const Float:origin[3], flags)
{
    // Create an explosion entity.
    new explosion = CreateEntityByName("env_explosion");
    if (explosion == -1)
    {
        LogMgr_Print(g_moduleSFX, LogType_Error, "env_explosion", "env_explosion not supported in this game!");
        return;
    }
    
    // Get and modify flags on explosion.
    //new spawnflags = GetEntProp(explosion, Prop_Data, "m_spawnflags");
    //spawnflags = spawnflags | EXP_NODAMAGE | EXP_NODECAL | flags;
    
    // Set spawn flags.
    SetEntProp(explosion, Prop_Data, "m_spawnflags", flags);
    
    // Then spawn it.
    DispatchSpawn(explosion);
    
    // Now set its location.
    DispatchKeyValueVector(explosion, "origin", origin);
    
    // Set fireball material if the fireball used.
    if (~flags & EXP_NOFIREBALL)
    {
        PrecacheModel("materials/sprites/xfireball3.vmt");
        DispatchKeyValue(explosion, "fireballsprite", "materials/sprites/xfireball3.vmt");
    }
    
    // Tell the entity to explode.
    AcceptEntityInput(explosion, "Explode");
    
    // Clean up.
    RemoveEdict(explosion);
}

/**
 * Shake a client's screen with specific parameters.
 * 
 * @param client        The client index.
 * @param amplitude     The amplitude (intensity) of the shaking.
 * @param frequency     The frequency (speed) of the shaking, in inverse seconds?
 * @param duration      The duration (time) of the shaking, in seconds.
 */
static stock SFX_Shake(client, Float:amplitude, Float:frequency, Float:duration)
{
    // If shake usermsg isn't invalid, then stop.
    new Handle:hShake = StartMessageOne("Shake", client);
    if (hShake == INVALID_HANDLE)
    {
        LogMgr_Print(g_moduleSFX, LogType_Error, "Shake usermessage", "\"Shake\" usermessage not supported in this game!");
        return;
    }
    
    // Write shake information and send.
    BfWriteByte(hShake, 0);
    BfWriteFloat(hShake, amplitude);
    BfWriteFloat(hShake, frequency);
    BfWriteFloat(hShake, duration);
    EndMessage();
}
