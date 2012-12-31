// ogl.h

#ifndef _OGL_H
#define _OGL_H

LOGICAL GetOglInfo(HINSTANCE hInstance, struct PLAY_OGL *info, BOOL Messages);

BOOL oglBegin(HWND hwnd, struct PIXEL_FORMAT_DESC *pixinfo);
BOOL oglInit(struct PLAY_OGL *info);
void oglEnd(void);

void Write_Debug(char *str, ...);

#endif // _OGL_H
