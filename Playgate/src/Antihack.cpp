#include "Antihack.h"
#include <map>
#include <winsock2.h>
#include <psapi.h>
#include <bitset>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <time.h>

using namespace std;

typedef map<int,wstring> PidMap;

#define PRINTF(x) { OutputDebugString(x); }
#define PRINTF2(x,y) { char_t ttt[1024]; sprintf_s(ttt,x,y); OutputDebugString(ttt); }
#define PRINTF3(x,y,z) { char_t ttt[1024]; sprintf_s(ttt,x,y,z); OutputDebugString(ttt); }

#define IP_TO_INT(d,c,b,a) ((a<<24L) | (b<<16L) | (c<<8L) | (d))

static void getProcesses( PidMap& pids )
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	pe32.dwSize = sizeof( PROCESSENTRY32 );
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		CloseHandle( hProcessSnap );
		return;
	}

	do
	{
		pids[pe32.th32ProcessID] = wstring(pe32.szExeFile);
	} while( Process32Next( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );
}

Antihack::Antihack(void)
{
}

static const wchar_t hexes[17]=L"0123456789ABCDEF";

// -serverip 66.28.224.153 -serverport 27015 -serverotp 102561691 -side 0 -o C36011928C06028785F5FDB2A13603841460B9019474

std::wstring Antihack::hide(std::wstring text, UINT32 seed)
{
	srand(seed);
	wstring result;

	for ( size_t i=0; i<text.length(); i++ )
	{
		wchar_t wc = text.at(i);
		wc = wc ^ rand();
		result += hexes[wc&0xF];
		result += hexes[(wc>>4)&0xF];
		result += hexes[(wc>>8)&0xF];
		result += hexes[(wc>>12)&0xF];
	}
	return result;
}

// returns a string of nonsense or a string with a hidden username from the hack.exe name

wstring Antihack::check()
{
	PidMap pids;
	getProcesses(pids);

	MIB_TCPTABLE_OWNER_MODULE *tcpTable = nullptr;
	DWORD size = sizeof(MIB_TCPTABLE_OWNER_MODULE)*1;

	DWORD result = ERROR_INSUFFICIENT_BUFFER;
	do
	{
		if ( tcpTable == nullptr ) free (tcpTable);
		tcpTable = (MIB_TCPTABLE_OWNER_MODULE*)calloc( 1, size );
		result = GetExtendedTcpTable( tcpTable, &size, false, AF_INET,
			TCP_TABLE_OWNER_MODULE_CONNECTIONS, 0 );
	} while ( result == ERROR_INSUFFICIENT_BUFFER );

	struct in_addr IpAddr;

	for ( DWORD i=0; i<tcpTable->dwNumEntries; i++ )
	{
		MIB_TCPROW_OWNER_MODULE* entry = &tcpTable->table[i];
		if ( entry->dwState == MIB_TCP_STATE_ESTAB )
		{
			IpAddr.S_un.S_addr = entry->dwRemoteAddr;
			DWORD pid = entry->dwOwningPid;

			const char*s = inet_ntoa( IpAddr );
			// Filter the address against addresses we know.
			// Original address is 85.17.235
			enum { BitFlip = 129 };
#ifndef INTERNAL_TESTING
			enum { Oct1 = BitFlip ^ 85, Oct2 = BitFlip ^ 17, Oct3 = BitFlip ^ 235 };
#else
			enum { Oct1 = BitFlip ^ 192, Oct2 = BitFlip ^ 168, Oct3 = BitFlip ^ 0 };
#endif
			long oct1 = (entry->dwRemoteAddr & 255) ^ BitFlip;
			long oct2 = ((entry->dwRemoteAddr >> 8) & 255) ^ BitFlip;
			long oct3 = ((entry->dwRemoteAddr >> 16) & 255) ^ BitFlip;
			long oct4 = ((entry->dwRemoteAddr >> 24) & 255);

			if ( oct1 == Oct1 && oct2 == Oct2 && oct3 == Oct3 )
			{
				wstring st = L":cfp";
				PidMap::iterator ref = pids.find(pid);
				if ( ref != pids.end() )
				{
					wstring st = ref->second;
					size_t pos = st.find(L".");
					if ( pos != wstring::npos )
					{
						st = st.substr(pos);
					}
					if ( st.length() > 10 )
					{
						st = st.substr(0,10);
					}
				}
				return (st+L"[          ").substr(0,11); // 11 characters with [ after the name
			}
		}
	}

	wchar_t s[12];

	srand((unsigned int)time(nullptr));
	for ( int i=0; i<11; i++ )
	{
		wchar_t c = 'A'+(rand()%26);
		s[i] = c;
	}
	s[11] = 0; // 11 characters no [

	return wstring(s); // should be random
}
