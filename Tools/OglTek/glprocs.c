/*
** GLprocs utility for getting function addresses for OpenGL(R) 1.2, 
** OpenGL 1.3 and OpenGL extension functions.
**
** Version:  1.0
**
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: This software was created using the
** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
** not been independently verified as being compliant with the OpenGL(R)
** version 1.2.1 Specification.
**
** Initial version of glprocs.{c,h} contributed by Intel(R) Corporation.
*/

#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32
  #include <windows.h>
  #include "gl.h"  /* Include local "gl.h". Don't include vc32 <GL/gl.h>. */
  #include "glprocs.h"
  #define glGetProcAddress wglGetProcAddress
#else /* GLX */
  #include "gl.h"
  //#include <glx.h>
  #include "glprocs.h"
  #include "agl.h"
#endif

#define _ASSERT(a) assert(a)

#ifdef __ppc__

#import <mach-o/dyld.h>
#import <stdlib.h>
#import <string.h>
void *NSGLGetProcAddress(const char *name)
{
    NSSymbol symbol;
    char *symbolName;
    // Prepend a '_' for the Unix C symbol mangling convention
    symbolName = malloc (strlen (name) + 2);
    strcpy(symbolName + 1, name);
    symbolName[0] = '_';
    symbol = NULL;
    if (NSIsSymbolNameDefined (symbolName))
        symbol = NSLookupAndBindSymbol (symbolName);
    free (symbolName);
    return symbol ? NSAddressOfSymbol (symbol) : NULL;
}

#define glGetProcAddress NSGLGetProcAddress

#endif	// __ppc__

static void APIENTRY InitBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBlendColor");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBlendColor = extproc;

	glBlendColor(red, green, blue, alpha);
}

static void APIENTRY InitBlendEquation (GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBlendEquation");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBlendEquation = extproc;

	glBlendEquation(mode);
}

static void APIENTRY InitDrawRangeElements (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDrawRangeElements");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDrawRangeElements = extproc;

	glDrawRangeElements(mode, start, end, count, type, indices);
}

static void APIENTRY InitColorTable (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTable");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTable = extproc;

	glColorTable(target, internalformat, width, format, type, table);
}

static void APIENTRY InitColorTableParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTableParameterfv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTableParameterfv = extproc;

	glColorTableParameterfv(target, pname, params);
}

static void APIENTRY InitColorTableParameteriv (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTableParameteriv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTableParameteriv = extproc;

	glColorTableParameteriv(target, pname, params);
}

static void APIENTRY InitCopyColorTable (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyColorTable");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyColorTable = extproc;

	glCopyColorTable(target, internalformat, x, y, width);
}

static void APIENTRY InitGetColorTable (GLenum target, GLenum format, GLenum type, GLvoid *table)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTable");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTable = extproc;

	glGetColorTable(target, format, type, table);
}

static void APIENTRY InitGetColorTableParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableParameterfv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableParameterfv = extproc;

	glGetColorTableParameterfv(target, pname, params);
}

static void APIENTRY InitGetColorTableParameteriv (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableParameteriv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableParameteriv = extproc;

	glGetColorTableParameteriv(target, pname, params);
}

static void APIENTRY InitColorSubTable (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorSubTable");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorSubTable = extproc;

	glColorSubTable(target, start, count, format, type, data);
}

static void APIENTRY InitCopyColorSubTable (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyColorSubTable");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyColorSubTable = extproc;

	glCopyColorSubTable(target, start, x, y, width);
}

static void APIENTRY InitConvolutionFilter1D (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionFilter1D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionFilter1D = extproc;

	glConvolutionFilter1D(target, internalformat, width, format, type, image);
}

static void APIENTRY InitConvolutionFilter2D (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionFilter2D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionFilter2D = extproc;

	glConvolutionFilter2D(target, internalformat, width, height, format, type, image);
}

static void APIENTRY InitConvolutionParameterf (GLenum target, GLenum pname, GLfloat params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameterf");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameterf = extproc;

	glConvolutionParameterf(target, pname, params);
}

static void APIENTRY InitConvolutionParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameterfv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameterfv = extproc;

	glConvolutionParameterfv(target, pname, params);
}

static void APIENTRY InitConvolutionParameteri (GLenum target, GLenum pname, GLint params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameteri");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameteri = extproc;

	glConvolutionParameteri(target, pname, params);
}

static void APIENTRY InitConvolutionParameteriv (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameteriv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameteriv = extproc;

	glConvolutionParameteriv(target, pname, params);
}

static void APIENTRY InitCopyConvolutionFilter1D (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyConvolutionFilter1D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyConvolutionFilter1D = extproc;

	glCopyConvolutionFilter1D(target, internalformat, x, y, width);
}

static void APIENTRY InitCopyConvolutionFilter2D (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyConvolutionFilter2D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyConvolutionFilter2D = extproc;

	glCopyConvolutionFilter2D(target, internalformat, x, y, width, height);
}

static void APIENTRY InitGetConvolutionFilter (GLenum target, GLenum format, GLenum type, GLvoid *image)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetConvolutionFilter");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetConvolutionFilter = extproc;

	glGetConvolutionFilter(target, format, type, image);
}

static void APIENTRY InitGetConvolutionParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetConvolutionParameterfv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetConvolutionParameterfv = extproc;

	glGetConvolutionParameterfv(target, pname, params);
}

static void APIENTRY InitGetConvolutionParameteriv (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetConvolutionParameteriv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetConvolutionParameteriv = extproc;

	glGetConvolutionParameteriv(target, pname, params);
}

static void APIENTRY InitGetSeparableFilter (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetSeparableFilter");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetSeparableFilter = extproc;

	glGetSeparableFilter(target, format, type, row, column, span);
}

static void APIENTRY InitSeparableFilter2D (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSeparableFilter2D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSeparableFilter2D = extproc;

	glSeparableFilter2D(target, internalformat, width, height, format, type, row, column);
}

static void APIENTRY InitGetHistogram (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetHistogram");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetHistogram = extproc;

	glGetHistogram(target, reset, format, type, values);
}

static void APIENTRY InitGetHistogramParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetHistogramParameterfv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetHistogramParameterfv = extproc;

	glGetHistogramParameterfv(target, pname, params);
}

static void APIENTRY InitGetHistogramParameteriv (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetHistogramParameteriv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetHistogramParameteriv = extproc;

	glGetHistogramParameteriv(target, pname, params);
}

static void APIENTRY InitGetMinmax (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMinmax");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMinmax = extproc;

	glGetMinmax(target, reset, format, type, values);
}

static void APIENTRY InitGetMinmaxParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMinmaxParameterfv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMinmaxParameterfv = extproc;

	glGetMinmaxParameterfv(target, pname, params);
}

static void APIENTRY InitGetMinmaxParameteriv (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMinmaxParameteriv");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMinmaxParameteriv = extproc;

	glGetMinmaxParameteriv(target, pname, params);
}

static void APIENTRY InitHistogram (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glHistogram");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glHistogram = extproc;

	glHistogram(target, width, internalformat, sink);
}

static void APIENTRY InitMinmax (GLenum target, GLenum internalformat, GLboolean sink)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMinmax");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMinmax = extproc;

	glMinmax(target, internalformat, sink);
}

static void APIENTRY InitResetHistogram (GLenum target)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glResetHistogram");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glResetHistogram = extproc;

	glResetHistogram(target);
}

static void APIENTRY InitResetMinmax (GLenum target)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glResetMinmax");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glResetMinmax = extproc;

	glResetMinmax(target);
}

static void APIENTRY InitTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexImage3D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexImage3D = extproc;

	glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

static void APIENTRY InitTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexSubImage3D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexSubImage3D = extproc;

	glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

static void APIENTRY InitCopyTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyTexSubImage3D");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyTexSubImage3D = extproc;

	glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

static void APIENTRY InitActiveTextureARB (GLenum texture)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glActiveTextureARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glActiveTextureARB = extproc;

	glActiveTextureARB(texture);
}

static void APIENTRY InitClientActiveTextureARB (GLenum texture)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glClientActiveTextureARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glClientActiveTextureARB = extproc;

	glClientActiveTextureARB(texture);
}

static void APIENTRY InitMultiTexCoord1dARB (GLenum target, GLdouble s)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1dARB = extproc;

	glMultiTexCoord1dARB(target, s);
}

static void APIENTRY InitMultiTexCoord1dvARB (GLenum target, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1dvARB = extproc;

	glMultiTexCoord1dvARB(target, v);
}

static void APIENTRY InitMultiTexCoord1fARB (GLenum target, GLfloat s)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1fARB = extproc;

	glMultiTexCoord1fARB(target, s);
}

static void APIENTRY InitMultiTexCoord1fvARB (GLenum target, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1fvARB = extproc;

	glMultiTexCoord1fvARB(target, v);
}

static void APIENTRY InitMultiTexCoord1iARB (GLenum target, GLint s)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1iARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1iARB = extproc;

	glMultiTexCoord1iARB(target, s);
}

static void APIENTRY InitMultiTexCoord1ivARB (GLenum target, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1ivARB = extproc;

	glMultiTexCoord1ivARB(target, v);
}

static void APIENTRY InitMultiTexCoord1sARB (GLenum target, GLshort s)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1sARB = extproc;

	glMultiTexCoord1sARB(target, s);
}

static void APIENTRY InitMultiTexCoord1svARB (GLenum target, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord1svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord1svARB = extproc;

	glMultiTexCoord1svARB(target, v);
}

static void APIENTRY InitMultiTexCoord2dARB (GLenum target, GLdouble s, GLdouble t)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2dARB = extproc;

	glMultiTexCoord2dARB(target, s, t);
}

static void APIENTRY InitMultiTexCoord2dvARB (GLenum target, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2dvARB = extproc;

	glMultiTexCoord2dvARB(target, v);
}

static void APIENTRY InitMultiTexCoord2fARB (GLenum target, GLfloat s, GLfloat t)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2fARB = extproc;

	glMultiTexCoord2fARB(target, s, t);
}

static void APIENTRY InitMultiTexCoord2fvARB (GLenum target, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2fvARB = extproc;

	glMultiTexCoord2fvARB(target, v);
}

static void APIENTRY InitMultiTexCoord2iARB (GLenum target, GLint s, GLint t)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2iARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2iARB = extproc;

	glMultiTexCoord2iARB(target, s, t);
}

static void APIENTRY InitMultiTexCoord2ivARB (GLenum target, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2ivARB = extproc;

	glMultiTexCoord2ivARB(target, v);
}

static void APIENTRY InitMultiTexCoord2sARB (GLenum target, GLshort s, GLshort t)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2sARB = extproc;

	glMultiTexCoord2sARB(target, s, t);
}

static void APIENTRY InitMultiTexCoord2svARB (GLenum target, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord2svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord2svARB = extproc;

	glMultiTexCoord2svARB(target, v);
}

static void APIENTRY InitMultiTexCoord3dARB (GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3dARB = extproc;

	glMultiTexCoord3dARB(target, s, t, r);
}

static void APIENTRY InitMultiTexCoord3dvARB (GLenum target, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3dvARB = extproc;

	glMultiTexCoord3dvARB(target, v);
}

static void APIENTRY InitMultiTexCoord3fARB (GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3fARB = extproc;

	glMultiTexCoord3fARB(target, s, t, r);
}

static void APIENTRY InitMultiTexCoord3fvARB (GLenum target, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3fvARB = extproc;

	glMultiTexCoord3fvARB(target, v);
}

static void APIENTRY InitMultiTexCoord3iARB (GLenum target, GLint s, GLint t, GLint r)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3iARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3iARB = extproc;

	glMultiTexCoord3iARB(target, s, t, r);
}

static void APIENTRY InitMultiTexCoord3ivARB (GLenum target, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3ivARB = extproc;

	glMultiTexCoord3ivARB(target, v);
}

static void APIENTRY InitMultiTexCoord3sARB (GLenum target, GLshort s, GLshort t, GLshort r)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3sARB = extproc;

	glMultiTexCoord3sARB(target, s, t, r);
}

static void APIENTRY InitMultiTexCoord3svARB (GLenum target, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord3svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord3svARB = extproc;

	glMultiTexCoord3svARB(target, v);
}

static void APIENTRY InitMultiTexCoord4dARB (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4dARB = extproc;

	glMultiTexCoord4dARB(target, s, t, r, q);
}

static void APIENTRY InitMultiTexCoord4dvARB (GLenum target, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4dvARB = extproc;

	glMultiTexCoord4dvARB(target, v);
}

static void APIENTRY InitMultiTexCoord4fARB (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4fARB = extproc;

	glMultiTexCoord4fARB(target, s, t, r, q);
}

static void APIENTRY InitMultiTexCoord4fvARB (GLenum target, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4fvARB = extproc;

	glMultiTexCoord4fvARB(target, v);
}

static void APIENTRY InitMultiTexCoord4iARB (GLenum target, GLint s, GLint t, GLint r, GLint q)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4iARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4iARB = extproc;

	glMultiTexCoord4iARB(target, s, t, r, q);
}

static void APIENTRY InitMultiTexCoord4ivARB (GLenum target, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4ivARB = extproc;

	glMultiTexCoord4ivARB(target, v);
}

static void APIENTRY InitMultiTexCoord4sARB (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4sARB = extproc;

	glMultiTexCoord4sARB(target, s, t, r, q);
}

static void APIENTRY InitMultiTexCoord4svARB (GLenum target, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiTexCoord4svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiTexCoord4svARB = extproc;

	glMultiTexCoord4svARB(target, v);
}

static void APIENTRY InitLoadTransposeMatrixfARB (const GLfloat *m)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glLoadTransposeMatrixfARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glLoadTransposeMatrixfARB = extproc;

	glLoadTransposeMatrixfARB(m);
}

static void APIENTRY InitLoadTransposeMatrixdARB (const GLdouble *m)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glLoadTransposeMatrixdARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glLoadTransposeMatrixdARB = extproc;

	glLoadTransposeMatrixdARB(m);
}

static void APIENTRY InitMultTransposeMatrixfARB (const GLfloat *m)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultTransposeMatrixfARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultTransposeMatrixfARB = extproc;

	glMultTransposeMatrixfARB(m);
}

static void APIENTRY InitMultTransposeMatrixdARB (const GLdouble *m)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultTransposeMatrixdARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultTransposeMatrixdARB = extproc;

	glMultTransposeMatrixdARB(m);
}

static void APIENTRY InitSampleCoverageARB (GLclampf value, GLboolean invert)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSampleCoverageARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSampleCoverageARB = extproc;

	glSampleCoverageARB(value, invert);
}

static void APIENTRY InitCompressedTexImage3DARB (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCompressedTexImage3DARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCompressedTexImage3DARB = extproc;

	glCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data);
}

static void APIENTRY InitCompressedTexImage2DARB (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCompressedTexImage2DARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCompressedTexImage2DARB = extproc;

	glCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data);
}

static void APIENTRY InitCompressedTexImage1DARB (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCompressedTexImage1DARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCompressedTexImage1DARB = extproc;

	glCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data);
}

static void APIENTRY InitCompressedTexSubImage3DARB (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCompressedTexSubImage3DARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCompressedTexSubImage3DARB = extproc;

	glCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

static void APIENTRY InitCompressedTexSubImage2DARB (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCompressedTexSubImage2DARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCompressedTexSubImage2DARB = extproc;

	glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

static void APIENTRY InitCompressedTexSubImage1DARB (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCompressedTexSubImage1DARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCompressedTexSubImage1DARB = extproc;

	glCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data);
}

static void APIENTRY InitGetCompressedTexImageARB (GLenum target, GLint level, void *img)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetCompressedTexImageARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetCompressedTexImageARB = extproc;

	glGetCompressedTexImageARB(target, level, img);
}

static void APIENTRY InitWeightbvARB (GLint size, const GLbyte *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightbvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightbvARB = extproc;

	glWeightbvARB(size, weights);
}

static void APIENTRY InitWeightsvARB (GLint size, const GLshort *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightsvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightsvARB = extproc;

	glWeightsvARB(size, weights);
}

static void APIENTRY InitWeightivARB (GLint size, const GLint *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightivARB = extproc;

	glWeightivARB(size, weights);
}

static void APIENTRY InitWeightfvARB (GLint size, const GLfloat *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightfvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightfvARB = extproc;

	glWeightfvARB(size, weights);
}

static void APIENTRY InitWeightdvARB (GLint size, const GLdouble *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightdvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightdvARB = extproc;

	glWeightdvARB(size, weights);
}

static void APIENTRY InitWeightubvARB (GLint size, const GLubyte *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightubvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightubvARB = extproc;

	glWeightubvARB(size, weights);
}

static void APIENTRY InitWeightusvARB (GLint size, const GLushort *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightusvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightusvARB = extproc;

	glWeightusvARB(size, weights);
}

static void APIENTRY InitWeightuivARB (GLint size, const GLuint *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightuivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightuivARB = extproc;

	glWeightuivARB(size, weights);
}

static void APIENTRY InitWeightPointerARB (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWeightPointerARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWeightPointerARB = extproc;

	glWeightPointerARB(size, type, stride, pointer);
}

static void APIENTRY InitVertexBlendARB (GLint count)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexBlendARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexBlendARB = extproc;

	glVertexBlendARB(count);
}

static void APIENTRY InitCurrentPaletteMatrixARB (GLint index)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCurrentPaletteMatrixARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCurrentPaletteMatrixARB = extproc;

	glCurrentPaletteMatrixARB(index);
}

static void APIENTRY InitMatrixIndexubvARB (GLint size, const GLubyte *indices)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMatrixIndexubvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMatrixIndexubvARB = extproc;

	glMatrixIndexubvARB(size, indices);
}

static void APIENTRY InitMatrixIndexusvARB (GLint size, const GLushort *indices)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMatrixIndexusvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMatrixIndexusvARB = extproc;

	glMatrixIndexusvARB(size, indices);
}

static void APIENTRY InitMatrixIndexuivARB (GLint size, const GLuint *indices)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMatrixIndexuivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMatrixIndexuivARB = extproc;

	glMatrixIndexuivARB(size, indices);
}

static void APIENTRY InitMatrixIndexPointerARB (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMatrixIndexPointerARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMatrixIndexPointerARB = extproc;

	glMatrixIndexPointerARB(size, type, stride, pointer);
}

static void APIENTRY InitWindowPos2dARB (GLdouble x, GLdouble y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2dARB = extproc;

	glWindowPos2dARB(x, y);
}

static void APIENTRY InitWindowPos2dvARB (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2dvARB = extproc;

	glWindowPos2dvARB(v);
}

static void APIENTRY InitWindowPos2fARB (GLfloat x, GLfloat y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2fARB = extproc;

	glWindowPos2fARB(x, y);
}

static void APIENTRY InitWindowPos2fvARB (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2fvARB = extproc;

	glWindowPos2fvARB(v);
}

static void APIENTRY InitWindowPos2iARB (GLint x, GLint y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2iARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2iARB = extproc;

	glWindowPos2iARB(x, y);
}

static void APIENTRY InitWindowPos2ivARB (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2ivARB = extproc;

	glWindowPos2ivARB(v);
}

static void APIENTRY InitWindowPos2sARB (GLshort x, GLshort y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2sARB = extproc;

	glWindowPos2sARB(x, y);
}

static void APIENTRY InitWindowPos2svARB (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2svARB = extproc;

	glWindowPos2svARB(v);
}

static void APIENTRY InitWindowPos3dARB (GLdouble x, GLdouble y, GLdouble z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3dARB = extproc;

	glWindowPos3dARB(x, y, z);
}

static void APIENTRY InitWindowPos3dvARB (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3dvARB = extproc;

	glWindowPos3dvARB(v);
}

static void APIENTRY InitWindowPos3fARB (GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3fARB = extproc;

	glWindowPos3fARB(x, y, z);
}

static void APIENTRY InitWindowPos3fvARB (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3fvARB = extproc;

	glWindowPos3fvARB(v);
}

static void APIENTRY InitWindowPos3iARB (GLint x, GLint y, GLint z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3iARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3iARB = extproc;

	glWindowPos3iARB(x, y, z);
}

static void APIENTRY InitWindowPos3ivARB (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3ivARB = extproc;

	glWindowPos3ivARB(v);
}

static void APIENTRY InitWindowPos3sARB (GLshort x, GLshort y, GLshort z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3sARB = extproc;

	glWindowPos3sARB(x, y, z);
}

static void APIENTRY InitWindowPos3svARB (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3svARB = extproc;

	glWindowPos3svARB(v);
}

static void APIENTRY InitVertexAttrib1dARB (GLuint index, GLdouble x)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1dARB = extproc;

	glVertexAttrib1dARB(index, x);
}

static void APIENTRY InitVertexAttrib1dvARB (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1dvARB = extproc;

	glVertexAttrib1dvARB(index, v);
}

static void APIENTRY InitVertexAttrib1fARB (GLuint index, GLfloat x)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1fARB = extproc;

	glVertexAttrib1fARB(index, x);
}

static void APIENTRY InitVertexAttrib1fvARB (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1fvARB = extproc;

	glVertexAttrib1fvARB(index, v);
}

static void APIENTRY InitVertexAttrib1sARB (GLuint index, GLshort x)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1sARB = extproc;

	glVertexAttrib1sARB(index, x);
}

static void APIENTRY InitVertexAttrib1svARB (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1svARB = extproc;

	glVertexAttrib1svARB(index, v);
}

static void APIENTRY InitVertexAttrib2dARB (GLuint index, GLdouble x, GLdouble y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2dARB = extproc;

	glVertexAttrib2dARB(index, x, y);
}

static void APIENTRY InitVertexAttrib2dvARB (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2dvARB = extproc;

	glVertexAttrib2dvARB(index, v);
}

static void APIENTRY InitVertexAttrib2fARB (GLuint index, GLfloat x, GLfloat y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2fARB = extproc;

	glVertexAttrib2fARB(index, x, y);
}

static void APIENTRY InitVertexAttrib2fvARB (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2fvARB = extproc;

	glVertexAttrib2fvARB(index, v);
}

static void APIENTRY InitVertexAttrib2sARB (GLuint index, GLshort x, GLshort y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2sARB = extproc;

	glVertexAttrib2sARB(index, x, y);
}

static void APIENTRY InitVertexAttrib2svARB (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2svARB = extproc;

	glVertexAttrib2svARB(index, v);
}

static void APIENTRY InitVertexAttrib3dARB (GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3dARB = extproc;

	glVertexAttrib3dARB(index, x, y, z);
}

static void APIENTRY InitVertexAttrib3dvARB (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3dvARB = extproc;

	glVertexAttrib3dvARB(index, v);
}

static void APIENTRY InitVertexAttrib3fARB (GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3fARB = extproc;

	glVertexAttrib3fARB(index, x, y, z);
}

static void APIENTRY InitVertexAttrib3fvARB (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3fvARB = extproc;

	glVertexAttrib3fvARB(index, v);
}

static void APIENTRY InitVertexAttrib3sARB (GLuint index, GLshort x, GLshort y, GLshort z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3sARB = extproc;

	glVertexAttrib3sARB(index, x, y, z);
}

static void APIENTRY InitVertexAttrib3svARB (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3svARB = extproc;

	glVertexAttrib3svARB(index, v);
}

static void APIENTRY InitVertexAttrib4NbvARB (GLuint index, const GLbyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NbvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NbvARB = extproc;

	glVertexAttrib4NbvARB(index, v);
}

static void APIENTRY InitVertexAttrib4NivARB (GLuint index, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NivARB = extproc;

	glVertexAttrib4NivARB(index, v);
}

static void APIENTRY InitVertexAttrib4NsvARB (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NsvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NsvARB = extproc;

	glVertexAttrib4NsvARB(index, v);
}

static void APIENTRY InitVertexAttrib4NubARB (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NubARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NubARB = extproc;

	glVertexAttrib4NubARB(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4NubvARB (GLuint index, const GLubyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NubvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NubvARB = extproc;

	glVertexAttrib4NubvARB(index, v);
}

static void APIENTRY InitVertexAttrib4NuivARB (GLuint index, const GLuint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NuivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NuivARB = extproc;

	glVertexAttrib4NuivARB(index, v);
}

static void APIENTRY InitVertexAttrib4NusvARB (GLuint index, const GLushort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4NusvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4NusvARB = extproc;

	glVertexAttrib4NusvARB(index, v);
}

static void APIENTRY InitVertexAttrib4bvARB (GLuint index, const GLbyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4bvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4bvARB = extproc;

	glVertexAttrib4bvARB(index, v);
}

static void APIENTRY InitVertexAttrib4dARB (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4dARB = extproc;

	glVertexAttrib4dARB(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4dvARB (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4dvARB = extproc;

	glVertexAttrib4dvARB(index, v);
}

static void APIENTRY InitVertexAttrib4fARB (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4fARB = extproc;

	glVertexAttrib4fARB(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4fvARB (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4fvARB = extproc;

	glVertexAttrib4fvARB(index, v);
}

static void APIENTRY InitVertexAttrib4ivARB (GLuint index, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4ivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4ivARB = extproc;

	glVertexAttrib4ivARB(index, v);
}

static void APIENTRY InitVertexAttrib4sARB (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4sARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4sARB = extproc;

	glVertexAttrib4sARB(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4svARB (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4svARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4svARB = extproc;

	glVertexAttrib4svARB(index, v);
}

static void APIENTRY InitVertexAttrib4ubvARB (GLuint index, const GLubyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4ubvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4ubvARB = extproc;

	glVertexAttrib4ubvARB(index, v);
}

static void APIENTRY InitVertexAttrib4uivARB (GLuint index, const GLuint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4uivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4uivARB = extproc;

	glVertexAttrib4uivARB(index, v);
}

static void APIENTRY InitVertexAttrib4usvARB (GLuint index, const GLushort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4usvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4usvARB = extproc;

	glVertexAttrib4usvARB(index, v);
}

static void APIENTRY InitVertexAttribPointerARB (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribPointerARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribPointerARB = extproc;

	glVertexAttribPointerARB(index, size, type, normalized, stride, pointer);
}

static void APIENTRY InitEnableVertexAttribArrayARB (GLuint index)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glEnableVertexAttribArrayARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glEnableVertexAttribArrayARB = extproc;

	glEnableVertexAttribArrayARB(index);
}

static void APIENTRY InitDisableVertexAttribArrayARB (GLuint index)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDisableVertexAttribArrayARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDisableVertexAttribArrayARB = extproc;

	glDisableVertexAttribArrayARB(index);
}

static void APIENTRY InitProgramStringARB (GLenum target, GLenum format, GLsizei len, const GLvoid *string)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramStringARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramStringARB = extproc;

	glProgramStringARB(target, format, len, string);
}

static void APIENTRY InitBindProgramARB (GLenum target, GLuint program)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBindProgramARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBindProgramARB = extproc;

	glBindProgramARB(target, program);
}

static void APIENTRY InitDeleteProgramsARB (GLsizei n, const GLuint *programs)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteProgramsARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteProgramsARB = extproc;

	glDeleteProgramsARB(n, programs);
}

static void APIENTRY InitGenProgramsARB (GLsizei n, GLuint *programs)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenProgramsARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenProgramsARB = extproc;

	glGenProgramsARB(n, programs);
}

static void APIENTRY InitProgramEnvParameter4dARB (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramEnvParameter4dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramEnvParameter4dARB = extproc;

	glProgramEnvParameter4dARB(target, index, x, y, z, w);
}

static void APIENTRY InitProgramEnvParameter4dvARB (GLenum target, GLuint index, const GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramEnvParameter4dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramEnvParameter4dvARB = extproc;

	glProgramEnvParameter4dvARB(target, index, params);
}

static void APIENTRY InitProgramEnvParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramEnvParameter4fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramEnvParameter4fARB = extproc;

	glProgramEnvParameter4fARB(target, index, x, y, z, w);
}

static void APIENTRY InitProgramEnvParameter4fvARB (GLenum target, GLuint index, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramEnvParameter4fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramEnvParameter4fvARB = extproc;

	glProgramEnvParameter4fvARB(target, index, params);
}

static void APIENTRY InitProgramLocalParameter4dARB (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramLocalParameter4dARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramLocalParameter4dARB = extproc;

	glProgramLocalParameter4dARB(target, index, x, y, z, w);
}

static void APIENTRY InitProgramLocalParameter4dvARB (GLenum target, GLuint index, const GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramLocalParameter4dvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramLocalParameter4dvARB = extproc;

	glProgramLocalParameter4dvARB(target, index, params);
}

static void APIENTRY InitProgramLocalParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramLocalParameter4fARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramLocalParameter4fARB = extproc;

	glProgramLocalParameter4fARB(target, index, x, y, z, w);
}

static void APIENTRY InitProgramLocalParameter4fvARB (GLenum target, GLuint index, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramLocalParameter4fvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramLocalParameter4fvARB = extproc;

	glProgramLocalParameter4fvARB(target, index, params);
}

static void APIENTRY InitGetProgramEnvParameterdvARB (GLenum target, GLuint index, GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramEnvParameterdvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramEnvParameterdvARB = extproc;

	glGetProgramEnvParameterdvARB(target, index, params);
}

static void APIENTRY InitGetProgramEnvParameterfvARB (GLenum target, GLuint index, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramEnvParameterfvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramEnvParameterfvARB = extproc;

	glGetProgramEnvParameterfvARB(target, index, params);
}

static void APIENTRY InitGetProgramLocalParameterdvARB (GLenum target, GLuint index, GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramLocalParameterdvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramLocalParameterdvARB = extproc;

	glGetProgramLocalParameterdvARB(target, index, params);
}

static void APIENTRY InitGetProgramLocalParameterfvARB (GLenum target, GLuint index, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramLocalParameterfvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramLocalParameterfvARB = extproc;

	glGetProgramLocalParameterfvARB(target, index, params);
}

static void APIENTRY InitGetProgramivARB (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramivARB = extproc;

	glGetProgramivARB(target, pname, params);
}

static void APIENTRY InitGetProgramStringARB (GLenum target, GLenum pname, GLvoid *string)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramStringARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramStringARB = extproc;

	glGetProgramStringARB(target, pname, string);
}

static void APIENTRY InitGetVertexAttribdvARB (GLuint index, GLenum pname, GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribdvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribdvARB = extproc;

	glGetVertexAttribdvARB(index, pname, params);
}

static void APIENTRY InitGetVertexAttribfvARB (GLuint index, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribfvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribfvARB = extproc;

	glGetVertexAttribfvARB(index, pname, params);
}

static void APIENTRY InitGetVertexAttribivARB (GLuint index, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribivARB = extproc;

	glGetVertexAttribivARB(index, pname, params);
}

static void APIENTRY InitGetVertexAttribPointervARB (GLuint index, GLenum pname, GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribPointervARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribPointervARB = extproc;

	glGetVertexAttribPointervARB(index, pname, pointer);
}

static GLboolean APIENTRY InitIsProgramARB (GLuint program)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsProgramARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsProgramARB = extproc;

	return glIsProgramARB(program);
}

static void APIENTRY InitBindBufferARB (GLenum target, GLuint buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBindBufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBindBufferARB = extproc;

	glBindBufferARB(target, buffer);
}

static void APIENTRY InitDeleteBuffersARB (GLsizei n, const GLuint *buffers)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteBuffersARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteBuffersARB = extproc;

	glDeleteBuffersARB(n, buffers);
}

static void APIENTRY InitGenBuffersARB (GLsizei n, GLuint *buffers)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenBuffersARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenBuffersARB = extproc;

	glGenBuffersARB(n, buffers);
}

static GLboolean APIENTRY InitIsBufferARB (GLuint buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsBufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsBufferARB = extproc;

	return glIsBufferARB(buffer);
}

static void APIENTRY InitBufferDataARB (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBufferDataARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBufferDataARB = extproc;

	glBufferDataARB(target, size, data, usage);
}

static void APIENTRY InitBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBufferSubDataARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBufferSubDataARB = extproc;

	glBufferSubDataARB(target, offset, size, data);
}

static void APIENTRY InitGetBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetBufferSubDataARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetBufferSubDataARB = extproc;

	glGetBufferSubDataARB(target, offset, size, data);
}

static GLvoid* APIENTRY InitMapBufferARB (GLenum target, GLenum access)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMapBufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glMapBufferARB = extproc;

	return glMapBufferARB(target, access);
}

static GLboolean APIENTRY InitUnmapBufferARB (GLenum target)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glUnmapBufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glUnmapBufferARB = extproc;

	return glUnmapBufferARB(target);
}

static void APIENTRY InitGetBufferParameterivARB (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetBufferParameterivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetBufferParameterivARB = extproc;

	glGetBufferParameterivARB(target, pname, params);
}

static void APIENTRY InitGetBufferPointervARB (GLenum target, GLenum pname, GLvoid* *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetBufferPointervARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetBufferPointervARB = extproc;

	glGetBufferPointervARB(target, pname, params);
}

static void APIENTRY InitBlendColorEXT (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBlendColorEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBlendColorEXT = extproc;

	glBlendColorEXT(red, green, blue, alpha);
}

static void APIENTRY InitPolygonOffsetEXT (GLfloat factor, GLfloat bias)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPolygonOffsetEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPolygonOffsetEXT = extproc;

	glPolygonOffsetEXT(factor, bias);
}

static void APIENTRY InitTexImage3DEXT (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexImage3DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexImage3DEXT = extproc;

	glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

static void APIENTRY InitTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexSubImage3DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexSubImage3DEXT = extproc;

	glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

static void APIENTRY InitGetTexFilterFuncSGIS (GLenum target, GLenum filter, GLfloat *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetTexFilterFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetTexFilterFuncSGIS = extproc;

	glGetTexFilterFuncSGIS(target, filter, weights);
}

static void APIENTRY InitTexFilterFuncSGIS (GLenum target, GLenum filter, GLsizei n, const GLfloat *weights)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexFilterFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexFilterFuncSGIS = extproc;

	glTexFilterFuncSGIS(target, filter, n, weights);
}

static void APIENTRY InitTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexSubImage1DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexSubImage1DEXT = extproc;

	glTexSubImage1DEXT(target, level, xoffset, width, format, type, pixels);
}

static void APIENTRY InitTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexSubImage2DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexSubImage2DEXT = extproc;

	glTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void APIENTRY InitCopyTexImage1DEXT (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyTexImage1DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyTexImage1DEXT = extproc;

	glCopyTexImage1DEXT(target, level, internalformat, x, y, width, border);
}

static void APIENTRY InitCopyTexImage2DEXT (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyTexImage2DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyTexImage2DEXT = extproc;

	glCopyTexImage2DEXT(target, level, internalformat, x, y, width, height, border);
}

static void APIENTRY InitCopyTexSubImage1DEXT (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyTexSubImage1DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyTexSubImage1DEXT = extproc;

	glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width);
}

static void APIENTRY InitCopyTexSubImage2DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyTexSubImage2DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyTexSubImage2DEXT = extproc;

	glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height);
}

static void APIENTRY InitCopyTexSubImage3DEXT (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyTexSubImage3DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyTexSubImage3DEXT = extproc;

	glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

static void APIENTRY InitGetHistogramEXT (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetHistogramEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetHistogramEXT = extproc;

	glGetHistogramEXT(target, reset, format, type, values);
}

static void APIENTRY InitGetHistogramParameterfvEXT (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetHistogramParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetHistogramParameterfvEXT = extproc;

	glGetHistogramParameterfvEXT(target, pname, params);
}

static void APIENTRY InitGetHistogramParameterivEXT (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetHistogramParameterivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetHistogramParameterivEXT = extproc;

	glGetHistogramParameterivEXT(target, pname, params);
}

static void APIENTRY InitGetMinmaxEXT (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMinmaxEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMinmaxEXT = extproc;

	glGetMinmaxEXT(target, reset, format, type, values);
}

static void APIENTRY InitGetMinmaxParameterfvEXT (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMinmaxParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMinmaxParameterfvEXT = extproc;

	glGetMinmaxParameterfvEXT(target, pname, params);
}

static void APIENTRY InitGetMinmaxParameterivEXT (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMinmaxParameterivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMinmaxParameterivEXT = extproc;

	glGetMinmaxParameterivEXT(target, pname, params);
}

static void APIENTRY InitHistogramEXT (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glHistogramEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glHistogramEXT = extproc;

	glHistogramEXT(target, width, internalformat, sink);
}

static void APIENTRY InitMinmaxEXT (GLenum target, GLenum internalformat, GLboolean sink)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMinmaxEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMinmaxEXT = extproc;

	glMinmaxEXT(target, internalformat, sink);
}

static void APIENTRY InitResetHistogramEXT (GLenum target)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glResetHistogramEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glResetHistogramEXT = extproc;

	glResetHistogramEXT(target);
}

static void APIENTRY InitResetMinmaxEXT (GLenum target)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glResetMinmaxEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glResetMinmaxEXT = extproc;

	glResetMinmaxEXT(target);
}

static void APIENTRY InitConvolutionFilter1DEXT (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionFilter1DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionFilter1DEXT = extproc;

	glConvolutionFilter1DEXT(target, internalformat, width, format, type, image);
}

static void APIENTRY InitConvolutionFilter2DEXT (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionFilter2DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionFilter2DEXT = extproc;

	glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, image);
}

static void APIENTRY InitConvolutionParameterfEXT (GLenum target, GLenum pname, GLfloat params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameterfEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameterfEXT = extproc;

	glConvolutionParameterfEXT(target, pname, params);
}

static void APIENTRY InitConvolutionParameterfvEXT (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameterfvEXT = extproc;

	glConvolutionParameterfvEXT(target, pname, params);
}

static void APIENTRY InitConvolutionParameteriEXT (GLenum target, GLenum pname, GLint params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameteriEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameteriEXT = extproc;

	glConvolutionParameteriEXT(target, pname, params);
}

static void APIENTRY InitConvolutionParameterivEXT (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glConvolutionParameterivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glConvolutionParameterivEXT = extproc;

	glConvolutionParameterivEXT(target, pname, params);
}

static void APIENTRY InitCopyConvolutionFilter1DEXT (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyConvolutionFilter1DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyConvolutionFilter1DEXT = extproc;

	glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width);
}

static void APIENTRY InitCopyConvolutionFilter2DEXT (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyConvolutionFilter2DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyConvolutionFilter2DEXT = extproc;

	glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height);
}

static void APIENTRY InitGetConvolutionFilterEXT (GLenum target, GLenum format, GLenum type, GLvoid *image)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetConvolutionFilterEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetConvolutionFilterEXT = extproc;

	glGetConvolutionFilterEXT(target, format, type, image);
}

static void APIENTRY InitGetConvolutionParameterfvEXT (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetConvolutionParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetConvolutionParameterfvEXT = extproc;

	glGetConvolutionParameterfvEXT(target, pname, params);
}

static void APIENTRY InitGetConvolutionParameterivEXT (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetConvolutionParameterivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetConvolutionParameterivEXT = extproc;

	glGetConvolutionParameterivEXT(target, pname, params);
}

static void APIENTRY InitGetSeparableFilterEXT (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetSeparableFilterEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetSeparableFilterEXT = extproc;

	glGetSeparableFilterEXT(target, format, type, row, column, span);
}

static void APIENTRY InitSeparableFilter2DEXT (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSeparableFilter2DEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSeparableFilter2DEXT = extproc;

	glSeparableFilter2DEXT(target, internalformat, width, height, format, type, row, column);
}

static void APIENTRY InitColorTableSGI (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTableSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTableSGI = extproc;

	glColorTableSGI(target, internalformat, width, format, type, table);
}

static void APIENTRY InitColorTableParameterfvSGI (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTableParameterfvSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTableParameterfvSGI = extproc;

	glColorTableParameterfvSGI(target, pname, params);
}

static void APIENTRY InitColorTableParameterivSGI (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTableParameterivSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTableParameterivSGI = extproc;

	glColorTableParameterivSGI(target, pname, params);
}

static void APIENTRY InitCopyColorTableSGI (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyColorTableSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyColorTableSGI = extproc;

	glCopyColorTableSGI(target, internalformat, x, y, width);
}

static void APIENTRY InitGetColorTableSGI (GLenum target, GLenum format, GLenum type, GLvoid *table)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableSGI = extproc;

	glGetColorTableSGI(target, format, type, table);
}

static void APIENTRY InitGetColorTableParameterfvSGI (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableParameterfvSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableParameterfvSGI = extproc;

	glGetColorTableParameterfvSGI(target, pname, params);
}

static void APIENTRY InitGetColorTableParameterivSGI (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableParameterivSGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableParameterivSGI = extproc;

	glGetColorTableParameterivSGI(target, pname, params);
}

static void APIENTRY InitPixelTexGenSGIX (GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTexGenSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTexGenSGIX = extproc;

	glPixelTexGenSGIX(mode);
}

static void APIENTRY InitPixelTexGenParameteriSGIS (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTexGenParameteriSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTexGenParameteriSGIS = extproc;

	glPixelTexGenParameteriSGIS(pname, param);
}

static void APIENTRY InitPixelTexGenParameterivSGIS (GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTexGenParameterivSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTexGenParameterivSGIS = extproc;

	glPixelTexGenParameterivSGIS(pname, params);
}

static void APIENTRY InitPixelTexGenParameterfSGIS (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTexGenParameterfSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTexGenParameterfSGIS = extproc;

	glPixelTexGenParameterfSGIS(pname, param);
}

static void APIENTRY InitPixelTexGenParameterfvSGIS (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTexGenParameterfvSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTexGenParameterfvSGIS = extproc;

	glPixelTexGenParameterfvSGIS(pname, params);
}

static void APIENTRY InitGetPixelTexGenParameterivSGIS (GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetPixelTexGenParameterivSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetPixelTexGenParameterivSGIS = extproc;

	glGetPixelTexGenParameterivSGIS(pname, params);
}

static void APIENTRY InitGetPixelTexGenParameterfvSGIS (GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetPixelTexGenParameterfvSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetPixelTexGenParameterfvSGIS = extproc;

	glGetPixelTexGenParameterfvSGIS(pname, params);
}

static void APIENTRY InitTexImage4DSGIS (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexImage4DSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexImage4DSGIS = extproc;

	glTexImage4DSGIS(target, level, internalformat, width, height, depth, size4d, border, format, type, pixels);
}

static void APIENTRY InitTexSubImage4DSGIS (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexSubImage4DSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexSubImage4DSGIS = extproc;

	glTexSubImage4DSGIS(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels);
}

static GLboolean APIENTRY InitAreTexturesResidentEXT (GLsizei n, const GLuint *textures, GLboolean *residences)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAreTexturesResidentEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glAreTexturesResidentEXT = extproc;

	return glAreTexturesResidentEXT(n, textures, residences);
}

static void APIENTRY InitBindTextureEXT (GLenum target, GLuint texture)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBindTextureEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBindTextureEXT = extproc;

	glBindTextureEXT(target, texture);
}

static void APIENTRY InitDeleteTexturesEXT (GLsizei n, const GLuint *textures)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteTexturesEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteTexturesEXT = extproc;

	glDeleteTexturesEXT(n, textures);
}

static void APIENTRY InitGenTexturesEXT (GLsizei n, GLuint *textures)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenTexturesEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenTexturesEXT = extproc;

	glGenTexturesEXT(n, textures);
}

static GLboolean APIENTRY InitIsTextureEXT (GLuint texture)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsTextureEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsTextureEXT = extproc;

	return glIsTextureEXT(texture);
}

static void APIENTRY InitPrioritizeTexturesEXT (GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPrioritizeTexturesEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPrioritizeTexturesEXT = extproc;

	glPrioritizeTexturesEXT(n, textures, priorities);
}

static void APIENTRY InitDetailTexFuncSGIS (GLenum target, GLsizei n, const GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDetailTexFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDetailTexFuncSGIS = extproc;

	glDetailTexFuncSGIS(target, n, points);
}

static void APIENTRY InitGetDetailTexFuncSGIS (GLenum target, GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetDetailTexFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetDetailTexFuncSGIS = extproc;

	glGetDetailTexFuncSGIS(target, points);
}

static void APIENTRY InitSharpenTexFuncSGIS (GLenum target, GLsizei n, const GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSharpenTexFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSharpenTexFuncSGIS = extproc;

	glSharpenTexFuncSGIS(target, n, points);
}

static void APIENTRY InitGetSharpenTexFuncSGIS (GLenum target, GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetSharpenTexFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetSharpenTexFuncSGIS = extproc;

	glGetSharpenTexFuncSGIS(target, points);
}

static void APIENTRY InitSampleMaskSGIS (GLclampf value, GLboolean invert)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSampleMaskSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSampleMaskSGIS = extproc;

	glSampleMaskSGIS(value, invert);
}

static void APIENTRY InitSamplePatternSGIS (GLenum pattern)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSamplePatternSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSamplePatternSGIS = extproc;

	glSamplePatternSGIS(pattern);
}

static void APIENTRY InitArrayElementEXT (GLint i)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glArrayElementEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glArrayElementEXT = extproc;

	glArrayElementEXT(i);
}

static void APIENTRY InitColorPointerEXT (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorPointerEXT = extproc;

	glColorPointerEXT(size, type, stride, count, pointer);
}

static void APIENTRY InitDrawArraysEXT (GLenum mode, GLint first, GLsizei count)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDrawArraysEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDrawArraysEXT = extproc;

	glDrawArraysEXT(mode, first, count);
}

static void APIENTRY InitEdgeFlagPointerEXT (GLsizei stride, GLsizei count, const GLboolean *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glEdgeFlagPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glEdgeFlagPointerEXT = extproc;

	glEdgeFlagPointerEXT(stride, count, pointer);
}

static void APIENTRY InitGetPointervEXT (GLenum pname, GLvoid* *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetPointervEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetPointervEXT = extproc;

	glGetPointervEXT(pname, params);
}

static void APIENTRY InitIndexPointerEXT (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIndexPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glIndexPointerEXT = extproc;

	glIndexPointerEXT(type, stride, count, pointer);
}

static void APIENTRY InitNormalPointerEXT (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalPointerEXT = extproc;

	glNormalPointerEXT(type, stride, count, pointer);
}

static void APIENTRY InitTexCoordPointerEXT (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoordPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoordPointerEXT = extproc;

	glTexCoordPointerEXT(size, type, stride, count, pointer);
}

static void APIENTRY InitVertexPointerEXT (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexPointerEXT = extproc;

	glVertexPointerEXT(size, type, stride, count, pointer);
}

static void APIENTRY InitBlendEquationEXT (GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBlendEquationEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBlendEquationEXT = extproc;

	glBlendEquationEXT(mode);
}

static void APIENTRY InitSpriteParameterfSGIX (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSpriteParameterfSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSpriteParameterfSGIX = extproc;

	glSpriteParameterfSGIX(pname, param);
}

static void APIENTRY InitSpriteParameterfvSGIX (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSpriteParameterfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSpriteParameterfvSGIX = extproc;

	glSpriteParameterfvSGIX(pname, params);
}

static void APIENTRY InitSpriteParameteriSGIX (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSpriteParameteriSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSpriteParameteriSGIX = extproc;

	glSpriteParameteriSGIX(pname, param);
}

static void APIENTRY InitSpriteParameterivSGIX (GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSpriteParameterivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSpriteParameterivSGIX = extproc;

	glSpriteParameterivSGIX(pname, params);
}

static void APIENTRY InitPointParameterfARB (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterfARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterfARB = extproc;

	glPointParameterfARB(pname, param);
}

static void APIENTRY InitPointParameterfvARB (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterfvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterfvARB = extproc;

	glPointParameterfvARB(pname, params);
}

static void APIENTRY InitPointParameterfEXT (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterfEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterfEXT = extproc;

	glPointParameterfEXT(pname, param);
}

static void APIENTRY InitPointParameterfvEXT (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterfvEXT = extproc;

	glPointParameterfvEXT(pname, params);
}

static void APIENTRY InitPointParameterfSGIS (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterfSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterfSGIS = extproc;

	glPointParameterfSGIS(pname, param);
}

static void APIENTRY InitPointParameterfvSGIS (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterfvSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterfvSGIS = extproc;

	glPointParameterfvSGIS(pname, params);
}

static GLint APIENTRY InitGetInstrumentsSGIX (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetInstrumentsSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glGetInstrumentsSGIX = extproc;

	return glGetInstrumentsSGIX();
}

static void APIENTRY InitInstrumentsBufferSGIX (GLsizei size, GLint *buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glInstrumentsBufferSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glInstrumentsBufferSGIX = extproc;

	glInstrumentsBufferSGIX(size, buffer);
}

static GLint APIENTRY InitPollInstrumentsSGIX (GLint *marker_p)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPollInstrumentsSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glPollInstrumentsSGIX = extproc;

	return glPollInstrumentsSGIX(marker_p);
}

static void APIENTRY InitReadInstrumentsSGIX (GLint marker)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReadInstrumentsSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReadInstrumentsSGIX = extproc;

	glReadInstrumentsSGIX(marker);
}

static void APIENTRY InitStartInstrumentsSGIX (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glStartInstrumentsSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glStartInstrumentsSGIX = extproc;

	glStartInstrumentsSGIX();
}

static void APIENTRY InitStopInstrumentsSGIX (GLint marker)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glStopInstrumentsSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glStopInstrumentsSGIX = extproc;

	glStopInstrumentsSGIX(marker);
}

static void APIENTRY InitFrameZoomSGIX (GLint factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFrameZoomSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFrameZoomSGIX = extproc;

	glFrameZoomSGIX(factor);
}

static void APIENTRY InitTagSampleBufferSGIX (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTagSampleBufferSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTagSampleBufferSGIX = extproc;

	glTagSampleBufferSGIX();
}

static void APIENTRY InitDeformationMap3dSGIX (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeformationMap3dSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeformationMap3dSGIX = extproc;

	glDeformationMap3dSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points);
}

static void APIENTRY InitDeformationMap3fSGIX (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeformationMap3fSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeformationMap3fSGIX = extproc;

	glDeformationMap3fSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points);
}

static void APIENTRY InitDeformSGIX (GLbitfield mask)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeformSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeformSGIX = extproc;

	glDeformSGIX(mask);
}

static void APIENTRY InitLoadIdentityDeformationMapSGIX (GLbitfield mask)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glLoadIdentityDeformationMapSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glLoadIdentityDeformationMapSGIX = extproc;

	glLoadIdentityDeformationMapSGIX(mask);
}

static void APIENTRY InitReferencePlaneSGIX (const GLdouble *equation)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReferencePlaneSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReferencePlaneSGIX = extproc;

	glReferencePlaneSGIX(equation);
}

static void APIENTRY InitFlushRasterSGIX (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFlushRasterSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFlushRasterSGIX = extproc;

	glFlushRasterSGIX();
}

static void APIENTRY InitFogFuncSGIS (GLsizei n, const GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogFuncSGIS = extproc;

	glFogFuncSGIS(n, points);
}

static void APIENTRY InitGetFogFuncSGIS (GLfloat *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFogFuncSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFogFuncSGIS = extproc;

	glGetFogFuncSGIS(points);
}

static void APIENTRY InitImageTransformParameteriHP (GLenum target, GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glImageTransformParameteriHP");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glImageTransformParameteriHP = extproc;

	glImageTransformParameteriHP(target, pname, param);
}

static void APIENTRY InitImageTransformParameterfHP (GLenum target, GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glImageTransformParameterfHP");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glImageTransformParameterfHP = extproc;

	glImageTransformParameterfHP(target, pname, param);
}

static void APIENTRY InitImageTransformParameterivHP (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glImageTransformParameterivHP");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glImageTransformParameterivHP = extproc;

	glImageTransformParameterivHP(target, pname, params);
}

static void APIENTRY InitImageTransformParameterfvHP (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glImageTransformParameterfvHP");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glImageTransformParameterfvHP = extproc;

	glImageTransformParameterfvHP(target, pname, params);
}

static void APIENTRY InitGetImageTransformParameterivHP (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetImageTransformParameterivHP");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetImageTransformParameterivHP = extproc;

	glGetImageTransformParameterivHP(target, pname, params);
}

static void APIENTRY InitGetImageTransformParameterfvHP (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetImageTransformParameterfvHP");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetImageTransformParameterfvHP = extproc;

	glGetImageTransformParameterfvHP(target, pname, params);
}

static void APIENTRY InitColorSubTableEXT (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorSubTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorSubTableEXT = extproc;

	glColorSubTableEXT(target, start, count, format, type, data);
}

static void APIENTRY InitCopyColorSubTableEXT (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCopyColorSubTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCopyColorSubTableEXT = extproc;

	glCopyColorSubTableEXT(target, start, x, y, width);
}

static void APIENTRY InitHintPGI (GLenum target, GLint mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glHintPGI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glHintPGI = extproc;

	glHintPGI(target, mode);
}

static void APIENTRY InitColorTableEXT (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorTableEXT = extproc;

	glColorTableEXT(target, internalFormat, width, format, type, table);
}

static void APIENTRY InitGetColorTableEXT (GLenum target, GLenum format, GLenum type, GLvoid *data)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableEXT = extproc;

	glGetColorTableEXT(target, format, type, data);
}

static void APIENTRY InitGetColorTableParameterivEXT (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableParameterivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableParameterivEXT = extproc;

	glGetColorTableParameterivEXT(target, pname, params);
}

static void APIENTRY InitGetColorTableParameterfvEXT (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetColorTableParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetColorTableParameterfvEXT = extproc;

	glGetColorTableParameterfvEXT(target, pname, params);
}

static void APIENTRY InitGetListParameterfvSGIX (GLuint list, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetListParameterfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetListParameterfvSGIX = extproc;

	glGetListParameterfvSGIX(list, pname, params);
}

static void APIENTRY InitGetListParameterivSGIX (GLuint list, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetListParameterivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetListParameterivSGIX = extproc;

	glGetListParameterivSGIX(list, pname, params);
}

static void APIENTRY InitListParameterfSGIX (GLuint list, GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glListParameterfSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glListParameterfSGIX = extproc;

	glListParameterfSGIX(list, pname, param);
}

static void APIENTRY InitListParameterfvSGIX (GLuint list, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glListParameterfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glListParameterfvSGIX = extproc;

	glListParameterfvSGIX(list, pname, params);
}

static void APIENTRY InitListParameteriSGIX (GLuint list, GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glListParameteriSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glListParameteriSGIX = extproc;

	glListParameteriSGIX(list, pname, param);
}

static void APIENTRY InitListParameterivSGIX (GLuint list, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glListParameterivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glListParameterivSGIX = extproc;

	glListParameterivSGIX(list, pname, params);
}

static void APIENTRY InitIndexMaterialEXT (GLenum face, GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIndexMaterialEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glIndexMaterialEXT = extproc;

	glIndexMaterialEXT(face, mode);
}

static void APIENTRY InitIndexFuncEXT (GLenum func, GLclampf ref)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIndexFuncEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glIndexFuncEXT = extproc;

	glIndexFuncEXT(func, ref);
}

static void APIENTRY InitLockArraysEXT (GLint first, GLsizei count)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glLockArraysEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glLockArraysEXT = extproc;

	glLockArraysEXT(first, count);
}

static void APIENTRY InitUnlockArraysEXT (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glUnlockArraysEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glUnlockArraysEXT = extproc;

	glUnlockArraysEXT();
}

static void APIENTRY InitCullParameterdvEXT (GLenum pname, GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCullParameterdvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCullParameterdvEXT = extproc;

	glCullParameterdvEXT(pname, params);
}

static void APIENTRY InitCullParameterfvEXT (GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCullParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCullParameterfvEXT = extproc;

	glCullParameterfvEXT(pname, params);
}

static void APIENTRY InitFragmentColorMaterialSGIX (GLenum face, GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentColorMaterialSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentColorMaterialSGIX = extproc;

	glFragmentColorMaterialSGIX(face, mode);
}

static void APIENTRY InitFragmentLightfSGIX (GLenum light, GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightfSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightfSGIX = extproc;

	glFragmentLightfSGIX(light, pname, param);
}

static void APIENTRY InitFragmentLightfvSGIX (GLenum light, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightfvSGIX = extproc;

	glFragmentLightfvSGIX(light, pname, params);
}

static void APIENTRY InitFragmentLightiSGIX (GLenum light, GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightiSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightiSGIX = extproc;

	glFragmentLightiSGIX(light, pname, param);
}

static void APIENTRY InitFragmentLightivSGIX (GLenum light, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightivSGIX = extproc;

	glFragmentLightivSGIX(light, pname, params);
}

static void APIENTRY InitFragmentLightModelfSGIX (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightModelfSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightModelfSGIX = extproc;

	glFragmentLightModelfSGIX(pname, param);
}

static void APIENTRY InitFragmentLightModelfvSGIX (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightModelfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightModelfvSGIX = extproc;

	glFragmentLightModelfvSGIX(pname, params);
}

static void APIENTRY InitFragmentLightModeliSGIX (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightModeliSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightModeliSGIX = extproc;

	glFragmentLightModeliSGIX(pname, param);
}

static void APIENTRY InitFragmentLightModelivSGIX (GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentLightModelivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentLightModelivSGIX = extproc;

	glFragmentLightModelivSGIX(pname, params);
}

static void APIENTRY InitFragmentMaterialfSGIX (GLenum face, GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentMaterialfSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentMaterialfSGIX = extproc;

	glFragmentMaterialfSGIX(face, pname, param);
}

static void APIENTRY InitFragmentMaterialfvSGIX (GLenum face, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentMaterialfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentMaterialfvSGIX = extproc;

	glFragmentMaterialfvSGIX(face, pname, params);
}

static void APIENTRY InitFragmentMaterialiSGIX (GLenum face, GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentMaterialiSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentMaterialiSGIX = extproc;

	glFragmentMaterialiSGIX(face, pname, param);
}

static void APIENTRY InitFragmentMaterialivSGIX (GLenum face, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFragmentMaterialivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFragmentMaterialivSGIX = extproc;

	glFragmentMaterialivSGIX(face, pname, params);
}

static void APIENTRY InitGetFragmentLightfvSGIX (GLenum light, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFragmentLightfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFragmentLightfvSGIX = extproc;

	glGetFragmentLightfvSGIX(light, pname, params);
}

static void APIENTRY InitGetFragmentLightivSGIX (GLenum light, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFragmentLightivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFragmentLightivSGIX = extproc;

	glGetFragmentLightivSGIX(light, pname, params);
}

static void APIENTRY InitGetFragmentMaterialfvSGIX (GLenum face, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFragmentMaterialfvSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFragmentMaterialfvSGIX = extproc;

	glGetFragmentMaterialfvSGIX(face, pname, params);
}

static void APIENTRY InitGetFragmentMaterialivSGIX (GLenum face, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFragmentMaterialivSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFragmentMaterialivSGIX = extproc;

	glGetFragmentMaterialivSGIX(face, pname, params);
}

static void APIENTRY InitLightEnviSGIX (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glLightEnviSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glLightEnviSGIX = extproc;

	glLightEnviSGIX(pname, param);
}

static void APIENTRY InitDrawRangeElementsEXT (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDrawRangeElementsEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDrawRangeElementsEXT = extproc;

	glDrawRangeElementsEXT(mode, start, end, count, type, indices);
}

static void APIENTRY InitApplyTextureEXT (GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glApplyTextureEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glApplyTextureEXT = extproc;

	glApplyTextureEXT(mode);
}

static void APIENTRY InitTextureLightEXT (GLenum pname)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTextureLightEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTextureLightEXT = extproc;

	glTextureLightEXT(pname);
}

static void APIENTRY InitTextureMaterialEXT (GLenum face, GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTextureMaterialEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTextureMaterialEXT = extproc;

	glTextureMaterialEXT(face, mode);
}

static void APIENTRY InitAsyncMarkerSGIX (GLuint marker)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAsyncMarkerSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glAsyncMarkerSGIX = extproc;

	glAsyncMarkerSGIX(marker);
}

static GLint APIENTRY InitFinishAsyncSGIX (GLuint *markerp)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFinishAsyncSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glFinishAsyncSGIX = extproc;

	return glFinishAsyncSGIX(markerp);
}

static GLint APIENTRY InitPollAsyncSGIX (GLuint *markerp)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPollAsyncSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glPollAsyncSGIX = extproc;

	return glPollAsyncSGIX(markerp);
}

static GLuint APIENTRY InitGenAsyncMarkersSGIX (GLsizei range)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenAsyncMarkersSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glGenAsyncMarkersSGIX = extproc;

	return glGenAsyncMarkersSGIX(range);
}

static void APIENTRY InitDeleteAsyncMarkersSGIX (GLuint marker, GLsizei range)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteAsyncMarkersSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteAsyncMarkersSGIX = extproc;

	glDeleteAsyncMarkersSGIX(marker, range);
}

static GLboolean APIENTRY InitIsAsyncMarkerSGIX (GLuint marker)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsAsyncMarkerSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsAsyncMarkerSGIX = extproc;

	return glIsAsyncMarkerSGIX(marker);
}

static void APIENTRY InitVertexPointervINTEL (GLint size, GLenum type, const GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexPointervINTEL");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexPointervINTEL = extproc;

	glVertexPointervINTEL(size, type, pointer);
}

static void APIENTRY InitNormalPointervINTEL (GLenum type, const GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalPointervINTEL");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalPointervINTEL = extproc;

	glNormalPointervINTEL(type, pointer);
}

static void APIENTRY InitColorPointervINTEL (GLint size, GLenum type, const GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorPointervINTEL");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorPointervINTEL = extproc;

	glColorPointervINTEL(size, type, pointer);
}

static void APIENTRY InitTexCoordPointervINTEL (GLint size, GLenum type, const GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoordPointervINTEL");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoordPointervINTEL = extproc;

	glTexCoordPointervINTEL(size, type, pointer);
}

static void APIENTRY InitPixelTransformParameteriEXT (GLenum target, GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTransformParameteriEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTransformParameteriEXT = extproc;

	glPixelTransformParameteriEXT(target, pname, param);
}

static void APIENTRY InitPixelTransformParameterfEXT (GLenum target, GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTransformParameterfEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTransformParameterfEXT = extproc;

	glPixelTransformParameterfEXT(target, pname, param);
}

static void APIENTRY InitPixelTransformParameterivEXT (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTransformParameterivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTransformParameterivEXT = extproc;

	glPixelTransformParameterivEXT(target, pname, params);
}

static void APIENTRY InitPixelTransformParameterfvEXT (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPixelTransformParameterfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPixelTransformParameterfvEXT = extproc;

	glPixelTransformParameterfvEXT(target, pname, params);
}

static void APIENTRY InitSecondaryColor3bEXT (GLbyte red, GLbyte green, GLbyte blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3bEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3bEXT = extproc;

	glSecondaryColor3bEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3bvEXT (const GLbyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3bvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3bvEXT = extproc;

	glSecondaryColor3bvEXT(v);
}

static void APIENTRY InitSecondaryColor3dEXT (GLdouble red, GLdouble green, GLdouble blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3dEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3dEXT = extproc;

	glSecondaryColor3dEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3dvEXT (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3dvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3dvEXT = extproc;

	glSecondaryColor3dvEXT(v);
}

static void APIENTRY InitSecondaryColor3fEXT (GLfloat red, GLfloat green, GLfloat blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3fEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3fEXT = extproc;

	glSecondaryColor3fEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3fvEXT (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3fvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3fvEXT = extproc;

	glSecondaryColor3fvEXT(v);
}

static void APIENTRY InitSecondaryColor3iEXT (GLint red, GLint green, GLint blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3iEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3iEXT = extproc;

	glSecondaryColor3iEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3ivEXT (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3ivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3ivEXT = extproc;

	glSecondaryColor3ivEXT(v);
}

static void APIENTRY InitSecondaryColor3sEXT (GLshort red, GLshort green, GLshort blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3sEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3sEXT = extproc;

	glSecondaryColor3sEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3svEXT (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3svEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3svEXT = extproc;

	glSecondaryColor3svEXT(v);
}

static void APIENTRY InitSecondaryColor3ubEXT (GLubyte red, GLubyte green, GLubyte blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3ubEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3ubEXT = extproc;

	glSecondaryColor3ubEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3ubvEXT (const GLubyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3ubvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3ubvEXT = extproc;

	glSecondaryColor3ubvEXT(v);
}

static void APIENTRY InitSecondaryColor3uiEXT (GLuint red, GLuint green, GLuint blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3uiEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3uiEXT = extproc;

	glSecondaryColor3uiEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3uivEXT (const GLuint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3uivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3uivEXT = extproc;

	glSecondaryColor3uivEXT(v);
}

static void APIENTRY InitSecondaryColor3usEXT (GLushort red, GLushort green, GLushort blue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3usEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3usEXT = extproc;

	glSecondaryColor3usEXT(red, green, blue);
}

static void APIENTRY InitSecondaryColor3usvEXT (const GLushort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColor3usvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColor3usvEXT = extproc;

	glSecondaryColor3usvEXT(v);
}

static void APIENTRY InitSecondaryColorPointerEXT (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColorPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColorPointerEXT = extproc;

	glSecondaryColorPointerEXT(size, type, stride, pointer);
}

static void APIENTRY InitTextureNormalEXT (GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTextureNormalEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTextureNormalEXT = extproc;

	glTextureNormalEXT(mode);
}

static void APIENTRY InitMultiDrawArraysEXT (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiDrawArraysEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiDrawArraysEXT = extproc;

	glMultiDrawArraysEXT(mode, first, count, primcount);
}

static void APIENTRY InitMultiDrawElementsEXT (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiDrawElementsEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiDrawElementsEXT = extproc;

	glMultiDrawElementsEXT(mode, count, type, indices, primcount);
}

static void APIENTRY InitFogCoordfEXT (GLfloat coord)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogCoordfEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogCoordfEXT = extproc;

	glFogCoordfEXT(coord);
}

static void APIENTRY InitFogCoordfvEXT (const GLfloat *coord)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogCoordfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogCoordfvEXT = extproc;

	glFogCoordfvEXT(coord);
}

static void APIENTRY InitFogCoorddEXT (GLdouble coord)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogCoorddEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogCoorddEXT = extproc;

	glFogCoorddEXT(coord);
}

static void APIENTRY InitFogCoorddvEXT (const GLdouble *coord)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogCoorddvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogCoorddvEXT = extproc;

	glFogCoorddvEXT(coord);
}

static void APIENTRY InitFogCoordPointerEXT (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogCoordPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogCoordPointerEXT = extproc;

	glFogCoordPointerEXT(type, stride, pointer);
}

static void APIENTRY InitTangent3bEXT (GLbyte tx, GLbyte ty, GLbyte tz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3bEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3bEXT = extproc;

	glTangent3bEXT(tx, ty, tz);
}

static void APIENTRY InitTangent3bvEXT (const GLbyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3bvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3bvEXT = extproc;

	glTangent3bvEXT(v);
}

static void APIENTRY InitTangent3dEXT (GLdouble tx, GLdouble ty, GLdouble tz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3dEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3dEXT = extproc;

	glTangent3dEXT(tx, ty, tz);
}

static void APIENTRY InitTangent3dvEXT (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3dvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3dvEXT = extproc;

	glTangent3dvEXT(v);
}

static void APIENTRY InitTangent3fEXT (GLfloat tx, GLfloat ty, GLfloat tz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3fEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3fEXT = extproc;

	glTangent3fEXT(tx, ty, tz);
}

static void APIENTRY InitTangent3fvEXT (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3fvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3fvEXT = extproc;

	glTangent3fvEXT(v);
}

static void APIENTRY InitTangent3iEXT (GLint tx, GLint ty, GLint tz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3iEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3iEXT = extproc;

	glTangent3iEXT(tx, ty, tz);
}

static void APIENTRY InitTangent3ivEXT (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3ivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3ivEXT = extproc;

	glTangent3ivEXT(v);
}

static void APIENTRY InitTangent3sEXT (GLshort tx, GLshort ty, GLshort tz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3sEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3sEXT = extproc;

	glTangent3sEXT(tx, ty, tz);
}

static void APIENTRY InitTangent3svEXT (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangent3svEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangent3svEXT = extproc;

	glTangent3svEXT(v);
}

static void APIENTRY InitBinormal3bEXT (GLbyte bx, GLbyte by, GLbyte bz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3bEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3bEXT = extproc;

	glBinormal3bEXT(bx, by, bz);
}

static void APIENTRY InitBinormal3bvEXT (const GLbyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3bvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3bvEXT = extproc;

	glBinormal3bvEXT(v);
}

static void APIENTRY InitBinormal3dEXT (GLdouble bx, GLdouble by, GLdouble bz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3dEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3dEXT = extproc;

	glBinormal3dEXT(bx, by, bz);
}

static void APIENTRY InitBinormal3dvEXT (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3dvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3dvEXT = extproc;

	glBinormal3dvEXT(v);
}

static void APIENTRY InitBinormal3fEXT (GLfloat bx, GLfloat by, GLfloat bz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3fEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3fEXT = extproc;

	glBinormal3fEXT(bx, by, bz);
}

static void APIENTRY InitBinormal3fvEXT (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3fvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3fvEXT = extproc;

	glBinormal3fvEXT(v);
}

static void APIENTRY InitBinormal3iEXT (GLint bx, GLint by, GLint bz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3iEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3iEXT = extproc;

	glBinormal3iEXT(bx, by, bz);
}

static void APIENTRY InitBinormal3ivEXT (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3ivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3ivEXT = extproc;

	glBinormal3ivEXT(v);
}

static void APIENTRY InitBinormal3sEXT (GLshort bx, GLshort by, GLshort bz)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3sEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3sEXT = extproc;

	glBinormal3sEXT(bx, by, bz);
}

static void APIENTRY InitBinormal3svEXT (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormal3svEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormal3svEXT = extproc;

	glBinormal3svEXT(v);
}

static void APIENTRY InitTangentPointerEXT (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTangentPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTangentPointerEXT = extproc;

	glTangentPointerEXT(type, stride, pointer);
}

static void APIENTRY InitBinormalPointerEXT (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBinormalPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBinormalPointerEXT = extproc;

	glBinormalPointerEXT(type, stride, pointer);
}

static void APIENTRY InitFinishTextureSUNX (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFinishTextureSUNX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFinishTextureSUNX = extproc;

	glFinishTextureSUNX();
}

static void APIENTRY InitGlobalAlphaFactorbSUN (GLbyte factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactorbSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactorbSUN = extproc;

	glGlobalAlphaFactorbSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactorsSUN (GLshort factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactorsSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactorsSUN = extproc;

	glGlobalAlphaFactorsSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactoriSUN (GLint factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactoriSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactoriSUN = extproc;

	glGlobalAlphaFactoriSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactorfSUN (GLfloat factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactorfSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactorfSUN = extproc;

	glGlobalAlphaFactorfSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactordSUN (GLdouble factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactordSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactordSUN = extproc;

	glGlobalAlphaFactordSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactorubSUN (GLubyte factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactorubSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactorubSUN = extproc;

	glGlobalAlphaFactorubSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactorusSUN (GLushort factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactorusSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactorusSUN = extproc;

	glGlobalAlphaFactorusSUN(factor);
}

static void APIENTRY InitGlobalAlphaFactoruiSUN (GLuint factor)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGlobalAlphaFactoruiSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGlobalAlphaFactoruiSUN = extproc;

	glGlobalAlphaFactoruiSUN(factor);
}

static void APIENTRY InitReplacementCodeuiSUN (GLuint code)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiSUN = extproc;

	glReplacementCodeuiSUN(code);
}

static void APIENTRY InitReplacementCodeusSUN (GLushort code)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeusSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeusSUN = extproc;

	glReplacementCodeusSUN(code);
}

static void APIENTRY InitReplacementCodeubSUN (GLubyte code)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeubSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeubSUN = extproc;

	glReplacementCodeubSUN(code);
}

static void APIENTRY InitReplacementCodeuivSUN (const GLuint *code)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuivSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuivSUN = extproc;

	glReplacementCodeuivSUN(code);
}

static void APIENTRY InitReplacementCodeusvSUN (const GLushort *code)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeusvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeusvSUN = extproc;

	glReplacementCodeusvSUN(code);
}

static void APIENTRY InitReplacementCodeubvSUN (const GLubyte *code)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeubvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeubvSUN = extproc;

	glReplacementCodeubvSUN(code);
}

static void APIENTRY InitReplacementCodePointerSUN (GLenum type, GLsizei stride, const GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodePointerSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodePointerSUN = extproc;

	glReplacementCodePointerSUN(type, stride, pointer);
}

static void APIENTRY InitColor4ubVertex2fSUN (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor4ubVertex2fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor4ubVertex2fSUN = extproc;

	glColor4ubVertex2fSUN(r, g, b, a, x, y);
}

static void APIENTRY InitColor4ubVertex2fvSUN (const GLubyte *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor4ubVertex2fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor4ubVertex2fvSUN = extproc;

	glColor4ubVertex2fvSUN(c, v);
}

static void APIENTRY InitColor4ubVertex3fSUN (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor4ubVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor4ubVertex3fSUN = extproc;

	glColor4ubVertex3fSUN(r, g, b, a, x, y, z);
}

static void APIENTRY InitColor4ubVertex3fvSUN (const GLubyte *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor4ubVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor4ubVertex3fvSUN = extproc;

	glColor4ubVertex3fvSUN(c, v);
}

static void APIENTRY InitColor3fVertex3fSUN (GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor3fVertex3fSUN = extproc;

	glColor3fVertex3fSUN(r, g, b, x, y, z);
}

static void APIENTRY InitColor3fVertex3fvSUN (const GLfloat *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor3fVertex3fvSUN = extproc;

	glColor3fVertex3fvSUN(c, v);
}

static void APIENTRY InitNormal3fVertex3fSUN (GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormal3fVertex3fSUN = extproc;

	glNormal3fVertex3fSUN(nx, ny, nz, x, y, z);
}

static void APIENTRY InitNormal3fVertex3fvSUN (const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormal3fVertex3fvSUN = extproc;

	glNormal3fVertex3fvSUN(n, v);
}

static void APIENTRY InitColor4fNormal3fVertex3fSUN (GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor4fNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor4fNormal3fVertex3fSUN = extproc;

	glColor4fNormal3fVertex3fSUN(r, g, b, a, nx, ny, nz, x, y, z);
}

static void APIENTRY InitColor4fNormal3fVertex3fvSUN (const GLfloat *c, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColor4fNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColor4fNormal3fVertex3fvSUN = extproc;

	glColor4fNormal3fVertex3fvSUN(c, n, v);
}

static void APIENTRY InitTexCoord2fVertex3fSUN (GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fVertex3fSUN = extproc;

	glTexCoord2fVertex3fSUN(s, t, x, y, z);
}

static void APIENTRY InitTexCoord2fVertex3fvSUN (const GLfloat *tc, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fVertex3fvSUN = extproc;

	glTexCoord2fVertex3fvSUN(tc, v);
}

static void APIENTRY InitTexCoord4fVertex4fSUN (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord4fVertex4fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord4fVertex4fSUN = extproc;

	glTexCoord4fVertex4fSUN(s, t, p, q, x, y, z, w);
}

static void APIENTRY InitTexCoord4fVertex4fvSUN (const GLfloat *tc, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord4fVertex4fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord4fVertex4fvSUN = extproc;

	glTexCoord4fVertex4fvSUN(tc, v);
}

static void APIENTRY InitTexCoord2fColor4ubVertex3fSUN (GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fColor4ubVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fColor4ubVertex3fSUN = extproc;

	glTexCoord2fColor4ubVertex3fSUN(s, t, r, g, b, a, x, y, z);
}

static void APIENTRY InitTexCoord2fColor4ubVertex3fvSUN (const GLfloat *tc, const GLubyte *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fColor4ubVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fColor4ubVertex3fvSUN = extproc;

	glTexCoord2fColor4ubVertex3fvSUN(tc, c, v);
}

static void APIENTRY InitTexCoord2fColor3fVertex3fSUN (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fColor3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fColor3fVertex3fSUN = extproc;

	glTexCoord2fColor3fVertex3fSUN(s, t, r, g, b, x, y, z);
}

static void APIENTRY InitTexCoord2fColor3fVertex3fvSUN (const GLfloat *tc, const GLfloat *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fColor3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fColor3fVertex3fvSUN = extproc;

	glTexCoord2fColor3fVertex3fvSUN(tc, c, v);
}

static void APIENTRY InitTexCoord2fNormal3fVertex3fSUN (GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fNormal3fVertex3fSUN = extproc;

	glTexCoord2fNormal3fVertex3fSUN(s, t, nx, ny, nz, x, y, z);
}

static void APIENTRY InitTexCoord2fNormal3fVertex3fvSUN (const GLfloat *tc, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fNormal3fVertex3fvSUN = extproc;

	glTexCoord2fNormal3fVertex3fvSUN(tc, n, v);
}

static void APIENTRY InitTexCoord2fColor4fNormal3fVertex3fSUN (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fColor4fNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fColor4fNormal3fVertex3fSUN = extproc;

	glTexCoord2fColor4fNormal3fVertex3fSUN(s, t, r, g, b, a, nx, ny, nz, x, y, z);
}

static void APIENTRY InitTexCoord2fColor4fNormal3fVertex3fvSUN (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord2fColor4fNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord2fColor4fNormal3fVertex3fvSUN = extproc;

	glTexCoord2fColor4fNormal3fVertex3fvSUN(tc, c, n, v);
}

static void APIENTRY InitTexCoord4fColor4fNormal3fVertex4fSUN (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord4fColor4fNormal3fVertex4fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord4fColor4fNormal3fVertex4fSUN = extproc;

	glTexCoord4fColor4fNormal3fVertex4fSUN(s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w);
}

static void APIENTRY InitTexCoord4fColor4fNormal3fVertex4fvSUN (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoord4fColor4fNormal3fVertex4fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoord4fColor4fNormal3fVertex4fvSUN = extproc;

	glTexCoord4fColor4fNormal3fVertex4fvSUN(tc, c, n, v);
}

static void APIENTRY InitReplacementCodeuiVertex3fSUN (GLenum rc, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiVertex3fSUN = extproc;

	glReplacementCodeuiVertex3fSUN(rc, x, y, z);
}

static void APIENTRY InitReplacementCodeuiVertex3fvSUN (const GLenum *rc, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiVertex3fvSUN = extproc;

	glReplacementCodeuiVertex3fvSUN(rc, v);
}

static void APIENTRY InitReplacementCodeuiColor4ubVertex3fSUN (GLenum rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiColor4ubVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiColor4ubVertex3fSUN = extproc;

	glReplacementCodeuiColor4ubVertex3fSUN(rc, r, g, b, a, x, y, z);
}

static void APIENTRY InitReplacementCodeuiColor4ubVertex3fvSUN (const GLenum *rc, const GLubyte *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiColor4ubVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiColor4ubVertex3fvSUN = extproc;

	glReplacementCodeuiColor4ubVertex3fvSUN(rc, c, v);
}

static void APIENTRY InitReplacementCodeuiColor3fVertex3fSUN (GLenum rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiColor3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiColor3fVertex3fSUN = extproc;

	glReplacementCodeuiColor3fVertex3fSUN(rc, r, g, b, x, y, z);
}

static void APIENTRY InitReplacementCodeuiColor3fVertex3fvSUN (const GLenum *rc, const GLfloat *c, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiColor3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiColor3fVertex3fvSUN = extproc;

	glReplacementCodeuiColor3fVertex3fvSUN(rc, c, v);
}

static void APIENTRY InitReplacementCodeuiNormal3fVertex3fSUN (GLenum rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiNormal3fVertex3fSUN = extproc;

	glReplacementCodeuiNormal3fVertex3fSUN(rc, nx, ny, nz, x, y, z);
}

static void APIENTRY InitReplacementCodeuiNormal3fVertex3fvSUN (const GLenum *rc, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiNormal3fVertex3fvSUN = extproc;

	glReplacementCodeuiNormal3fVertex3fvSUN(rc, n, v);
}

static void APIENTRY InitReplacementCodeuiColor4fNormal3fVertex3fSUN (GLenum rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiColor4fNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiColor4fNormal3fVertex3fSUN = extproc;

	glReplacementCodeuiColor4fNormal3fVertex3fSUN(rc, r, g, b, a, nx, ny, nz, x, y, z);
}

static void APIENTRY InitReplacementCodeuiColor4fNormal3fVertex3fvSUN (const GLenum *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiColor4fNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiColor4fNormal3fVertex3fvSUN = extproc;

	glReplacementCodeuiColor4fNormal3fVertex3fvSUN(rc, c, n, v);
}

static void APIENTRY InitReplacementCodeuiTexCoord2fVertex3fSUN (GLenum rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiTexCoord2fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiTexCoord2fVertex3fSUN = extproc;

	glReplacementCodeuiTexCoord2fVertex3fSUN(rc, s, t, x, y, z);
}

static void APIENTRY InitReplacementCodeuiTexCoord2fVertex3fvSUN (const GLenum *rc, const GLfloat *tc, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiTexCoord2fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiTexCoord2fVertex3fvSUN = extproc;

	glReplacementCodeuiTexCoord2fVertex3fvSUN(rc, tc, v);
}

static void APIENTRY InitReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (GLenum rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN = extproc;

	glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN(rc, s, t, nx, ny, nz, x, y, z);
}

static void APIENTRY InitReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (const GLenum *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN = extproc;

	glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN(rc, tc, n, v);
}

static void APIENTRY InitReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (GLenum rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN = extproc;

	glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN(rc, s, t, r, g, b, a, nx, ny, nz, x, y, z);
}

static void APIENTRY InitReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (const GLenum *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN = extproc;

	glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN(rc, tc, c, n, v);
}

static void APIENTRY InitBlendFuncSeparateEXT (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBlendFuncSeparateEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBlendFuncSeparateEXT = extproc;

	glBlendFuncSeparateEXT(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

static void APIENTRY InitBlendFuncSeparateINGR (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBlendFuncSeparateINGR");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBlendFuncSeparateINGR = extproc;

	glBlendFuncSeparateINGR(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

static void APIENTRY InitVertexWeightfEXT (GLfloat weight)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexWeightfEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexWeightfEXT = extproc;

	glVertexWeightfEXT(weight);
}

static void APIENTRY InitVertexWeightfvEXT (const GLfloat *weight)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexWeightfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexWeightfvEXT = extproc;

	glVertexWeightfvEXT(weight);
}

static void APIENTRY InitVertexWeightPointerEXT (GLsizei size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexWeightPointerEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexWeightPointerEXT = extproc;

	glVertexWeightPointerEXT(size, type, stride, pointer);
}

static void APIENTRY InitFlushVertexArrayRangeNV (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFlushVertexArrayRangeNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFlushVertexArrayRangeNV = extproc;

	glFlushVertexArrayRangeNV();
}

static void APIENTRY InitVertexArrayRangeNV (GLsizei length, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexArrayRangeNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexArrayRangeNV = extproc;

	glVertexArrayRangeNV(length, pointer);
}

static void APIENTRY InitCombinerParameterfvNV (GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerParameterfvNV = extproc;

	glCombinerParameterfvNV(pname, params);
}

static void APIENTRY InitCombinerParameterfNV (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerParameterfNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerParameterfNV = extproc;

	glCombinerParameterfNV(pname, param);
}

static void APIENTRY InitCombinerParameterivNV (GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerParameterivNV = extproc;

	glCombinerParameterivNV(pname, params);
}

static void APIENTRY InitCombinerParameteriNV (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerParameteriNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerParameteriNV = extproc;

	glCombinerParameteriNV(pname, param);
}

static void APIENTRY InitCombinerInputNV (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerInputNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerInputNV = extproc;

	glCombinerInputNV(stage, portion, variable, input, mapping, componentUsage);
}

static void APIENTRY InitCombinerOutputNV (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerOutputNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerOutputNV = extproc;

	glCombinerOutputNV(stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum);
}

static void APIENTRY InitFinalCombinerInputNV (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFinalCombinerInputNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFinalCombinerInputNV = extproc;

	glFinalCombinerInputNV(variable, input, mapping, componentUsage);
}

static void APIENTRY InitGetCombinerInputParameterfvNV (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetCombinerInputParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetCombinerInputParameterfvNV = extproc;

	glGetCombinerInputParameterfvNV(stage, portion, variable, pname, params);
}

static void APIENTRY InitGetCombinerInputParameterivNV (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetCombinerInputParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetCombinerInputParameterivNV = extproc;

	glGetCombinerInputParameterivNV(stage, portion, variable, pname, params);
}

static void APIENTRY InitGetCombinerOutputParameterfvNV (GLenum stage, GLenum portion, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetCombinerOutputParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetCombinerOutputParameterfvNV = extproc;

	glGetCombinerOutputParameterfvNV(stage, portion, pname, params);
}

static void APIENTRY InitGetCombinerOutputParameterivNV (GLenum stage, GLenum portion, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetCombinerOutputParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetCombinerOutputParameterivNV = extproc;

	glGetCombinerOutputParameterivNV(stage, portion, pname, params);
}

static void APIENTRY InitGetFinalCombinerInputParameterfvNV (GLenum variable, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFinalCombinerInputParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFinalCombinerInputParameterfvNV = extproc;

	glGetFinalCombinerInputParameterfvNV(variable, pname, params);
}

static void APIENTRY InitGetFinalCombinerInputParameterivNV (GLenum variable, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFinalCombinerInputParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFinalCombinerInputParameterivNV = extproc;

	glGetFinalCombinerInputParameterivNV(variable, pname, params);
}

static void APIENTRY InitResizeBuffersMESA (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glResizeBuffersMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glResizeBuffersMESA = extproc;

	glResizeBuffersMESA();
}

static void APIENTRY InitWindowPos2dMESA (GLdouble x, GLdouble y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2dMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2dMESA = extproc;

	glWindowPos2dMESA(x, y);
}

static void APIENTRY InitWindowPos2dvMESA (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2dvMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2dvMESA = extproc;

	glWindowPos2dvMESA(v);
}

static void APIENTRY InitWindowPos2fMESA (GLfloat x, GLfloat y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2fMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2fMESA = extproc;

	glWindowPos2fMESA(x, y);
}

static void APIENTRY InitWindowPos2fvMESA (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2fvMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2fvMESA = extproc;

	glWindowPos2fvMESA(v);
}

static void APIENTRY InitWindowPos2iMESA (GLint x, GLint y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2iMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2iMESA = extproc;

	glWindowPos2iMESA(x, y);
}

static void APIENTRY InitWindowPos2ivMESA (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2ivMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2ivMESA = extproc;

	glWindowPos2ivMESA(v);
}

static void APIENTRY InitWindowPos2sMESA (GLshort x, GLshort y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2sMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2sMESA = extproc;

	glWindowPos2sMESA(x, y);
}

static void APIENTRY InitWindowPos2svMESA (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos2svMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos2svMESA = extproc;

	glWindowPos2svMESA(v);
}

static void APIENTRY InitWindowPos3dMESA (GLdouble x, GLdouble y, GLdouble z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3dMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3dMESA = extproc;

	glWindowPos3dMESA(x, y, z);
}

static void APIENTRY InitWindowPos3dvMESA (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3dvMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3dvMESA = extproc;

	glWindowPos3dvMESA(v);
}

static void APIENTRY InitWindowPos3fMESA (GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3fMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3fMESA = extproc;

	glWindowPos3fMESA(x, y, z);
}

static void APIENTRY InitWindowPos3fvMESA (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3fvMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3fvMESA = extproc;

	glWindowPos3fvMESA(v);
}

static void APIENTRY InitWindowPos3iMESA (GLint x, GLint y, GLint z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3iMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3iMESA = extproc;

	glWindowPos3iMESA(x, y, z);
}

static void APIENTRY InitWindowPos3ivMESA (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3ivMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3ivMESA = extproc;

	glWindowPos3ivMESA(v);
}

static void APIENTRY InitWindowPos3sMESA (GLshort x, GLshort y, GLshort z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3sMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3sMESA = extproc;

	glWindowPos3sMESA(x, y, z);
}

static void APIENTRY InitWindowPos3svMESA (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos3svMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos3svMESA = extproc;

	glWindowPos3svMESA(v);
}

static void APIENTRY InitWindowPos4dMESA (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4dMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4dMESA = extproc;

	glWindowPos4dMESA(x, y, z, w);
}

static void APIENTRY InitWindowPos4dvMESA (const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4dvMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4dvMESA = extproc;

	glWindowPos4dvMESA(v);
}

static void APIENTRY InitWindowPos4fMESA (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4fMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4fMESA = extproc;

	glWindowPos4fMESA(x, y, z, w);
}

static void APIENTRY InitWindowPos4fvMESA (const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4fvMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4fvMESA = extproc;

	glWindowPos4fvMESA(v);
}

static void APIENTRY InitWindowPos4iMESA (GLint x, GLint y, GLint z, GLint w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4iMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4iMESA = extproc;

	glWindowPos4iMESA(x, y, z, w);
}

static void APIENTRY InitWindowPos4ivMESA (const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4ivMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4ivMESA = extproc;

	glWindowPos4ivMESA(v);
}

static void APIENTRY InitWindowPos4sMESA (GLshort x, GLshort y, GLshort z, GLshort w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4sMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4sMESA = extproc;

	glWindowPos4sMESA(x, y, z, w);
}

static void APIENTRY InitWindowPos4svMESA (const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glWindowPos4svMESA");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glWindowPos4svMESA = extproc;

	glWindowPos4svMESA(v);
}

static void APIENTRY InitMultiModeDrawArraysIBM (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiModeDrawArraysIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiModeDrawArraysIBM = extproc;

	glMultiModeDrawArraysIBM(mode, first, count, primcount, modestride);
}

static void APIENTRY InitMultiModeDrawElementsIBM (const GLenum *mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount, GLint modestride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMultiModeDrawElementsIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMultiModeDrawElementsIBM = extproc;

	glMultiModeDrawElementsIBM(mode, count, type, indices, primcount, modestride);
}

static void APIENTRY InitColorPointerListIBM (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorPointerListIBM = extproc;

	glColorPointerListIBM(size, type, stride, pointer, ptrstride);
}

static void APIENTRY InitSecondaryColorPointerListIBM (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSecondaryColorPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSecondaryColorPointerListIBM = extproc;

	glSecondaryColorPointerListIBM(size, type, stride, pointer, ptrstride);
}

static void APIENTRY InitEdgeFlagPointerListIBM (GLint stride, const GLboolean* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glEdgeFlagPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glEdgeFlagPointerListIBM = extproc;

	glEdgeFlagPointerListIBM(stride, pointer, ptrstride);
}

static void APIENTRY InitFogCoordPointerListIBM (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFogCoordPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFogCoordPointerListIBM = extproc;

	glFogCoordPointerListIBM(type, stride, pointer, ptrstride);
}

static void APIENTRY InitIndexPointerListIBM (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIndexPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glIndexPointerListIBM = extproc;

	glIndexPointerListIBM(type, stride, pointer, ptrstride);
}

static void APIENTRY InitNormalPointerListIBM (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalPointerListIBM = extproc;

	glNormalPointerListIBM(type, stride, pointer, ptrstride);
}

static void APIENTRY InitTexCoordPointerListIBM (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexCoordPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexCoordPointerListIBM = extproc;

	glTexCoordPointerListIBM(size, type, stride, pointer, ptrstride);
}

static void APIENTRY InitVertexPointerListIBM (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexPointerListIBM");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexPointerListIBM = extproc;

	glVertexPointerListIBM(size, type, stride, pointer, ptrstride);
}

static void APIENTRY InitTbufferMask3DFX (GLuint mask)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTbufferMask3DFX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTbufferMask3DFX = extproc;

	glTbufferMask3DFX(mask);
}

static void APIENTRY InitSampleMaskEXT (GLclampf value, GLboolean invert)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSampleMaskEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSampleMaskEXT = extproc;

	glSampleMaskEXT(value, invert);
}

static void APIENTRY InitSamplePatternEXT (GLenum pattern)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSamplePatternEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSamplePatternEXT = extproc;

	glSamplePatternEXT(pattern);
}

static void APIENTRY InitTextureColorMaskSGIS (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTextureColorMaskSGIS");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTextureColorMaskSGIS = extproc;

	glTextureColorMaskSGIS(red, green, blue, alpha);
}

static void APIENTRY InitIglooInterfaceSGIX (GLenum pname, const GLvoid *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIglooInterfaceSGIX");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glIglooInterfaceSGIX = extproc;

	glIglooInterfaceSGIX(pname, params);
}

static void APIENTRY InitGenFencesNV (GLsizei n, GLuint *fences)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenFencesNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenFencesNV = extproc;

	glGenFencesNV(n, fences);
}

static void APIENTRY InitDeleteFencesNV (GLsizei n, const GLuint *fences)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteFencesNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteFencesNV = extproc;

	glDeleteFencesNV(n, fences);
}

static void APIENTRY InitSetFenceNV (GLuint fence, GLenum condition)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSetFenceNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSetFenceNV = extproc;

	glSetFenceNV(fence, condition);
}

static GLboolean APIENTRY InitTestFenceNV (GLuint fence)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTestFenceNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glTestFenceNV = extproc;

	return glTestFenceNV(fence);
}

static void APIENTRY InitFinishFenceNV (GLuint fence)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFinishFenceNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFinishFenceNV = extproc;

	glFinishFenceNV(fence);
}

static GLboolean APIENTRY InitIsFenceNV (GLuint fence)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsFenceNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsFenceNV = extproc;

	return glIsFenceNV(fence);
}

static void APIENTRY InitGetFenceivNV (GLuint fence, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetFenceivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetFenceivNV = extproc;

	glGetFenceivNV(fence, pname, params);
}

static void APIENTRY InitMapControlPointsNV (GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const GLvoid *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMapControlPointsNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMapControlPointsNV = extproc;

	glMapControlPointsNV(target, index, type, ustride, vstride, uorder, vorder, packed, points);
}

static void APIENTRY InitMapParameterivNV (GLenum target, GLenum pname, const GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMapParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMapParameterivNV = extproc;

	glMapParameterivNV(target, pname, params);
}

static void APIENTRY InitMapParameterfvNV (GLenum target, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMapParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMapParameterfvNV = extproc;

	glMapParameterfvNV(target, pname, params);
}

static void APIENTRY InitGetMapControlPointsNV (GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid *points)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMapControlPointsNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMapControlPointsNV = extproc;

	glGetMapControlPointsNV(target, index, type, ustride, vstride, packed, points);
}

static void APIENTRY InitGetMapParameterivNV (GLenum target, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMapParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMapParameterivNV = extproc;

	glGetMapParameterivNV(target, pname, params);
}

static void APIENTRY InitGetMapParameterfvNV (GLenum target, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMapParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMapParameterfvNV = extproc;

	glGetMapParameterfvNV(target, pname, params);
}

static void APIENTRY InitGetMapAttribParameterivNV (GLenum target, GLuint index, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMapAttribParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMapAttribParameterivNV = extproc;

	glGetMapAttribParameterivNV(target, index, pname, params);
}

static void APIENTRY InitGetMapAttribParameterfvNV (GLenum target, GLuint index, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetMapAttribParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetMapAttribParameterfvNV = extproc;

	glGetMapAttribParameterfvNV(target, index, pname, params);
}

static void APIENTRY InitEvalMapsNV (GLenum target, GLenum mode)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glEvalMapsNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glEvalMapsNV = extproc;

	glEvalMapsNV(target, mode);
}

static void APIENTRY InitCombinerStageParameterfvNV (GLenum stage, GLenum pname, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glCombinerStageParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glCombinerStageParameterfvNV = extproc;

	glCombinerStageParameterfvNV(stage, pname, params);
}

static void APIENTRY InitGetCombinerStageParameterfvNV (GLenum stage, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetCombinerStageParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetCombinerStageParameterfvNV = extproc;

	glGetCombinerStageParameterfvNV(stage, pname, params);
}

static void APIENTRY InitBindProgramNV (GLenum target, GLuint id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBindProgramNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBindProgramNV = extproc;

	glBindProgramNV(target, id);
}

static void APIENTRY InitDeleteProgramsNV (GLsizei n, const GLuint *ids)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteProgramsNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteProgramsNV = extproc;

	glDeleteProgramsNV(n, ids);
}

static void APIENTRY InitExecuteProgramNV (GLenum target, GLuint id, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glExecuteProgramNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glExecuteProgramNV = extproc;

	glExecuteProgramNV(target, id, params);
}

static void APIENTRY InitGenProgramsNV (GLsizei n, GLuint *ids)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenProgramsNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenProgramsNV = extproc;

	glGenProgramsNV(n, ids);
}

static GLboolean APIENTRY InitAreProgramsResidentNV (GLsizei n, const GLuint *ids, GLboolean *residences)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAreProgramsResidentNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glAreProgramsResidentNV = extproc;

	return glAreProgramsResidentNV(n, ids, residences);
}

static void APIENTRY InitRequestResidentProgramsNV (GLsizei n, const GLuint *ids)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glRequestResidentProgramsNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glRequestResidentProgramsNV = extproc;

	glRequestResidentProgramsNV(n, ids);
}

static void APIENTRY InitGetProgramParameterfvNV (GLenum target, GLuint index, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramParameterfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramParameterfvNV = extproc;

	glGetProgramParameterfvNV(target, index, pname, params);
}

static void APIENTRY InitGetProgramParameterdvNV (GLenum target, GLuint index, GLenum pname, GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramParameterdvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramParameterdvNV = extproc;

	glGetProgramParameterdvNV(target, index, pname, params);
}

static void APIENTRY InitGetProgramivNV (GLuint id, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramivNV = extproc;

	glGetProgramivNV(id, pname, params);
}

static void APIENTRY InitGetProgramStringNV (GLuint id, GLenum pname, GLubyte *program)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetProgramStringNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetProgramStringNV = extproc;

	glGetProgramStringNV(id, pname, program);
}

static void APIENTRY InitGetTrackMatrixivNV (GLenum target, GLuint address, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetTrackMatrixivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetTrackMatrixivNV = extproc;

	glGetTrackMatrixivNV(target, address, pname, params);
}

static void APIENTRY InitGetVertexAttribdvNV (GLuint index, GLenum pname, GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribdvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribdvNV = extproc;

	glGetVertexAttribdvNV(index, pname, params);
}

static void APIENTRY InitGetVertexAttribfvNV (GLuint index, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribfvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribfvNV = extproc;

	glGetVertexAttribfvNV(index, pname, params);
}

static void APIENTRY InitGetVertexAttribivNV (GLuint index, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribivNV = extproc;

	glGetVertexAttribivNV(index, pname, params);
}

static void APIENTRY InitGetVertexAttribPointervNV (GLuint index, GLenum pname, GLvoid* *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribPointervNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribPointervNV = extproc;

	glGetVertexAttribPointervNV(index, pname, pointer);
}

static GLboolean APIENTRY InitIsProgramNV (GLuint id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsProgramNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsProgramNV = extproc;

	return glIsProgramNV(id);
}

static void APIENTRY InitLoadProgramNV (GLenum target, GLuint id, GLsizei len, const GLubyte *program)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glLoadProgramNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glLoadProgramNV = extproc;

	glLoadProgramNV(target, id, len, program);
}

static void APIENTRY InitProgramParameter4fNV (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramParameter4fNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramParameter4fNV = extproc;

	glProgramParameter4fNV(target, index, x, y, z, w);
}

static void APIENTRY InitProgramParameter4dNV (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramParameter4dNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramParameter4dNV = extproc;

	glProgramParameter4dNV(target, index, x, y, z, w);
}

static void APIENTRY InitProgramParameter4dvNV (GLenum target, GLuint index, const GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramParameter4dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramParameter4dvNV = extproc;

	glProgramParameter4dvNV(target, index, params);
}

static void APIENTRY InitProgramParameter4fvNV (GLenum target, GLuint index, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramParameter4fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramParameter4fvNV = extproc;

	glProgramParameter4fvNV(target, index, params);
}

static void APIENTRY InitProgramParameters4dvNV (GLenum target, GLuint index, GLuint num, const GLdouble *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramParameters4dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramParameters4dvNV = extproc;

	glProgramParameters4dvNV(target, index, num, params);
}

static void APIENTRY InitProgramParameters4fvNV (GLenum target, GLuint index, GLuint num, const GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glProgramParameters4fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glProgramParameters4fvNV = extproc;

	glProgramParameters4fvNV(target, index, num, params);
}

static void APIENTRY InitTrackMatrixNV (GLenum target, GLuint address, GLenum matrix, GLenum transform)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTrackMatrixNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTrackMatrixNV = extproc;

	glTrackMatrixNV(target, address, matrix, transform);
}

static void APIENTRY InitVertexAttribPointerNV (GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribPointerNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribPointerNV = extproc;

	glVertexAttribPointerNV(index, size, type, stride, pointer);
}

static void APIENTRY InitVertexAttrib1sNV (GLuint index, GLshort x)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1sNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1sNV = extproc;

	glVertexAttrib1sNV(index, x);
}

static void APIENTRY InitVertexAttrib1fNV (GLuint index, GLfloat x)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1fNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1fNV = extproc;

	glVertexAttrib1fNV(index, x);
}

static void APIENTRY InitVertexAttrib1dNV (GLuint index, GLdouble x)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1dNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1dNV = extproc;

	glVertexAttrib1dNV(index, x);
}

static void APIENTRY InitVertexAttrib2sNV (GLuint index, GLshort x, GLshort y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2sNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2sNV = extproc;

	glVertexAttrib2sNV(index, x, y);
}

static void APIENTRY InitVertexAttrib2fNV (GLuint index, GLfloat x, GLfloat y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2fNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2fNV = extproc;

	glVertexAttrib2fNV(index, x, y);
}

static void APIENTRY InitVertexAttrib2dNV (GLuint index, GLdouble x, GLdouble y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2dNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2dNV = extproc;

	glVertexAttrib2dNV(index, x, y);
}

static void APIENTRY InitVertexAttrib3sNV (GLuint index, GLshort x, GLshort y, GLshort z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3sNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3sNV = extproc;

	glVertexAttrib3sNV(index, x, y, z);
}

static void APIENTRY InitVertexAttrib3fNV (GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3fNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3fNV = extproc;

	glVertexAttrib3fNV(index, x, y, z);
}

static void APIENTRY InitVertexAttrib3dNV (GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3dNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3dNV = extproc;

	glVertexAttrib3dNV(index, x, y, z);
}

static void APIENTRY InitVertexAttrib4sNV (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4sNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4sNV = extproc;

	glVertexAttrib4sNV(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4fNV (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4fNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4fNV = extproc;

	glVertexAttrib4fNV(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4dNV (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4dNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4dNV = extproc;

	glVertexAttrib4dNV(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib4ubNV (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4ubNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4ubNV = extproc;

	glVertexAttrib4ubNV(index, x, y, z, w);
}

static void APIENTRY InitVertexAttrib1svNV (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1svNV = extproc;

	glVertexAttrib1svNV(index, v);
}

static void APIENTRY InitVertexAttrib1fvNV (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1fvNV = extproc;

	glVertexAttrib1fvNV(index, v);
}

static void APIENTRY InitVertexAttrib1dvNV (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib1dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib1dvNV = extproc;

	glVertexAttrib1dvNV(index, v);
}

static void APIENTRY InitVertexAttrib2svNV (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2svNV = extproc;

	glVertexAttrib2svNV(index, v);
}

static void APIENTRY InitVertexAttrib2fvNV (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2fvNV = extproc;

	glVertexAttrib2fvNV(index, v);
}

static void APIENTRY InitVertexAttrib2dvNV (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib2dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib2dvNV = extproc;

	glVertexAttrib2dvNV(index, v);
}

static void APIENTRY InitVertexAttrib3svNV (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3svNV = extproc;

	glVertexAttrib3svNV(index, v);
}

static void APIENTRY InitVertexAttrib3fvNV (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3fvNV = extproc;

	glVertexAttrib3fvNV(index, v);
}

static void APIENTRY InitVertexAttrib3dvNV (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib3dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib3dvNV = extproc;

	glVertexAttrib3dvNV(index, v);
}

static void APIENTRY InitVertexAttrib4svNV (GLuint index, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4svNV = extproc;

	glVertexAttrib4svNV(index, v);
}

static void APIENTRY InitVertexAttrib4fvNV (GLuint index, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4fvNV = extproc;

	glVertexAttrib4fvNV(index, v);
}

static void APIENTRY InitVertexAttrib4dvNV (GLuint index, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4dvNV = extproc;

	glVertexAttrib4dvNV(index, v);
}

static void APIENTRY InitVertexAttrib4ubvNV (GLuint index, const GLubyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttrib4ubvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttrib4ubvNV = extproc;

	glVertexAttrib4ubvNV(index, v);
}

static void APIENTRY InitVertexAttribs1svNV (GLuint index, GLsizei n, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs1svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs1svNV = extproc;

	glVertexAttribs1svNV(index, n, v);
}

static void APIENTRY InitVertexAttribs1fvNV (GLuint index, GLsizei n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs1fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs1fvNV = extproc;

	glVertexAttribs1fvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs1dvNV (GLuint index, GLsizei n, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs1dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs1dvNV = extproc;

	glVertexAttribs1dvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs2svNV (GLuint index, GLsizei n, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs2svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs2svNV = extproc;

	glVertexAttribs2svNV(index, n, v);
}

static void APIENTRY InitVertexAttribs2fvNV (GLuint index, GLsizei n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs2fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs2fvNV = extproc;

	glVertexAttribs2fvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs2dvNV (GLuint index, GLsizei n, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs2dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs2dvNV = extproc;

	glVertexAttribs2dvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs3svNV (GLuint index, GLsizei n, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs3svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs3svNV = extproc;

	glVertexAttribs3svNV(index, n, v);
}

static void APIENTRY InitVertexAttribs3fvNV (GLuint index, GLsizei n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs3fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs3fvNV = extproc;

	glVertexAttribs3fvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs3dvNV (GLuint index, GLsizei n, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs3dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs3dvNV = extproc;

	glVertexAttribs3dvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs4svNV (GLuint index, GLsizei n, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs4svNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs4svNV = extproc;

	glVertexAttribs4svNV(index, n, v);
}

static void APIENTRY InitVertexAttribs4fvNV (GLuint index, GLsizei n, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs4fvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs4fvNV = extproc;

	glVertexAttribs4fvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs4dvNV (GLuint index, GLsizei n, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs4dvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs4dvNV = extproc;

	glVertexAttribs4dvNV(index, n, v);
}

static void APIENTRY InitVertexAttribs4ubvNV (GLuint index, GLsizei n, const GLubyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribs4ubvNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribs4ubvNV = extproc;

	glVertexAttribs4ubvNV(index, n, v);
}

static void APIENTRY InitAddSwapHintRectWIN (GLint x, GLint y, GLsizei width, GLsizei height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAddSwapHintRectWIN");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glAddSwapHintRectWIN = extproc;

	glAddSwapHintRectWIN(x, y, width, height);
}

////
/*************************************************************/

static void APIENTRY InitTexBumpParameterivATI (GLenum pname, GLint *param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexBumpParameterivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexBumpParameterivATI = extproc;

	glTexBumpParameterivATI(pname, param);
}

static void APIENTRY InitTexBumpParameterfvATI (GLenum pname, GLint *param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glTexBumpParameterfvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glTexBumpParameterfvATI = extproc;

	glTexBumpParameterfvATI(pname, param);
}

static void APIENTRY InitGetTexBumpParameterivATI (GLenum pname, GLint *param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetTexBumpParameterivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetTexBumpParameterivATI = extproc;

	glGetTexBumpParameterivATI(pname, param);
}

static void APIENTRY InitGetTexBumpParameterfvATI (GLenum pname, GLint *param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetTexBumpParameterfvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetTexBumpParameterfvATI = extproc;

	glGetTexBumpParameterfvATI(pname, param);
}

static void APIENTRY InitGenFragmentShadersATI (GLuint range)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenFragmentShadersATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenFragmentShadersATI = extproc;

	glGenFragmentShadersATI(range);
}

static void APIENTRY InitBindFragmentShaderATI (GLuint id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBindFragmentShaderATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBindFragmentShaderATI = extproc;

	glBindFragmentShaderATI(id);
}

static void APIENTRY InitDeleteFragmentShadersATI (GLuint id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteFragmentShadersATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteFragmentShadersATI = extproc;

	glDeleteFragmentShadersATI(id);
}

static void APIENTRY InitBeginFragmentShadersATI (GLvoid)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBeginFragmentShadersATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBeginFragmentShadersATI = extproc;

	glBeginFragmentShadersATI();
}

static void APIENTRY InitEndFragmentShadersATI (GLuint dst, GLuint coord, GLenum swizzle)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glEndFragmentShadersATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glEndFragmentShadersATI = extproc;

	glEndFragmentShadersATI(dst, coord, swizzle);
}

static void APIENTRY InitPassTexCoordATI (GLuint dst, GLuint interp, GLenum swizzle)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPassTexCoordATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPassTexCoordATI = extproc;

	glPassTexCoordATI(dst, interp, swizzle);
}

static void APIENTRY InitSampleMapATI (GLuint dst, GLuint interp, GLenum swizzle)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSampleMapATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSampleMapATI = extproc;

	glSampleMapATI(dst, interp, swizzle);
}

static void APIENTRY InitColorFragmentOp1ATI (GLenum op, GLuint dst, GLuint dstMask,
                                              GLuint dstMod, GLuint arg1, GLuint arg1Rep,
                                              GLuint arg1Mod)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorFragmentOp1ATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorFragmentOp1ATI = extproc;

	glColorFragmentOp1ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod);
}

static void APIENTRY InitColorFragmentOp2ATI (GLenum op, GLuint dst, GLuint dstMask,
                                               GLuint dstMod, GLuint arg1, GLuint arg1Rep,
                                               GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
                                               GLuint arg2Mod)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorFragmentOp2ATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorFragmentOp2ATI = extproc;

	glColorFragmentOp2ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);
}

static void APIENTRY InitColorFragmentOp3ATI (GLenum op, GLuint dst, GLuint dstMask,
                                              GLuint dstMod, GLuint arg1, GLuint arg1Rep,
                                              GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
                                              GLuint arg2Mod, GLuint arg3, GLuint arg3Rep,
                                              GLuint arg3Mod)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glColorFragmentOp3ATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glColorFragmentOp3ATI = extproc;

	glColorFragmentOp3ATI(op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
}

static void APIENTRY InitAlphaFragmentOp1ATI (GLenum op, GLuint dst, GLuint dstMod,
                                              GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAlphaFragmentOp1ATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glAlphaFragmentOp1ATI = extproc;

	glAlphaFragmentOp1ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod);
}

static void APIENTRY InitAlphaFragmentOp2ATI (GLenum op, GLuint dst, GLuint dstMod,
                                              GLuint arg1, GLuint arg1Rep, GLuint arg1Mod,
                                              GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAlphaFragmentOp2ATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glAlphaFragmentOp2ATI = extproc;

	glAlphaFragmentOp2ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod);
}

static void APIENTRY InitAlphaFragmentOp3ATI (GLenum op, GLuint dst, GLuint dstMod,
                                              GLuint arg1, GLuint arg1Rep, GLuint arg1Mod,
                                              GLuint arg2, GLuint arg2Rep, GLuint arg2Mod,
                                              GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glAlphaFragmentOp3ATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glAlphaFragmentOp3ATI = extproc;

	glAlphaFragmentOp3ATI(op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod);
}

static void APIENTRY InitSetFragmentShaderConstantATI (GLuint dst, const GLfloat *value)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glSetFragmentShaderConstantATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glSetFragmentShaderConstantATI = extproc;

	glSetFragmentShaderConstantATI(dst, value);
}

static void APIENTRY InitPnTrianglesiATI (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPnTrianglesiATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPnTrianglesiATI = extproc;

	glPnTrianglesiATI(pname, param);
}

static void APIENTRY InitPnTrianglesfATI (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPnTrianglesfATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPnTrianglesfATI = extproc;

	glPnTrianglesfATI(pname, param);
}

static void APIENTRY InitStencilOpSeparatorATI (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glStencilOpSeparatorATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glStencilOpSeparatorATI = extproc;

	glStencilOpSeparatorATI(face, sfail, dpfail, dppass);
}

static void APIENTRY InitStencilFuncSeparatorATI (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glStencilFuncSeparatorATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glStencilFuncSeparatorATI = extproc;

	glStencilFuncSeparatorATI(frontfunc, backfunc, ref, mask);
}

static void APIENTRY InitClientActiveVertexStreamATI (GLenum stream)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glClientActiveVertexStreamATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glClientActiveVertexStreamATI = extproc;

	glClientActiveVertexStreamATI(stream);
}

static void APIENTRY InitVertexBlendEnviATI (GLenum pname, GLint param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexBlendEnviATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexBlendEnviATI = extproc;

	glVertexBlendEnviATI(pname, param);
}

static void APIENTRY InitVertexBlendEnvfATI (GLenum pname, GLfloat param)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexBlendEnvfATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexBlendEnvfATI = extproc;

	glVertexBlendEnvfATI(pname, param);
}

static void APIENTRY InitVertexStream2sATI (GLenum stream, GLshort x, GLshort y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2sATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2sATI = extproc;

	glVertexStream2sATI(stream, x, y);
}

static void APIENTRY InitVertexStream2svATI (GLenum stream, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2svATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2svATI = extproc;

	glVertexStream2svATI(stream, v);
}

static void APIENTRY InitVertexStream2iATI (GLenum stream, GLint x, GLint y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2iATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2iATI = extproc;

	glVertexStream2iATI(stream, x, y);
}

static void APIENTRY InitVertexStream2ivATI (GLenum stream, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2ivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2ivATI = extproc;

	glVertexStream2ivATI(stream, v);
}

static void APIENTRY InitVertexStream2fATI (GLenum stream, GLfloat x, GLfloat y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2fATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2fATI = extproc;

	glVertexStream2fATI(stream, x, y);
}

static void APIENTRY InitVertexStream2fvATI (GLenum stream, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2fvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2fvATI = extproc;

	glVertexStream2fvATI(stream, v);
}

static void APIENTRY InitVertexStream2dATI (GLenum stream, GLdouble x, GLdouble y)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2dATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2dATI = extproc;

	glVertexStream2dATI(stream, x, y);
}

static void APIENTRY InitVertexStream2dvATI (GLenum stream, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream2dvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream2dvATI = extproc;

	glVertexStream2dvATI(stream, v);
}

static void APIENTRY InitVertexStream3sATI (GLenum stream, GLshort x, GLshort y, GLshort z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3sATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3sATI = extproc;

	glVertexStream3sATI(stream, x, y, z);
}

static void APIENTRY InitVertexStream3svATI (GLenum stream, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3svATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3svATI = extproc;

	glVertexStream3svATI(stream, v);
}

static void APIENTRY InitVertexStream3iATI (GLenum stream, GLint x, GLint y, GLint z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3iATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3iATI = extproc;

	glVertexStream3iATI(stream, x, y, z);
}

static void APIENTRY InitVertexStream3ivATI (GLenum stream, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3ivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3ivATI = extproc;

	glVertexStream3ivATI(stream, v);
}

static void APIENTRY InitVertexStream3fATI (GLenum stream, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3fATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3fATI = extproc;

	glVertexStream3fATI(stream, x, y, z);
}

static void APIENTRY InitVertexStream3fvATI (GLenum stream, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3fvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3fvATI = extproc;

	glVertexStream3fvATI(stream, v);
}

static void APIENTRY InitVertexStream3dATI (GLenum stream, GLdouble x, GLdouble y, GLdouble z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3dATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3dATI = extproc;

	glVertexStream3dATI(stream, x, y, z);
}

static void APIENTRY InitVertexStream3dvATI (GLenum stream, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream3dvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream3dvATI = extproc;

	glVertexStream3dvATI(stream, v);
}

static void APIENTRY InitVertexStream4sATI (GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4sATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4sATI = extproc;

	glVertexStream4sATI(stream, x, y, z, w);
}

static void APIENTRY InitVertexStream4svATI (GLenum stream, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4svATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4svATI = extproc;

	glVertexStream4svATI(stream, v);
}

static void APIENTRY InitVertexStream4iATI (GLenum stream, GLint x, GLint y, GLint z, GLint w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4iATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4iATI = extproc;

	glVertexStream4iATI(stream, x, y, z, w);
}

static void APIENTRY InitVertexStream4ivATI (GLenum stream, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4ivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4ivATI = extproc;

	glVertexStream4ivATI(stream, v);
}

static void APIENTRY InitVertexStream4fATI (GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4fATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4fATI = extproc;

	glVertexStream4fATI(stream, x, y, z, w);
}

static void APIENTRY InitVertexStream4fvATI (GLenum stream, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4fvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4fvATI = extproc;

	glVertexStream4fvATI(stream, v);
}

static void APIENTRY InitVertexStream4dATI (GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4dATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4dATI = extproc;

	glVertexStream4dATI(stream, x, y, z, w);
}

static void APIENTRY InitVertexStream4dvATI (GLenum stream, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexStream4dvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexStream4dvATI = extproc;

	glVertexStream4dvATI(stream, v);
}

static void APIENTRY InitNormalStream3bATI (GLenum stream, GLbyte x, GLbyte y, GLbyte z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3bATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3bATI = extproc;

	glNormalStream3bATI(stream, x, y, z);
}

static void APIENTRY InitNormalStream3bvATI (GLenum stream, const GLbyte *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3bvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3bvATI = extproc;

	glNormalStream3bvATI(stream, v);
}

static void APIENTRY InitNormalStream3sATI (GLenum stream, GLshort x, GLshort y, GLshort z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3sATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3sATI = extproc;

	glNormalStream3sATI(stream, x, y, z);
}

static void APIENTRY InitNormalStream3svATI (GLenum stream, const GLshort *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3svATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3svATI = extproc;

	glNormalStream3svATI(stream, v);
}

static void APIENTRY InitNormalStream3iATI (GLenum stream, GLint x, GLint y, GLint z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3iATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3iATI = extproc;

	glNormalStream3iATI(stream, x, y, z);
}

static void APIENTRY InitNormalStream3ivATI (GLenum stream, const GLint *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3ivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3ivATI = extproc;

	glNormalStream3ivATI(stream, v);
}

static void APIENTRY InitNormalStream3fATI (GLenum stream, GLfloat x, GLfloat y, GLfloat z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3fATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3fATI = extproc;

	glNormalStream3fATI(stream, x, y, z);
}

static void APIENTRY InitNormalStream3fvATI (GLenum stream, const GLfloat *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3fvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3fvATI = extproc;

	glNormalStream3fvATI(stream, v);
}

static void APIENTRY InitNormalStream3dATI (GLenum stream, GLdouble x, GLdouble y, GLdouble z)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3dATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3dATI = extproc;

	glNormalStream3dATI(stream, x, y, z);
}

static void APIENTRY InitNormalStream3dvATI (GLenum stream, const GLdouble *v)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNormalStream3dvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glNormalStream3dvATI = extproc;

	glNormalStream3dvATI(stream, v);
}

static GLuint APIENTRY InitNewObjectBufferATI (GLsizei size, const GLvoid *pointer, GLenum usage)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glNewObjectBufferATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glNewObjectBufferATI = extproc;

	return (glNewObjectBufferATI(size, pointer, usage));
}

static GLboolean APIENTRY InitIsObjectBufferATI (GLuint buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsObjectBufferATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsObjectBufferATI = extproc;

	return (glIsObjectBufferATI(buffer));
}

static void APIENTRY InitUpdateObjectBufferATI (GLuint buffer, GLuint offset, GLsizei size,
                                             const GLvoid *pointer, GLenum preserve)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glUpdateObjectBufferATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glUpdateObjectBufferATI = extproc;

	glUpdateObjectBufferATI(buffer, offset, size, pointer, preserve);
}

static void APIENTRY InitGetObjectBufferfvATI (GLuint buffer, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetObjectBufferfvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetObjectBufferfvATI = extproc;

	glGetObjectBufferfvATI(buffer, pname, params);
}

static void APIENTRY InitGetObjectBufferivATI (GLuint buffer, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetObjectBufferivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetObjectBufferivATI = extproc;

	glGetObjectBufferivATI(buffer, pname, params);
}

static void APIENTRY InitFreeObjectBufferATI (GLuint buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glFreeObjectBufferATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glFreeObjectBufferATI = extproc;

	glFreeObjectBufferATI(buffer);
}

static void APIENTRY InitArrayObjectATI (GLenum array, GLint size, GLenum type,
                                      GLsizei stride, GLuint buffer, GLuint offset)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glArrayObjectATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glArrayObjectATI = extproc;

	glArrayObjectATI(array, size, type, stride, buffer, offset);
}

static void APIENTRY InitGetArrayObjectfvATI (GLenum array, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetArrayObjectfvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetArrayObjectfvATI = extproc;

	glGetArrayObjectfvATI(array, pname, params);
}

static void APIENTRY InitGetArrayObjectivATI (GLenum array, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetArrayObjectivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetArrayObjectivATI = extproc;

	glGetArrayObjectivATI(array, pname, params);
}

static void APIENTRY InitVariantArrayObjectATI (GLuint id, GLenum type,
                                             GLsizei stride, GLuint buffer, GLuint offset)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVariantArrayObjectATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVariantArrayObjectATI = extproc;

	glVariantArrayObjectATI(id, type, stride, buffer, offset);
}

static void APIENTRY InitGetVariantArrayObjectfvATI (GLuint id, GLenum pname, GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVariantArrayObjectfvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVariantArrayObjectfvATI = extproc;

	glGetVariantArrayObjectfvATI(id, pname, params);
}

static void APIENTRY InitGetVariantArrayObjectivATI (GLuint id, GLenum pname, GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVariantArrayObjectivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVariantArrayObjectivATI = extproc;

	glGetVariantArrayObjectivATI(id, pname, params);
}

static void APIENTRY InitElementPointerATI (GLenum type, const GLvoid *pointer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glElementPointerATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glElementPointerATI = extproc;

	glElementPointerATI(type, pointer);
}

static void APIENTRY InitDrawElementArrayATI (GLenum mode, GLsizei count)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDrawElementArrayATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDrawElementArrayATI = extproc;

	glDrawElementArrayATI(mode, count);
}

static void APIENTRY InitDrawRangeElementArrayATI (GLenum mode, GLuint start, GLuint end, GLsizei count)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDrawRangeElementArrayATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDrawRangeElementArrayATI = extproc;

	glDrawRangeElementArrayATI(mode, start, end, count);
}

static void APIENTRY InitMapObjectBufferATI (GLuint buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glMapObjectBufferATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glMapObjectBufferATI = extproc;

	glMapObjectBufferATI(buffer);
}

static void APIENTRY InitUnmapObjectBufferATI (GLuint buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glUnmapObjectBufferATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glUnmapObjectBufferATI = extproc;

	glUnmapObjectBufferATI(buffer);
}

static void APIENTRY InitVertexAttribArrayObjectATI (GLuint index, GLint size, GLenum type,
                                                  GLboolean normalized, GLsizei stride,
                                                  GLuint buffer, GLuint offset)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glVertexAttribArrayObjectATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glVertexAttribArrayObjectATI = extproc;

	glVertexAttribArrayObjectATI(index, size, type, normalized, stride, buffer, offset);
}

static void APIENTRY InitGetVertexAttribArrayObjectfvATI (GLuint index, GLenum pname,
                                                       GLfloat *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribArrayObjectfvATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribArrayObjectfvATI = extproc;

	glGetVertexAttribArrayObjectfvATI(index, pname, params);
}

static void APIENTRY InitGetVertexAttribArrayObjectivATI (GLuint index, GLenum pname,
                                                       GLint *params)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetVertexAttribArrayObjectivATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetVertexAttribArrayObjectivATI = extproc;

	glGetVertexAttribArrayObjectivATI(index, pname, params);
}

static void APIENTRY InitDrawBuffersATI (GLsizei n, const GLenum *bufs)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDrawBuffersATI");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDrawBuffersATI = extproc;

	glDrawBuffersATI(n, bufs);
}

static void APIENTRY InitGenOcclusionQueriesNV (GLsizei num, GLuint *buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGenOcclusionQueriesNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGenOcclusionQueriesNV = extproc;

	glGenOcclusionQueriesNV(num, buffer);
}

static void APIENTRY InitDeleteOcclusionQueriesNV (GLsizei num, const GLuint *buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glDeleteOcclusionQueriesNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glDeleteOcclusionQueriesNV = extproc;

	glDeleteOcclusionQueriesNV(num, buffer);
}

static GLboolean APIENTRY InitIsOcclusionQueryNV (GLuint id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glIsOcclusionQueryNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	glIsOcclusionQueryNV = extproc;

	return (glIsOcclusionQueryNV(id));
}

static void APIENTRY InitBeginOcclusionQueryNV (GLuint id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glBeginOcclusionQueryNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glBeginOcclusionQueryNV = extproc;

	glBeginOcclusionQueryNV(id);
}

static void APIENTRY InitEndOcclusionQueryNV (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glEndOcclusionQueryNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glEndOcclusionQueryNV = extproc;

	glEndOcclusionQueryNV();
}

static void APIENTRY InitGetOcclusionQueryivNV (GLuint id, GLenum sym, GLint *buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetOcclusionQueryivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetOcclusionQueryivNV = extproc;

	glGetOcclusionQueryivNV(id, sym, buffer);
}

static void APIENTRY InitGetOcclusionQueryuivNV (GLuint id, GLenum sym, GLuint *buffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glGetOcclusionQueryuivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glGetOcclusionQueryuivNV = extproc;

	glGetOcclusionQueryuivNV(id, sym, buffer);
}

static void APIENTRY InitPointParameteriNV (GLenum sym, GLint val)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameteriNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameteriNV = extproc;

	glPointParameteriNV(sym, val);
}

static void APIENTRY InitPointParameterivNV (GLenum sym, const GLint *vals)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("glPointParameterivNV");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	glPointParameterivNV = extproc;

	glPointParameterivNV(sym, vals);
}

////

#ifdef _WIN32

static HANDLE WINAPI InitCreateBufferRegionARB (HDC hDC, int iLayerPlane, UINT uType)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglCreateBufferRegionARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglCreateBufferRegionARB = extproc;

	return wglCreateBufferRegionARB(hDC, iLayerPlane, uType);
}

static VOID WINAPI InitDeleteBufferRegionARB (HANDLE hRegion)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglDeleteBufferRegionARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	wglDeleteBufferRegionARB = extproc;

	wglDeleteBufferRegionARB(hRegion);
}

static BOOL WINAPI InitSaveBufferRegionARB (HANDLE hRegion, int x, int y, int width, int height)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglSaveBufferRegionARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglSaveBufferRegionARB = extproc;

	return wglSaveBufferRegionARB(hRegion, x, y, width, height);
}

static BOOL WINAPI InitRestoreBufferRegionARB (HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglRestoreBufferRegionARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglRestoreBufferRegionARB = extproc;

	return wglRestoreBufferRegionARB(hRegion, x, y, width, height, xSrc, ySrc);
}

static const WINAPI InitGetExtensionsStringARB (HDC hdc)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetExtensionsStringARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetExtensionsStringARB = extproc;

	return wglGetExtensionsStringARB(hdc);
}

static BOOL WINAPI InitGetPixelFormatAttribivARB (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetPixelFormatAttribivARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetPixelFormatAttribivARB = extproc;

	return wglGetPixelFormatAttribivARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);
}

static BOOL WINAPI InitGetPixelFormatAttribfvARB (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetPixelFormatAttribfvARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetPixelFormatAttribfvARB = extproc;

	return wglGetPixelFormatAttribfvARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);
}

static BOOL WINAPI InitChoosePixelFormatARB (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglChoosePixelFormatARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglChoosePixelFormatARB = extproc;

	return wglChoosePixelFormatARB(hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
}

static BOOL WINAPI InitMakeContextCurrentARB (HDC hDrawDC, HDC hReadDC, HGLRC hglrc)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglMakeContextCurrentARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglMakeContextCurrentARB = extproc;

	return wglMakeContextCurrentARB(hDrawDC, hReadDC, hglrc);
}

static HDC WINAPI InitGetCurrentReadDCARB (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetCurrentReadDCARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetCurrentReadDCARB = extproc;

	return wglGetCurrentReadDCARB();
}

static HPBUFFERARB WINAPI InitCreatePbufferARB (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglCreatePbufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglCreatePbufferARB = extproc;

	return wglCreatePbufferARB(hDC, iPixelFormat, iWidth, iHeight, piAttribList);
}

static HDC WINAPI InitGetPbufferDCARB (HPBUFFERARB hPbuffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetPbufferDCARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetPbufferDCARB = extproc;

	return wglGetPbufferDCARB(hPbuffer);
}

static int WINAPI InitReleasePbufferDCARB (HPBUFFERARB hPbuffer, HDC hDC)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglReleasePbufferDCARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglReleasePbufferDCARB = extproc;

	return wglReleasePbufferDCARB(hPbuffer, hDC);
}

static BOOL WINAPI InitDestroyPbufferARB (HPBUFFERARB hPbuffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglDestroyPbufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglDestroyPbufferARB = extproc;

	return wglDestroyPbufferARB(hPbuffer);
}

static BOOL WINAPI InitQueryPbufferARB (HPBUFFERARB hPbuffer, int iAttribute, int *piValue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglQueryPbufferARB");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglQueryPbufferARB = extproc;

	return wglQueryPbufferARB(hPbuffer, iAttribute, piValue);
}

static GLboolean WINAPI InitCreateDisplayColorTableEXT (GLushort id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglCreateDisplayColorTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglCreateDisplayColorTableEXT = extproc;

	return wglCreateDisplayColorTableEXT(id);
}

static GLboolean WINAPI InitLoadDisplayColorTableEXT (const GLushort *table, GLuint length)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglLoadDisplayColorTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglLoadDisplayColorTableEXT = extproc;

	return wglLoadDisplayColorTableEXT(table, length);
}

static GLboolean WINAPI InitBindDisplayColorTableEXT (GLushort id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglBindDisplayColorTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglBindDisplayColorTableEXT = extproc;

	return wglBindDisplayColorTableEXT(id);
}

static VOID WINAPI InitDestroyDisplayColorTableEXT (GLushort id)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglDestroyDisplayColorTableEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return;
	}

	wglDestroyDisplayColorTableEXT = extproc;

	wglDestroyDisplayColorTableEXT(id);
}

static const WINAPI InitGetExtensionsStringEXT (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetExtensionsStringEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetExtensionsStringEXT = extproc;

	return wglGetExtensionsStringEXT();
}

static BOOL WINAPI InitMakeContextCurrentEXT (HDC hDrawDC, HDC hReadDC, HGLRC hglrc)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglMakeContextCurrentEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglMakeContextCurrentEXT = extproc;

	return wglMakeContextCurrentEXT(hDrawDC, hReadDC, hglrc);
}

static HDC WINAPI InitGetCurrentReadDCEXT (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetCurrentReadDCEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetCurrentReadDCEXT = extproc;

	return wglGetCurrentReadDCEXT();
}

static HPBUFFEREXT WINAPI InitCreatePbufferEXT (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglCreatePbufferEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglCreatePbufferEXT = extproc;

	return wglCreatePbufferEXT(hDC, iPixelFormat, iWidth, iHeight, piAttribList);
}

static HDC WINAPI InitGetPbufferDCEXT (HPBUFFEREXT hPbuffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetPbufferDCEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetPbufferDCEXT = extproc;

	return wglGetPbufferDCEXT(hPbuffer);
}

static int WINAPI InitReleasePbufferDCEXT (HPBUFFEREXT hPbuffer, HDC hDC)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglReleasePbufferDCEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglReleasePbufferDCEXT = extproc;

	return wglReleasePbufferDCEXT(hPbuffer, hDC);
}

static BOOL WINAPI InitDestroyPbufferEXT (HPBUFFEREXT hPbuffer)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglDestroyPbufferEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglDestroyPbufferEXT = extproc;

	return wglDestroyPbufferEXT(hPbuffer);
}

static BOOL WINAPI InitQueryPbufferEXT (HPBUFFEREXT hPbuffer, int iAttribute, int *piValue)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglQueryPbufferEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglQueryPbufferEXT = extproc;

	return wglQueryPbufferEXT(hPbuffer, iAttribute, piValue);
}

static BOOL WINAPI InitGetPixelFormatAttribivEXT (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetPixelFormatAttribivEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetPixelFormatAttribivEXT = extproc;

	return wglGetPixelFormatAttribivEXT(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);
}

static BOOL WINAPI InitGetPixelFormatAttribfvEXT (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetPixelFormatAttribfvEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetPixelFormatAttribfvEXT = extproc;

	return wglGetPixelFormatAttribfvEXT(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);
}

static BOOL WINAPI InitChoosePixelFormatEXT (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglChoosePixelFormatEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglChoosePixelFormatEXT = extproc;

	return wglChoosePixelFormatEXT(hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
}

static BOOL WINAPI InitSwapIntervalEXT (int interval)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglSwapIntervalEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglSwapIntervalEXT = extproc;

	return wglSwapIntervalEXT(interval);
}

static int WINAPI InitGetSwapIntervalEXT (void)
{
	void *extproc;

	extproc = (void *) glGetProcAddress("wglGetSwapIntervalEXT");

	if (extproc == NULL) {
		_ASSERT(0);
		return 0;
	}

	wglGetSwapIntervalEXT = extproc;

	return wglGetSwapIntervalEXT();
}

#endif /* _WIN32 */

_GLextensionProcs _extensionProcs = {
	InitBlendColor,
	InitBlendEquation,
	InitDrawRangeElements,
	InitColorTable,
	InitColorTableParameterfv,
	InitColorTableParameteriv,
	InitCopyColorTable,
	InitGetColorTable,
	InitGetColorTableParameterfv,
	InitGetColorTableParameteriv,
	InitColorSubTable,
	InitCopyColorSubTable,
	InitConvolutionFilter1D,
	InitConvolutionFilter2D,
	InitConvolutionParameterf,
	InitConvolutionParameterfv,
	InitConvolutionParameteri,
	InitConvolutionParameteriv,
	InitCopyConvolutionFilter1D,
	InitCopyConvolutionFilter2D,
	InitGetConvolutionFilter,
	InitGetConvolutionParameterfv,
	InitGetConvolutionParameteriv,
	InitGetSeparableFilter,
	InitSeparableFilter2D,
	InitGetHistogram,
	InitGetHistogramParameterfv,
	InitGetHistogramParameteriv,
	InitGetMinmax,
	InitGetMinmaxParameterfv,
	InitGetMinmaxParameteriv,
	InitHistogram,
	InitMinmax,
	InitResetHistogram,
	InitResetMinmax,
	InitTexImage3D,
	InitTexSubImage3D,
	InitCopyTexSubImage3D,
	InitActiveTextureARB,
	InitClientActiveTextureARB,
	InitMultiTexCoord1dARB,
	InitMultiTexCoord1dvARB,
	InitMultiTexCoord1fARB,
	InitMultiTexCoord1fvARB,
	InitMultiTexCoord1iARB,
	InitMultiTexCoord1ivARB,
	InitMultiTexCoord1sARB,
	InitMultiTexCoord1svARB,
	InitMultiTexCoord2dARB,
	InitMultiTexCoord2dvARB,
	InitMultiTexCoord2fARB,
	InitMultiTexCoord2fvARB,
	InitMultiTexCoord2iARB,
	InitMultiTexCoord2ivARB,
	InitMultiTexCoord2sARB,
	InitMultiTexCoord2svARB,
	InitMultiTexCoord3dARB,
	InitMultiTexCoord3dvARB,
	InitMultiTexCoord3fARB,
	InitMultiTexCoord3fvARB,
	InitMultiTexCoord3iARB,
	InitMultiTexCoord3ivARB,
	InitMultiTexCoord3sARB,
	InitMultiTexCoord3svARB,
	InitMultiTexCoord4dARB,
	InitMultiTexCoord4dvARB,
	InitMultiTexCoord4fARB,
	InitMultiTexCoord4fvARB,
	InitMultiTexCoord4iARB,
	InitMultiTexCoord4ivARB,
	InitMultiTexCoord4sARB,
	InitMultiTexCoord4svARB,
	InitLoadTransposeMatrixfARB,
	InitLoadTransposeMatrixdARB,
	InitMultTransposeMatrixfARB,
	InitMultTransposeMatrixdARB,
	InitSampleCoverageARB,
	InitCompressedTexImage3DARB,
	InitCompressedTexImage2DARB,
	InitCompressedTexImage1DARB,
	InitCompressedTexSubImage3DARB,
	InitCompressedTexSubImage2DARB,
	InitCompressedTexSubImage1DARB,
	InitGetCompressedTexImageARB,
	InitWeightbvARB,
	InitWeightsvARB,
	InitWeightivARB,
	InitWeightfvARB,
	InitWeightdvARB,
	InitWeightubvARB,
	InitWeightusvARB,
	InitWeightuivARB,
	InitWeightPointerARB,
	InitVertexBlendARB,
	InitCurrentPaletteMatrixARB,
	InitMatrixIndexubvARB,
	InitMatrixIndexusvARB,
	InitMatrixIndexuivARB,
	InitMatrixIndexPointerARB,
    InitWindowPos2dARB,
    InitWindowPos2dvARB,
    InitWindowPos2fARB,
    InitWindowPos2fvARB,
    InitWindowPos2iARB,
    InitWindowPos2ivARB,
    InitWindowPos2sARB,
    InitWindowPos2svARB,
    InitWindowPos3dARB,
    InitWindowPos3dvARB,
    InitWindowPos3fARB,
    InitWindowPos3fvARB,
    InitWindowPos3iARB,
    InitWindowPos3ivARB,
    InitWindowPos3sARB,
    InitWindowPos3svARB,
    InitVertexAttrib1dARB,
    InitVertexAttrib1dvARB,
    InitVertexAttrib1fARB,
    InitVertexAttrib1fvARB,
    InitVertexAttrib1sARB,
    InitVertexAttrib1svARB,
    InitVertexAttrib2dARB,
    InitVertexAttrib2dvARB,
    InitVertexAttrib2fARB,
    InitVertexAttrib2fvARB,
    InitVertexAttrib2sARB,
    InitVertexAttrib2svARB,
    InitVertexAttrib3dARB,
    InitVertexAttrib3dvARB,
    InitVertexAttrib3fARB,
    InitVertexAttrib3fvARB,
    InitVertexAttrib3sARB,
    InitVertexAttrib3svARB,
    InitVertexAttrib4NbvARB,
    InitVertexAttrib4NivARB,
    InitVertexAttrib4NsvARB,
    InitVertexAttrib4NubARB,
    InitVertexAttrib4NubvARB,
    InitVertexAttrib4NuivARB,
    InitVertexAttrib4NusvARB,
    InitVertexAttrib4bvARB,
    InitVertexAttrib4dARB,
    InitVertexAttrib4dvARB,
    InitVertexAttrib4fARB,
    InitVertexAttrib4fvARB,
    InitVertexAttrib4ivARB,
    InitVertexAttrib4sARB,
    InitVertexAttrib4svARB,
    InitVertexAttrib4ubvARB,
    InitVertexAttrib4uivARB,
    InitVertexAttrib4usvARB,
    InitVertexAttribPointerARB,
    InitEnableVertexAttribArrayARB,
    InitDisableVertexAttribArrayARB,
    InitProgramStringARB,
    InitBindProgramARB,
    InitDeleteProgramsARB,
    InitGenProgramsARB,
    InitProgramEnvParameter4dARB,
    InitProgramEnvParameter4dvARB,
    InitProgramEnvParameter4fARB,
    InitProgramEnvParameter4fvARB,
    InitProgramLocalParameter4dARB,
    InitProgramLocalParameter4dvARB,
    InitProgramLocalParameter4fARB,
    InitProgramLocalParameter4fvARB,
    InitGetProgramEnvParameterdvARB,
    InitGetProgramEnvParameterfvARB,
    InitGetProgramLocalParameterdvARB,
    InitGetProgramLocalParameterfvARB,
    InitGetProgramivARB,
    InitGetProgramStringARB,
    InitGetVertexAttribdvARB,
    InitGetVertexAttribfvARB,
    InitGetVertexAttribivARB,
    InitGetVertexAttribPointervARB,
    InitIsProgramARB,
    InitBindBufferARB,
    InitDeleteBuffersARB,
    InitGenBuffersARB,
    InitIsBufferARB,
    InitBufferDataARB,
    InitBufferSubDataARB,
    InitGetBufferSubDataARB,
    InitMapBufferARB,
    InitUnmapBufferARB,
    InitGetBufferParameterivARB,
    InitGetBufferPointervARB,
	InitBlendColorEXT,
	InitPolygonOffsetEXT,
	InitTexImage3DEXT,
	InitTexSubImage3DEXT,
	InitGetTexFilterFuncSGIS,
	InitTexFilterFuncSGIS,
	InitTexSubImage1DEXT,
	InitTexSubImage2DEXT,
	InitCopyTexImage1DEXT,
	InitCopyTexImage2DEXT,
	InitCopyTexSubImage1DEXT,
	InitCopyTexSubImage2DEXT,
	InitCopyTexSubImage3DEXT,
	InitGetHistogramEXT,
	InitGetHistogramParameterfvEXT,
	InitGetHistogramParameterivEXT,
	InitGetMinmaxEXT,
	InitGetMinmaxParameterfvEXT,
	InitGetMinmaxParameterivEXT,
	InitHistogramEXT,
	InitMinmaxEXT,
	InitResetHistogramEXT,
	InitResetMinmaxEXT,
	InitConvolutionFilter1DEXT,
	InitConvolutionFilter2DEXT,
	InitConvolutionParameterfEXT,
	InitConvolutionParameterfvEXT,
	InitConvolutionParameteriEXT,
	InitConvolutionParameterivEXT,
	InitCopyConvolutionFilter1DEXT,
	InitCopyConvolutionFilter2DEXT,
	InitGetConvolutionFilterEXT,
	InitGetConvolutionParameterfvEXT,
	InitGetConvolutionParameterivEXT,
	InitGetSeparableFilterEXT,
	InitSeparableFilter2DEXT,
	InitColorTableSGI,
	InitColorTableParameterfvSGI,
	InitColorTableParameterivSGI,
	InitCopyColorTableSGI,
	InitGetColorTableSGI,
	InitGetColorTableParameterfvSGI,
	InitGetColorTableParameterivSGI,
	InitPixelTexGenSGIX,
	InitPixelTexGenParameteriSGIS,
	InitPixelTexGenParameterivSGIS,
	InitPixelTexGenParameterfSGIS,
	InitPixelTexGenParameterfvSGIS,
	InitGetPixelTexGenParameterivSGIS,
	InitGetPixelTexGenParameterfvSGIS,
	InitTexImage4DSGIS,
	InitTexSubImage4DSGIS,
	InitAreTexturesResidentEXT,
	InitBindTextureEXT,
	InitDeleteTexturesEXT,
	InitGenTexturesEXT,
	InitIsTextureEXT,
	InitPrioritizeTexturesEXT,
	InitDetailTexFuncSGIS,
	InitGetDetailTexFuncSGIS,
	InitSharpenTexFuncSGIS,
	InitGetSharpenTexFuncSGIS,
	InitSampleMaskSGIS,
	InitSamplePatternSGIS,
	InitArrayElementEXT,
	InitColorPointerEXT,
	InitDrawArraysEXT,
	InitEdgeFlagPointerEXT,
	InitGetPointervEXT,
	InitIndexPointerEXT,
	InitNormalPointerEXT,
	InitTexCoordPointerEXT,
	InitVertexPointerEXT,
	InitBlendEquationEXT,
	InitSpriteParameterfSGIX,
	InitSpriteParameterfvSGIX,
	InitSpriteParameteriSGIX,
	InitSpriteParameterivSGIX,
	InitPointParameterfARB,
	InitPointParameterfvARB,
	InitPointParameterfEXT,
	InitPointParameterfvEXT,
	InitPointParameterfSGIS,
	InitPointParameterfvSGIS,
	InitGetInstrumentsSGIX,
	InitInstrumentsBufferSGIX,
	InitPollInstrumentsSGIX,
	InitReadInstrumentsSGIX,
	InitStartInstrumentsSGIX,
	InitStopInstrumentsSGIX,
	InitFrameZoomSGIX,
	InitTagSampleBufferSGIX,
	InitDeformationMap3dSGIX,
	InitDeformationMap3fSGIX,
	InitDeformSGIX,
	InitLoadIdentityDeformationMapSGIX,
	InitReferencePlaneSGIX,
	InitFlushRasterSGIX,
	InitFogFuncSGIS,
	InitGetFogFuncSGIS,
	InitImageTransformParameteriHP,
	InitImageTransformParameterfHP,
	InitImageTransformParameterivHP,
	InitImageTransformParameterfvHP,
	InitGetImageTransformParameterivHP,
	InitGetImageTransformParameterfvHP,
	InitColorSubTableEXT,
	InitCopyColorSubTableEXT,
	InitHintPGI,
	InitColorTableEXT,
	InitGetColorTableEXT,
	InitGetColorTableParameterivEXT,
	InitGetColorTableParameterfvEXT,
	InitGetListParameterfvSGIX,
	InitGetListParameterivSGIX,
	InitListParameterfSGIX,
	InitListParameterfvSGIX,
	InitListParameteriSGIX,
	InitListParameterivSGIX,
	InitIndexMaterialEXT,
	InitIndexFuncEXT,
	InitLockArraysEXT,
	InitUnlockArraysEXT,
	InitCullParameterdvEXT,
	InitCullParameterfvEXT,
	InitFragmentColorMaterialSGIX,
	InitFragmentLightfSGIX,
	InitFragmentLightfvSGIX,
	InitFragmentLightiSGIX,
	InitFragmentLightivSGIX,
	InitFragmentLightModelfSGIX,
	InitFragmentLightModelfvSGIX,
	InitFragmentLightModeliSGIX,
	InitFragmentLightModelivSGIX,
	InitFragmentMaterialfSGIX,
	InitFragmentMaterialfvSGIX,
	InitFragmentMaterialiSGIX,
	InitFragmentMaterialivSGIX,
	InitGetFragmentLightfvSGIX,
	InitGetFragmentLightivSGIX,
	InitGetFragmentMaterialfvSGIX,
	InitGetFragmentMaterialivSGIX,
	InitLightEnviSGIX,
	InitDrawRangeElementsEXT,
	InitApplyTextureEXT,
	InitTextureLightEXT,
	InitTextureMaterialEXT,
	InitAsyncMarkerSGIX,
	InitFinishAsyncSGIX,
	InitPollAsyncSGIX,
	InitGenAsyncMarkersSGIX,
	InitDeleteAsyncMarkersSGIX,
	InitIsAsyncMarkerSGIX,
	InitVertexPointervINTEL,
	InitNormalPointervINTEL,
	InitColorPointervINTEL,
	InitTexCoordPointervINTEL,
	InitPixelTransformParameteriEXT,
	InitPixelTransformParameterfEXT,
	InitPixelTransformParameterivEXT,
	InitPixelTransformParameterfvEXT,
	InitSecondaryColor3bEXT,
	InitSecondaryColor3bvEXT,
	InitSecondaryColor3dEXT,
	InitSecondaryColor3dvEXT,
	InitSecondaryColor3fEXT,
	InitSecondaryColor3fvEXT,
	InitSecondaryColor3iEXT,
	InitSecondaryColor3ivEXT,
	InitSecondaryColor3sEXT,
	InitSecondaryColor3svEXT,
	InitSecondaryColor3ubEXT,
	InitSecondaryColor3ubvEXT,
	InitSecondaryColor3uiEXT,
	InitSecondaryColor3uivEXT,
	InitSecondaryColor3usEXT,
	InitSecondaryColor3usvEXT,
	InitSecondaryColorPointerEXT,
	InitTextureNormalEXT,
	InitMultiDrawArraysEXT,
	InitMultiDrawElementsEXT,
	InitFogCoordfEXT,
	InitFogCoordfvEXT,
	InitFogCoorddEXT,
	InitFogCoorddvEXT,
	InitFogCoordPointerEXT,
	InitTangent3bEXT,
	InitTangent3bvEXT,
	InitTangent3dEXT,
	InitTangent3dvEXT,
	InitTangent3fEXT,
	InitTangent3fvEXT,
	InitTangent3iEXT,
	InitTangent3ivEXT,
	InitTangent3sEXT,
	InitTangent3svEXT,
	InitBinormal3bEXT,
	InitBinormal3bvEXT,
	InitBinormal3dEXT,
	InitBinormal3dvEXT,
	InitBinormal3fEXT,
	InitBinormal3fvEXT,
	InitBinormal3iEXT,
	InitBinormal3ivEXT,
	InitBinormal3sEXT,
	InitBinormal3svEXT,
	InitTangentPointerEXT,
	InitBinormalPointerEXT,
	InitFinishTextureSUNX,
	InitGlobalAlphaFactorbSUN,
	InitGlobalAlphaFactorsSUN,
	InitGlobalAlphaFactoriSUN,
	InitGlobalAlphaFactorfSUN,
	InitGlobalAlphaFactordSUN,
	InitGlobalAlphaFactorubSUN,
	InitGlobalAlphaFactorusSUN,
	InitGlobalAlphaFactoruiSUN,
	InitReplacementCodeuiSUN,
	InitReplacementCodeusSUN,
	InitReplacementCodeubSUN,
	InitReplacementCodeuivSUN,
	InitReplacementCodeusvSUN,
	InitReplacementCodeubvSUN,
	InitReplacementCodePointerSUN,
	InitColor4ubVertex2fSUN,
	InitColor4ubVertex2fvSUN,
	InitColor4ubVertex3fSUN,
	InitColor4ubVertex3fvSUN,
	InitColor3fVertex3fSUN,
	InitColor3fVertex3fvSUN,
	InitNormal3fVertex3fSUN,
	InitNormal3fVertex3fvSUN,
	InitColor4fNormal3fVertex3fSUN,
	InitColor4fNormal3fVertex3fvSUN,
	InitTexCoord2fVertex3fSUN,
	InitTexCoord2fVertex3fvSUN,
	InitTexCoord4fVertex4fSUN,
	InitTexCoord4fVertex4fvSUN,
	InitTexCoord2fColor4ubVertex3fSUN,
	InitTexCoord2fColor4ubVertex3fvSUN,
	InitTexCoord2fColor3fVertex3fSUN,
	InitTexCoord2fColor3fVertex3fvSUN,
	InitTexCoord2fNormal3fVertex3fSUN,
	InitTexCoord2fNormal3fVertex3fvSUN,
	InitTexCoord2fColor4fNormal3fVertex3fSUN,
	InitTexCoord2fColor4fNormal3fVertex3fvSUN,
	InitTexCoord4fColor4fNormal3fVertex4fSUN,
	InitTexCoord4fColor4fNormal3fVertex4fvSUN,
	InitReplacementCodeuiVertex3fSUN,
	InitReplacementCodeuiVertex3fvSUN,
	InitReplacementCodeuiColor4ubVertex3fSUN,
	InitReplacementCodeuiColor4ubVertex3fvSUN,
	InitReplacementCodeuiColor3fVertex3fSUN,
	InitReplacementCodeuiColor3fVertex3fvSUN,
	InitReplacementCodeuiNormal3fVertex3fSUN,
	InitReplacementCodeuiNormal3fVertex3fvSUN,
	InitReplacementCodeuiColor4fNormal3fVertex3fSUN,
	InitReplacementCodeuiColor4fNormal3fVertex3fvSUN,
	InitReplacementCodeuiTexCoord2fVertex3fSUN,
	InitReplacementCodeuiTexCoord2fVertex3fvSUN,
	InitReplacementCodeuiTexCoord2fNormal3fVertex3fSUN,
	InitReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN,
	InitReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN,
	InitReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN,
	InitBlendFuncSeparateEXT,
	InitBlendFuncSeparateINGR,
	InitVertexWeightfEXT,
	InitVertexWeightfvEXT,
	InitVertexWeightPointerEXT,
	InitFlushVertexArrayRangeNV,
	InitVertexArrayRangeNV,
	InitCombinerParameterfvNV,
	InitCombinerParameterfNV,
	InitCombinerParameterivNV,
	InitCombinerParameteriNV,
	InitCombinerInputNV,
	InitCombinerOutputNV,
	InitFinalCombinerInputNV,
	InitGetCombinerInputParameterfvNV,
	InitGetCombinerInputParameterivNV,
	InitGetCombinerOutputParameterfvNV,
	InitGetCombinerOutputParameterivNV,
	InitGetFinalCombinerInputParameterfvNV,
	InitGetFinalCombinerInputParameterivNV,
	InitResizeBuffersMESA,
	InitWindowPos2dMESA,
	InitWindowPos2dvMESA,
	InitWindowPos2fMESA,
	InitWindowPos2fvMESA,
	InitWindowPos2iMESA,
	InitWindowPos2ivMESA,
	InitWindowPos2sMESA,
	InitWindowPos2svMESA,
	InitWindowPos3dMESA,
	InitWindowPos3dvMESA,
	InitWindowPos3fMESA,
	InitWindowPos3fvMESA,
	InitWindowPos3iMESA,
	InitWindowPos3ivMESA,
	InitWindowPos3sMESA,
	InitWindowPos3svMESA,
	InitWindowPos4dMESA,
	InitWindowPos4dvMESA,
	InitWindowPos4fMESA,
	InitWindowPos4fvMESA,
	InitWindowPos4iMESA,
	InitWindowPos4ivMESA,
	InitWindowPos4sMESA,
	InitWindowPos4svMESA,
	InitMultiModeDrawArraysIBM,
	InitMultiModeDrawElementsIBM,
	InitColorPointerListIBM,
	InitSecondaryColorPointerListIBM,
	InitEdgeFlagPointerListIBM,
	InitFogCoordPointerListIBM,
	InitIndexPointerListIBM,
	InitNormalPointerListIBM,
	InitTexCoordPointerListIBM,
	InitVertexPointerListIBM,
	InitTbufferMask3DFX,
	InitSampleMaskEXT,
	InitSamplePatternEXT,
	InitTextureColorMaskSGIS,
	InitIglooInterfaceSGIX,
	InitGenFencesNV,
	InitDeleteFencesNV,
	InitSetFenceNV,
	InitTestFenceNV,
	InitFinishFenceNV,
	InitIsFenceNV,
	InitGetFenceivNV,
	InitMapControlPointsNV,
	InitMapParameterivNV,
	InitMapParameterfvNV,
	InitGetMapControlPointsNV,
	InitGetMapParameterivNV,
	InitGetMapParameterfvNV,
	InitGetMapAttribParameterivNV,
	InitGetMapAttribParameterfvNV,
	InitEvalMapsNV,
	InitCombinerStageParameterfvNV,
	InitGetCombinerStageParameterfvNV,
	InitBindProgramNV,
	InitDeleteProgramsNV,
	InitExecuteProgramNV,
	InitGenProgramsNV,
	InitAreProgramsResidentNV,
	InitRequestResidentProgramsNV,
	InitGetProgramParameterfvNV,
	InitGetProgramParameterdvNV,
	InitGetProgramivNV,
	InitGetProgramStringNV,
	InitGetTrackMatrixivNV,
	InitGetVertexAttribdvNV,
	InitGetVertexAttribfvNV,
	InitGetVertexAttribivNV,
	InitGetVertexAttribPointervNV,
	InitIsProgramNV,
	InitLoadProgramNV,
	InitProgramParameter4fNV,
	InitProgramParameter4dNV,
	InitProgramParameter4dvNV,
	InitProgramParameter4fvNV,
	InitProgramParameters4dvNV,
	InitProgramParameters4fvNV,
	InitTrackMatrixNV,
	InitVertexAttribPointerNV,
	InitVertexAttrib1sNV,
	InitVertexAttrib1fNV,
	InitVertexAttrib1dNV,
	InitVertexAttrib2sNV,
	InitVertexAttrib2fNV,
	InitVertexAttrib2dNV,
	InitVertexAttrib3sNV,
	InitVertexAttrib3fNV,
	InitVertexAttrib3dNV,
	InitVertexAttrib4sNV,
	InitVertexAttrib4fNV,
	InitVertexAttrib4dNV,
	InitVertexAttrib4ubNV,
	InitVertexAttrib1svNV,
	InitVertexAttrib1fvNV,
	InitVertexAttrib1dvNV,
	InitVertexAttrib2svNV,
	InitVertexAttrib2fvNV,
	InitVertexAttrib2dvNV,
	InitVertexAttrib3svNV,
	InitVertexAttrib3fvNV,
	InitVertexAttrib3dvNV,
	InitVertexAttrib4svNV,
	InitVertexAttrib4fvNV,
	InitVertexAttrib4dvNV,
	InitVertexAttrib4ubvNV,
	InitVertexAttribs1svNV,
	InitVertexAttribs1fvNV,
	InitVertexAttribs1dvNV,
	InitVertexAttribs2svNV,
	InitVertexAttribs2fvNV,
	InitVertexAttribs2dvNV,
	InitVertexAttribs3svNV,
	InitVertexAttribs3fvNV,
	InitVertexAttribs3dvNV,
	InitVertexAttribs4svNV,
	InitVertexAttribs4fvNV,
	InitVertexAttribs4dvNV,
	InitVertexAttribs4ubvNV,
	InitAddSwapHintRectWIN,
	InitTexBumpParameterivATI,
	InitTexBumpParameterfvATI,
	InitGetTexBumpParameterivATI,
	InitGetTexBumpParameterfvATI,
	InitGenFragmentShadersATI,
	InitBindFragmentShaderATI,
	InitDeleteFragmentShadersATI,
	InitBeginFragmentShadersATI,
	InitEndFragmentShadersATI,
	InitPassTexCoordATI,
	InitSampleMapATI,
	InitColorFragmentOp1ATI,
	InitColorFragmentOp2ATI,
	InitColorFragmentOp3ATI,
	InitAlphaFragmentOp1ATI,
	InitAlphaFragmentOp2ATI,
	InitAlphaFragmentOp3ATI,
	InitSetFragmentShaderConstantATI,
	InitPnTrianglesiATI,
	InitPnTrianglesfATI,
	InitStencilOpSeparatorATI,
	InitStencilFuncSeparatorATI,
	InitClientActiveVertexStreamATI,
	InitVertexBlendEnviATI,
	InitVertexBlendEnvfATI,
	InitVertexStream2sATI,
	InitVertexStream2svATI,
	InitVertexStream2iATI,
	InitVertexStream2ivATI,
	InitVertexStream2fATI,
	InitVertexStream2fvATI,
	InitVertexStream2dATI,
	InitVertexStream2dvATI,
	InitVertexStream3sATI,
	InitVertexStream3svATI,
	InitVertexStream3iATI,
	InitVertexStream3ivATI,
	InitVertexStream3fATI,
	InitVertexStream3fvATI,
	InitVertexStream3dATI,
	InitVertexStream3dvATI,
	InitVertexStream4sATI,
	InitVertexStream4svATI,
	InitVertexStream4iATI,
	InitVertexStream4ivATI,
	InitVertexStream4fATI,
	InitVertexStream4fvATI,
	InitVertexStream4dATI,
	InitVertexStream4dvATI,
	InitNormalStream3bATI,
	InitNormalStream3bvATI,
	InitNormalStream3sATI,
	InitNormalStream3svATI,
	InitNormalStream3iATI,
	InitNormalStream3ivATI,
	InitNormalStream3fATI,
	InitNormalStream3fvATI,
	InitNormalStream3dATI,
	InitNormalStream3dvATI,
	InitNewObjectBufferATI,
	InitIsObjectBufferATI,
	InitUpdateObjectBufferATI,
	InitGetObjectBufferfvATI,
	InitGetObjectBufferivATI,
	InitFreeObjectBufferATI,
	InitArrayObjectATI,
	InitGetArrayObjectfvATI,
	InitGetArrayObjectivATI,
	InitVariantArrayObjectATI,
	InitGetVariantArrayObjectfvATI,
	InitGetVariantArrayObjectivATI,
	InitElementPointerATI,
	InitDrawElementArrayATI,
	InitDrawRangeElementArrayATI,
	InitMapObjectBufferATI,
	InitUnmapObjectBufferATI,
	InitVertexAttribArrayObjectATI,
	InitGetVertexAttribArrayObjectfvATI,
	InitGetVertexAttribArrayObjectivATI,
	InitDrawBuffersATI,
	InitGenOcclusionQueriesNV,
	InitDeleteOcclusionQueriesNV,
	InitIsOcclusionQueryNV,
	InitBeginOcclusionQueryNV,
	InitEndOcclusionQueryNV,
	InitGetOcclusionQueryivNV,
	InitGetOcclusionQueryuivNV,
	InitPointParameteriNV,
	InitPointParameterivNV,
#ifdef _WIN32
	InitCreateBufferRegionARB,
	InitDeleteBufferRegionARB,
	InitSaveBufferRegionARB,
	InitRestoreBufferRegionARB,
	InitGetExtensionsStringARB,
	InitGetPixelFormatAttribivARB,
	InitGetPixelFormatAttribfvARB,
	InitChoosePixelFormatARB,
	InitMakeContextCurrentARB,
	InitGetCurrentReadDCARB,
	InitCreatePbufferARB,
	InitGetPbufferDCARB,
	InitReleasePbufferDCARB,
	InitDestroyPbufferARB,
	InitQueryPbufferARB,
	InitCreateDisplayColorTableEXT,
	InitLoadDisplayColorTableEXT,
	InitBindDisplayColorTableEXT,
	InitDestroyDisplayColorTableEXT,
	InitGetExtensionsStringEXT,
	InitMakeContextCurrentEXT,
	InitGetCurrentReadDCEXT,
	InitCreatePbufferEXT,
	InitGetPbufferDCEXT,
	InitReleasePbufferDCEXT,
	InitDestroyPbufferEXT,
	InitQueryPbufferEXT,
	InitGetPixelFormatAttribivEXT,
	InitGetPixelFormatAttribfvEXT,
	InitChoosePixelFormatEXT,
	InitSwapIntervalEXT,
	InitGetSwapIntervalEXT,
#endif /* _WIN32 */
};
