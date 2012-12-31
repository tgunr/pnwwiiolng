#ifndef PN_SYSMAC_SYSLAUNCHURL_H
#define PN_SYSMAC_SYSLAUNCHURL_H 1


	// launches the default web browser
	// returns true if it successfully
	// opened the browser
	// if local == true, we mangle the
	// path in a OS dependent way as
	// needed
	LOGICAL syslaunchurl(const char* url, LOGICAL local = 0);

#endif // PN_SYSMAC_SYSLAUNCHURL_H
