/*
 * ============================================================================
 *
 *  SourceMod Project Base
 *
 *  File:          translationsmanager.inc
 *  Type:          Base
 *  Description:   Manages translations throughout the project.
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
 * Provides the plugin a way to know if the translations manager is included in the project.
 */
#define TRANSLATIONS_MANAGER

// ---------------
//     Public
// ---------------

/**
 * All available methods of sending text to clients.
 */
enum MsgTypes
{
    MsgType_Server,     /** Text printed to the server's console */
    MsgType_Console,    /** Text printed in client's console. */
    MsgType_Chat,       /** Text printed at the bottom of the screen. */
    MsgType_Reply,      /** Replies with the same msg type as the client used to trigger a command */
    MsgType_Center,     /** Small, white text printed in the center of the screen. */
    MsgType_Hint        /** Translucent grey box containing white text at the bottom center of the screen. */
}

/**
 * An array to know which msg types be colored.
 */
new g_bMsgTypeColor[MsgTypes] = {false, false, true, false, false, false};

/**
 * The types of automatic message formatting.
 * Look in project.inc for editable defines regarding these options.
 */
enum MsgFormatting
{
    MsgFormat_None = -1,    /** Don't add anything to the text. */
    MsgFormat_Plugin,       /** Prepend the plugin name to the text. */
    MsgFormat_Module,       /** Prepend the module name to the text. */
    MsgFormat_Both          /** Prepend the both the plugin name and module name to the text. */
}

/**
 * Different options for translating bools into phrases.
 */
enum BoolPhrases
{
    BoolPhrase_YesNo,           /** Translates boolean values 1/0 into "yes/no" phrases. */
    BoolPhrase_OnOff,           /** Translates boolean values 1/0 into "on/off" phrases. */
    BoolPhrase_EnabledDisabled  /** Translates boolean values 1/0 into "enabled/disabled" phrases. */
}

/**
 * Return results for translating phrases into a boolean.
 * Note these can be read as bools. Ex:
 * new PhraseToBool:ptb = TranslatePhraseToBool(...);
 * if (ptb != PhraseToBool_Neither) new bool:b = bool:ptb;
 */
enum PhraseToBool
{
    PhraseToBool_False = 0, /** The phrase is equivalent to boolean 'false'. */
    PhraseToBool_True = 1,  /** The phrase is equivalent to boolean 'true'. */
    PhraseToBool_Neither    /** The phrase is equivalent to neither boolean value. */
}

// ---------------
//     Private
// ---------------


// **********************************************
//                 Forwards
// **********************************************

/**
 * Plugin is loading.
 */
TransMgr_OnPluginStart()
{
    // Load all the translations for the project.
    TransMgr_LoadTranslations();
}

/**
 * Plugin is ending.
 */
TransMgr_OnPluginEnd()
{
}

/**
 * Base command is printing a module's info.
 * Print the module data allocated by the translation manager.
 * Note: |stock| tag will stop this function from being compiled if the base command is disabled.
 * 
 * @param client    The client index the text is being printed to.
 * @param module    The module to print info for.
 */
stock TransMgr_OnPrintModuleInfo(client, Module:module)
{
}

// **********************************************
//                Public API
// **********************************************

/**
 * Print text to a client with special formatting and translated text.
 * 
 * @param client    The client index.  Use SERVER_INDEX when printing to the server.
 * @param msgformat How to format the text before sending.  See enum MsgFormatting.
 * @param msgtype   The type of message to print to the client.  See enum MsgTypes.
 * @param module    The module this is coming from.  Needed for MsgFormat_Module or MsgFormat_Both.
 * @param literal   True to print the last variable as a string, instead of translating it.
 * @param any       The translations phrase or literal string followed by formatting parameters.
 */
stock TransMgr_PrintText(client, MsgFormatting:msgformat, MsgTypes:msgtype, Module:module = INVALID_MODULE, bool:literal = false, any:...)
{
    // Tells SourceMod's translations system to translate to this client's (or server's) language.
    SetGlobalTransTarget(client);
    
    decl String:translated[512]; // Give plenty of room for long messages, better too much than not enough.
    
    // Method for getting text from the ... param depends on if literal is true or false.
    if (literal)
        VFormat(translated, sizeof(translated), "%s", 6);
    else
        VFormat(translated, sizeof(translated), "%t", 6);
    
    // Format the message.
    
    // Update the part of the array that contains the info for coloring text with the current reply source.
    g_bMsgTypeColor[MsgType_Reply] = bool:(GetCmdReplySource() == SM_REPLY_TO_CHAT);
    
    // Get the name of the module if a module identifier was given.
    new String:modulefullname[MM_DATA_FULLNAME] = "Invalid Module";
    if (ModuleMgr_IsModuleValid(module))
    {
        ModuleMgr_ReadString(module, ModuleData_FullName, modulefullname, sizeof(modulefullname));
    }
    
    // If this type of message supports color, then use the color formatting defines.
    if (g_bMsgTypeColor[msgtype])
    {
        switch(msgformat)
        {
            case MsgFormat_Plugin:  Format(translated, sizeof(translated), TM_MSGFORMAT_PLUGIN_COLOR);
            case MsgFormat_Module:  Format(translated, sizeof(translated), TM_MSGFORMAT_MODULE_COLOR);
            case MsgFormat_Both:    Format(translated, sizeof(translated), TM_MSGFORMAT_BOTH_COLOR);
        }
    }
    // This type of message doesn't support color.
    else
    {
        switch(msgformat)
        {
            case MsgFormat_Plugin:  Format(translated, sizeof(translated), TM_MSGFORMAT_PLUGIN_NOCOLOR);
            case MsgFormat_Module:  Format(translated, sizeof(translated), TM_MSGFORMAT_MODULE_NOCOLOR);
            case MsgFormat_Both:    Format(translated, sizeof(translated), TM_MSGFORMAT_BOTH_NOCOLOR);
        }
    }
    
    // Now print the text to the client.
    switch(msgtype)
    {
        case MsgType_Server:    PrintToServer(translated);
        case MsgType_Console:   PrintToConsole(client, translated);
        case MsgType_Center:    PrintCenterText(client, translated);
        case MsgType_Hint:      PrintHintText(client, translated);
        
        #if defined _colors_included
        case MsgType_Chat:      CPrintToChat(client, translated);
        case MsgType_Reply:
        {
            if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
                CPrintToChat(client, translated);
            else
            {
                CRemoveTags(translated, sizeof(translated));
                ReplyToCommand(client, translated);
            }
        }
        #else
        case MsgType_Chat:      PrintToChat(client, translated);
        case MsgType_Reply:     ReplyToCommand(client, translated);
        #endif
    }
}

/**
 * Print text to all clients with special formatting and translated text.
 * 
 * @param server    True to print this phrase to server, along with all clients in the game.
 * @param admin     True to only print to clients with access defined by accessmanager.inc.
 *                  This is forced to false if the access manager isn't compiled into the plugin.
 * @param msgformat How to format the text before sending.  See enum MsgFormatting.
 * @param msgtype   The type of message to print to the clients.  See enum MsgTypes.
 * @param literal   True to print the last variable as a string, instead of translating it.
 * @param any       The translations phrase follow by formatting parameters.
 * 
 * @error           Function doesn't support MsgType_Server or MsgType_Reply because there's no target client.
 */
stock TransMgr_PrintTextAll(bool:server, bool:admin, MsgFormatting:msgformat, MsgTypes:msgtype, Module:module = INVALID_MODULE, bool:literal = false, any:...)
{
    #if !defined ACCESS_MANAGER
        admin = false;
    #endif
    
    // There's only 1 server.
    if (msgtype == MsgType_Server)
    {
        ThrowError("TransMgr_PrintTextAll doesn't support MsgType_Server.");
    }
    
    // Can't reply to all clients simultaneously.
    if (msgtype == MsgType_Reply)
    {
        ThrowError("TransMgr_PrintTextAll doesn't support MsgType_Reply.");
    }
    
    decl String:translated[512]; // Give plenty of room for long messages, better too much than not enough.
    
    // Print the phrase in the server's language.
    if (server)
    {
        SetGlobalTransTarget(LANG_SERVER);
        
        // Method for getting text from the ... param depends on if literal is true or false.
        if (literal)
            VFormat(translated, sizeof(translated), "%s", 7);
        else
            VFormat(translated, sizeof(translated), "%t", 7);
        
        // Remove color tags before printing in server console.
        #if defined _colors_included
            CRemoveTags(translated, sizeof(translated));
        #endif
        
        TransMgr_PrintText(SERVER_INDEX, msgformat, MsgType_Server, module, true, translated);
    }
    
    for (new client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client))
            continue;
        
        // If the client must be an admin, and the access manager says they aren't, then skip the client.
        if (admin && !AccessMgr_HasAccess(client, module))
            continue;
        
        // Print the phrase in the client's language.
        SetGlobalTransTarget(client);
        
        // Method for getting text from the ... param depends on if literal is true or false.
        if (literal)
            VFormat(translated, sizeof(translated), "%s", 7);
        else
            VFormat(translated, sizeof(translated), "%t", 7);
        
        TransMgr_PrintText(client, msgformat, msgtype, module, true, translated);
    }
}

/**
 * Replies to a client with a given message describing a targetting 
 * failure reason. (formatted for project)
 *
 * Note: The translation phrases are found in common.phrases.txt.
 *
 * @param client		Client index, or 0 for server.
 * @param reason		COMMAND_TARGET reason.
 */
stock TransMgr_ReplyToTargetError(client, reason)
{
    switch (reason)
    {
        case COMMAND_TARGET_NONE:         TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No matching client");
        case COMMAND_TARGET_NOT_ALIVE:    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Target must be alive");
        case COMMAND_TARGET_NOT_DEAD:     TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Target must be dead");
        case COMMAND_TARGET_NOT_IN_GAME:  TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Target is not in game");
        case COMMAND_TARGET_IMMUNE:       TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Unable to target");
        case COMMAND_TARGET_EMPTY_FILTER: TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "No matching clients");
        case COMMAND_TARGET_NOT_HUMAN:    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "Cannot target bot");
        case COMMAND_TARGET_AMBIGUOUS:    TransMgr_PrintText(client, MsgFormat_Plugin, MsgType_Reply, INVALID_MODULE, false, "More than one client matched");
    }
}

/**
 * Translates a boolean value into a text phrase.
 * 
 * @param target        The client index.  LANG_SERVER to use server's language.
 * @param boolean       The boolean value to translate.
 * @param boolphrase    The boolean phrase pair to translate to.
 * @param phrase        The output string with the translated prase.
 * @param maxlen        The max length of the output string.
 * @param lowercase     True to force the output to lower case.
 */
stock TransMgr_BoolToPhrase(target, bool:boolean, BoolPhrases:boolphrase, String:phrase[], maxlen, bool:lowercase = false)
{
    SetGlobalTransTarget(target);
    
    switch (boolphrase)
    {
        case BoolPhrase_YesNo:
            boolean ? Format(phrase, maxlen, "%t", "Yes") : Format(phrase, maxlen, "%t", "No");
        
        case BoolPhrase_OnOff:
            boolean ? Format(phrase, maxlen, "%t", "On") : Format(phrase, maxlen, "%t", "Off");
        
        case BoolPhrase_EnabledDisabled:
            boolean ? Format(phrase, maxlen, "%t", "_Enabled") : Format(phrase, maxlen, "%t", "_Disabled");
    }
    
    // Force all chars to lowercase.
    if (lowercase)
    {
        new length = strlen(phrase);
        for (new x = 0; x < length; x++)
        {
            phrase[x] = CharToLower(phrase[x]);
        }
    }
}

/**
 * Translates a text phrase into a boolean value.
 * 
 * @param target        The client index.  LANG_SERVER to use server's language.
 * @param boolphrase    The boolean phrase pair to translate from.
 * @param phrase        The bool phrase to translate to a bool.
 * 
 * @return              A value from enum PhraseToBool.  See top of file.
 */
stock PhraseToBool:TransMgr_PhraseToBool(target, BoolPhrases:boolphrase, const String:phrase[])
{
    SetGlobalTransTarget(target);
    
    decl String:boolphrasetrue[16];
    decl String:boolphrasefalse[16];
    switch (boolphrase)
    {
        case BoolPhrase_YesNo:
        {
            Format(boolphrasetrue, sizeof(boolphrasetrue), "%t", "Yes");
            Format(boolphrasefalse, sizeof(boolphrasefalse), "%t", "No");
        }
        case BoolPhrase_OnOff:
        {
            Format(boolphrasetrue, sizeof(boolphrasetrue), "%t", "On");
            Format(boolphrasefalse, sizeof(boolphrasefalse), "%t", "Off");
        }
        case BoolPhrase_EnabledDisabled:
        {
            Format(boolphrasetrue, sizeof(boolphrasetrue), "%t", "_Enabled");
            Format(boolphrasefalse, sizeof(boolphrasefalse), "%t", "_Disabled");
        }
    }
    
    if (StrEqual(phrase, boolphrasetrue, false))
        return PhraseToBool_True;
    
    if (StrEqual(phrase, boolphrasefalse, false))
        return PhraseToBool_False;
    
    return PhraseToBool_Neither;
}

/**
 * Translates a text phrase into a boolean value.  Uses english language.  Use this for config files.
 * 
 * @param boolphrase    The boolean phrase pair to translate from.  See enum BoolPhrases.
 * @param phrase        The bool phrase to translate to a bool.
 * 
 * @return              A value from enum PhraseToBool.  See top of file.
 */
stock PhraseToBool:TransMgr_PhraseToBoolEx(BoolPhrases:boolphrase, const String:phrase[])
{
    decl String:boolphrasetrue[16];
    decl String:boolphrasefalse[16];
    switch (boolphrase)
    {
        case BoolPhrase_YesNo:
        {
            strcopy(boolphrasetrue, sizeof(boolphrasetrue), "yes");
            strcopy(boolphrasefalse, sizeof(boolphrasefalse), "no");
        }
        case BoolPhrase_OnOff:
        {
            strcopy(boolphrasetrue, sizeof(boolphrasetrue), "on");
            strcopy(boolphrasefalse, sizeof(boolphrasefalse), "off");
        }
        case BoolPhrase_EnabledDisabled:
        {
            strcopy(boolphrasetrue, sizeof(boolphrasetrue), "enable");
            strcopy(boolphrasefalse, sizeof(boolphrasefalse), "disable");
        }
    }
    
    if (StrEqual(phrase, boolphrasetrue, false))
        return PhraseToBool_True;
    
    if (StrEqual(phrase, boolphrasefalse, false))
        return PhraseToBool_False;
    
    return PhraseToBool_Neither;
}

/**
 * Same as TransMgr_PhraseToBoolEx, except gets the phrase from a keyvalues structure.
 * 
 * @param kv            The kv handle.
 * @param key           The keyvalue key to get value from.
 * @param boolphrase    The boolean phrase pair to translate from.  See enum BoolPhrases.
 * @param defvalue      The string value used if the key doesn't exist.
 * 
 * @return              A value from enum PhraseToBool.  See top of file.
 */
stock PhraseToBool:TransMgr_KvPhraseToBoolEx(Handle:kv, BoolPhrases:boolphrase, const String:key[], const String:defvalue[] = "no")
{
    decl String:phrase[16];
    KvGetString(kv, key, phrase, sizeof(phrase), defvalue);
    
    return TransMgr_PhraseToBoolEx(boolphrase, phrase);
}

/**
 * Interprets a PhraseToBool enum member as a boolean value.
 * 
 * @param phrasebool    The PhraseToBool member to interpret.
 * @param neither       The boolean value to return for PhraseToBool_Neither
 */
stock bool:TransMgr_InterpretPhraseBool(PhraseToBool:phrasebool, bool:neither)
{
    return (phrasebool == PhraseToBool_Neither) ? neither : bool:phrasebool;
}

// **********************************************
//   Private API (For base project files only)
// **********************************************

/**
 * (Re)loads all project translation files.
 */
stock TransMgr_LoadTranslations()
{
    #if defined TM_BASE_DIR
        // Build a path to the directory with translations.
        decl String:fulltransfilepath[PLATFORM_MAX_PATH];
        BuildPath(Path_SM, fulltransfilepath, sizeof(fulltransfilepath), "translations/%s", TM_BASE_DIR);
        
        // Open directory, log errors if it doesn't exist.
        new Handle:hDir = OpenDirectory(fulltransfilepath);
        if (hDir == INVALID_HANDLE)
        {
            LogError("Translations file directory (%s) does not exist on the server", fulltransfilepath);
            return;
        }
        
        decl String:filename[64];
        new const String:fileext[32] = ".phrases.txt";
        new FileType:type;
        decl String:transfilepath[PLATFORM_MAX_PATH];
        
        // Loop through all the files in the directory.
        while (ReadDirEntry(hDir, filename, sizeof(filename), type))
        {
            if (type != FileType_File)
                continue;
            
            // Load the file if it ends with ".phrases.txt"
            if (StrContains(filename, fileext, false) == strlen(filename) - strlen(fileext))
            {
                // Format a path to the file, relative to the "translations" folder.
                Format(transfilepath, sizeof(transfilepath), "%s/%s", TM_BASE_DIR, filename);
                LoadTranslations(transfilepath);
            }
        }
        CloseHandle(hDir);
        LogMessage("[Translations Manager] Successfully loaded all translation files from %s", fulltransfilepath);
    #endif
    
    // Loads extra translations from the main project.inc file.
    Project_LoadExtraTranslations(true);
}
