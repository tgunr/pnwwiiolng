// ddp.h
#ifndef _DDP_H
#define _DDP_H

BOOL ddpChooseScreenFormat(HDC hdc, PIXELFORMATDESCRIPTOR *pfd, struct PIXEL_FORMAT_DESC *pixInfo);
BOOL ddpSetDisplay(UINT32 flags);
void ddpResetDisplay(void);

#endif // _INFO_MESSAGE_TYPES_H
