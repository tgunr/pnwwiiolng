#pragma once
// $Id: ftp.h 477 2010-05-21 22:12:33Z osmith@CRS $
// Interface to the FTP functions

extern void ftpStart() ;
extern void ftpStop() ;
extern void ftpQuit() ;
extern int ftpProcess() ;

#define STATE_IDLE     0
#define STATE_DOWNLOAD 1
#define STATE_CLEANUP  2
#define STATE_AUTO	   3
#define STATE_EXECUTE  4