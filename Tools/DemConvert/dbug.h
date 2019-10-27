// dbug.c
//


#define EVENT_LOG
//#undef EVENT_LOG

#define EVENT_LOG_EXTREME
#undef EVENT_LOG_EXTREME

#ifdef EVENT_LOG
	BOOL        Open_Debug(char *filename);
	void        Close_Debug(void);
	void        __cdecl Write_Debug(char *str, ...);
#else
	#define     Open_Debug
	#define     Close_Debug()
	#define     Write_Debug
	#define		WriteLog
#endif
