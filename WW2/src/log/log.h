#pragma once

//#define USE_LOG_TIMER

/************************************************************************************************
	log
*************************************************************************************************/
// new debug logging

// subsystems for logging
enum LogMask
{
	LOG_NONE	= 0x0,
	LOG_OLD 	= 0x00000001, // Old WriteDebug
	LOG_TERRAIN	= 0x00000002,
    LOG_WEAP    = 0x00000004,
    LOG_WEAPON  = 0x00000004,
	LOG_FPS		= 0x00000008,
	LOG_RENDER	= 0x00000010,
	LOG_SOUND	= 0x00000020,
    LOG_UI      = 0x00000040,
   	LOG_VEHICLE	= 0x00000080,
   	LOG_PHYSICS	= 0x00000100,
   	LOG_COMM	= 0x00000200,
   	LOG_FX		= 0x00000400,
   	LOG_TIMER	= 0x00000800,
    LOG_TEXTURE = 0x00001000,
	LOG_CONSOLE = 0x00002000,
	LOG_FIO		= 0x00004000,
    LOG_TEUL    = 0x00008000,
    LOG_MAP     = 0x00010000,
	LOG_1		= 0x00020000,
	LOG_2		= 0x00040000,
	LOG_3		= 0x00080000,
	LOG_4		= 0x00100000,
	LOG_5		= 0x00200000,
	LOG_6		= 0x00400000,
	LOG_7		= 0x00800000,
	LOG_8		= 0x01000000,
	LOG_9		= 0x02000000,
	LOG_10		= 0x03000000,
	LOG_11		= 0x04000000,
	LOG_12		= 0x08000000,
	LOG_13		= 0x10000000,
	LOG_14		= 0x20000000,
	LOG_15		= 0x40000000,
	LOG_16		= 0x80000000,

	// Add others as needed
	LOG_ALL		= 0xffffffff
};

static const char* LOG_SYSTEM_NAMES[32] =
        {
                "old",
                "terrain",
                "weapon",
                "fps",
                "render",
                "sound",
                "ui",
                "vehicle",
                "physics",
                "comm",
                "fx",
                "timer",
                "texture",
                "console",
                "fio",
                "teul",
                "map"
                // NULLS the rest out
        };

// logging levels
enum LogLevel
{
	LOG_FATAL = 1,
    LOG_ABORT,
    LOG_RECOVER,
	LOG_WARN,
    LOG_MINOR,
    LOG_NOTE,
	LOG_INFO,
	LOG_DEBUG
};

#ifdef EVENT_LOG
extern void Write_Where(const char* file, int line, const char* function, UINT32 system, SINT32 level, const char* str, ...);
extern void Write_Log(UINT32 system, SINT32 level, const char* str, ...);
extern void Enable_Log(UINT32 system, LOGICAL enable);
extern void Set_Log_Level(SINT32 level);
extern void Set_Log_Mask(const char *system);

extern LOGICAL	Open_Debug(const char* filename);
extern void Write_Debug(const char* str, ...); // equivalent to LOG_OLD, LOG_INFO
extern void Close_Debug();

extern UINT32 Log_System_Mask(const char* system);
extern const char* Log_System_Name(UINT32 mask);
extern UINT32 Get_Log_Mask();

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define WriteLog(system, level, fmt, ...) Write_Where(__FILENAME__, __LINE__, __FUNCTION__, system, level, fmt, ## __VA_ARGS__)
#define WriteDebug(fmt, ...) Write_Where(__FILENAME__, __LINE__, __FUNCTION__, LOG_ALL, LOG_DEBUG, fmt, ## __VA_ARGS__)

#else // EVENT_LOG
#define Write_Where
#define Write_Log
#define WriteLog
#define Enable_Log
#define Set_Log_Level
#define Set_Log_Mask

#define Open_Debug(...)
#define	Write_Debug(...)
#define	Close_Debug()

#define Log_System_Mask(x) 0
#define Log_System_Name(x) nullptr
#define Get_Log_Mask() 0

#endif // EVENT_LOG

#ifdef USE_LOG_TIMER
// logTimer
extern void logTimerReset();

// initialize timer id to negative, pass returned value with each call to same function,
// usually you can just call logTimerBegin/EndMacro below instead to handle everything for you
extern void logTimerBegin(const char* parent, const char* child, int* timerid);
extern int logTimerEnd(int timerid);

extern void logTimerPrint(int maintimerid, int frameidx); // specify main loop timer id to calc total time for percentages

// begin/end convience macros
#define logTimerBeginMacro() 			\
{										\
static SINT32 timerid = -1; 			\
logTimerBegin(__FUNCTION__, "", &timerid);

#define logTimerEndMacro() \
logTimerEnd(timerid);		\
}

#define logTimerBeginExtMacro(x) 	\
{ 									\
static SINT32 timerid_##x = -1; 	\
logTimerBegin(__FUNCTION__, #x, &timerid_##x);

#define logTimerEndExtMacro(x)	\
logTimerEnd(timerid_##x); 		\
}

extern float logTimerToMs(int);

#define logTimerCallVoid(x) \
logTimerBeginExtMacro(x) \
x(); \
logTimerEndExtMacro(x)
#define logTimerCallArgs(x, ...) \
logTimerBeginExtMacro(x) \
x(__VA_VARGS__); \
logTimerEndExtMacro(x)
#else //#ifdef USE_LOG_TIMER
#define logTimerReset()
#define logTimerBegin(x, z, y)
#define logTimerEnd(x) 0.0f
#define logTimerPrint(x,y)
#define logTimerBeginMacro()
#define logTimerEndMacro()
#define logTimerBeginExtMacro(x)
#define logTimerEndExtMacro(x)

#define logTimerToMs(x)

#define logTimerCallVoid(x)  x()
#define logTimerCallArgs(x, ...) x(__VA_ARGS__)
#endif //#ifdef USE_LOG_TIMER
