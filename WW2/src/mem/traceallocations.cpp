#include <syscfg.h> // Path is project specific.
#if defined(WIN32) && defined(DETECT_LEAKS)
#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <dbghelp.h>
#include <string>
#include <assert.h>
#include <WinNT.h>

using namespace std;

typedef std::basic_string<TCHAR, char_traits<TCHAR> > tcstring;

// Setup how much buffer is used for a single path fetch, increase if you get AV's during leak dump (4096 is plenty though)
#define BUFFERSIZE 4096

// Define how many levels of callstack that should be fetched for each allocation.
// Each level costs 2*sizof(DWORD64) bytes / allocation.
#define MAXSTACK 50

// Define size of no mans land
#define NO_MANS_LAND_SIZE 16

// Define frequency of no mans land checking
#define NML_CHECK_EVERY 1000

// Define number of call stack levels to ignore (usually 2, TraceAlloc and operator new)
#define NUM_LEVELS_TO_IGNORE 3

// Define STACKWALK_PRECISE to get more precise stackwalking at the cost of speed
//#define STACKWALK_PRECISE

// Value writing in no mans land before memory allocation ("begin")
#define LEADING_NOMANSLAND_NUMBER_BYTE 0xbe

// Value writing in no mans land after memory allocation ("end")
#define TAIL_NOMANSLAND_NUMBER_BYTE 0xed

// The magic number
#define MAGIC_NUMBER 0x55555555

void GetStackTrace(HANDLE hThread, DWORD64 ranOffsets[][2] );
bool WriteStackTrace(DWORD64 ranOffsets[][2], tcstring& roOut);
void* TraceAlloc(size_t nSize);
void TraceDealloc(void* poMem);
BOOL GetFunctionInfoFromAddresses( DWORD64 fnAddress, DWORD64 stackAddress, LPTSTR lpszSymbol, rsize_t lpszSymbolSize );

USHORT (WINAPI *s_pfnCaptureStackBackTrace)(ULONG, ULONG, PVOID*, PULONG) = 0;
bool bTraceAlloactions = true;

static int nMaxStackLevelUsage = 0;
static size_t nCurrentMemoryUsage = 0;
static size_t nLargestMemoryUsage = 0;
static size_t nLargestAllocationSize = 0;

void OutputDebugStringFormat( LPCTSTR lpszFormat, ... )
{
  TCHAR    lpszBuffer[BUFFERSIZE];
  va_list  fmtList;

  va_start( fmtList, lpszFormat );
  _vstprintf_s( lpszBuffer, BUFFERSIZE, lpszFormat, fmtList );
  va_end( fmtList );

   ::OutputDebugString( lpszBuffer );
}

// Unicode safe char* -> TCHAR* conversion
void PCSTR2LPTSTR( PCSTR lpszIn, LPTSTR lpszOut, rsize_t outSize )
{
#if defined(UNICODE)||defined(_UNICODE)
   ULONG index = 0;
   PCSTR lpAct = lpszIn;

  for( ; ; lpAct++ )
  {
    lpszOut[index++] = (TCHAR)(*lpAct);
    if ( *lpAct == 0 )
      break;
  }
#else
   // This is trivial :)
  strcpy_s( lpszOut, outSize, lpszIn );
#endif
}

BOOL __stdcall myReadProcMem(
    HANDLE      hProcess,
    DWORD64     qwBaseAddress,
    PVOID       lpBuffer,
    DWORD       nSize,
    LPDWORD     lpNumberOfBytesRead
) {
  SIZE_T st;
  BOOL bRet = ReadProcessMemory(hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, &st);
  *lpNumberOfBytesRead = (DWORD) st;
  //printf("ReadMemory: hProcess: %p, baseAddr: %p, buffer: %p, size: %d, read: %d, result: %d\n", hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, (DWORD) st, (DWORD) bRet);
  return bRet;
}


// Let's figure out the path for the symbol files
// Search path= ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;" + lpszIniPath
// Note: There is no size check for lpszSymbolPath!
void InitSymbolPath( PSTR lpszSymbolPath, rsize_t symPathSize, PCSTR lpszIniPath )
{
  CHAR lpszPath[BUFFERSIZE];

   // Creating the default path
   // ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
  strcpy_s( lpszSymbolPath, symPathSize, "." );

  // environment variable _NT_SYMBOL_PATH
  if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
  {
    strcat_s( lpszSymbolPath, symPathSize, ";" );
    strcat_s( lpszSymbolPath, symPathSize, lpszPath );
  }

  // environment variable _NT_ALTERNATE_SYMBOL_PATH
  if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
  {
    strcat_s( lpszSymbolPath, symPathSize, ";" );
    strcat_s( lpszSymbolPath, symPathSize, lpszPath );
  }

  // environment variable SYSTEMROOT
  if ( GetEnvironmentVariableA( "SYSTEMROOT", lpszPath, BUFFERSIZE ) )
  {
    strcat_s( lpszSymbolPath, symPathSize, ";" );
    strcat_s( lpszSymbolPath, symPathSize, lpszPath );
    strcat_s( lpszSymbolPath, symPathSize, ";" );

    // SYSTEMROOT\System32
    strcat_s( lpszSymbolPath, symPathSize, lpszPath );
    strcat_s( lpszSymbolPath, symPathSize, "\\System32" );
  }

   // Add user defined path
  if ( lpszIniPath != NULL )
    if ( lpszIniPath[0] != '\0' )
    {
      strcat_s( lpszSymbolPath, symPathSize, ";" );
      strcat_s( lpszSymbolPath, symPathSize, lpszIniPath );
    }
}

// Uninitialize the loaded symbol files
BOOL UninitSymInfo()
{
  return SymCleanup( GetCurrentProcess() );
}

// Initializes the symbol files
BOOL InitSymInfo( PCSTR lpszInitialSymbolPath )
{
  CHAR     lpszSymbolPath[BUFFERSIZE];
   DWORD    symOptions = SymGetOptions();

  symOptions |= SYMOPT_LOAD_LINES;
  symOptions &= ~SYMOPT_UNDNAME;
  SymSetOptions( symOptions );

   // Get the search path for the symbol files
  InitSymbolPath( lpszSymbolPath, sizeof(lpszSymbolPath), lpszInitialSymbolPath );

  return SymInitialize( GetCurrentProcess(), lpszSymbolPath, TRUE);
}

// Get the module name from a given address
BOOL GetModuleNameFromAddress( DWORD64 address, LPTSTR lpszModule, rsize_t lpszModuleSize )
{
  BOOL              ret = FALSE;
  IMAGEHLP_MODULE64   moduleInfo;

  ::ZeroMemory( &moduleInfo, sizeof(moduleInfo) );
  moduleInfo.SizeOfStruct = sizeof(moduleInfo);

  if ( SymGetModuleInfo64( GetCurrentProcess(), (DWORD)address, &moduleInfo ) )
  {
     // Got it!
    PCSTR2LPTSTR( moduleInfo.ModuleName, lpszModule, lpszModuleSize );
    ret = TRUE;
  }
  else
     // Not found :(
    _tcscpy_s( lpszModule, lpszModuleSize, _T("?") );

  return ret;
}

// Get function prototype and parameter info from ip address and stack address
BOOL GetFunctionInfoFromAddresses( DWORD64 fnAddress, DWORD64 stackAddress, LPTSTR lpszSymbol, rsize_t lpszSymbolSize )
{
  BOOL              ret = FALSE;
  DWORD             dwDisp = 0;
  DWORD             dwSymSize = 10000;
   TCHAR             lpszUnDSymbol[BUFFERSIZE]=_T("?");
  CHAR              lpszNonUnicodeUnDSymbol[BUFFERSIZE]="?";
  LPTSTR            lpszParamSep = NULL;
  LPCTSTR           lpszParsed = lpszUnDSymbol;
  PIMAGEHLP_SYMBOL  pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc( GMEM_FIXED, dwSymSize );

  ::ZeroMemory( pSym, dwSymSize );
  pSym->SizeOfStruct = dwSymSize;
  pSym->MaxNameLength = dwSymSize - sizeof(IMAGEHLP_SYMBOL);

   // Set the default to unknown
  _tcscpy_s( lpszSymbol, lpszSymbolSize, _T("?") );

  // Get symbol info for IP
  if ( SymGetSymFromAddr( GetCurrentProcess(), (ULONG)fnAddress, &dwDisp, pSym ) )
  {
     // Make the symbol readable for humans
    UnDecorateSymbolName( pSym->Name, lpszNonUnicodeUnDSymbol, BUFFERSIZE,
      UNDNAME_COMPLETE |
      UNDNAME_NO_THISTYPE |
      UNDNAME_NO_SPECIAL_SYMS |
      UNDNAME_NO_MEMBER_TYPE |
      UNDNAME_NO_MS_KEYWORDS |
      UNDNAME_NO_ACCESS_SPECIFIERS );

      // Symbol information is ANSI string
    PCSTR2LPTSTR( lpszNonUnicodeUnDSymbol, lpszUnDSymbol, sizeof(lpszUnDSymbol) );

      // I am just smarter than the symbol file :)
    if ( _tcscmp(lpszUnDSymbol, _T("_WinMain@16")) == 0 )
      _tcscpy_s(lpszUnDSymbol, sizeof(lpszUnDSymbol), _T("WinMain(HINSTANCE,HINSTANCE,LPCTSTR,int)"));
    else
    if ( _tcscmp(lpszUnDSymbol, _T("_main")) == 0 )
      _tcscpy_s(lpszUnDSymbol, sizeof(lpszUnDSymbol), _T("main(int,TCHAR * *)"));
    else
    if ( _tcscmp(lpszUnDSymbol, _T("_mainCRTStartup")) == 0 )
      _tcscpy_s(lpszUnDSymbol, sizeof(lpszUnDSymbol), _T("mainCRTStartup()"));
    else
    if ( _tcscmp(lpszUnDSymbol, _T("_wmain")) == 0 )
      _tcscpy_s(lpszUnDSymbol, sizeof(lpszUnDSymbol), _T("wmain(int,TCHAR * *,TCHAR * *)"));
    else
    if ( _tcscmp(lpszUnDSymbol, _T("_wmainCRTStartup")) == 0 )
      _tcscpy_s(lpszUnDSymbol, sizeof(lpszUnDSymbol), _T("wmainCRTStartup()"));

    lpszSymbol[0] = _T('\0');

    // Skip all templates
    if (_tcschr( lpszParsed, _T('<') ) == NULL) {
      // Let's go through the stack, and modify the function prototype, and insert the actual
      // parameter values from the stack
      if ( _tcsstr( lpszUnDSymbol, _T("(void)") ) == NULL && _tcsstr( lpszUnDSymbol, _T("()") ) == NULL)
      {
        ULONG index = 0;
        for( ; ; index++ )
        {
          lpszParamSep = const_cast<LPTSTR>(_tcschr( lpszParsed, _T(',') ));
          if ( lpszParamSep == NULL )
            break;

          *lpszParamSep = _T('\0');

          _tcscat_s( lpszSymbol, lpszSymbolSize, lpszParsed );
          _stprintf_s( lpszSymbol + _tcslen(lpszSymbol), lpszSymbolSize-_tcslen(lpszSymbol), _T("=0x%08X,"), *((ULONG*)(stackAddress) + 2 + index) );

          lpszParsed = lpszParamSep + 1;
        }

        lpszParamSep = const_cast<LPTSTR>(_tcschr( lpszParsed, _T(')') ));
        if ( lpszParamSep != NULL )
        {
          *lpszParamSep = _T('\0');

          _tcscat_s( lpszSymbol, lpszSymbolSize, lpszParsed );
          _stprintf_s( lpszSymbol + _tcslen(lpszSymbol), lpszSymbolSize - _tcslen(lpszSymbol), _T("=0x%08X)"), *((ULONG*)(stackAddress) + 2 + index) );

          lpszParsed = lpszParamSep + 1;
        }
      }
    }

    _tcscat_s( lpszSymbol, lpszSymbolSize, lpszParsed );

    ret = TRUE;
  }

  GlobalFree( pSym );

  return ret;
}

// Get source file name and line number from IP address
// The output format is: "sourcefile(linenumber)" or
//                       "modulename!address" or
//                       "address"

BOOL GetSourceInfoFromAddress( DWORD64 address, LPTSTR lpszSourceInfo, rsize_t lpszSourceInfoSize )
{
  BOOL            ret = FALSE;
  IMAGEHLP_LINE64 lineInfo;
  DWORD           dwDisp;
  TCHAR           lpszFileName[BUFFERSIZE] = _T("");
  TCHAR           lpModuleInfo[BUFFERSIZE] = _T("");

  _tcscpy_s( lpszSourceInfo, lpszSourceInfoSize, _T("?(?)") );

  ::ZeroMemory( &lineInfo, sizeof( lineInfo ) );
  lineInfo.SizeOfStruct = sizeof( lineInfo );

  if ( SymGetLineFromAddr64( GetCurrentProcess(), address, &dwDisp, &lineInfo ) )
  {
     // Got it. Let's use "sourcefile(linenumber)" format
    PCSTR2LPTSTR( lineInfo.FileName, lpszFileName, sizeof(lpszFileName) );
    _stprintf_s( lpszSourceInfo, lpszSourceInfoSize, _T("%s(%d)"), lpszFileName, lineInfo.LineNumber );
    ret = TRUE;
  }
  else
  {
      // There is no source file information. :(
      // Let's use the "modulename!address" format
      GetModuleNameFromAddress( address, lpModuleInfo, sizeof(lpModuleInfo) );

    if ( lpModuleInfo[0] == _T('?') || lpModuleInfo[0] == _T('\0'))
       // There is no modulename information. :((
         // Let's use the "address" format
      _stprintf_s( lpszSourceInfo, lpszSourceInfoSize, _T("0x%08X"), lpModuleInfo, address );
    else
      _stprintf_s( lpszSourceInfo, lpszSourceInfoSize, _T("%s!0x%08X"), lpModuleInfo, address );

    ret = FALSE;
  }

  return ret;
}


void GetStackTrace(HANDLE hThread, DWORD64 ranOffsets[][2])
{
#ifndef STACKWALK_PRECISE
  if (s_pfnCaptureStackBackTrace) {
    void* stacktrace[MAXSTACK+1];
    int capcount = s_pfnCaptureStackBackTrace(NUM_LEVELS_TO_IGNORE, MAXSTACK, stacktrace, NULL);
    int index;
    for (index = 0; index < capcount; index++) {
      ranOffsets[index][0] = (DWORD64) stacktrace[index];
      ranOffsets[index][1] = (DWORD64) stacktrace[index];
    }

  if (index < MAXSTACK) {
      ranOffsets[index][0] = 0;
      ranOffsets[index][1] = 0;
    }

  if( capcount > nMaxStackLevelUsage )
    nMaxStackLevelUsage = capcount;
  }
#else
  STACKFRAME64   callStack;
  BOOL           bResult;
  CONTEXT        context;
  HANDLE         hProcess = GetCurrentProcess();

  memset(&context, 0, sizeof(CONTEXT));
  context.ContextFlags = CONTEXT_FULL;
  if (hThread == GetCurrentThread())
  {
    RtlCaptureContext(&context);
  }
  else
  {
    if ( SuspendThread( hThread ) == -1 )
    {
       // whaaat ?!
       OutputDebugStringFormat( _T("Call stack info(thread=0x%X) SuspendThread failed.\n") );
      return;
    }

    if (GetThreadContext(hThread, &context) == FALSE)
    {
      OutputDebugStringFormat( _T("Call stack info(thread=0x%X) GetThreadContext failed.\n") );
      ResumeThread(hThread);
      return;
    }
  }

  ::ZeroMemory( &callStack, sizeof(callStack) );

  DWORD imageType;
#ifdef _M_IX86
  // normally, call ImageNtHeader() and use machine info from PE header
  imageType = IMAGE_FILE_MACHINE_I386;
  callStack.AddrPC.Offset = context.Eip;
  callStack.AddrPC.Mode = AddrModeFlat;
  callStack.AddrFrame.Offset = context.Ebp;
  callStack.AddrFrame.Mode = AddrModeFlat;
  callStack.AddrStack.Offset = context.Esp;
  callStack.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  imageType = IMAGE_FILE_MACHINE_AMD64;
  callStack.AddrPC.Offset = context.Rip;
  callStack.AddrPC.Mode = AddrModeFlat;
  callStack.AddrFrame.Offset = context.Rsp;
  callStack.AddrFrame.Mode = AddrModeFlat;
  callStack.AddrStack.Offset = context.Rsp;
  callStack.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
  imageType = IMAGE_FILE_MACHINE_IA64;
  callStack.AddrPC.Offset = context.StIIP;
  callStack.AddrPC.Mode = AddrModeFlat;
  callStack.AddrFrame.Offset = context.IntSp;
  callStack.AddrFrame.Mode = AddrModeFlat;
  callStack.AddrBStore.Offset = context.RsBSP;
  callStack.AddrBStore.Mode = AddrModeFlat;
  callStack.AddrStack.Offset = context.IntSp;
  callStack.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif

  for( ULONG index = 0;; index++ )
  {
    bResult = StackWalk64(
      imageType,
      hProcess,
      hThread,
      &callStack,
      &context,
      NULL, //myReadProcMem,
      SymFunctionTableAccess64,
      SymGetModuleBase64,
      NULL);

    // Ignore the first two levels (it's only TraceAlloc and operator new anyhow)
    if ( index < NUM_LEVELS_TO_IGNORE )
      continue;

    // Break if we have fetched MAXSTACK levels
    if ( index-NUM_LEVELS_TO_IGNORE == MAXSTACK)
      break;

    // If we are at the top of the stackframe then break.
    if( !bResult || callStack.AddrFrame.Offset == 0) {
      ranOffsets[index-NUM_LEVELS_TO_IGNORE][0] = 0;
      ranOffsets[index-NUM_LEVELS_TO_IGNORE][1] = 0;
      break;
    }

    // Remember program counter and frame pointer
    ranOffsets[index-NUM_LEVELS_TO_IGNORE][0] = callStack.AddrPC.Offset;
    ranOffsets[index-NUM_LEVELS_TO_IGNORE][1] = callStack.AddrFrame.Offset;
  }

  if ( hThread != GetCurrentThread() )
    ResumeThread( hThread );
#endif
}


bool WriteStackTrace(DWORD64 ranOffsets[][2], tcstring& roOut)
{
  TCHAR symInfo[BUFFERSIZE] = _T("?");
  TCHAR srcInfo[BUFFERSIZE] = _T("?");

  for (ULONG index = 0; index < MAXSTACK && ranOffsets[index][0] != 0 && ranOffsets[index][1] != 0; index++) {
    GetFunctionInfoFromAddresses( ranOffsets[index][0], ranOffsets[index][1], symInfo, sizeof(symInfo) );
    GetSourceInfoFromAddress( ranOffsets[index][0], srcInfo, sizeof(srcInfo) );

    if(wcsstr(srcInfo, L"1.34") == NULL)
        continue;

    if(wcsstr(srcInfo, L"phui") != NULL)
    {
        roOut.clear();
        return false;
    }

    if(wcsstr(srcInfo, L"\\ui\\") != NULL)
    {
        roOut.clear();
        return false;
    }

    roOut += _T("     ");
    roOut += srcInfo;
    roOut += _T(" : ");
    roOut += symInfo;
    roOut += _T("\n");
  }

  return true;
}


struct sdAllocBlock {
  unsigned long nMagicNumber;
  sdAllocBlock* poNext;
  sdAllocBlock* poPrev;
  size_t nSize;
  DWORD64 anStack[MAXSTACK][2];
  char pzNoMansLand[NO_MANS_LAND_SIZE];

  sdAllocBlock()
  {
    Init();
  }

  void Init() {
    poNext = this;
    poPrev = this;
    nMagicNumber = MAGIC_NUMBER;
  }

  void Disconnect() {
    if (poNext != this) {
      poNext->poPrev = poPrev;
      poPrev->poNext = poNext;
      poNext = this;
      poPrev = this;
    }
  }

  void ConnectTo(sdAllocBlock* poPos) {
    Disconnect();
    poPrev = poPos;
    poNext = poPos->poNext;
    poPos->poNext->poPrev = this;
    poPos->poNext = this;
  }
};


void LeakDump(tcstring& roOut);


class CS {
private:
  CRITICAL_SECTION cs;

public:
  CS() { InitializeCriticalSection(&cs); }
  ~CS() { }
  operator CRITICAL_SECTION& () { return cs; }
};


class Guard {
private:
  CRITICAL_SECTION& rcs;

public:
  Guard(CRITICAL_SECTION& rcs)
  : rcs(rcs) { EnterCriticalSection(&rcs); }
  ~Guard() { LeaveCriticalSection(&rcs); }
};

class cLeakDetector
{
public:
  cLeakDetector() {
    if (s_pfnCaptureStackBackTrace == 0) {
      const HMODULE hNtDll = ::GetModuleHandle(L"ntdll.dll");
      reinterpret_cast<void*&>(s_pfnCaptureStackBackTrace) = ::GetProcAddress(hNtDll, "RtlCaptureStackBackTrace");
    }
    InitSymInfo(NULL);
  }

  ~cLeakDetector() {
    bTraceAlloactions = false; // All subsequent allocations will be plain malloc/free
    tcstring leaks;
    LeakDump(leaks);
    Sleep(1000);
    UninitSymInfo();
  }
};


static unsigned int nNumAllocs = 0;
static unsigned int nCurrentAllocs = 0;
static unsigned int nMaxConcurrent = 0;



CS& Gate() {
  static CS cs;
  return cs;
}


sdAllocBlock& Head()
{
  static cLeakDetector oDetector;
  static sdAllocBlock oHead;
  return oHead;
}


class cInitializer {
  public: cInitializer() { Head(); };
} oInitalizer;


void LeakDump(tcstring& roOut)
{
  Guard at(Gate());

  TCHAR buffer[65];
  int leakCount = 0;

  sdAllocBlock* poBlock = Head().poNext;
  while (poBlock != &Head())
  {
    tcstring stack;

      roOut.clear();

    if(!WriteStackTrace(poBlock->anStack, stack))
    {
        poBlock = poBlock->poNext;
        continue;
    }

    bool bIsKnownLeak = false;

    // afxMap leaks is MFC. Not ours.
    if (stack.find(_T(": afxMap")) != tcstring::npos)
      bIsKnownLeak = true;

    if (!bIsKnownLeak) {
      roOut += _T("\nLeak of ");
      _itot_s(poBlock->nSize, buffer, sizeof(buffer), 10);
      roOut += buffer;
      roOut += _T(" bytes detected (");
      _itot_s(poBlock->nSize + sizeof(sdAllocBlock) + NO_MANS_LAND_SIZE, buffer, sizeof(buffer), 10);
      roOut += buffer;
      roOut += _T(" bytes with headers and no mans land)\n");
      roOut += stack;

      leakCount++;

      OutputDebugString(roOut.c_str());
    }

    poBlock = poBlock->poNext;
  }

  roOut += _T("\nMemory statistics\n-----------------\n");
  if( leakCount > 0 )
  {
    roOut += _T("Number of leaks: ");
    _itot_s(leakCount, buffer, sizeof(buffer), 10);
    roOut += buffer;
    roOut += _T("\n");
  }

  roOut += _T("Total allocations: ");
  _itot_s(nNumAllocs, buffer, sizeof(buffer), 10);
  roOut += buffer;
  roOut += _T("\n");
  roOut += _T("Max concurrent allocations: ");
  _itot_s(nMaxConcurrent, buffer, sizeof(buffer), 10);
  roOut += buffer;
  roOut += _T("\n");
  roOut += _T("Largest total allocations: ");
  _itot_s(nLargestMemoryUsage, buffer, sizeof(buffer), 10);
  roOut += buffer;
  roOut += _T("\n");
  roOut += _T("Largest single allocation: ");
  _itot_s(nLargestAllocationSize, buffer, sizeof(buffer), 10);
  roOut += buffer;
  roOut += _T("\n");
  roOut += _T("Max stack level usage: ");
  _itot_s(nMaxStackLevelUsage, buffer, sizeof(buffer), 10);
  roOut += buffer;
  roOut += _T("\n\n");

    OutputDebugStringFormat(roOut.c_str());
}


bool AssertMem(char* m, char c, size_t s)
{
  size_t i;
  for (i = 0; i < s; i++)
    if (m[i] != c) break;
  return i >= s;
}


void CheckNoMansLand()
{
  Guard at(Gate());

  sdAllocBlock* poBlock = Head().poNext;
  while (poBlock != &Head()) {
    if (!AssertMem(poBlock->pzNoMansLand, (char) LEADING_NOMANSLAND_NUMBER_BYTE, NO_MANS_LAND_SIZE)) {
      bool MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_LEAD = false;
      tcstring stack;
      WriteStackTrace(poBlock->anStack, stack);
      assert(MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_LEAD);
    }
    char* pzNoMansLand = ((char*)poBlock) + sizeof(sdAllocBlock) + poBlock->nSize;
    if (!AssertMem(pzNoMansLand, (char) TAIL_NOMANSLAND_NUMBER_BYTE, NO_MANS_LAND_SIZE)) {
      bool MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_TAIL = false;
      tcstring stack;
      WriteStackTrace(poBlock->anStack, stack);
      assert(MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_TAIL);
    }
    poBlock = poBlock->poNext;
  }
}


void* TraceAlloc(size_t nSize)
{
  Guard at(Gate());

  if (bTraceAlloactions) {
    nNumAllocs++;
#ifdef DETECT_OVERWRITES
    if (nNumAllocs % NML_CHECK_EVERY == 0) {
      CheckNoMansLand();
    }
#endif

    sdAllocBlock* poBlock = (sdAllocBlock*) malloc(nSize + sizeof(sdAllocBlock) + NO_MANS_LAND_SIZE);
    poBlock->Init();
    poBlock->nSize = nSize;
    char* pzNoMansLand = ((char*)poBlock) + sizeof(sdAllocBlock) + poBlock->nSize;
    memset(poBlock->pzNoMansLand, LEADING_NOMANSLAND_NUMBER_BYTE, NO_MANS_LAND_SIZE);
    memset(pzNoMansLand, TAIL_NOMANSLAND_NUMBER_BYTE, NO_MANS_LAND_SIZE);

    GetStackTrace(GetCurrentThread(), poBlock->anStack );

    poBlock->ConnectTo(&Head());
    nCurrentAllocs++;
    if (nCurrentAllocs > nMaxConcurrent)
      nMaxConcurrent = nCurrentAllocs;

  nCurrentMemoryUsage += nSize;
  if( nLargestMemoryUsage < nCurrentMemoryUsage )
    nLargestMemoryUsage = nCurrentMemoryUsage;
  if( nLargestAllocationSize < nSize )
    nLargestAllocationSize = nSize;

    return (void*)(((char*) poBlock) + sizeof(sdAllocBlock));
  }
  else {
    return malloc(nSize);
  }
}


void TraceDealloc(void* poMem)
{
  Guard at(Gate());

  if (!poMem) return; // delete NULL; = do nothing

  if (bTraceAlloactions) {
    sdAllocBlock* poBlock = (sdAllocBlock*) ((char*)poMem - sizeof(sdAllocBlock));
    char* pzNoMansLand = ((char*)poBlock) + sizeof(sdAllocBlock) + poBlock->nSize;

  nCurrentMemoryUsage -= poBlock->nSize;

  if (poBlock->nMagicNumber != MAGIC_NUMBER) {
    // Whupps, something fishy is going on

      // Validate the address against our list of allocated blocks
      sdAllocBlock* poLoopBlock = Head().poNext;
      while (poLoopBlock != &Head() && poLoopBlock != poBlock)
        poLoopBlock = poLoopBlock->poNext;
      if (poLoopBlock == &Head()) {
        // Hell we didn't allocate this block.
        // Just free the memory and hope for the best.
        free(poMem);
      }
      else {
        bool MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_LEAD = false;
        assert(MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_LEAD);
      }
    }
    else if (!AssertMem(poBlock->pzNoMansLand, (char) LEADING_NOMANSLAND_NUMBER_BYTE, NO_MANS_LAND_SIZE)) {
      bool MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_LEAD = false;
      assert(MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_LEAD);
    }
    else if (!AssertMem(pzNoMansLand, (char) TAIL_NOMANSLAND_NUMBER_BYTE, NO_MANS_LAND_SIZE)) {
      bool MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_TAIL = false;
      assert(MEMORYERROR_STUFF_WRITTEN_IN_NOMANSLAND_TAIL);
    }
    else {
      poBlock->Disconnect();
      free(poBlock);
      nCurrentAllocs--;
    }
  }
  else {
    free(poMem);
  }
}
#endif
