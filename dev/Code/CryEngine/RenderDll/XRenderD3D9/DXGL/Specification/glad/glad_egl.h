/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef __glad_egl_h_

#ifdef __egl_h_
#error EGL header already included, remove this include, glad already provides it
#endif

#define __glad_egl_h_
#define __egl_h_

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __gladloadproc__
#define __gladloadproc__
typedef void* (* GLADloadproc)(const char* name);
#endif
GLAPI int gladLoadEGLLoader(GLADloadproc);

GLAPI int gladLoadEGL(void);
GLAPI int gladLoadEGLLoader(GLADloadproc);

#include "khrplatform.h"
#include <EGL/eglplatform.h>
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef intptr_t EGLAttribKHR;
typedef intptr_t EGLAttrib;
typedef void* EGLClientBuffer;
typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLDeviceEXT;
typedef void* EGLDisplay;
typedef void* EGLImage;
typedef void* EGLImageKHR;
typedef void* EGLOutputLayerEXT;
typedef void* EGLOutputPortEXT;
typedef void* EGLStreamKHR;
typedef void* EGLSurface;
typedef void* EGLSync;
typedef void* EGLSyncKHR;
typedef void* EGLSyncNV;
typedef void (* __eglMustCastToProperFunctionPointerType)(void);
typedef khronos_utime_nanoseconds_t EGLTimeKHR;
typedef khronos_utime_nanoseconds_t EGLTime;
typedef khronos_utime_nanoseconds_t EGLTimeNV;
typedef khronos_utime_nanoseconds_t EGLuint64NV;
typedef khronos_uint64_t EGLuint64KHR;
typedef int EGLNativeFileDescriptorKHR;
typedef khronos_ssize_t EGLsizeiANDROID;
typedef void (* EGLSetBlobFuncANDROID) (const void* key, EGLsizeiANDROID keySize, const void* value, EGLsizeiANDROID valueSize);
typedef EGLsizeiANDROID (* EGLGetBlobFuncANDROID) (const void* key, EGLsizeiANDROID keySize, void* value, EGLsizeiANDROID valueSize);
struct EGLClientPixmapHI
{
    void* pData;
    EGLint iWidth;
    EGLint iHeight;
    EGLint iStride;
};
#define EGL_ALPHA_SIZE 0x3021
#define EGL_BAD_ACCESS 0x3002
#define EGL_BAD_ALLOC 0x3003
#define EGL_BAD_ATTRIBUTE 0x3004
#define EGL_BAD_CONFIG 0x3005
#define EGL_BAD_CONTEXT 0x3006
#define EGL_BAD_CURRENT_SURFACE 0x3007
#define EGL_BAD_DISPLAY 0x3008
#define EGL_BAD_MATCH 0x3009
#define EGL_BAD_NATIVE_PIXMAP 0x300A
#define EGL_BAD_NATIVE_WINDOW 0x300B
#define EGL_BAD_PARAMETER 0x300C
#define EGL_BAD_SURFACE 0x300D
#define EGL_BLUE_SIZE 0x3022
#define EGL_BUFFER_SIZE 0x3020
#define EGL_CONFIG_CAVEAT 0x3027
#define EGL_CONFIG_ID 0x3028
#define EGL_CORE_NATIVE_ENGINE 0x305B
#define EGL_DEPTH_SIZE 0x3025
#define EGL_DONT_CARE ((EGLint)-1)
#define EGL_DRAW 0x3059
#define EGL_EXTENSIONS 0x3055
#define EGL_FALSE 0
#define EGL_GREEN_SIZE 0x3023
#define EGL_HEIGHT 0x3056
#define EGL_LARGEST_PBUFFER 0x3058
#define EGL_LEVEL 0x3029
#define EGL_MAX_PBUFFER_HEIGHT 0x302A
#define EGL_MAX_PBUFFER_PIXELS 0x302B
#define EGL_MAX_PBUFFER_WIDTH 0x302C
#define EGL_NATIVE_RENDERABLE 0x302D
#define EGL_NATIVE_VISUAL_ID 0x302E
#define EGL_NATIVE_VISUAL_TYPE 0x302F
#define EGL_NONE 0x3038
#define EGL_NON_CONFORMANT_CONFIG 0x3051
#define EGL_NOT_INITIALIZED 0x3001
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_SURFACE ((EGLSurface)0)
#define EGL_PBUFFER_BIT 0x0001
#define EGL_PIXMAP_BIT 0x0002
#define EGL_READ 0x305A
#define EGL_RED_SIZE 0x3024
#define EGL_SAMPLES 0x3031
#define EGL_SAMPLE_BUFFERS 0x3032
#define EGL_SLOW_CONFIG 0x3050
#define EGL_STENCIL_SIZE 0x3026
#define EGL_SUCCESS 0x3000
#define EGL_SURFACE_TYPE 0x3033
#define EGL_TRANSPARENT_BLUE_VALUE 0x3035
#define EGL_TRANSPARENT_GREEN_VALUE 0x3036
#define EGL_TRANSPARENT_RED_VALUE 0x3037
#define EGL_TRANSPARENT_RGB 0x3052
#define EGL_TRANSPARENT_TYPE 0x3034
#define EGL_TRUE 1
#define EGL_VENDOR 0x3053
#define EGL_VERSION 0x3054
#define EGL_WIDTH 0x3057
#define EGL_WINDOW_BIT 0x0004
#define EGL_BACK_BUFFER 0x3084
#define EGL_BIND_TO_TEXTURE_RGB 0x3039
#define EGL_BIND_TO_TEXTURE_RGBA 0x303A
#define EGL_CONTEXT_LOST 0x300E
#define EGL_MIN_SWAP_INTERVAL 0x303B
#define EGL_MAX_SWAP_INTERVAL 0x303C
#define EGL_MIPMAP_TEXTURE 0x3082
#define EGL_MIPMAP_LEVEL 0x3083
#define EGL_NO_TEXTURE 0x305C
#define EGL_TEXTURE_2D 0x305F
#define EGL_TEXTURE_FORMAT 0x3080
#define EGL_TEXTURE_RGB 0x305D
#define EGL_TEXTURE_RGBA 0x305E
#define EGL_TEXTURE_TARGET 0x3081
#define EGL_ALPHA_FORMAT 0x3088
#define EGL_ALPHA_FORMAT_NONPRE 0x308B
#define EGL_ALPHA_FORMAT_PRE 0x308C
#define EGL_ALPHA_MASK_SIZE 0x303E
#define EGL_BUFFER_PRESERVED 0x3094
#define EGL_BUFFER_DESTROYED 0x3095
#define EGL_CLIENT_APIS 0x308D
#define EGL_COLORSPACE 0x3087
#define EGL_COLORSPACE_sRGB 0x3089
#define EGL_COLORSPACE_LINEAR 0x308A
#define EGL_COLOR_BUFFER_TYPE 0x303F
#define EGL_CONTEXT_CLIENT_TYPE 0x3097
#define EGL_DISPLAY_SCALING 10000
#define EGL_HORIZONTAL_RESOLUTION 0x3090
#define EGL_LUMINANCE_BUFFER 0x308F
#define EGL_LUMINANCE_SIZE 0x303D
#define EGL_OPENGL_ES_BIT 0x0001
#define EGL_OPENVG_BIT 0x0002
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_OPENVG_API 0x30A1
#define EGL_OPENVG_IMAGE 0x3096
#define EGL_PIXEL_ASPECT_RATIO 0x3092
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_RENDER_BUFFER 0x3086
#define EGL_RGB_BUFFER 0x308E
#define EGL_SINGLE_BUFFER 0x3085
#define EGL_SWAP_BEHAVIOR 0x3093
#define EGL_UNKNOWN ((EGLint)-1)
#define EGL_VERTICAL_RESOLUTION 0x3091
#define EGL_CONFORMANT 0x3042
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_MATCH_NATIVE_PIXMAP 0x3041
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_VG_ALPHA_FORMAT 0x3088
#define EGL_VG_ALPHA_FORMAT_NONPRE 0x308B
#define EGL_VG_ALPHA_FORMAT_PRE 0x308C
#define EGL_VG_ALPHA_FORMAT_PRE_BIT 0x0040
#define EGL_VG_COLORSPACE 0x3087
#define EGL_VG_COLORSPACE_sRGB 0x3089
#define EGL_VG_COLORSPACE_LINEAR 0x308A
#define EGL_VG_COLORSPACE_LINEAR_BIT 0x0020
#define EGL_DEFAULT_DISPLAY ((EGLNativeDisplayType)0)
#define EGL_MULTISAMPLE_RESOLVE_BOX_BIT 0x0200
#define EGL_MULTISAMPLE_RESOLVE 0x3099
#define EGL_MULTISAMPLE_RESOLVE_DEFAULT 0x309A
#define EGL_MULTISAMPLE_RESOLVE_BOX 0x309B
#define EGL_OPENGL_API 0x30A2
#define EGL_OPENGL_BIT 0x0008
#define EGL_SWAP_BEHAVIOR_PRESERVED_BIT 0x0400
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY 0x31BD
#define EGL_NO_RESET_NOTIFICATION 0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET 0x31BF
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT 0x00000001
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT 0x00000002
#define EGL_CONTEXT_OPENGL_DEBUG 0x31B0
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE 0x31B1
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS 0x31B2
#define EGL_OPENGL_ES3_BIT 0x00000040
#define EGL_CL_EVENT_HANDLE 0x309C
#define EGL_SYNC_CL_EVENT 0x30FE
#define EGL_SYNC_CL_EVENT_COMPLETE 0x30FF
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE 0x30F0
#define EGL_SYNC_TYPE 0x30F7
#define EGL_SYNC_STATUS 0x30F1
#define EGL_SYNC_CONDITION 0x30F8
#define EGL_SIGNALED 0x30F2
#define EGL_UNSIGNALED 0x30F3
#define EGL_SYNC_FLUSH_COMMANDS_BIT 0x0001
#define EGL_FOREVER 0xFFFFFFFFFFFFFFFF
#define EGL_TIMEOUT_EXPIRED 0x30F5
#define EGL_CONDITION_SATISFIED 0x30F6
#define EGL_NO_SYNC ((EGLSync)0)
#define EGL_SYNC_FENCE 0x30F9
#define EGL_GL_COLORSPACE 0x309D
#define EGL_GL_COLORSPACE_SRGB 0x3089
#define EGL_GL_COLORSPACE_LINEAR 0x308A
#define EGL_GL_RENDERBUFFER 0x30B9
#define EGL_GL_TEXTURE_2D 0x30B1
#define EGL_GL_TEXTURE_LEVEL 0x30BC
#define EGL_GL_TEXTURE_3D 0x30B2
#define EGL_GL_TEXTURE_ZOFFSET 0x30BD
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x30B3
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x30B4
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x30B5
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x30B6
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x30B7
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x30B8
#define EGL_IMAGE_PRESERVED 0x30D2
#define EGL_NO_IMAGE ((EGLImage)0)
EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
EGLBoolean eglCopyBuffers(EGLDisplay, EGLSurface, EGLNativePixmapType);
EGLContext eglCreateContext(EGLDisplay, EGLConfig, EGLContext, const EGLint*);
EGLSurface eglCreatePbufferSurface(EGLDisplay, EGLConfig, const EGLint*);
EGLSurface eglCreatePixmapSurface(EGLDisplay, EGLConfig, EGLNativePixmapType, const EGLint*);
EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*);
EGLBoolean eglDestroyContext(EGLDisplay, EGLContext);
EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface);
EGLBoolean eglGetConfigAttrib(EGLDisplay, EGLConfig, EGLint, EGLint*);
EGLBoolean eglGetConfigs(EGLDisplay, EGLConfig*, EGLint, EGLint*);
EGLDisplay eglGetCurrentDisplay();
EGLSurface eglGetCurrentSurface(EGLint);
EGLDisplay eglGetDisplay(EGLNativeDisplayType);
EGLint eglGetError();
__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char*);
EGLBoolean eglInitialize(EGLDisplay, EGLint*, EGLint*);
EGLBoolean eglMakeCurrent(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
EGLBoolean eglQueryContext(EGLDisplay, EGLContext, EGLint, EGLint*);
const char* eglQueryString(EGLDisplay, EGLint);
EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface, EGLint, EGLint*);
EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface);
EGLBoolean eglTerminate(EGLDisplay);
EGLBoolean eglWaitGL();
EGLBoolean eglWaitNative(EGLint);
EGLBoolean eglBindTexImage(EGLDisplay, EGLSurface, EGLint);
EGLBoolean eglReleaseTexImage(EGLDisplay, EGLSurface, EGLint);
EGLBoolean eglSurfaceAttrib(EGLDisplay, EGLSurface, EGLint, EGLint);
EGLBoolean eglSwapInterval(EGLDisplay, EGLint);
EGLBoolean eglBindAPI(EGLenum);
EGLenum eglQueryAPI();
EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay, EGLenum, EGLClientBuffer, EGLConfig, const EGLint*);
EGLBoolean eglReleaseThread();
EGLBoolean eglWaitClient();
EGLContext eglGetCurrentContext();
EGLSync eglCreateSync(EGLDisplay, EGLenum, const EGLAttrib*);
EGLBoolean eglDestroySync(EGLDisplay, EGLSync);
EGLint eglClientWaitSync(EGLDisplay, EGLSync, EGLint, EGLTime);
EGLBoolean eglGetSyncAttrib(EGLDisplay, EGLSync, EGLint, EGLAttrib*);
EGLImage eglCreateImage(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*);
EGLBoolean eglDestroyImage(EGLDisplay, EGLImage);
EGLDisplay eglGetPlatformDisplay(EGLenum, void*, const EGLAttrib*);
EGLSurface eglCreatePlatformWindowSurface(EGLDisplay, EGLConfig, void*, const EGLAttrib*);
EGLSurface eglCreatePlatformPixmapSurface(EGLDisplay, EGLConfig, void*, const EGLAttrib*);
EGLBoolean eglWaitSync(EGLDisplay, EGLSync, EGLint);

#ifdef __cplusplus
}
#endif

#endif
