#ifndef G_OPENGL_CONTEXT_H
#define G_OPENGL_CONTEXT_H


#include <GL/glew.h>
//#include <memory> // :-( they do the C compiler

extern GLEWContext* G_OpenGL_Context;
#define glewGetContext() G_OpenGL_Context

#endif