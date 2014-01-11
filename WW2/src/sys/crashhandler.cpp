#include <syscfg.h> // Path is project specific.
#ifdef CRASH_HANDLER
#include <iostream>
#include <sstream>
#include <eh.h>

#include <fstream>

#include <windows.h>
#include <dbghelp.h>
#include <exception>

#include "wwii/wwiiErrorTypes.h"
#include "wwii/wwiiClientMessages.h"
#include "callstack.h"
#include "ui/uiUTF.h"
#include "fio/fio.h"

#include <WinNT.h>

extern void GetFileVersion(const TCHAR** ppFFI);
extern int WINAPI WinMainX(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

typedef unsigned int AnAddress; // note 32bit address

class StackWalkerToString : public StackWalker
{
private:
	std::string& str;
	std::vector<AnAddress> stackAddresses;
public:
	StackWalkerToString(std::string&s) : str(s){}
	std::string& getStr(){return str;}
	std::vector<AnAddress>& getAddresses(){return stackAddresses;}
protected:
  virtual void OnOutput(LPCSTR s)
  {
    str.append(s); // format is [address]... if stack address
	unsigned int n;
	if ( s[0]=='[' && 1 == sscanf(s,"[%x]",&n))
	{
		stackAddresses.push_back(n);
	}
  }
};

static void addGenericInfo(std::stringstream& sstr, std::stringstream& miniInfo)
{
	SYSTEMTIME SystemTime;
    GetLocalTime(&SystemTime);
	char s[256];
	sprintf(s,"Date and time: %d/%d/%d %d:%02d:%02d\n",
				SystemTime.wMonth,
				SystemTime.wDay,
				SystemTime.wYear,
				SystemTime.wHour,
				SystemTime.wMinute,
				SystemTime.wSecond);
	sstr << s;

	sprintf(s, "Program compile time: %s %s\n", __DATE__, __TIME__);
	sstr << s;

	const TCHAR* pFFI;
	GetFileVersion(&pFFI);
	if (pFFI != nullptr)
	{
		sprintf(s, "File version: %s\n", UTF8(pFFI));
		sstr << s;
	}

	char* vendor = (char*)glGetString(GL_VENDOR);
	char* renderer = (char*)glGetString(GL_RENDERER);
	sprintf(s, "GL Vendor: %s\nRenderer: %s\n", vendor, renderer);
	sstr << s;
	miniInfo << s;
}

// this routine handles reporting of piExitProgram only
void nonExceptionErrorReport(const SINT8* file, SINT32 line, const SINT8* message)
{
	WWII_ERROR_INFO errInfo;
	memset(&errInfo, 0x0, sizeof(errInfo));

	std::stringstream sstr, miniInfo;
	addGenericInfo( sstr, miniInfo );

	std::string text = message;
	text.resize(WWII_ERR_STRING::_MAX_LEN);
	strcpy(errInfo.data.piExitInfo.errorMessage.string,text.c_str());
	errInfo.data.piExitInfo.errorMessage.length = strlen(text.c_str());

	text = file;
	text.resize(WWII_ERR_STRING::_MAX_LEN);
	strcpy(errInfo.data.piExitInfo.srcFilename.string,text.c_str());
	errInfo.data.piExitInfo.srcFilename.length = strlen(text.c_str());

	errInfo.data.piExitInfo.srcLine=line;

	FILE* fp = Fio::OpenLogFile();
	if ( fp )
	{
		fprintf( fp, sstr.str().c_str() );
		fprintf( fp, message );
		fprintf(fp, "Exited Program: file: %s, line: %d\n", file, line);
		fclose(fp);
	}

	errInfo.errorType = WW_ERR_PIEXIT;
	wwiiLog_WWII_ERR_INFO(&errInfo);
}

static void buildExceptionReport(EXCEPTION_POINTERS * eps,
								 std::string & buffer,
								 std::string& exceptionInfo,
								 std::string& stackInfo,
								 std::vector<AnAddress>& addresses,
								 WWII_ERROR_INFO* errInfo)
{
	std::stringstream sstr, miniInfo, temp;
	const EXCEPTION_RECORD & er = *eps->ExceptionRecord;
	int skip = 0;

	errInfo->data.exceptInfo.exceptionAddr = (UINT32)er.ExceptionAddress;
	errInfo->data.exceptInfo.exceptionCode = er.ExceptionCode;

	if ( addresses.size()>WWII_EXCEPTION_INFO::_STACK_LEN)
	{
		addresses.resize(WWII_EXCEPTION_INFO::_STACK_LEN);
	}

	int n=0;
	for( std::vector<AnAddress>::iterator it=addresses.begin(); it!=addresses.end(); it++)
	{
		errInfo->data.exceptInfo.stackAddrs[n++] = *it;
	}

	addGenericInfo( sstr, miniInfo );

	char* exceptionDesc;
	switch (er.ExceptionCode)
	{
	case STATUS_WAIT_0:
		exceptionDesc = "STATUS_WAIT_0";
		break;
	case STATUS_ABANDONED_WAIT_0:
		exceptionDesc = "STATUS_ABANDONED_WAIT_0";
		break;
	case STATUS_USER_APC:
		exceptionDesc = "STATUS_USER_APC";
		break;
	case STATUS_TIMEOUT:
		exceptionDesc = "STATUS_TIMEOUT";
		break;
	case STATUS_PENDING:
		exceptionDesc = "STATUS_PENDING";
		break;
	case STATUS_SEGMENT_NOTIFICATION:
		exceptionDesc = "STATUS_SEGMENT_NOTIFICATION";
		break;
	case STATUS_GUARD_PAGE_VIOLATION:
		exceptionDesc = "STATUS_GUARD_PAGE_VIOLATION";
		break;
	case STATUS_DATATYPE_MISALIGNMENT:
		exceptionDesc = "STATUS_DATATYPE_MISALIGNMENT";
		break;
	case STATUS_BREAKPOINT:
		exceptionDesc = "STATUS_BREAKPOINT";
		break;
	case STATUS_SINGLE_STEP:
		exceptionDesc = "STATUS_SINGLE_STEP";
		break;
	case STATUS_ACCESS_VIOLATION:
		exceptionDesc = "STATUS_ACCESS_VIOLATION";
		break;
	case STATUS_IN_PAGE_ERROR:
		exceptionDesc = "STATUS_IN_PAGE_ERROR";
		break;
	case STATUS_INVALID_HANDLE:
		exceptionDesc = "STATUS_INVALID_HANDLE";
		break;
	case STATUS_NO_MEMORY:
		exceptionDesc = "STATUS_NO_MEMORY";
		break;
	case STATUS_ILLEGAL_INSTRUCTION:
		exceptionDesc = "STATUS_ILLEGAL_INSTRUCTION";
		break;
	case STATUS_NONCONTINUABLE_EXCEPTION:
		exceptionDesc = "STATUS_NONCONTINUABLE_EXCEPTION";
		break;
	case STATUS_INVALID_DISPOSITION:
		exceptionDesc = "STATUS_INVALID_DISPOSITION";
		break;
	case STATUS_ARRAY_BOUNDS_EXCEEDED:
		exceptionDesc = "STATUS_ARRAY_BOUNDS_EXCEEDED";
		break;
	case STATUS_FLOAT_DENORMAL_OPERAND:
		exceptionDesc = "STATUS_FLOAT_DENORMAL_OPERAND";
		break;
	case STATUS_FLOAT_DIVIDE_BY_ZERO:
		exceptionDesc = "STATUS_FLOAT_DIVIDE_BY_ZERO";
		break;
	case STATUS_FLOAT_INEXACT_RESULT:
		exceptionDesc = "STATUS_FLOAT_INEXACT_RESULT";
		break;
	case STATUS_FLOAT_INVALID_OPERATION:
		exceptionDesc = "STATUS_FLOAT_INVALID_OPERATION";
		break;
	case STATUS_FLOAT_OVERFLOW:
		exceptionDesc = "STATUS_FLOAT_OVERFLOW";
		break;
	case STATUS_FLOAT_STACK_CHECK:
		exceptionDesc = "STATUS_FLOAT_STACK_CHECK";
		break;
	case STATUS_FLOAT_UNDERFLOW:
		exceptionDesc = "STATUS_FLOAT_UNDERFLOW";
		break;
	case STATUS_INTEGER_DIVIDE_BY_ZERO:
		exceptionDesc = "STATUS_INTEGER_DIVIDE_BY_ZERO";
		break;
	case STATUS_INTEGER_OVERFLOW:
		exceptionDesc = "STATUS_INTEGER_OVERFLOW";
		break;
	case STATUS_PRIVILEGED_INSTRUCTION:
		exceptionDesc = "STATUS_PRIVILEGED_INSTRUCTION";
		break;
	case STATUS_STACK_OVERFLOW:
		exceptionDesc = "STATUS_STACK_OVERFLOW";
		break;
	case STATUS_CONTROL_C_EXIT:
		exceptionDesc = "STATUS_CONTROL_C_EXIT";
		break;
	case STATUS_FLOAT_MULTIPLE_FAULTS:
		exceptionDesc = "STATUS_FLOAT_MULTIPLE_FAULTS";
		break;
	case STATUS_FLOAT_MULTIPLE_TRAPS:
		exceptionDesc = "STATUS_FLOAT_MULTIPLE_TRAPS";
		break;
	default:
		exceptionDesc = "Unknown exception";
		break;
	}

	switch (er.ExceptionCode)
	{
	case 0xE06D7363:
		{ // C++ exception
			if ( exceptionInfo.length()==0)
			{
				temp << "C++ Exception: unknown " << "at address " << er.ExceptionAddress;
			}
			else
			{
				temp << "C++ Exception: " << exceptionInfo  << " at address " << er.ExceptionAddress;
			}
		}
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		{
			temp << "Access violation: Illegal "
				<< (er.ExceptionInformation[0] ? "write" : "read")
				<< " by "
				<< er.ExceptionAddress
				<< " at "
				<< reinterpret_cast<void* >(er.ExceptionInformation[1]);
		}
		break;
	default:
		{
			temp << "SEH Exception code: "
				<< exceptionDesc << "="
				<< std::hex << std::uppercase << er.ExceptionCode
				<< " at "
				<< er.ExceptionAddress;
		}
	}

	sstr << temp.str();
	miniInfo << temp.str();
	sstr << stackInfo;
	sstr << "\n";

//	sstr << "\nLogfile at \\Documents\\WW2_Log.txt\n";

	buffer = sstr.str();

	std::string text = miniInfo.str();
	text.resize(WWII_EXCEPTION_INFO::_STR_LEN-1);
	strcpy(errInfo->data.exceptInfo.infoText,text.c_str());
}

static DWORD reportException(EXCEPTION_POINTERS * eps,
							 std::string & buffer,
							 std::string& exceptionInfo,
							 StackWalkerToString& sw,
							 WWII_ERROR_INFO* errInfo)
{
	__try
	{
		sw.ShowCallstack(GetCurrentThread(), eps->ContextRecord);
		buildExceptionReport(eps,buffer,exceptionInfo,sw.getStr(),sw.getAddresses(),errInfo);
		return EXCEPTION_EXECUTE_HANDLER;
	} __except (EXCEPTION_EXECUTE_HANDLER)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}


#ifdef ANTIHACK
#include "tools/diddle.h"

LOCAL_DIDDLE;
#endif

static int exceptioncatcher(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow,
							std::string & buffer)
{
	try
	{
		return WinMainX(hInstance,hPrevInstance,lpCmdLine,nCmdShow);
	}
	catch(std::exception& e)
	{
		buffer.append(typeid(e).name());
		buffer.append(" '");
		buffer.append(e.what());
		buffer.append("'");
		throw;
	}
	catch(const char* s)
	{
		buffer.append(" '");
		buffer.append(s);
		buffer.append("'");
		throw;
	}
}

static int starter(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow,
				   std::string & buffer,std::string& exceptionInfo, StackWalkerToString& sw,
				   WWII_ERROR_INFO* errInfo)
{
	__try
	{
		return exceptioncatcher(hInstance,hPrevInstance,lpCmdLine,nCmdShow,exceptionInfo);
	}
	__except(reportException(GetExceptionInformation(),buffer,exceptionInfo,sw,errInfo))
	{
		FILE* fp = Fio::OpenLogFile();
		if ( fp )
		{
			fprintf( fp, buffer.c_str() );
			fclose(fp);
		}

		errInfo->errorType = WW_ERR_EXCEPTION;
		wwiiLog_WWII_ERR_INFO(errInfo);

		return -1;
	}
}

#ifdef ANTIHACK
HideawayStorage* storage1;
void* Imagebase;
#endif
int crashhandler(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef ANTIHACK
	HideawayStorage st1;
	storage1 = &st1;
	Imagebase = (void*) &__ImageBase;
#endif
	std::string exceptionInfo;
	std::string buffer;
	std::string stackInfo;
	StackWalkerToString sw(stackInfo);
	buffer.reserve(1024*10);
	WWII_ERROR_INFO errInfo;
	memset(&errInfo, 0x0, sizeof(errInfo));
	return starter(hInstance,hPrevInstance,lpCmdLine,nCmdShow, buffer, exceptionInfo, sw, &errInfo);
}

#endif
