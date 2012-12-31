// dbug.h
//

#ifndef dbug_HEADER

	#define dbug_HEADER

	BOOL		dbugOpen(char *filename);
	void		dbugClose(void);
	void		__cdecl Write_Debug(char *str, ...);

#endif
