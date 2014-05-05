/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          versioninfo.inc
 *  Type:          Base
 *  Description:   Command to display Mercurial version information in-game.
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
 * Provides the plugin a way to know if the versioninfo.inc is included in the project.
 */
#define VERSION_INFO

#if defined PROJECT_IN_VCS
    #include "zr/base/vcsversion"
#endif

/**
 * Plugin is loading.
 */
VersionInfo_OnPluginStart()
{
    #if !defined PROJECT_BASE_CMD
        // Create a version information print command.
        Project_RegConsoleCmd("version", VersionInfo_Command, "Prints version info about this project.");
    #endif
    
    #if defined PROJECT_PUBLIC_CVAR
        decl String:publiccvarvalue[32];
        strcopy(publiccvarvalue, sizeof(publiccvarvalue), PROJECT_VERSION);
        
        #if defined PROJECT_IN_VCS
            Format(publiccvarvalue, sizeof(publiccvarvalue), "%s r%s", publiccvarvalue, PROJECT_VCS_REVISION);
        #endif
        
        CreateConVar(PROJECT_PUBLIC_CVAR, publiccvarvalue, "Project version information.", FCVAR_PLUGIN|FCVAR_SPONLY|FCVAR_UNLOGGED|FCVAR_DONTRECORD|FCVAR_REPLICATED|FCVAR_NOTIFY);
    #endif
}

/**
 * Plugin is ending.
 */
VersionInfo_OnPluginEnd()
{
}

/**
 * Command callback: project_version
 * 
 * @param client    The client index.  Or SERVER_INDEX if coming from the server.
 * @param argc      The number of arguments that the client sent with the command.
 */
public Action:VersionInfo_Command(client, argc)
{
    VersionPrint(client);
    
    // Say that we handled the command so the game doesn't see it and print "Unknown command"
    return Plugin_Handled;
}

/**
 * Print available version information to a client. (SERVER_INDEX) to print to server.
 * 
 * @param client    The client index.
 */
stock VersionPrint(client)
{
    decl String:buffer[512];
    decl String:linebuffer[128];
    buffer[0] = 0;
    
    // %24s is a formatting token that lines up text automatically.
    #define FORMATSTRING "%24s: %s\n"
    
    Format(linebuffer, sizeof(linebuffer), "%s\n", PROJECT_FULLNAME);
    StrCat(buffer, sizeof(buffer), linebuffer);
    
    Format(linebuffer, sizeof(linebuffer), "%s\n \n", PROJECT_COPYRIGHT);
    StrCat(buffer, sizeof(buffer), linebuffer);
    
    Format(linebuffer, sizeof(linebuffer), FORMATSTRING, "License", PROJECT_LICENSE);
    StrCat(buffer, sizeof(buffer), linebuffer);
    
    Format(linebuffer, sizeof(linebuffer), FORMATSTRING, "Version", PROJECT_VERSION);
    StrCat(buffer, sizeof(buffer), linebuffer);
    
    #if defined PROJECT_IN_VCS
        Format(linebuffer, sizeof(linebuffer), FORMATSTRING, "Development Branch", PROJECT_BRANCH);
        StrCat(buffer, sizeof(buffer), linebuffer);
        
        Format(linebuffer, sizeof(linebuffer), FORMATSTRING, "Build ID", PROJECT_VCS_REVISION);
        StrCat(buffer, sizeof(buffer), linebuffer);
        
        Format(linebuffer, sizeof(linebuffer), FORMATSTRING, "Compile Date", PROJECT_VCS_DATE);
        StrCat(buffer, sizeof(buffer), linebuffer);
    #endif
    
    // Send back reply in the correct place.  (Chat, client console, server console)
    ReplyToCommand(client, buffer);
}
