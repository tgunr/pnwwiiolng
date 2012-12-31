#include <syscfg.h> // Path is project specific.
//! @file cpuFeatures.cpp
//! @brief CPU Feature description strings.
//! Provides the CPU Feature constants cpuFeatureNames
//! and cpuExtendedFeatureNames.

const char* const cpuFeatureNames[32] =
{
	"x87 FPU On Chip"							// 0x000000001
,	"VME: Virtual-8086 Mode Enhancement"		// 0x000000002
,	"DE: Debugging Extensions"					// 0x000000004
,	"PSE: Page Size Extensions"					// 0x000000008
,	"TSC: Time Stamp Counter"					// 0x000000010
,	"MSR: RDMSR and WRMSR Support"				// 0x000000020
,	"PAE: Physical Address Extensions"			// 0x000000040
,	"MCE: Machine Check Exception"				// 0x000000080
,	"CXS: CMPXCHG8B Instruction"				// 0x000000100
,	"APIC: APIC On Chip"						// 0x000000200
,	"Unknown1"									// 0x000000400
,	"SYSENTER and SYSEXIT"						// 0x000000800
,	"MTRR: Memory Type Range Registers"			// 0x000001000
,	"PGE/PTE Global Bit"						// 0x000002000
,	"MCA: Machine Check Architecture"			// 0x000004000
,	"CMOV: Conditional Move/Compare Instruction"// 0x000008000
,	"PAT: Page Attribute Table"					// 0x000010000
,	"PSE-36: Page Size Extension"				// 0x000020000
,	"Processor Serial Number"					// 0x000040000
,	"CLFLUSH: CLFLUSH Extension"				// 0x000080000
,	"Unknown2"									// 0x000100000
,	"DS: Debug Store"							// 0x000200000
,	"ACPI: Thermal Monitor and Clock Ctrl"		// 0x000400000
,	"MMX: MMX Technology"						// 0x000800000
,	"FXSR: FXSAVE/FXRSTOR"						// 0x001000000
,	"SSE: SSE Extensions"						// 0x002000000
,	"SSE2: SSE2 Extensions"						// 0x004000000
,	"Self Snoop"								// 0x008000000
,	"HT: Hyper-threading Technology"			// 0x010000000
,	"TM: Thermal Monitor"						// 0x020000000
,	"Unknown4"									// 0x040000000
,	"PBE: Pending Break Enable"					// 0x080000000
} ;

const char* const cpuExtendedFeatureNames[32] =
{
	"SSE3: SSE3 Instructions"				// Bit 00
,	"PCLMULQDQ Instruction support"			// Bit 01
,	"Ext Unknown 1"							// Bit 02
,	"MONITOR/MWAIT support"					// Bit 03
,	"DS-CPL: CPL qualified debug store"		// Bit 04
,	"VMX: Virtual Machine Technology"		// Bit 05
,	"SMX: Secure Virtual Machine Technology"	// Bit 06
,	"EST: Enhanced Intel SpeedStep Technology"	// Bit 07
,	"TM2: Thermal Monitor 2"				// Bit 08
,	"SSE3E: SSE3E Instructions"				// Bit 09
,	"CNXT-ID: L1 context ID"				// Bit 10
,	"Ext Unknown 2"							// Bit 11
,	"FMA Instructions support"				// Bit 12
,	"CMPXCHG16B: CMPXCHG16B instruction support"	// Bit 13
,	"xTPR update control"					// Bit 14
,	"Ext Unknown 3"							// Bit 15
,	"Ext Unknown 4"							// Bit 16
,	"Ext Unknown 5"							// Bit 17
,	"DCA: Direct Cache Access"				// Bit 18
,	"SSE4.1 Instructions"					// Bit 19
,	"SSE4.2 Instructions"					// Bit 20
,	"X2APIC"								// Bit 21
,	"MOVBE instruction"						// Bit 22
,	"POPCNT instruction"					// Bit 23
,	"Reserved"								// Bit 24
,	"AES Instructions"						// Bit 25
,	"XSAVE extensions support"				// Bit 26
,	"OSXSAVE support"						// Bit 27
,	"AVX: AVX extension support"			// Bit 28
,	"Ext Unknown 6"							// Bit 29
,	"Ext Unknown 7"							// Bit 30
,	"Ext Unknown 8"							// Bit 31
} ;

