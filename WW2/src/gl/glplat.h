#pragma once
//  Created by Andrew Wulf on 10/23/09.

#if platOS == OS_MACOS
  #include <OpenGL/OpenGL.h>

extern CGLContextObj cgl_ctx;

// stolen from OpenGL/CGLMacro.h ----------------------------
  #include <OpenGL/CGLTypes.h>
  #include <OpenGL/CGLContext.h>

  /* Macro context name (CGLContextObj) */
  #if !defined(CGL_MACRO_CONTEXT)
  #define CGL_MACRO_CONTEXT cgl_ctx
  #endif

  /* Macro renderer name (GLIContext) */
  #if !defined(CGL_MACRO_RENDERER)
  #define CGL_MACRO_RENDERER cgl_rend
  #endif

  #define CGL_MACRO_DECLARE_CONTEXT()  CGLContextObj CGL_MACRO_CONTEXT = CGLGetCurrentContext();

  /* Repeated renderer lookups may be avoided by defining CGL_MACRO_CACHE_RENDERER before including CGLMacro.h.
   Note: If you change the value of CGL_MACRO_CONTEXT partway through a function, you will have to
   keep CGL_MACRO_RENDERER up to date with with it's context. */
  #if defined(CGL_MACRO_CACHE_RENDERER)
  #define CGL_MACRO_DECLARE_RENDERER()  GLIContext CGL_MACRO_RENDERER = CGL_MACRO_CONTEXT->rend;
  #define CGL_MACRO_CONTEXT_RENDERER  CGL_MACRO_RENDERER
  #else
  #define CGL_MACRO_DECLARE_RENDERER()
  #define CGL_MACRO_CONTEXT_RENDERER  CGL_MACRO_CONTEXT->rend
  #endif

  /* Use the following function macro to declare the local CGLMacro variables */
  #define CGL_MACRO_DECLARE_VARIABLES() \
  CGL_MACRO_DECLARE_CONTEXT(); \
  CGL_MACRO_DECLARE_RENDERER();

  #define glBegin(mode) \
  (*(CGL_MACRO_CONTEXT)->disp.begin)(CGL_MACRO_CONTEXT_RENDERER, mode)
  #define glDrawElements(mode, count, type, indices) \
  (*(CGL_MACRO_CONTEXT)->disp.draw_elements)(CGL_MACRO_CONTEXT_RENDERER, mode, count, type, indices)
  #define glDrawArrays(mode, first, count) \
  (*(CGL_MACRO_CONTEXT)->disp.draw_arrays)(CGL_MACRO_CONTEXT_RENDERER, mode, first, count)
  #define glVertex2f(x, y) \
  (*(CGL_MACRO_CONTEXT)->disp.vertex2f)(CGL_MACRO_CONTEXT_RENDERER, x, y)
  #define glVertex2fv(v) \
  (*(CGL_MACRO_CONTEXT)->disp.vertex2fv)(CGL_MACRO_CONTEXT_RENDERER, v)
  #define glTexCoord2f(s, t) \
  (*(CGL_MACRO_CONTEXT)->disp.tex_coord2f)(CGL_MACRO_CONTEXT_RENDERER, s, t)
  #define glTexCoord2fv(v) \
  (*(CGL_MACRO_CONTEXT)->disp.tex_coord2fv)(CGL_MACRO_CONTEXT_RENDERER, v)
  #define glEnd() \
  (*(CGL_MACRO_CONTEXT)->disp.end)(CGL_MACRO_CONTEXT_RENDERER)
  #define glPopAttrib() \
  (*(CGL_MACRO_CONTEXT)->disp.pop_attrib)(CGL_MACRO_CONTEXT_RENDERER)
#undef glDrawRangeElements
  #define glDrawRangeElements(mode, start, end, count, type, indices) \
  (*(CGL_MACRO_CONTEXT)->disp.draw_range_elements)(CGL_MACRO_CONTEXT_RENDERER, mode, start, end, count, type, indices)
  #define glGetError() \
  (*(CGL_MACRO_CONTEXT)->disp.get_error)(CGL_MACRO_CONTEXT_RENDERER)
  #define glCallList(list) \
  (*(CGL_MACRO_CONTEXT)->disp.call_list)(CGL_MACRO_CONTEXT_RENDERER, list)
  #define glTexCoordPointer(size, type, stride, pointer) \
  (*(CGL_MACRO_CONTEXT)->disp.tex_coord_pointer)(CGL_MACRO_CONTEXT_RENDERER, size, type, stride, pointer)
  #define glNormalPointer(type, stride, pointer) \
  (*(CGL_MACRO_CONTEXT)->disp.normal_pointer)(CGL_MACRO_CONTEXT_RENDERER, type, stride, pointer)
  #define glVertexPointer(size, type, stride, pointer) \
  (*(CGL_MACRO_CONTEXT)->disp.vertex_pointer)(CGL_MACRO_CONTEXT_RENDERER, size, type, stride, pointer)
  #define glLoadMatrixf(m) \
  (*(CGL_MACRO_CONTEXT)->disp.load_matrixf)(CGL_MACRO_CONTEXT_RENDERER, m)

#else
  #include "gl/gl.h"
#endif
