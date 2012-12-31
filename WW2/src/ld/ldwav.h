#pragma once

extern BOOL ldwavParse(BYTE* infile, BYTE* outbuffer, WAVEFORMATEX* WaveFormat, DWORD* length);
extern BOOL ldwavLoadFile(char* filename, BYTE** outbuffer, WAVEFORMATEX* WaveFormat, DWORD* length);
