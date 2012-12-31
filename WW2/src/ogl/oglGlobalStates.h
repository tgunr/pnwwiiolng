#pragma once

#include "ogl/ogl.h"

namespace ogl
{
	void SetVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
	void SetNormalPointer(GLenum type, GLsizei stride, const GLvoid* pointer);
	void SetColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
	void SetTextureUnitCoordPointer(GLenum unit, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
    void SetVertexAttribPointer(unsigned int index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

	void BindElementArrayBuffer(GLuint name);
	void BindArrayBuffer(GLuint name);
	void DrawArrays(GLenum mode, GLint first, GLsizei count);
	void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, int primcount);
	void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
	void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, int primcount);
	void DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices);
	void Disable();
    void SetUseDeprecatedPath(bool flag);

    bool UseDeprecatedPath();
}//namespace ogl

