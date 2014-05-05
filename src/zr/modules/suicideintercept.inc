/*
 * ============================================================================
 *
 *  Zombie:Reloaded
 *
 *  File:          suicideintercept.inc
 *  Type:          Module
 *  Description:   Prevents clients from suiciding in certain situations.
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
new Module:g_moduleSuiciIntcpt;

/**
 * Cvar handles.
 */
new Handle:g_hCvarSuicideBlockZombies;
new Handle:g_hCvarSuicideBlockMZombies;
new Handle:g_hCvarSuicideBlockPreOutbreak;
new Handle:g_hCvarSuicideBlockPostOutbreak;

/**
 * Hooked commands adt array handle.
 */
new Handle:g_hSuiciIntcptCmds;

/**
 * Register this module.
 */
SuiciIntcpt_Register()
{
    // Define all the module's data as layed out by enum ModuleData in project.inc.
    new moduledata[ModuleData];
    
    moduledata[ModuleData_Disabled] = false;
    moduledata[ModuleData_Hidden] = false;
    strcopy(moduledata[ModuleData_FullName], MM_DATA_FULLNAME, "Suicide Intercept");
    strcopy(moduledata[ModuleData_ShortName], MM_DATA_SHORTNAME, "suicideintercept");
    strcopy(moduledata[ModuleData_Description], MM_DATA_DESCRIPTION, "Prevents clients from suiciding in certain situations.");
    moduledata[ModuleData_Dependencies][0] = INVALID_MODULE;
    
    // Send this array of data to the module manager.
    g_moduleSuiciIntcpt = ModuleMgr_Register(moduledata);
    
    // Register the OnEventsRegister event to register all events in it.
    EventMgr_RegisterEvent(g_moduleSuiciIntcpt, "Event_OnEventsRegister",       "SuiciIntcpt_OnEventsRegister");
}

/**
 * Register all events here.
 */
public SuiciIntcpt_OnEventsRegister()
{
    // Register all the events needed for this module.
    EventMgr_RegisterEvent(g_moduleSuiciIntcpt, "Event_OnMyModuleEnable",       "SuiciIntcpt_OnMyModuleEnable");
    EventMgr_RegisterEvent(g_moduleSuiciIntcpt, "Event_OnMyModuleDisable",      "SuiciIntcpt_OnMyModuleDisable");
}

/**
 * Plugin is loading.
 */
SuiciIntcpt_OnPluginStart()
{
    // Register the module.
    SuiciIntcpt_Register();
    
    // Create cvars.
    g_hCvarSuicideBlockZombies = Project_CreateConVar("suicide_block_zombies", "0", "Prevent zombies from suiciding.");
    g_hCvarSuicideBlockMZombies = Project_CreateConVar("suicide_block_mzombies", "1", "Prevent mother zombies from suiciding.");
    g_hCvarSuicideBlockPreOutbreak = Project_CreateConVar("suicide_block_preoutbreak", "0", "Prevent humans from suiciding before outbreak of zombies.");
    g_hCvarSuicideBlockPostOutbreak = Project_CreateConVar("suicide_block_postoutbreak", "0", "Prevent humans from suiciding after outbreak of zombies.");
    
    SuiciIntcpt_AddListeners();
}

/**
 * The module that hooked this event callback has been enabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop enable, and Plugin_Continue to allow it.
 */
public Action:SuiciIntcpt_OnMyModuleEnable(String:refusalmsg[], maxlen)
{
    SuiciIntcpt_AddListeners();
}

/**
 * The module that hooked this event callback has been disabled.
 * 
 * @param refusalmsg    The string that is printed if Plugin_Handled is returned and it is non-empty.
 * @param maxlen        The max length of the string.
 * 
 * @return              Return Plugin_Handled to stop disable, and Plugin_Continue to allow it.
 */
public Action:SuiciIntcpt_OnMyModuleDisable(String:refusalmsg[], maxlen)
{
    SuiciIntcpt_RemoveListeners();
}

/**
 * Add and remove command listeners.
 */

SuiciIntcpt_AddListeners()
{
    // Create array to store suicide cmds.
    if (g_hSuiciIntcptCmds == INVALID_HANDLE)
    {
        g_hSuiciIntcptCmds = CreateArray(32);
        
        decl String:path[PLATFORM_MAX_PATH];
        BuildPath(Path_SM, path, sizeof(path), "gamedata/%s.txt", GAMEDATA_FILE);
        new Handle:kv = CreateKeyValues("Games");
        if (!FileToKeyValues(kv, path))
        {
            LogMgr_Print(g_moduleSuiciIntcpt, LogType_Error, "Gamedata file", "Can't open gamedata file: %s.txt", GAMEDATA_FILE);
            CloseHandle(kv);
            return;
        }
        
        if (KvJumpToKey(kv, "#default") && KvJumpToKey(kv, "Keys"))
        {
            decl String:cmdkey[16];
            decl String:cmd[32];
            new numcmds = KvGetNum(kv, "NumSuicideCmds");
            for (new i = 1; i <= numcmds; i++)
            {
                Format(cmdkey, sizeof(cmdkey), "SuicideCmd%d", i);
                KvGetString(kv, cmdkey, cmd, sizeof(cmd));
                
                AddCommandListener(SuiciIntcpt_Listener, cmd);
                PushArrayString(g_hSuiciIntcptCmds, cmd);
            }
        }
        else
        {
            LogMgr_Print(g_moduleSuiciIntcpt, LogType_Error, "Gamedata file", "Invalid gamedata file: %s.txt", GAMEDATA_FILE);
        }
        CloseHandle(kv);
    }
}

SuiciIntcpt_RemoveListeners()
{
    // Remove all command listeners and kill array.
    if (g_hSuiciIntcptCmds != INVALID_HANDLE)
    {
        // Remove the listeners and destroy the free the array memory.
        decl String:cmd[32];
        new size = GetArraySize(g_hSuiciIntcptCmds);
        for (new i = 0; i < size; i++)
        {
            GetArrayString(g_hSuiciIntcptCmds, i, cmd, sizeof(cmd));
            RemoveCommandListener(SuiciIntcpt_Listener, cmd);
        }
        CloseHandle(g_hSuiciIntcptCmds);
        g_hSuiciIntcptCmds = INVALID_HANDLE;
    }
}

/**
 * Command listener: (commands defined in gamedata)
 * Prevents clients from suiciding.
 * 
 * @param client    The client index.
 * @param argc      The number of arguments in command string.
 */
public Action:SuiciIntcpt_Listener(client, const String:command[], argc)
{
    // Can't suicide if you're not alive.
    if (!IsPlayerAlive(client))
        return Plugin_Continue;
    
    // Must pass all of these conditions to allow suicide.
    
    // Allow zombie suicide OR client is not a zombie.
    if (!GetConVarBool(g_hCvarSuicideBlockZombies) || !TeamMgr_IsClientZombie(client))
    {
        // Allow mother zombie suicide OR client is not a mother zombie.
        if (!GetConVarBool(g_hCvarSuicideBlockMZombies) || !IInfection_IsClientMZombie(client))
        {
            // Allow pre-outbreak AND game is pre-outbreak OR Allow post-outbreak AND game is post-outbreak OR client is not a human.
            new bool:zombiespresent = IInfection_AreZombiesPresent();
            if ((!GetConVarBool(g_hCvarSuicideBlockPreOutbreak) && !zombiespresent) || (!GetConVarBool(g_hCvarSuicideBlockPostOutbreak) && zombiespresent) || !TeamMgr_IsClientHuman(client))
                return Plugin_Continue;
        }
    }
    
    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Suicide intercept message");
    LogMgr_Print(g_moduleSuiciIntcpt, LogType_Normal, "Suicide Intercepted", "\"%L\" attempted suicide, but it was caught and blocked.", client);
    
    return Plugin_Handled;
}
