#pragma once

#define PHUI_MAX_BUFFER 16384

namespace phui
{
/// Callbacks for various I/O operations
struct IOCallbacks
{
	int (*openread)(const char*);
	int (*openwrite)(const char*);
	size_t (*read)(int, void*, size_t);
	size_t (*write)(int, const void*, size_t);
	int (*close)(int);
	size_t (*size)(const char*);
	size_t (*seek)(int fd, size_t offset);
};

/// default callbacks using ANSI fd and FILE* routines
extern const IOCallbacks fdIOCallbacks;
extern const IOCallbacks FILEIOCallbacks;
extern const IOCallbacks memIOCallbacks;

extern void memIOAddBuffer(const char* name, void*, int size);
extern void memIORemoveBuffer(const char* name);

// wchar stuff
extern wchar_t* str_to_wstr(wchar_t*, const char*, int maxlen);
extern char* wstr_to_str(char*, const wchar_t*, int maxlen);

// box vertices
struct BoxVerts
{
	float verts[8];
	operator float*() { return verts; }
	operator const float*() const { return verts; }
};

extern void box_verts_with_bounds(BoxVerts& box, float left, float bottom, float right, float top);
extern void box_verts_with_pos(BoxVerts& box, float x, float y, float width, float height);

extern void draw_gl_quad(const float* coord, const float* uv=0);

extern const wchar_t* utf8_to_utf16(const unsigned char* val);
extern const unsigned char* utf16_to_utf8(const wchar_t* val);

extern const wchar_t* html_encode(const wchar_t*);

}
