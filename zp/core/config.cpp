/**
 * ============================================================================
 *
 *  Zombie Plague Mod #3 Generation
 *
 *  File:          config.inc
 *  Type:          Core
 *  Description:   Config API and executing.
 *
 *  Copyright (C) 2015-2018  Greyscale, Richard Helgeby
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
 **/

/*
    Using config API:
    
    -Before any of these helper functions can be used on a config file you must
     "register" the module handling the data.
    
    Example:
    
    ConfigRegisterConfig(File_Example, Structure_List, "example");
    
    * The first parameter of this call is the config file we want to register.
      this needs to be listed in the "ConfigFile" enum in config.inc.
      
    * The second parameter is the structure of the config file we are loading.
      The supported structures are listed in the "ConfigStructure" enum in config.inc
      
    * The last parameter is the file alias.  Or what we use to refer to the
      config file from a non-developer point of view.  For example zp_config_reload
      requires the file alias to identify the config file the user wants to reload.
    
    -Next we need to define the config file path.  To do this we first need to
     retrieve the path file from cvar.
     
    Example:
    
    ConfigSetConfigPath(File_Example, pathexample);
    
    * The first parameter is the config file we want to set path to.
      
    * The second parameter is the path we want to set to the config file.
    
    -Next we load config file and prepare its nested array structure.
    
    Example:
    
    new bool:success = ConfigLoadConfig(File_Example, arrayExample);
    
    * The first parameter is the config file we want to load.
      
    * The second parameter is the array handle we want to prepare data structure in.
      
    * The return value is true if the file was successfully loaded, false if the
      config file couldn't be loaded.  (Invalid data, missing quotes, brackets, etc)
    
    -Next validate the config so far, stopping if no data was found or ifConfigLoadConfig
     returned false.
    
    -Then cache the config file data into the arrays (only for Keyvalue structures)
     by iterating through the data and pushing the values into the array.
    
    -Validate the values of the data.
    
    -Lastly we need to set specific info to the module now that it has successfully
     loaded.
    
    Example:
    
    ConfigSetConfigLoaded(File_Example, true);
    ConfigSetConfigReloadFunc(File_Example, GetFunctionByName(GetMyHandle(), "ExampleOnConfigReload"));
    ConfigSetConfigHandle(File_Example, arrayExample);
    
    These functions will modify the config file data for other things to use.
    (such as zp_config_reload)
    
    * The first function call will set the loaded state of the config file to
      true, failing to do this will cause the config module to think your
      config file isn't loaded, therefore causing some undesired erroring.
      
    * The second function sets the reload function of the config file.  This
      function will be called upon its config file being reloaded.
      
    * The third function stores the array handle for use by other parts of the
      module.
*/

/**
 * The max length of any config string value.
 **/
#define CONFIG_MAX_LENGTH 64

/**
 * @section Config file reference aliases.
 **/
#define CONFIG_FILE_ALIAS_DOWNLOADS     "downloads"
#define CONFIG_FILE_ALIAS_WEAPONS       "weapons"
#define CONFIG_FILE_ALIAS_SOUNDS        "sounds"
#define CONFIG_FILE_ALIAS_MENUS         "menus"
#define CONFIG_FILE_ALIAS_HITGROUPS     "hitgroups"
#define CONFIG_FILE_ALIAS_COSTUMES      "costumes"
/**
 * @endsection
 **/

/**
 * List of config formats used by the plugin.
 **/
enum ConfigStructure
{
    Structure_List,         /** Config is structured as a simple list of strings. */
    Structure_ArrayList,    /** Config is structured as an array list of strings. */
    Structure_Keyvalue,     /** Config is a keyvalue structure */
}

/**
 * List of config files used by the plugin.
 **/
enum ConfigFile
{
    File_Invalid = -1,      /** Invalid config file. */
    File_Downloads,         /** <sourcemod root>/zombieplague/downloads.ini (default) */
    File_Weapons,           /** <sourcemod root>/zombieplague/weapons.ini (default) */
    File_Sounds,            /** <sourcemod root>/zombieplague/sounds.ini (default) */
    File_Menus,             /** <sourcemod root>/zombieplague/menus.ini (default) */
    File_Hitgroups,         /** <sourcemod root>/zombieplague/hitgroups.ini (default) */
    File_Costumes           /** <sourcemod root>/zombieplague/costumes.ini (default) */
}

/**
 * Data container for each config file.
 **/
enum ConfigData
{
    bool:Data_Loaded,                       /** True if config is loaded, false if not. */
    ConfigStructure:Data_Structure,         /** Format of the config */
    Function:Data_ReloadFunc,               /** Function to call to reload config. */
    Handle:Data_Handle,                     /** Handle of the config file. */
    String:Data_Path[PLATFORM_MAX_PATH],    /** Full path to config file. */
    String:Data_Alias[CONFIG_MAX_LENGTH],   /** Config file alias, used for client interaction. */
}

/**
 * Stores all config data.
 **/
int gConfigData[ConfigFile][ConfigData];

/**
 * Actions to use when working on key/values.
 **/
enum ConfigKvAction
{
    KvAction_Create,    /** Create a key. */
    KvAction_KVDelete,  /** Delete a key. */
    KvAction_KVSet,     /** Modify setting of a key. */
    KvAction_KVGet,     /** Get setting of a key. */
}

/**
 * Create commands related to config here.
 **/
void ConfigOnCommandsCreate(/*void*/)
{
    // Create config admin commands
    RegAdminCmd("zp_config_reload", Command_ReloadCommand, ADMFLAG_CONFIG, "Reloads a config file. Usage: zp_config_reload <file alias>");
    RegAdminCmd("zp_config_reloadall", Command_ReloadAllCommand, ADMFLAG_CONFIG, "Reloads all config files. Usage: zp_config_reloadall");
}

/**
 * Config module init function.
 */
void ConfigInit(/*void*/)
{
    // Loads a gamedata configs file
    gServerData[Server_GameConfig][Game_Zombie] = LoadGameConfigFile(PLUGIN_CONFIG);

    // Validate config
    if(gServerData[Server_GameConfig][Game_Zombie] == INVALID_HANDLE)
    {
        // Log failure
        LogEvent(false, LogType_Fatal, LOG_CORE_EVENTS, LogModule_Config, "Config Validation", "Missing gamedata config file: \"%s\"", PLUGIN_CONFIG);
    }
    
    // Load others standart sourcemod configs
    gServerData[Server_GameConfig][Game_SDKHooks] = LoadGameConfigFile("sdkhooks.games");
    gServerData[Server_GameConfig][Game_SDKTools] = LoadGameConfigFile("sdktools.games");
    gServerData[Server_GameConfig][Game_CStrike] = LoadGameConfigFile("sm-cstrike.games");
}

/**
 * Load plugin configs. Executes map configs.
 **/
void ConfigLoad(/*void*/)
{
    // Resets number of rounds
    gServerData[Server_RoundNumber] = 0;
    
    // Gets current map name
    GetCurrentMap(gServerData[Server_MapName], sizeof(gServerData[Server_MapName]));
    
    // Initialize variables
    static char sFile[NORMAL_LINE_LENGTH];
    static char sPath[PLATFORM_MAX_PATH];

    // Format full path to main config
    Format(sFile, sizeof(sFile), "sourcemod/zombieplague.cfg"); 
    Format(sPath, sizeof(sPath), "cfg/%s", sFile); 
    
    // Check if the file exist
    if(FileExists(sPath))
    {
        // Execute config file
        ServerCommand("exec %s", sFile);
        
        // Log action
        LogEvent(true, LogType_Normal, LOG_CORE_EVENTS, LogModule_Config, "Map Configs", "Executed main config file: \"%s\"", sPath);
    }

    // Format full path to map config
    Format(sFile, sizeof(sFile), "sourcemod/zombieplague/%s.cfg", gServerData[Server_MapName]); 
    Format(sPath, sizeof(sPath), "cfg/%s", sFile); 
    
    // Check if the file exist
    if(FileExists(sPath))
    {
        // Execute config file
        ServerCommand("exec %s", sFile);

        // Log action
        LogEvent(true, LogType_Normal, LOG_CORE_EVENTS, LogModule_Config, "Map Configs", "Executed map config file: \"%s\"", sPath);
    }
}

/**
 * Unload plugin configs.
 **/
void ConfigUnload(/*void*/)
{
    // Unload the gamedata config
    delete gServerData[Server_GameConfig][Game_Zombie];
    delete gServerData[Server_GameConfig][Game_SDKHooks];
    delete gServerData[Server_GameConfig][Game_SDKTools];
    delete gServerData[Server_GameConfig][Game_CStrike];
}

/**
 * Used by modules that rely on configs to register their config file info.
 * (Don't forget to set 'loaded' to 'true' (ConfigSetConfigLoaded) in config load function)
 *
 * @param iFile              Config file entry to register.
 * @param sAlias             Config file alias, used for client interaction.
 **/
stock void ConfigRegisterConfig(const ConfigFile iFile, const ConfigStructure iStructure, const char[] sAlias = "")
{
    // Copy file info to data container
    gConfigData[iFile][Data_Loaded] = false;
    gConfigData[iFile][Data_Structure] = iStructure;
    gConfigData[iFile][Data_Handle] = INVALID_HANDLE;
    gConfigData[iFile][Data_ReloadFunc] = INVALID_FUNCTION;
    strcopy(gConfigData[iFile][Data_Path], PLATFORM_MAX_PATH, "");
    strcopy(gConfigData[iFile][Data_Alias], CONFIG_MAX_LENGTH, sAlias);
}

/**
 * Set the loaded state of a config file entry.
 * 
 * @param iConfig            Config file to set load state of.
 * @param bLoaded            True to set as loaded, false to set as unloaded.
 **/
stock void ConfigSetConfigLoaded(const ConfigFile iConfig, const bool bLoaded)
{
    // Sets load state
    gConfigData[iConfig][Data_Loaded] = bLoaded;
}

/**
 * Set the structure type of a config file entry.
 * 
 * @param iConfig            Config file to set structure type of.
 * @param iStructure         Structure to set as.
 **/
stock void ConfigSetConfigStructure(const ConfigFile iConfig, const ConfigStructure iStructure)
{
    // Sets load state
    gConfigData[iConfig][Data_Structure] = iStructure;
}

/**
 * Set the reload function of a config file entry.
 * 
 * @param iConfig            Config file to set reload function of.
 * @param iReloadfunc        Reload function.
 **/
stock void ConfigSetConfigReloadFunc(const ConfigFile iConfig, const Function iReloadfunc)
{
    // Sets reload function.
    gConfigData[iConfig][Data_ReloadFunc] = iReloadfunc;
}

/**
 * Set the file handle of a config file entry.
 * 
 * @param iConfig            Config file to set handle of.
 * @param iFile              Config file handle.
 **/
stock void ConfigSetConfigHandle(const ConfigFile iConfig, Handle iFile)
{
    // Sets file handle
    gConfigData[iConfig][Data_Handle] = iFile;
}

/**
 * Set the config file path of a config file entry.
 * 
 * @param iConfig            Config file to set file path of.
 * @param sPath              File path.
 **/
stock void ConfigSetConfigPath(const ConfigFile iConfig, const char[] sPath)
{
    // Sets config file path
    strcopy(gConfigData[iConfig][Data_Path], PLATFORM_MAX_PATH, sPath);
}

/**
 * Set the alias of a config file entry.
 * 
 * @param iConfig            Config file to set alias of.
 * @param sAlias             Alias of the config file entry.
 **/
stock void ConfigSetConfigAlias(const ConfigFile iConfig, const char[] sAlias)
{
    // Sets config alias
    strcopy(gConfigData[iConfig][Data_Alias], CONFIG_MAX_LENGTH, sAlias);
}

/**
 * Returns if a config was successfully loaded.
 * 
 * @param iConfig            Config file to check load status of.
 * @return                   True if config is loaded, false otherwise.
 **/
stock bool ConfigIsConfigLoaded(const ConfigFile iConfig)
{
    // Return load status
    return gConfigData[iConfig][Data_Loaded];
}

/**
 * Returns config structure type.
 * 
 * @param iConfig            Config file to get structure type of.
 * @return                   Config structure type.
 **/
stock ConfigStructure ConfigGetConfigStructure(const ConfigFile iConfig)
{
    // Return load status
    return gConfigData[iConfig][Data_Structure];
}

/**
 * Returns config reload function.
 * 
 * @param iConfig            Config file to get reload function of.
 * @return                   Config reload function.
 **/
stock Function ConfigGetConfigReloadFunc(const ConfigFile iConfig)
{
    // Return load status
    return gConfigData[iConfig][Data_ReloadFunc];
}

/**
 * Returns config file handle.
 * 
 * @param iConfig            Config file to get file handle of.
 * @return                   Config file handle.
 **/
stock Handle ConfigGetConfigHandle(const ConfigFile iConfig)
{
    // Return load status
    return gConfigData[iConfig][Data_Handle];
}

/**
 * Returns the path for a given config file entry.
 * 
 * @param iConfig            Config file to get path of. (see ConfigFile enum)
 **/
stock void ConfigGetConfigPath(const ConfigFile iConfig, char[] sPath, const int iMaxLen)
{
    // Copy path to return string
    strcopy(sPath, iMaxLen, gConfigData[iConfig][Data_Path]);
}

/**
 * Returns the alias for a given config file entry.
 * 
 * @param iConfig            Config file to get alias of. (see ConfigFile enum)
 **/
stock void ConfigGetConfigAlias(const ConfigFile iConfig, char[] sAlias, const int iMaxLen)
{
    // Copy alias to return string
    strcopy(sAlias, iMaxLen, gConfigData[iConfig][Data_Alias]);
}

/**
 * Loads a config file and sets up a nested array type data storage.
 * 
 * @param iConfig            The config file to load.
 * @param arrayConfig        Handle of the main array containing file data.
 * @param blockSize          The max length of the contained strings. 
 * @return                   True if file was loaded successfuly, false otherwise.
 **/
stock bool ConfigLoadConfig(const ConfigFile iConfig, ArrayList &arrayConfig, const int blockSize = CONFIG_MAX_LENGTH)
{
    // Gets config structure
    ConfigStructure iStructure = ConfigGetConfigStructure(iConfig);

    // Gets config alias
    static char sConfigAlias[CONFIG_MAX_LENGTH];
    ConfigGetConfigAlias(iConfig, sConfigAlias, sizeof(sConfigAlias));

    // Gets config file path
    static char sConfigPath[PLATFORM_MAX_PATH];
    ConfigGetConfigPath(iConfig, sConfigPath, sizeof(sConfigPath));

    // If array hasn't been created, then create
    if(arrayConfig == INVALID_HANDLE)
    {
        // Create array in handle
        arrayConfig = CreateArray(blockSize);
    }

    switch(iStructure)
    {
        case Structure_List :
        {
            // Open file
            Handle hFile;
            bool bSuccess = ConfigOpenConfigFile(iConfig, hFile);

            // If config file failed to open, then stop
            if(!bSuccess)
            {
                return false;
            }

            // Clear out array
            arrayConfig.Clear();

            static char sLine[PLATFORM_MAX_PATH];

            while(ReadFileLine(hFile, sLine, sizeof(sLine)))
            {
                // Cut out comments at the end of a line
                if(StrContains(sLine, "//") != -1)
                {
                    SplitString(sLine, "//", sLine, sizeof(sLine));
                }

                // Trim off whitespace
                TrimString(sLine);

                // If line is empty, then stop
                if(!strlen(sLine))
                {
                    continue;
                }

                // Push line into array
                arrayConfig.PushString(sLine);
            }

            // We're done this file, so now we can destory it from memory
            delete hFile;
            return true;
        }

        case Structure_ArrayList :
        {
            // Open file
            Handle hFile;
            bool bSuccess = ConfigOpenConfigFile(iConfig, hFile);
            
            // If config file failed to open, then stop
            if(!bSuccess)
            {
                return false;
            }
            
            // Destroy all old data
            ConfigClearKvArray(arrayConfig);
            
            static char sLine[PLATFORM_MAX_PATH];
            
            while(ReadFileLine(hFile, sLine, sizeof(sLine)))
            {
                // Cut out comments at the end of a line
                if(StrContains(sLine, "//") != -1)
                {
                    SplitString(sLine, "//", sLine, sizeof(sLine));
                }

                // Trim off whitespace
                TrimString(sLine);

                // If line is empty, then stop
                if(!strlen(sLine))
                {
                    continue;
                }

                // Create new array to store information for config entry
                ArrayList arrayConfigEntry = CreateArray(blockSize);

                // Push line into array
                arrayConfigEntry.PushString(sLine); // Index: 0

                // Store this handle in the main array
                arrayConfig.Push(arrayConfigEntry);
            }
            
            // We're done this file, so now we can destory it from memory
            delete hFile;
            return true;
        }
        
        case Structure_Keyvalue :
        {
            // Open file
            KeyValues hKeyvalue;
            bool bSuccess = ConfigOpenConfigFile(iConfig, hKeyvalue);
            
            // If config file failed to open, then stop
            if(!bSuccess)
            {
                return false;
            }
            
            // Destroy all old data
            ConfigClearKvArray(arrayConfig);
            
            if(hKeyvalue.GotoFirstSubKey())
            {
                do
                {
                    // Create new array to store information for config entry
                    ArrayList arrayConfigEntry = CreateArray(blockSize);
                    
                    // Push the key name into the config entry array
                    static char sKeyName[CONFIG_MAX_LENGTH];
                    hKeyvalue.GetSectionName(sKeyName, sizeof(sKeyName));
                    
                    arrayConfigEntry.PushString(sKeyName); // Index: 0
                    
                    // Store this handle in the main array
                    arrayConfig.Push(arrayConfigEntry);
                } 
                while(hKeyvalue.GotoNextKey());
            }
            
            // We're done this file for now, so now we can destory it from memory 
            delete hKeyvalue;
            return true;
        }
    }

    return false;
}

/**
 * Reload a config file.
 * 
 * @param iConfig            The config file entry to reload.
 * @return                   True if the config is loaded, false if not.
 **/
stock bool ConfigReloadConfig(const ConfigFile iConfig)
{
    // If file isn't loaded, then stop
    bool bLoaded = ConfigIsConfigLoaded(iConfig);
    if(!bLoaded)
    {
        return false;
    }
    
    // Call reload function
    Function iReloadfunc = ConfigGetConfigReloadFunc(iConfig);
    
    // Call reload function
    Call_StartFunction(GetMyHandle(), iReloadfunc);
    Call_Finish();
    
    // Return on the success
    return true;
}

/**
 * Opens a config file with appropriate method.
 * 
 * @param iConfig            The config file.
 * @param iStructure         The structure of the config file.
 * @param hConfig            The handle of the opened file.
 **/
stock bool ConfigOpenConfigFile(const ConfigFile iConfig, Handle &hConfig)
{
    // Gets config structure
    ConfigStructure iStructure = ConfigGetConfigStructure(iConfig);
    
    // Gets config file path
    static char sConfigPath[PLATFORM_MAX_PATH];
    ConfigGetConfigPath(iConfig, sConfigPath, sizeof(sConfigPath));
    
    // Gets config alias
    static char sConfigAlias[CONFIG_MAX_LENGTH];
    ConfigGetConfigAlias(iConfig, sConfigAlias, sizeof(sConfigAlias));
    
    switch(iStructure)
    {
        case Structure_Keyvalue :
        {
            hConfig = CreateKeyValues(sConfigAlias);
            return FileToKeyValues(hConfig, sConfigPath);
        }
        
        default :
        {
            // Open file
            hConfig = OpenFile(sConfigPath, "r");
            
            // If file couldn't be opened, then stop
            if(hConfig == INVALID_HANDLE)
            {
                return false;
            }
            
            return true;
        }
    }
    
    return false;
}

/**
 * Creates, deletes, sets, or gets any key/setting of any ZP config keyvalue file in memory.
 * Only use when interacting with a command or manipulating single keys/values,
 * using this function everywhere would be EXTREMELY inefficient.
 * 
 * @param iConfig           Config file to modify.
 * @param iAction           Action to perform on keyvalue tree. (see enum ConfigKeyvalueAction)
 * @param sKeys             Array containing keys to traverse into.
 * @param keysMax           The size of the 'keys' array.
 * @param sSetting          (Optional) The name of the setting to modify.
 * @param sValue            (Optional) The new value to set.
 * @param iMaxLen           (Optional) The maxlength of the retrieved value.
 * @return                  True if the change was made successfully, false otherwise. 
 **/
stock bool ConfigKeyvalueTreeSetting(const ConfigFile iConfig, const ConfigKvAction iAction = KvAction_Create, const char[][] sKeys, const int keysMax, const char[] sSetting = "", const char[] sValue = "", const int iMaxLen = 0)
{
    // Gets config file structure
    ConfigStructure iStructure = ConfigGetConfigStructure(iConfig);
    
    // If the config is any other structure beside keyvalue, then stop
    if(iStructure != Structure_Keyvalue)
    {
        return false;
    }
    
    // Retrieve handle of the keyvalue tree
    Handle hConfig;
    bool bSuccess = ConfigOpenConfigFile(iConfig, hConfig);
    
    // If the file couldn't be opened, then stop
    if(!bSuccess)
    {
        return false;
    }
    
    // Rewind keyvalue tree
    KvRewind(hConfig);
    
    // i = keys index.
    // Traverse into the keygroup, stop if it fails
    for(int i = 0; i < keysMax; v++)
    {
        // If key is empty, then break the loop
        if(!strlen(sKeys[i]))
        {
            break;
        }
        
        // Try to jump to next level in the transversal stack, create key if specified
        bool bExists = KvJumpToKey(hConfig, sKeys[i], (iAction == KvAction_Create));
        
        // If exists is false, then stop
        if(!bExists)
        {
            // Key doesn't exist
            return false;
        }
    }
    
    switch(iAction)
    {
        case KvAction_Create :
        {
            if(!strlen(sSetting) || !strlen(sValue))
            {
                // We created the key already, so return true
                return true;
            }
            
            // Sets new value
            KvSetString(hConfig, sSetting, sValue);
        }
        case KvAction_Delete :
        {
            // Return deletion result
            return KvDeleteKey(hConfig, sSetting);
        }
        case KvAction_Set :
        {
            // Sets new value
            KvSetString(hConfig, sSetting, sValue);
        }
        case KvAction_Get:
        {
            // Gets current value
            KvGetString(hConfig, sSetting, sValue, iMaxLen);
        }
    }
    
    // We successfully set or got the value
    return true;
}

/**
 * Destroy all array handles within an array, and clear main array.
 * 
 * @param arrayKv           The array converted from a keyvalue structure.
 **/
void ConfigClearKvArray(const ArrayList arrayKv)
{
    //  i = array index
    int iSize = arrayKv.Length;
    for(int i = 0; i < iSize; i++)
    {
        // Destroy nested arrays
        ArrayList arrayKvKey = arrayKv.Get(i);
        delete arrayKvKey;
    }
    
    // Now that all data within has been destroyed, we can clear the main array
    arrayKv.Clear();
}

/**
 * Finds a config file entry, (see ConfigFile enum) for a given alias.
 * 
 * @param sAlias            The alias to find config file entry of.
 *
 * @return                  Config file entry, ConfigInvalid is returned if alias was not found.
 **/
stock ConfigFile ConfigAliasToConfigFile(const char[] sAlias)
{
    static char sCheckAlias[CONFIG_MAX_LENGTH];
    
    // i = config file entry index
    for(int i = 0; i < sizeof(gConfigData); i++)
    {
        // Gets config alias.
        ConfigGetConfigAlias(view_as<ConfigFile>(i), sCheckAlias, sizeof(sCheckAlias));
        
        // If alias doesn't match, then skip
        if(!strcmp(sAlias, sCheckAlias, false))
        {
            // Return config file entry
            return view_as<ConfigFile>(i);
        }
    }
    
    // Invalid config file
    return File_Invalid;
}

/**
 * Command callback <!zp_config_reload>
 * Reloads a config file and forwards event to modules.
 * 
 * @param clientIndex        The client index.
 * @param iArguments         The number of arguments that were in the argument string.
 **/
public Action Command_ReloadCommand(const int clientIndex, const int iArguments)
{
    // If not enough arguments given, then stop
    if(iArguments < 1)
    {
        TranslationReplyToCommand(clientIndex, "config reload");
        TranslationReplyToCommand(clientIndex, "config reload commands");
        TranslationReplyToCommand(clientIndex, "config reload commands aliases", CONFIG_FILE_ALIAS_DOWNLOADS, CONFIG_FILE_ALIAS_WEAPONS, CONFIG_FILE_ALIAS_SOUNDS, CONFIG_FILE_ALIAS_MENUS, CONFIG_FILE_ALIAS_HITGROUPS, CONFIG_FILE_ALIAS_COSTUMES);
        return Plugin_Handled;
    }

    static char sFileAlias[CONFIG_MAX_LENGTH];
    static char sPath[PLATFORM_MAX_PATH];
    static char sLogMessage[PLATFORM_MAX_PATH];

    int iArgs = GetCmdArgs();
    for(int i = 1; i <= iArgs; i++)
    {
        // Gets alias to restrict
        GetCmdArg(i, sFileAlias, sizeof(sFileAlias));

        // If alias is invalid, then stop
        ConfigFile iConfig = ConfigAliasToConfigFile(sFileAlias);
        if(iConfig == File_Invalid)
        {
            TranslationReplyToCommand(clientIndex, "config reload invalid");
            return Plugin_Handled;
        }

        // Reload config file
        bool bLoaded = ConfigReloadConfig(iConfig);

        // Gets config file path
        ConfigGetConfigPath(iConfig, sPath, sizeof(sPath));

        // Format log message
        Format(sLogMessage, sizeof(sLogMessage), "[%N] reloaded config file \"%s\". (zp_config_reload)", clientIndex, sPath);

        // If file isn't loaded then tell client, then stop
        if(!bLoaded)
        {
            TranslationReplyToCommand(clientIndex, "config reload not load");

            // Format a failed attempt string to the end of the log message
            Format(sLogMessage, sizeof(sLogMessage), "\"%s\" -- attempt failed, config file not loaded", sLogMessage);
        }

        // Log action to game events
        LogEvent(true, LogType_Command, LOG_GAME_EVENTS, LogModule_Admin, "Admin Command", sLogMessage);
    }
    return Plugin_Handled;
}

/**
 * Command callback <!zp_config_reloadall>
 * Reloads all config files and forwards event to all modules.
 * 
 * @param clientIndex        The client index.
 * @param iArguments         The number of arguments that were in the argument string.
 **/
public Action Command_ReloadAllCommand(const int clientIndex, const int iArguments)
{
    // Begin statistics
    TranslationReplyToCommand(clientIndex, "config reload begin");

    static char sConfigAlias[CONFIG_MAX_LENGTH];

    // i = config file entry index
    for(int i = 0; i < sizeof(gConfigData); i++)
    {
        // Reload config file
        bool bSuccessful = ConfigReloadConfig(view_as<ConfigFile>(i));

        // Gets config alias
        ConfigGetConfigAlias(view_as<ConfigFile>(i), sConfigAlias, sizeof(sConfigAlias));

        if(bSuccessful)
        {
            TranslationReplyToCommand(clientIndex, "config reload finish");
        }
        else
        {
            TranslationReplyToCommand(clientIndex, "config reload falied");
        }
    }

    // Log action to game events
    LogEvent(true, LogType_Command, LOG_GAME_EVENTS, LogModule_Admin, "Admin Command", "[%N] reloaded all config files", clientIndex);
    return Plugin_Handled;
}

/**
 * Load config file.
 * 
 * @param iConVar              The cvar define of the path to the file.
 * @param sPath                The path output.
 * @return                     True if the file exists, false if not.
 **/
stock bool ConfigGetCvarFilePath(const CvarsList iConVar, char[] sPath)
{
    // Gets cvar path
    static char sFilePath[PLATFORM_MAX_PATH];
    gCvarList[iConVar].GetString(sFilePath, sizeof(sFilePath));

    // Build full path in return string
    BuildPath(Path_SM, sPath, PLATFORM_MAX_PATH, sFilePath);

    // If file is exist, then return true
    return FileExists(sPath);
}

/**
 * Converts string of "yes/on" or "no/off" to a boolean value.  Always uses english as yes/no/on/off language.
 * 
 * @param sOption           "yes/on" or "no/off" string to be converted.
 * @return                  True if string is "yes", false otherwise.
 **/
stock bool ConfigSettingToBool(const char[] sOption)
{
    // If option is equal to "yes", then return true
    if(!strcmp(sOption, "yes", false) || !strcmp(sOption, "on", false) || !strcmp(sOption, "1", false))
    {
        return true;
    }
    
    // Option isn't yes
    return false;
}

/**
 * Converts boolean value to "yes" or "no".
 * 
 * @param bOption           True/false value to be converted to "yes/on"/"no/off", respectively.
 * @param sOption           Destination string buffer to store "yes/on" or "no/off" in.
 * @param iMaxLen           Length of destination string buffer.
 * @param bYesNo            When true, returns "yes/no", false returns "on/off."
 * @param targetIndex       The target to use as translation language.
 **/
stock void ConfigBoolToSetting(const bool bOption, char[] sOption, const int iMaxLen, const bool bYesNo = true, const int targetIndex = LANG_SERVER)
{
    // Initialize variable
    static char sBuffer[10];
    
    // Sets the language to target
    SetGlobalTransTarget(targetIndex);
    
    // If option is true, then copy "yes" to return string
    if(bOption)
    {
        // Gets the yes/no translations for the target
        if(bYesNo)
        {
            Format(sBuffer, sizeof(sBuffer), "%t", "Yes");  
            strcopy(sOption, iMaxLen, sBuffer);
        }
        else
        {
            Format(sBuffer, sizeof(sBuffer), "%t", "On");
            strcopy(sOption, iMaxLen, sBuffer);
        }
    }
    // If option is false, then copy "no" to return string
    else
    {
        // Gets the yes/no translations for the target
        if(bYesNo)
        {
            Format(sBuffer, sizeof(sBuffer), "%t", "No");     
            strcopy(sOption, iMaxLen, sBuffer);
        }
        else
        {
            Format(sBuffer, sizeof(sBuffer), "%t", "Off");
            strcopy(sOption, iMaxLen, sBuffer);
        }
    }
}

/**
 * Returns a "yes/no" string from config as a bool.
 * 
 * @param kv                The keyvalue handle.
 * @param sKey              The keyname the value is under.
 * @param sDefaultValue     (Optional) Value to return if setting is missing.
 **/
stock bool ConfigKvGetStringBool(const KeyValues kv, const char[] sKey, const char[] sDefaultValue = "yes")
{
    // Gets string from key
    static char sValue[CONFIG_MAX_LENGTH];
    kv.GetString(sKey, sValue, sizeof(sValue), sDefaultValue);
    
    // Convert string to bool
    return ConfigSettingToBool(sValue);
}