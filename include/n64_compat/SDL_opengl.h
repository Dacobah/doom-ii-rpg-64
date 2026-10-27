#ifndef __N64_SDL_OPENGL_H__
#define __N64_SDL_OPENGL_H__

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef APIENTRYP
#define APIENTRYP *
#endif

#ifndef GLAPI
#define GLAPI extern
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#endif

#ifndef GL_COMBINE
#define GL_COMBINE 0x8570
#endif
#ifndef GL_COMBINE_RGB
#define GL_COMBINE_RGB 0x8571
#endif
#ifndef GL_COMBINE_ALPHA
#define GL_COMBINE_ALPHA 0x8572
#endif
#ifndef GL_SOURCE0_RGB
#define GL_SOURCE0_RGB 0x8580
#endif
#ifndef GL_SOURCE1_RGB
#define GL_SOURCE1_RGB 0x8581
#endif
#ifndef GL_SOURCE0_ALPHA
#define GL_SOURCE0_ALPHA 0x8588
#endif
#ifndef GL_SOURCE1_ALPHA
#define GL_SOURCE1_ALPHA 0x8589
#endif
#ifndef GL_OPERAND0_RGB
#define GL_OPERAND0_RGB 0x8590
#endif
#ifndef GL_OPERAND1_RGB
#define GL_OPERAND1_RGB 0x8591
#endif
#ifndef GL_OPERAND0_ALPHA
#define GL_OPERAND0_ALPHA 0x8598
#endif
#ifndef GL_OPERAND1_ALPHA
#define GL_OPERAND1_ALPHA 0x8599
#endif
#ifndef GL_CONSTANT
#define GL_CONSTANT 0x8576
#endif
#ifndef GL_PRIMARY_COLOR
#define GL_PRIMARY_COLOR 0x8577
#endif

typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRYP PFNGLCLIENTACTIVETEXTUREPROC) (GLenum texture);

#endif
