/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          wrappers.inc
 *  Type:          Base
 *  Description:   Contains simple native wrappers for the plugin.
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

/**
 * Wrappers for some non-client specific print natives that will prefix project-specific text on each message.
 * These should only be used if the plugin name needs to be prefixed onto the message. 
 */

stock Project_PrintToServer(const String:text[], any:...)
{
    decl String:formatted[512];
    VFormat(formatted, sizeof(formatted), text, 2);
    Format(formatted, sizeof(formatted), PROJECT_SERVER_PREFIX);
    PrintToServer(formatted);
}

/**
 * Wrapper of CreateConVar to prefix the cvars with the project's shortname.
 * Use <prefix> in the description to print the value of PROJECT_CVAR_PREFIX.
 * 
 * See native CreateConVar.
 */
stock Handle:Project_CreateConVar(const String:name[], const String:defaultValue[], const String:description[]="", flags=0, bool:hasMin=false, Float:min=0.0, bool:hasMax=false, Float:max=0.0)
{
    decl String:project_cvarname[64];
    Format(project_cvarname, sizeof(project_cvarname), "%s_%s", PROJECT_CVAR_PREFIX, name);
    
    // If the cvar exists, don't create it, return its handle.
    // This is useful because if multiple parts of the project create the same cvar, they can share the one that already exists.
    new Handle:cvar = FindConVar(project_cvarname);
    if (cvar != INVALID_HANDLE)
        return cvar;
    
    // Replace the define name with the actual cvar prefix.
    decl String:fmtdescription[1024];
    strcopy(fmtdescription, sizeof(fmtdescription), description);
    ReplaceString(fmtdescription, sizeof(fmtdescription), "<prefix>", PROJECT_CVAR_PREFIX, false);
    
    return CreateConVar(project_cvarname, defaultValue, fmtdescription, flags, hasMin, min, hasMax, max);
}

/**
 * Wrapper of RegServerCmd to prefix the commands with the project's shortname.
 * Use <prefix> in the description to print the value of PROJECT_CMD_PREFIX.
 * 
 * See native RegServerCmd.
 */
stock Project_RegServerCmd(const String:cmd[], SrvCmd:callback, const String:description[]="", flags=0)
{
    decl String:project_cmdname[64];
    Format(project_cmdname, sizeof(project_cmdname), "%s_%s", PROJECT_CMD_PREFIX, cmd);
    
    // Replace the define name with the actual server cmd prefix.
    decl String:fmtdescription[1024];
    strcopy(fmtdescription, sizeof(fmtdescription), description);
    ReplaceString(fmtdescription, sizeof(fmtdescription), "<prefix>", PROJECT_CMD_PREFIX, false);
    
    RegServerCmd(project_cmdname, callback, fmtdescription, flags);
}

/**
 * Wrapper of RegConsoleCmd to prefix the commands with the project's shortname.
 * 
 * See native RegConsoleCmd.
 */
stock Project_RegConsoleCmd(const String:cmd[], ConCmd:callback, const String:description[]="", flags=0)
{
    decl String:project_cmdname[64];
    Format(project_cmdname, sizeof(project_cmdname), "%s_%s", PROJECT_CMD_PREFIX, cmd);
    
    // Replace the define name with the actual server cmd prefix.
    decl String:fmtdescription[1024];
    strcopy(fmtdescription, sizeof(fmtdescription), description);
    ReplaceString(fmtdescription, sizeof(fmtdescription), "<prefix>", PROJECT_CMD_PREFIX, false);
    
    RegConsoleCmd(project_cmdname, callback, fmtdescription, flags);
}

/**
 * Copies the string into the buffer and resolves prefix tags.
 *
 * "<cmdprefix>" is resolved into the value of PROJECT_CMD_PREFIX
 * "<cvarprefix>" is resolved into the value of PROJECT_CVAR_PREFIX
 *
 * @param string        String to resolve.
 * @param maxlen        Size of string.
 *
 * @return              Number of characters copied.
 */
stock Project_PrefixString(String:string[], maxlen)
{
    ReplaceString(string, maxlen, "<cmdprefix>", PROJECT_CMD_PREFIX, true);
    ReplaceString(string, maxlen, "<cvarprefix>", PROJECT_CVAR_PREFIX, true);
}

stock Project_ReplyToCommand(client, const String:format[], any:...)
{
    decl String:formatted[1024];
    formatted[0] = 0;
    
    // Resolve formatting.
    VFormat(formatted, sizeof(formatted), format, 3);
    
    // Resolve prefix tags.
    Project_PrefixString(formatted, sizeof(formatted));
    
    ReplyToCommand(client, formatted);
}

