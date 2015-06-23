#include "stdafx.h"

/*
Author: Aaron "Snicker" Benzick
Date: 6/3/2000
*/
#define STATIC_LIBRARY
#include "ftp.h"
#include "fce.h"
#include "commctrl.h"
#include "string.h"
#include "launchApp.h"
#include "logon.h"
#include "launchAppWin.h"
#include "pnRegion.h"
#include "include/defs.h"

static HWND hwndCombo;
static int  NextState = STATE_AUTO;

static char	_fileSize[PN_MAX_PATH];
static UINT32 iFileSize;
static char	_fileName[PN_MAX_PATH];
static char	_localFileName[PN_MAX_PATH];

static long ByteCount;
static DWORD AppendStart;

bool ftpInProgress = false;

static void download();
static void FileCheck(const char*);

static UINT32 s_downloadStartedTime = 0;
static long   s_bytesDownloaded = -1;

void ftpStart()
{
	playgate.SetStatus(GetLoc(UPDATE_FOUND), AppProgress);
	playgate.m_progress = (CProgressCtrl*)(hMainWnd->GetDlgItem(IDC_FTP_PROGRESS));

	playgate.ftping(true);

	AppendStart = 0;
	ftpInProgress = true;

	teulSyncClock();
	s_downloadStartedTime = teulTime();
	s_bytesDownloaded = -1;
}

static void ftpStopped()
{
	hMainWnd->KillTimer('FTP');

	fceClose(0);
	fceRelease();

	playgate.ftping(false);

	ftpInProgress = false;
}

bool ftpStop(const TCHAR* reason, UINT32 cmd, LPARAM code)
{
	if ( ftpInProgress == false )
		return false;

	if ( nullptr != reason && 0 != reason[0] )
		playgate.SetStatus(reason);

	if ( 0 != code )
		playgate.PostCommand(cmd, code);

	ftpStopped();

	return true;
}

void ftpQuit()
{
	ftpStop(GetLoc(PATCH_CANCELLED), 0, 0);
}

static TCHAR* _tcharString(char* src)
{
	static TCHAR buffer[512];
	TCHAR* dst = buffer;
	do
	{
		*(dst) = *(src++);
	}
	while ( *(dst++) != 0 );
	return buffer;
}

static void _ftpError(SINT32 Code)
{
	//SINT32 n = fceErrorText(0,Code,(LPSTR)Temp,sizeof(Temp));
	//if(n>0) printf("ERROR %s\n", Buffer);

	//bug fix; check if file is actually complete despite error being reported
	FileCheck(_localFileName);
	if (AppendStart == iFileSize && AppendStart > 0)
	{
		NextState = STATE_EXECUTE;
	}
	else if (iFileSize > 0)
	{
		ftpStop(GetLoc(FTP_FAILED), webID, (LPARAM)ERR_FTP_CONNECT);
	}
	else 
	{
		ftpStop(GetLoc(FTP_FAILED), webID, (LPARAM)ERR_FTP_NOPATCH);
	}
}

static FLOAT32 _getDownloadRate(long bytesReceived)
{
	teulSyncClock();
	UINT32 secondsElapsed = teulTime() - s_downloadStartedTime;
	if ( bytesReceived == 0 || secondsElapsed == 0 )
		return 0.0f;
	return (FLOAT32)bytesReceived / (FLOAT32)secondsElapsed;
}

int ftpProcess()
{
	if ( ftpInProgress == false )
		return 0;

	int lastBytes, newBytes;
	long bytesReceived = 0;
	SINT32 Code;
	static TCHAR statusMessage[512];

	switch(NextState)
	{
		case STATE_IDLE:
			return 50;

		case STATE_AUTO:
			download();
			if ( playgate.m_progress )
				playgate.m_progress->SetPos(0);
			return 50;

		case STATE_EXECUTE:
			fceSetInteger(0,FCE_SET_AUTO_CALL_DRIVER ,1);
			ftpStopped();
			if ( AppLaunch.action == APPDL_INSTALL)
			{
				playgate.SetStatus(GetLoc(PATCH_EXECUTE), AppProgress);
				if ( launchApplication(_tcharString(_fileName), _T(""), g_dataPath) == false )
				{
					playgate.SetStatus(GetLoc(PATCH_FAILED), AppError);
					return 0;
				}
				playgate.SetStatus(GetLoc(PATCH_COMPLETE), AppProgress);
				playgate.Quit();
			}
			else if (AppLaunch.action == APPDL_REPAIR)
			{
				/// This shouldn't happen any more...
				hMainWnd->MessageBox(GetLoc(AUTH_BEHAVIOR), GetLoc(AUTH_ERROR), MB_ICONEXCLAMATION|MB_OK);
				exit(0);
			}
		break;

		case STATE_DOWNLOAD:
			NextState = STATE_IDLE;

			if ( s_bytesDownloaded == -1 )
			{
				FileCheck(_localFileName);
				s_bytesDownloaded = AppendStart;
			}

			lastBytes = fceGetInteger(0, FCE_GET_FILE_BYTES_RCVD);
			// Call fceDriver a bunch of times until there are
			// no new bytes received.
			for( UINT32 i = 0; i <= 4096; ++i )
			{
				Code = fceDriver(0);
				//if error returned, cleanup 
				if ( Code < 0 ) 
				{
					_ftpError(Code);
					return 0;
				}

				//Download is complete
				if ( Code == 0 )
				{
					NextState = STATE_CLEANUP;
					FLOAT32 mb = (FLOAT32)ByteCount / (FLOAT32)(1024 * 1024);
					//ftpStop(statusMessage, 0, 0);
					if ( playgate.m_progress )
						playgate.m_progress->SetPos(100);
					playgate.SetStatus(GetLoc(FTP_COMPLETE, _fileName, mb), AppProgress);
					NextState = STATE_EXECUTE;
					return 1;
				 }

				newBytes = fceGetInteger(0, FCE_GET_FILE_BYTES_RCVD);
				if ( newBytes == lastBytes )
					break;
			}

			//While still downloading, update downloaded byte count
			bytesReceived = fceGetInteger(0, FCE_GET_FILE_BYTES_RCVD);
			ByteCount = bytesReceived + AppendStart;
			if(ByteCount != s_bytesDownloaded && iFileSize != 0)
			{
				static UINT32 lastPercent = 999;
				UINT32 percent = (UINT32)((FLOAT32)ByteCount * 100L / (FLOAT32)iFileSize);
				FLOAT32 dlMb = (FLOAT32)ByteCount / (FLOAT32)(1024 * 1024);
				FLOAT32 fsMb = (FLOAT32)iFileSize / (FLOAT32)(1024 * 1024);
				FLOAT32 dlRate = _getDownloadRate(bytesReceived);
				playgate.SetStatus(GetLoc(FTP_STATUS, percent, _fileName, dlMb, fsMb, dlRate), AppNote);
				s_bytesDownloaded = ByteCount;
				if ( playgate.m_progress && percent != lastPercent )
				{
					playgate.m_progress->SetPos(percent);
					lastPercent = percent;
				}
			}

//			Sleep(0);

			NextState = STATE_DOWNLOAD;

			return 10;

			break;

		case STATE_CLEANUP:
			NextState = STATE_IDLE;
			ftpStop(nullptr, 0, 0);
			return 0;
			//NextState
			break;
			
		default:
			NextState = STATE_IDLE;

	} //end NextState switch

	return 50;
}

void download()
{
	int Code, fnLen;
	char buffer[4096];
	SCHAR *cWebURL;
	SCHAR *tmp;
	SCHAR filePath[PN_MAX_PATH];
	SCHAR pwdString[128];

	NextState = STATE_DOWNLOAD;
	//parse filename from URL
	memset(_fileName,0,sizeof(_fileName));
	memset(filePath,0,sizeof(filePath));

	//attach FCE
	Code = fceAttach(1, 1439896851);

	cWebURL = (char *)webURL;

	if (AppLaunch.serverPort == 0)
	{
		ftpStop(nullptr, MSG_DOWNLOAD, (LPARAM)ERR_FTP_NOPATCH);
		return;
	}

	if (AppLaunch.serverPort == 1)
	{
		ftpStop(nullptr, MSG_DOWNLOAD, (LPARAM)webURL);
		return;
	}

	if ( ftpURL[0] )
	{
		tmp = strrchr(ftpURL,'\n');

		if ( tmp )
		{
			strcpy_s(_fileName, tmp + 1);

			*tmp = '\0';

			tmp = strrchr(ftpURL,'\n');
			if (tmp)
			{
				*tmp = '\0';
				strcpy_s(filePath, tmp + 1);
			}
		}

		fceSetInteger(0, FCE_SET_FTP_PORT , AppLaunch.serverPort); 
	}
	else
	{
		tmp = strstr((SCHAR *)webURL,"version=");
		if (tmp)
		{
			strncpy_s(_fileName, tmp + sizeof("version="), 8);
			strcat_s(_fileName, ".exe");
		}

		fceSetInteger(0, FCE_SET_FTP_PORT , 21); 

		strcpy_s(ftpURL, 255, "downloads");
		strcat_s(ftpURL, 255, g_region.authSuffix().c_str());
	}

	fnLen = strlen(_fileName);

	if(! _fileName[0] ||  fnLen < 2 )
	{
		ftpStop(nullptr, webID, (LPARAM)ERR_FTP_NOPATCH);
		return;
	}

	playgate.SetStatus(GetLoc(FTP_CONNECTING), AppProgress);

	//Start log file
	fceSetString(0, FCE_SET_LOG_FILE, "ftplog.txt");
	fceSetString(0, FCE_WRITE_TO_LOG , (SCHAR *) webURL);
	fceSetString(0, FCE_WRITE_TO_LOG , (SCHAR *) ftpURL);
	fceSetString(0, FCE_WRITE_TO_LOG , (SCHAR *) filePath);
	fceSetString(0, FCE_WRITE_TO_LOG , (SCHAR *) _fileName);

	if(Code<0)
	{
		ftpStop(GetLoc(FTP_NO_CONNECT), webID, (LPARAM)ERR_FTP_CONNECT);
		return;

	}

	//connect to server
	sprintf(pwdString, "%s@playnetmail.com", (userName && userName[0]) ? userName : "playupdate");
	Code = fceConnect(0,(LPSTR)ftpURL,(LPSTR)"anonymous", (LPSTR)pwdString);
	if(Code<0)
	{
		ftpStop(GetLoc(FTP_NO_LOGIN), webID, (LPARAM)ERR_FTP_CONNECT);
		return;
	}

	fceSetInteger(0, FCE_SET_PASSIVE, 1); 

	sprintf(_localFileName, "%s",_fileName);

	//Check if local file exists and size
	FileCheck(_localFileName);

	//Get size of file on server
	if (! filePath[0])
		strcpy_s(filePath,"/");

	// FCE_FULL_LIST_FILE performs the "ls -l <buffer>" and writes the result into buffer.
	sprintf(buffer, "%s%s", filePath, _fileName);
	fceGetList(0, FCE_FULL_LIST_FILE, buffer, sizeof(buffer));

	// Extract parameter #5 from entry #1 (filesize on our FTP servers)
	fceExtract((LPSTR)buffer, 1, 5, (LPSTR)_fileSize, sizeof(_fileSize));

	iFileSize = (UINT32)atoi(_fileSize);

	//If Existing file is same size as file on server, execute
	if (AppendStart == iFileSize && AppendStart > 0) 
		NextState = STATE_EXECUTE;
	else 
	{
		// File is smaller than that on the server, get rid of the local file.
		if ( iFileSize < AppendStart )
		{
			_unlink(_localFileName);
			AppendStart = 0;
		}

		//divide bytes by 1048576 to get mb
		//set to binary mode
		fceSetMode(0, 'B');
		//enable direct driver calls
		fceSetInteger(0, FCE_SET_AUTO_CALL_DRIVER,0);

		//download the file
		strcat_s(filePath, _fileName);

		Code = fceGetFile(0, (LPSTR)filePath);

		if(Code<0) 
		{ 
			ftpStop(GetLoc(FTP_FAILED), webID, (LPARAM)ERR_FTP_NOPATCH);
			return;
		}
		//itoa(AppendStart,_fileSize,10);

		playgate.SetStatus(GetLoc(FTP_DOWNLOADING, _fileName), AppProgress);
	}
}

static void FileCheck(const char *File2)
{
	int Exists; //1=files exists
	HANDLE hFile;

	//Check and see if file exists
	hFile = CreateFileA(File2,		// file name 
	GENERIC_READ,                   // open for reading 
	1,                              // Share
	nullptr,                           // no security 
	OPEN_EXISTING,                  // existing file only 
	FILE_ATTRIBUTE_NORMAL,          // normal file 
	nullptr);                          // no attr. template 

	if (hFile == INVALID_HANDLE_VALUE) 
	{ 
		 Exists = 0;
	} 
	else 
	{
		Exists = 1;
		AppendStart = GetFileSize(hFile,nullptr);
		fceSetInteger(0, FCE_SET_APPEND_MODE, 1);
		fceSetInteger(0, FCE_SET_SERVER_OFFSET, AppendStart);
		fceSetInteger(0, FCE_SET_CLIENT_OFFSET, AppendStart);
	}

	CloseHandle(hFile);

	return;
}
