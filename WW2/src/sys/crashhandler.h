#pragma once

extern int crashhandler(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
extern void nonExceptionErrorReport(const SINT8* file, SINT32 line, const SINT8* message);
