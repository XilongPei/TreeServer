int OpenConfigFile( char *FileName );
        /* handle = OpenConfigFile("tg.cfg"); */
void CloseConfigFile( int handle );
        /* CloseConfigFile( handle ); */
int GetConfigKey(int handle, char *GroupKey, ...);
        /* char *fieldctl;
        GetConfigKey(handle, "ControlFile Path", "fieldctl", &fieldctl, NULL);
        */
int WriteConfigKey(int handle, char *GroupKey, ...);
        /* char *fieldctl="c:\\tg\\control\\fieldctl.dbf";
        WriteConfigKey(handle, "ControlFile Path", "fieldctl", fieldctl, NULL);
        */
