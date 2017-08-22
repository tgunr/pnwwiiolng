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
void sysmainKillApp();

#ifdef __cplusplus
}
#endif

#endif // PN_SYSMAC_SYSMAIN_H
