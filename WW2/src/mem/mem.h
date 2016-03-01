#pragma once
// Date: Mon Feb 02 14:36:43 2001
//
// Copyright (c) 2001 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: John "Kango" Lundy

#ifdef FUTURE
namespace Mem
{
	//! Allocate unitSize * numUnits bytes of memory. Terminates app if no memory available.
	void* Allocate(size_t unitSize, size_t numUnits, bool clear, const char* file, size_t lineNo);
	void Free(void* ptr, const char* file, size_t lineNo);
	void* Reallocate(void* ptr, size_t newSize, const char* file, size_t lineNo);
	void* Dup(void* src, size_t srcSize, const char* file, size_t lineNo)
	{
		void* dst = Allocate(srcSize, 1, false, file, lineNo);
		memcpy(dst, src, srcSize);
		return dst;
	}
}

#define PNMemAlloc(Size)			Mem::Allocate((Size), 1, false, __FILE__, __LINE__)
#define PNMemCalloc(Size)			Mem::Allocate((Size), 1, true, __FILE__, __LINE__)
#define PNMemAllocN(Size, Units)	Mem::Allocate((Size), (Units), false, __FILE__, __LINE__)
#define PNMemCallocN(Size, Units)	Mem::Allocate((Size), Units, true, __FILE__, __LINE__)
#define PNMemDup(Src, Size)			Mem::Dup(Src, Size)
#endif

#if platOS == OS_MACOS
	#include <malloc/malloc.h>
	#define memAlloc(a)   memAllocRelease(a)
	#define memFree(a)   memFreeRelease(a)
	#define memRealloc(Ptr_, NewSize_) realloc(Ptr_, NewSize_)
	void* memAllocRelease(SINT32 allocSize);
	void memFreeRelease(void* pointer);
    static inline void* memCalloc(size_t members, size_t sz) { return calloc(members, sz); }
	extern void memInit(void);
	extern void memKill(void);
#else
	#if platOS == OS_LINUX || platBuild == TEUL_SERVER
		#if !defined(__STDC__)
		# include <malloc.h>
		#endif
		#if !defined(MEMORY_DEBUG)
			#define memAlloc(Size_) calloc(1, Size_)
			#define memCalloc(Units_, UnitSize_) calloc(Units_, UnitSize_)
			#define memRealloc(Ptr_, NewSize_) realloc(Ptr_, NewSize_)
			#define memFree(Ptr_) free(Ptr_)
			#define memReportJSON(...)
		#else
			extern void* memCallocDebug(const size_t, const size_t, const char* const file_, const size_t line);
			#define memAlloc(Size_) memCallocDebug(1, Size_, __FILE__, __LINE__)
			#define memCalloc(Units_, UnitSize_) memCallocDebug(Units_, UnitSize_, __FILE__, __LINE__)
			extern void* memReallocDebug(void* const, const size_t, const char* const file_, const size_t line);
			#define memRealloc(Ptr_, NewSize_) memReallocDebug(Ptr_, NewSize_, __FILE__, __LINE__)
			extern void memFreeDebug(void* const, const char* const file_, const size_t line);
			#define memFree(Ptr_) memFreeDebug(Ptr_, __FILE__, __LINE__)
			extern void memReportJSON(unsigned int fd, const char* const /*path*/, const char* const /*args*/);
		#endif
	#else
		#include <malloc.h>
		#ifdef MEMORY_DEBUG
			#define memAlloc(a)   memAllocDebug((a), __FILE__, __LINE__)
			#define memRealloc(ptr, size) memReallocDebug((ptr), (size), __FILE__, __LINE__)
			#define memFree(a)   memFreeDebug((a), __FILE__, __LINE__)
		#else
			#ifdef BETA_RELEASE
				void* memAllocReleaseX(size_t allocSize, const char* f, int n );
				void* memReallocReleaseX(void* ptr, size_t newSize, const char* f, int n);
				#define memAlloc(a)   memAllocReleaseX((a), __FILE__, __LINE__)
				#define memFree(a)   memFreeRelease((a))
				#define memRealloc(ptr, size) memReallocReleaseX((ptr), (size), __FILE__, __LINE__)
				#define memCalloc(a,b) memAllocReleaseX((a)*(b), __FILE__, __LINE__)
			#else
				#define memAlloc(a)   memAllocRelease((a))
				#define memFree(a)   memFreeRelease((a))
				#define memRealloc(ptr, size) memReallocRelease((ptr), (size))
				#define memCalloc(a,b) memAllocRelease((a)*(b))
			#endif
		#endif

		#ifdef MEMORY_DEBUG
			void memVerifyMemory(void);
			void* memAllocDebug(size_t Size, const char* file, int line);
			void memFreeDebug(void* Pointer, const char* file, int line);
			void* memReallocDebug(void* Pointer, size_t size, const char* file, int line);
			#define memCalloc(a,b) memAlloc((a)*(b))
		#else
			static inline void* memAllocRelease(size_t allocSize) { return calloc(1, allocSize); }
			static inline void memFreeRelease(void* pointer) { free(pointer); }
			static inline void* memReallocRelease(void* pointer, size_t newSize) { return realloc(pointer, newSize); }
	//		static inline void* memCalloc(size_t members, size_t size) { return calloc(members, size); }
		#endif
	#endif

	#if defined(MEMORY_DEBUG) || platOS == OS_LINUX || platBuild == TEUL_SERVER
		extern void memInit(void);
		extern void memKill(void);
	#else
		#define memInit()
		#define memKill()
	#endif
#endif

#ifdef __cplusplus
	#define memRelease(ptr) { if((ptr) != nullptr) { memFree(ptr); ptr = nullptr; } }
#endif
