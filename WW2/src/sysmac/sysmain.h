#ifndef PN_SYSMAC_SYSMAIN_H
#define PN_SYSMAC_SYSMAIN_H 1

#ifdef __cplusplus
extern "C"
{
#endif

int sysmain();
void sysmainQuit();
void sysGetMousePosition(int*,int*);
void sysSetMousePosition(int, int);
void sysmainCommandLine(int arg, const char** argv);
void sysmainKillApp();
void sysmainGetCommandLineArgs(int* argc, char*** argv);

#ifdef __cplusplus
}
#endif

#endif // PN_SYSMAC_SYSMAIN_H
