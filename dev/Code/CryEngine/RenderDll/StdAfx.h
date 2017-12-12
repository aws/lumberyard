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

#ifndef CRYINCLUDE_STDAFX_H
#define CRYINCLUDE_STDAFX_H

#if (_MSC_VER > 1000) && !defined(_ORBIS)
#pragma once
#endif // _MSC_VER > 1000

//on mac the precompiled header is auto included in every .c and .cpp file, no include line necessary.
//.c files don't like the cpp things in here
#ifdef __cplusplus

#ifndef _WINSOCKAPI_
#   define _WINSOCKAPI_
#   define _DID_SKIP_WINSOCK_
#endif

#include <vector>
#include <array>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <memory>

#include <platform.h>
#include <CrySizer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>

#define SUPPORTS_WINDOWS_10_SDK false
#if _MSC_VER && !defined(_XBOX_VER)
    #include <ntverp.h>
    #undef SUPPORTS_WINDOWS_10_SDK
    #define SUPPORTS_WINDOWS_10_SDK (VER_PRODUCTBUILD > 9600)
#endif

#if defined(WIN64) && defined (CRY_USE_DX12)
#define CRY_INTEGRATE_DX12
#endif

#ifdef _DEBUG
#define GFX_DEBUG
#endif
//defined in DX9, but not DX10
#if defined(DURANGO) || defined(OPENGL)
    #define D3D_OK  S_OK
#endif

#if !defined(_RELEASE)
# define RENDERER_ENABLE_BREAK_ON_ERROR 0
#endif
#if !defined(RENDERER_ENABLE_BREAK_ON_ERROR)
# define RENDERER_ENABLE_BREAK_ON_ERROR 0
#endif
#if RENDERER_ENABLE_BREAK_ON_ERROR
# include <winerror.h>
namespace detail
{
    const char* ToString(long const hr);
    bool CheckHResult(long const hr, bool breakOnError, const char* file, const int line);
}
//# undef FAILED
//# define FAILED(x) (detail::CheckHResult((x), false, __FILE__, __LINE__))
# define CHECK_HRESULT(x) (detail::CheckHResult((x), true, __FILE__, __LINE__))
#else
# define CHECK_HRESULT(x) (!FAILED(x))
#endif

#if defined(OPENGL) && (defined(DEBUG) || defined(_DEBUG))
    #define LY_ENABLE_OPENGL_ERROR_CHECKING
#endif
namespace Lumberyard
{
    namespace OpenGL
    {
        #if defined(LY_ENABLE_OPENGL_ERROR_CHECKING)
        unsigned int CheckError();
        void ClearErrors();
        #else
        static inline unsigned int CheckError() { return 0; }
        static inline void ClearErrors() {}
        #endif
    }
}

// enable support for D3D11.1 features if the platform supports it
#if defined(DURANGO) || defined(ORBIS) || defined(CRY_USE_DX12)
#   define DEVICE_SUPPORTS_D3D11_1
#endif



#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#endif //_DEBUG

#undef USE_STATIC_NAME_TABLE
#define USE_STATIC_NAME_TABLE

#if !defined(_RELEASE)
#define ENABLE_FRAME_PROFILER
#endif

#if !defined(NULL_RENDERER) && (!defined(_RELEASE) || defined(PERFORMANCE_BUILD))
#define ENABLE_SIMPLE_GPU_TIMERS
#define ENABLE_FRAME_PROFILER_LABELS
#endif

#ifdef ENABLE_FRAME_PROFILER
#   define PROFILE 1
#endif


#ifdef ENABLE_SCUE_VALIDATION
enum EVerifyType
{
    eVerify_Normal,
    eVerify_ConstantBuffer,
    eVerify_VertexBuffer,
    eVerify_SRVTexture,
    eVerify_SRVBuffer,
    eVerify_UAVTexture,
    eVerify_UAVBuffer,
};
#endif

#include <CryModuleDefs.h>

#undef eCryModule
#define eCryModule eCryM_Render

#if __HAS_SSE__
// <fvec.h> includes <assert.h>, include it before platform.h
#include <fvec.h>
#define CONST_INT32_PS(N, V3, V2, V1, V0)                                 \
    const _MM_ALIGN16 int _##N[] = { V0, V1, V2, V3 }; /*little endian!*/ \
    const F32vec4 N = _mm_load_ps((float*)_##N);
#endif

// enable support for baked meshes and decals on PC
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE) || defined(DURANGO)
//#define RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT // CryTek removed this #define in 3.8.  Not sure if we should strip out all related code or not, or if it may be a useful feature.
#define TEXTURE_GET_SYSTEM_COPY_SUPPORT
#endif

#if defined(_CPU_SSE) && !defined(WIN64)
#define __HAS_SSE__ 1
#endif

#define MAX_REND_RECURSION_LEVELS 2

#if defined(OPENGL)
#define CRY_OPENGL_ADAPT_CLIP_SPACE 1
#define CRY_OPENGL_FLIP_Y 1
#define CRY_OPENGL_MODIFY_PROJECTIONS !CRY_OPENGL_ADAPT_CLIP_SPACE
#endif // defined(OPENGL)

#ifdef STRIP_RENDER_THREAD
    #define m_nCurThreadFill 0
    #define m_nCurThreadProcess 0
#endif

#ifdef STRIP_RENDER_THREAD
    #define ASSERT_IS_RENDER_THREAD(rt)
    #define ASSERT_IS_MAIN_THREAD(rt)
#else
    #define ASSERT_IS_RENDER_THREAD(rt) assert((rt)->IsRenderThread());
    #define ASSERT_IS_MAIN_THREAD(rt) assert((rt)->IsMainThread());
    #define ASSERT_IS_MAIN_OR_RENDER_THREAD(rt) assert((rt)->IsMainThread() || (rt)->IsRenderThread());
#endif

//#define ASSERT_IN_SHADER( expr ) assert( expr );
#define ASSERT_IN_SHADER(expr)

// TODO: linux clang doesn't behave like orbis clang and linux gcc doesn't behave like darwin gcc, all linux instanced can't manage squish-template overloading
#if defined (NULL_RENDERER) || defined(LINUX) || defined(ORBIS) || defined(IOS) || defined(APPLETV)|| defined(CRY_USE_METAL)
#define EXCLUDE_SQUISH_SDK
#endif

#define _USE_MATH_DEFINES
#include <math.h>

// nv API
#if (defined(WIN32) || defined(WIN64)) && !defined(NULL_RENDERER) && !defined(OPENGL) && !defined(CRY_USE_DX12)
#define USE_NV_API
#endif

// windows desktop API available for usage
#if defined(WIN32) || defined(WIN64)
#define WINDOWS_DESKTOP_API
#endif

#if (defined(WIN32) || defined(WIN64)) && !defined(OPENGL)
#define LEGACY_D3D9_INCLUDE
#endif


#if !defined(NULL_RENDERER)

// all D3D10 blob related functions and struct will be deprecated in next DirectX APIs
// and replaced with regular D3DBlob counterparts
#define D3D10CreateBlob                                      D3DCreateBlob

// Direct3D11 includes
#if defined(OPENGL)
    #include "CryLibrary.h"
//  Confetti BEGIN: Igor Lobanchikov
//  Igor: enable metal for ios device only. Emulator is not supported for now.
    #if TARGET_OS_IPHONE || TARGET_OS_TV
        #ifndef __IPHONE_9_0
            #define __IPHONE_9_0    90000
        #endif  //  __IPHONE_9_0
    #endif

    #if defined(CRY_USE_METAL)
        #include "XRenderD3D9/DXMETAL/CryDXMETAL.hpp"
    #else
        #include "XRenderD3D9/DXGL/CryDXGL.hpp"
    #endif
//  Confetti END: Igor Lobanchikov
#elif defined(CRY_USE_DX12)
    #include "CryLibrary.h"
typedef uintptr_t SOCKET;
#else

#if   defined(WIN32) || defined(WIN64)
    #include "d3d11.h"
#endif

#if defined(LEGACY_D3D9_INCLUDE)
    #include "d3d9.h"
#endif

#endif
#else //defined(NULL_RENDERER)
    #if defined(WIN32)
        #include "windows.h"
    #endif
    #if (defined(LINUX) || defined(APPLE)) && !defined(DXGI_FORMAT_DEFINED)
        #include <AzDXGIFormat.h>
    #endif
#endif

#ifdef _DID_SKIP_WINSOCK_
#   undef _WINSOCKAPI_
#   undef _DID_SKIP_WINSOCK_
#endif


#if !defined(_RELEASE) || defined(ENABLE_STATOSCOPE_RELEASE)
#define ENABLE_TEXTURE_STREAM_LISTENER
#endif

///////////////////////////////////////////////////////////////////////////////
/* BUFFER ACCESS
 * Confetti Note -- David:
 * Buffer access related defines have been moved down as some preproc. if-branches rely on CRY_USE_METAL.
 */
///////////////////////////////////////////////////////////////////////////////
// BUFFER_ENABLE_DIRECT_ACCESS
// stores pointers to actual backing storage of vertex buffers. Can only be used on architectures
// that have a unified memory architecture and further guarantee that buffer storage does not change
// on repeated accesses.
#if defined(ORBIS) || defined(DURANGO) || defined(CRY_USE_DX12)
# define BUFFER_ENABLE_DIRECT_ACCESS 1
#endif

// BUFFER_USE_STAGED_UPDATES
// On platforms that support staging buffers, special buffers are allocated that act as a staging area
// for updating buffer contents on the fly.

// Confetti Igor: when staged updates are disabled CPU will have direct access to the pool's buffers' content
//  and update data directly. This cuts memory consumption and reduces the number of copies.
//  GPU won't be used to update buffer content but it will be used to performa defragmentation.
#if !(defined(ORBIS) || defined(DURANGO) || defined(CRY_USE_METAL))
# define BUFFER_USE_STAGED_UPDATES 1
#else
# define BUFFER_USE_STAGED_UPDATES 0
#endif

// BUFFER_SUPPORT_TRANSIENT_POOLS
// On d3d11 we want to separate the fire-and-forget allocations from the longer lived dynamic ones
#if !defined(NULL_RENDERER) && ((!defined(CONSOLE) && !defined(CRY_USE_DX12)) || defined(CRY_USE_METAL))
# define BUFFER_SUPPORT_TRANSIENT_POOLS 1
#else
# define BUFFER_SUPPORT_TRANSIENT_POOLS 0
#endif

#ifndef BUFFER_ENABLE_DIRECT_ACCESS
# define BUFFER_ENABLE_DIRECT_ACCESS 0
#endif

// CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS
// Enable if we have direct access to video memory and the device manager
// should manage constant buffers
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
#   if defined(DURANGO) || defined (CRY_USE_DX12)
#       define CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 1
#   else
#       define CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 0
#   endif
#else
#   define CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS 0
#endif

#if BUFFER_ENABLE_DIRECT_ACCESS && (defined(WIN32) || defined(WIN64)) && !defined(CRY_USE_DX12)
# error BUFFER_ENABLE_DIRECT_ACCESS is not supported on windows platforms
#endif

#if defined(WIN32) || defined(WIN64) || defined(ORBIS) || defined(LINUX) || defined(APPLE)
# define FEATURE_SILHOUETTE_POM
#endif


#if !defined(_RELEASE) && !defined(NULL_RENDERER) && (defined(WIN32) || defined(WIN64) || defined(DURANGO)) && !defined(OPENGL)
#   define SUPPORT_D3D_DEBUG_RUNTIME
#endif

#if !defined(NULL_RENDERER) && !defined(DURANGO) && !defined(ORBIS)
#   define SUPPORT_DEVICE_INFO
#   if defined(WIN32) || defined(WIN64)
#       define SUPPORT_DEVICE_INFO_MSG_PROCESSING
#       define SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES
#   endif
#endif

#include <I3DEngine.h>
#include <IGame.h>

#if !defined(NULL_RENDERER)
#   if defined(CRY_USE_DX12)
#include "XRenderD3D9/DX12/CryDX12.hpp"
#   elif defined(DEVICE_SUPPORTS_D3D11_1) && !defined(ORBIS)
typedef IDXGIFactory1           DXGIFactory;
typedef IDXGIDevice1            DXGIDevice;
typedef IDXGIAdapter1           DXGIAdapter;
typedef IDXGIOutput             DXGIOutput;
typedef IDXGISwapChain          DXGISwapChain;
typedef ID3D11DeviceContextX    D3DDeviceContext;
typedef ID3D11DeviceX           D3DDevice;
#   else
typedef IDXGIFactory1           DXGIFactory;
typedef IDXGIDevice1            DXGIDevice;
typedef IDXGIAdapter1           DXGIAdapter;
typedef IDXGIOutput             DXGIOutput;
typedef IDXGISwapChain          DXGISwapChain;
typedef ID3D11DeviceContext     D3DDeviceContext;
typedef ID3D11Device            D3DDevice;
#   endif

typedef ID3D11InputLayout       D3DVertexDeclaration;
typedef ID3D11VertexShader      D3DVertexShader;
typedef ID3D11PixelShader       D3DPixelShader;
typedef ID3D11Resource          D3DResource;

typedef ID3D11Resource          D3DBaseTexture;

typedef ID3D11Texture2D         D3DTexture;
typedef ID3D11Texture3D         D3DVolumeTexture;
typedef ID3D11Texture2D         D3DCubeTexture;
typedef ID3D11Buffer            D3DBuffer;
typedef ID3D11ShaderResourceView    D3DShaderResourceView;
typedef ID3D11UnorderedAccessView   D3DUnorderedAccessView;
typedef ID3D11RenderTargetView  D3DSurface;
typedef ID3D11DepthStencilView  D3DDepthSurface;
typedef ID3D11View              D3DBaseView;
typedef ID3D11Query             D3DQuery;
typedef D3D11_VIEWPORT          D3DViewPort;
typedef D3D11_RECT              D3DRectangle;
typedef DXGI_FORMAT             D3DFormat;
typedef D3D11_PRIMITIVE_TOPOLOGY    D3DPrimitiveType;
typedef ID3D10Blob              D3DBlob;
typedef ID3D11SamplerState      D3DSamplerState;

#if !defined(USE_D3DX)

// this should be moved into seperate D3DX defintion file which should still be used
// for console builds, until everything in the engine has been cleaned up which still
// references this

#if defined(APPLE)
//interface is also a objective c keyword 
typedef struct ID3DXConstTable       ID3DXConstTable;
typedef struct ID3DXConstTable*  LPD3DXCONSTANTTABLE;
#else
typedef interface ID3DXConstTable       ID3DXConstTable;
typedef interface ID3DXConstTable*  LPD3DXCONSTANTTABLE;
#endif




#if defined(DURANGO) || defined(APPLE) || defined(CRY_USE_DX12)
// D3DPOOL define still used as function parameters, so defined to backwards compatible with D3D9
typedef enum _D3DPOOL
{
    D3DPOOL_DEFAULT                       = 0,
    D3DPOOL_MANAGED                       = 1,
    D3DPOOL_SYSTEMMEM                     = 2,
    D3DPOOL_SCRATCH                           = 3,
    D3DPOOL_FORCE_DWORD = 0x7fffffff
} D3DPOOL;
#endif


#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                                            \
    ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | \
     ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24))
#endif // defined(MAKEFOURCC)

#endif // !defined(WIN32) && !defined(WIN64)

const int32 g_nD3D10MaxSupportedSubres = (6 * 15);
//////////////////////////////////////////////////////////////////////////
#else
typedef void D3DTexture;
typedef void D3DSurface;
typedef void D3DShaderResourceView;
typedef void D3DUnorderedAccessView;
typedef void D3DDepthSurface;
typedef void D3DSamplerState;
typedef int D3DFormat;
typedef void D3DBuffer;
#endif // NULL_RENDERER
//////////////////////////////////////////////////////////////////////////

#define USAGE_WRITEONLY 8

//////////////////////////////////////////////////////////////////////////
// Linux specific defines for Renderer.
//////////////////////////////////////////////////////////////////////////

#if defined(_AMD64_) && !defined(LINUX)
#include <io.h>
#endif

#define SIZEOF_ARRAY(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef DEBUGALLOC

#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK

// memman
#define   calloc(s, t)       _calloc_dbg(s, t, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)

#endif

#include <CryName.h>
#include "Common/CryNameR.h"

#define MAX_TMU 32
#define MAX_STREAMS 16

//! Include main interfaces.
#include <ICryPak.h>
#include <IProcess.h>
#include <ITimer.h>
#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <IRenderer.h>
#include <IStreamEngine.h>
#include <CrySizer.h>
#include <smartptr.h>
#include <CryArray.h>
#include <PoolAllocator.h>

#include <CryArray.h>

enum eRenderPrimitiveType
{
#if defined(NULL_RENDERER)
    eptUnknown = -1,
    eptTriangleList,
    eptTriangleStrip,
    eptLineList,
    eptLineStrip,
    eptPointList,
#else
    eptUnknown = -1,
    eptTriangleList = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    eptTriangleStrip = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    eptLineList = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
    eptLineStrip = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
    eptPointList = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
    ept1ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
    ept2ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,
    ept3ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
    ept4ControlPointPatchList = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,
#endif

    // non-real primitives, used for logical batching
    eptHWSkinGroups = 0x3f
};

inline eRenderPrimitiveType GetInternalPrimitiveType(PublicRenderPrimitiveType t)
{
    switch (t)
    {
    case prtTriangleList:
    default:
        return eptTriangleList;
    case prtTriangleStrip:
        return eptTriangleStrip;
    case prtLineList:
        return eptLineList;
    case prtLineStrip:
        return eptLineStrip;
    }
}

#if !defined(NULL_RENDERER) && (defined(WIN32) || defined(WIN64) || defined(ORBIS) || defined(DURANGO) || defined(LINUX) || defined(APPLE))
#   define SUPPORT_FLEXIBLE_INDEXBUFFER // supports 16 as well as 32 bit indices AND index buffer bind offset
#endif

enum RenderIndexType
{
#if defined(SUPPORT_FLEXIBLE_INDEXBUFFER)
    Index16 = DXGI_FORMAT_R16_UINT,
    Index32 = DXGI_FORMAT_R32_UINT
#else
    Index16,
    Index32
#endif
};

// Interfaces from the Game
extern ILog* iLog;
extern IConsole* iConsole;
extern ITimer* iTimer;
extern ISystem* iSystem;

template <class Container>
unsigned sizeOfVP (Container& arr)
{
    int i;
    unsigned size = 0;
    for (i = 0; i < (int)arr.size(); i++)
    {
        typename Container::value_type& T = arr[i];
        size += T->Size();
    }
    size += (arr.capacity() - arr.size()) * sizeof(typename Container::value_type);
    return size;
}

template <class Container>
unsigned sizeOfV (Container& arr)
{
    int i;
    unsigned size = 0;
    for (i = 0; i < (int)arr.size(); i++)
    {
        typename Container::value_type& T = arr[i];
        size += T.Size();
    }
    size += (arr.capacity() - arr.size()) * sizeof(typename Container::value_type);
    return size;
}
template <class Container>
unsigned sizeOfA (Container& arr)
{
    int i;
    unsigned size = 0;
    for (i = 0; i < arr.size(); i++)
    {
        typename Container::value_type& T = arr[i];
        size += T.Size();
    }
    return size;
}
template <class Map>
unsigned sizeOfMap (Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += T.Size();
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}
template <class Map>
unsigned sizeOfMapStr (Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += T.capacity();
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}
template <class Map>
unsigned sizeOfMapP (Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += T->Size();
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}
template <class Map>
unsigned sizeOfMapS (Map& map)
{
    unsigned size = 0;
    for (typename Map::iterator it = map.begin(); it != map.end(); it++)
    {
        typename Map::mapped_type& T = it->second;
        size += sizeof(T);
    }
    size += map.size() * sizeof(stl::MapLikeStruct);
    return size;
}

#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
#   define VOLUMETRIC_FOG_SHADOWS
#endif

#if (defined(WIN32) || defined(WIN64)) && !(defined(OPENGL) && defined(RELEASE))
#if !defined(CRY_USE_DX12)
#   define ENABLE_NULL_D3D11DEVICE
#endif
#endif




// Enable to eliminate DevTextureDataSize calls during stream updates - costs 4 bytes per mip header
#define TEXSTRM_STORE_DEVSIZES

#ifndef TEXSTRM_BYTECENTRIC_MEMORY
#define TEXSTRM_TEXTURECENTRIC_MEMORY
#endif

#if !defined(CONSOLE) && !defined(NULL_RENDERER) && !defined(OPENGL) && !defined(CRY_INTEGRATE_DX12)
#define TEXSTRM_DEFERRED_UPLOAD
#endif

#if !defined(CONSOLE)
#define TEXSTRM_COMMIT_COOLDOWN
#endif



#if defined(_RELEASE)
#   define EXCLUDE_RARELY_USED_R_STATS
#endif

#include <Cry_Math.h>
#include <Cry_Geo.h>
//#include "_Malloc.h"
//#include "math.h"
#include <StlUtils.h>
#include "Common/DevBuffer.h"

#include "XRenderD3D9/DeviceManager/DeviceManager.h"

#include <VertexFormats.h>

#include "Common/CommonRender.h"
#include <IRenderAuxGeom.h>
#include "Common/Shaders/ShaderComponents.h"
#include "Common/Shaders/Shader.h"
//#include "Common/XFile/File.h"
//#include "Common/Image.h"
#include "Common/Shaders/CShader.h"
#include "Common/RenderMesh.h"
#include "Common/RenderPipeline.h"
#include "Common/RenderThread.h"

#include "Common/Renderer.h"

#include "Common/Textures/Texture.h"
#include "Common/Shaders/Parser.h"

#include "Common/FrameProfiler.h"
#include "Common/Shadow_Renderer.h"
#include "Common/DeferredRenderUtils.h"
#include "Common/ShadowUtils.h"
#include "Common/WaterUtils.h"

#include "Common/OcclQuery.h"

#include "Common/PostProcess/PostProcess.h"

// All handled render elements (except common ones included in "RendElement.h")
#include "Common/RendElements/CREBeam.h"
#include "Common/RendElements/CREClientPoly.h"
#include "Common/RendElements/CRELensOptics.h"
#include "Common/RendElements/CREHDRProcess.h"
#include "Common/RendElements/CRECloud.h"
#include "Common/RendElements/CREDeferredShading.h"
#include "Common/RendElements/CREMeshImpl.h"

#if !defined(NULL_RENDERER)
#include "../XRenderD3D9/DeviceManager/ConstantBufferCache.h"
#include "../XRenderD3D9/DeviceManager/DeviceWrapper12.h"
#endif

/*-----------------------------------------------------------------------------
    Vector transformations.
-----------------------------------------------------------------------------*/

_inline void TransformVector(Vec3& out, const Vec3& in, const Matrix44A& m)
{
    out.x = in.x * m(0, 0) + in.y * m(1, 0) + in.z * m(2, 0);
    out.y = in.x * m(0, 1) + in.y * m(1, 1) + in.z * m(2, 1);
    out.z = in.x * m(0, 2) + in.y * m(1, 2) + in.z * m(2, 2);
}

_inline void TransformPosition(Vec3& out, const Vec3& in, const Matrix44A& m)
{
    TransformVector (out, in, m);
    out += m.GetRow(3);
}


inline Plane TransformPlaneByUsingAdjointT(const Matrix44A& M, const Matrix44A& TA, const Plane plSrc)
{
    Vec3 newNorm;
    TransformVector (newNorm, plSrc.n, TA);
    newNorm.Normalize();

    if (M.Determinant() < 0.f)
    {
        newNorm *= -1;
    }

    Plane plane;
    Vec3 p;
    TransformPosition (p, plSrc.n * plSrc.d, M);
    plane.Set(newNorm, p | newNorm);

    return plane;
}

inline Matrix44 TransposeAdjoint(const Matrix44A& M)
{
    Matrix44 ta;

    ta(0, 0) = M(1, 1) * M(2, 2) - M(2, 1) * M(1, 2);
    ta(1, 0) = M(2, 1) * M(0, 2) - M(0, 1) * M(2, 2);
    ta(2, 0) = M(0, 1) * M(1, 2) - M(1, 1) * M(0, 2);

    ta(0, 1) = M(1, 2) * M(2, 0) - M(2, 2) * M(1, 0);
    ta(1, 1) = M(2, 2) * M(0, 0) - M(0, 2) * M(2, 0);
    ta(2, 1) = M(0, 2) * M(1, 0) - M(1, 2) * M(0, 0);

    ta(0, 2) = M(1, 0) * M(2, 1) - M(2, 0) * M(1, 1);
    ta(1, 2) = M(2, 0) * M(0, 1) - M(0, 0) * M(2, 1);
    ta(2, 2) = M(0, 0) * M(1, 1) - M(1, 0) * M(0, 1);

    ta(0, 3) = 0.f;
    ta(1, 3) = 0.f;
    ta(2, 3) = 0.f;


    return ta;
}

inline Plane TransformPlane(const Matrix44A& M, const Plane& plSrc)
{
    Matrix44 tmpTA = TransposeAdjoint(M);
    return TransformPlaneByUsingAdjointT(M, tmpTA, plSrc);
}

// Homogeneous plane transform.
inline Plane TransformPlane2(const Matrix34A& m, const Plane& src)
{
    Plane plDst;

    float v0 = src.n.x, v1 = src.n.y, v2 = src.n.z, v3 = src.d;
    plDst.n.x = v0 * m(0, 0) + v1 * m(1, 0) + v2 * m(2, 0);
    plDst.n.y = v0 * m(0, 1) + v1 * m(1, 1) + v2 * m(2, 1);
    plDst.n.z = v0 * m(0, 2) + v1 * m(1, 2) + v2 * m(2, 2);

    plDst.d = v0 * m(0, 3) + v1 * m(1, 3) + v2 * m(2, 3) + v3;

    return plDst;
}

// Homogeneous plane transform.
inline Plane TransformPlane2(const Matrix44A& m, const Plane& src)
{
    Plane plDst;

    float v0 = src.n.x, v1 = src.n.y, v2 = src.n.z, v3 = src.d;
    plDst.n.x = v0 * m(0, 0) + v1 * m(0, 1) + v2 * m(0, 2) + v3 * m(0, 3);
    plDst.n.y = v0 * m(1, 0) + v1 * m(1, 1) + v2 * m(1, 2) + v3 * m(1, 3);
    plDst.n.z = v0 * m(2, 0) + v1 * m(2, 1) + v2 * m(2, 2) + v3 * m(2, 3);

    plDst.d = v0 * m(3, 0) + v1 * m(3, 1) + v2 * m(3, 2) + v3 * m(3, 3);

    return plDst;
}
inline Plane TransformPlane2_NoTrans(const Matrix44A& m, const Plane& src)
{
    Plane plDst;
    TransformVector(plDst.n, src.n, m);
    plDst.d = src.d;

    return plDst;
}

inline Plane TransformPlane2Transposed(const Matrix44A& m, const Plane& src)
{
    Plane plDst;

    float v0 = src.n.x, v1 = src.n.y, v2 = src.n.z, v3 = src.d;
    plDst.n.x = v0 * m(0, 0) + v1 * m(1, 0) + v2 * m(2, 0) + v3 * m(3, 0);
    plDst.n.y = v0 * m(0, 1) + v1 * m(1, 1) + v2 * m(2, 1) + v3 * m(3, 1);
    plDst.n.z = v0 * m(0, 2) + v1 * m(2, 1) + v2 * m(2, 2) + v3 * m(3, 2);

    plDst.d   = v0 * m(0, 3) + v1 * m(1, 3) + v2 * m(2, 3) + v3 * m(3, 3);

    return plDst;
}

//===============================================================================================

#define MAX_PATH_LENGTH 512

#if !defined(LINUX) && !defined(APPLE) && !defined(ORBIS)   //than it does already exist
inline int vsnprintf(char* buf, int size, const char* format, va_list& args)
{
    int res = _vsnprintf_s(buf, size, size, format, args);
    assert(res >= 0 && res < size); // just to know if there was problems in past
    buf[size - 1] = 0;
    return res;
}
#endif

#if !defined(LINUX) && !defined(APPLE) && !defined(ORBIS)
inline int snprintf(char* buf, int size, const char* format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    int res = _vsnprintf_s(buf, size, size, format, arglist);
    va_end(arglist);
    return res;
}
#endif

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void Warning(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void Warning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, 0, NULL, format, args);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void LogWarning(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void LogWarning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, 0, NULL, format, args);
    }
    va_end(args);
}

inline void LogWarningEngineOnly(const char* format, ...) PRINTF_PARAMS(1, 2);
inline void LogWarningEngineOnly(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, VALIDATOR_FLAG_IGNORE_IN_EDITOR, NULL, format, args);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void FileWarning(const char* filename, const char* format, ...) PRINTF_PARAMS(2, 3);
inline void FileWarning(const char* filename, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, filename, format, args);
    }
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
// Report warning to validator.
//////////////////////////////////////////////////////////////////////////
inline void TextureWarning(const char* filename, const char* format, ...) PRINTF_PARAMS(2, 3);
inline void TextureWarning(const char* filename, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, (VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE), filename, format, args);
    }
    va_end(args);
}

inline void TextureError(const char* filename, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    if (iSystem)
    {
        iSystem->WarningV(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, (VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE), filename, format, args);
    }
    va_end(args);
}

_inline void _SetVar(const char* szVarName, int nVal)
{
    ICVar* var = iConsole->GetCVar(szVarName);
    if (var)
    {
        var->Set(nVal);
    }
    else
    {
        assert(0);
    }
}

// Get the sub-string starting at the last . in the string, or NULL if the string contains no dot
// Note: The returned pointer refers to a location inside the provided string, no allocation is performed
const char* fpGetExtension (const char* in);

// Remove extension from string, including the .
// If the string has no extension, the whole string will be copied into the buffer
// Note: The out buffer must have space to store a copy of the in-string and a null-terminator
void fpStripExtension (const char* in, char* out, size_t bytes);
template<size_t bytes>
void fpStripExtension (const char* in, char (&out)[bytes]) { fpStripExtension(in, out, bytes); }

// Adds an extension to the path, if an extension is already present the function does nothing
// The extension should include the .
// Note: The path buffer must have enough unused space to store a copy of the extension string
void fpAddExtension (char* path, const char* extension, size_t bytes);
template<size_t bytes>
void fpAddExtension (char (&path)[bytes], const char* extension) { fpAddExtension(path, extension, bytes); }

// Converts DOS slashes to UNIX slashes
// Note: The dst buffer must have space to store a copy of src and a null-terminator
void fpConvertDOSToUnixName (char* dst, const char* src, size_t bytes);
template<size_t bytes>
void fpConvertDOSToUnixName (char (&dst)[bytes], const char* src) { fpConvertDOSToUnixName(dst, src, bytes); }

// Converts UNIX slashes to DOS slashes
// Note: the dst buffer must have space to store a copy of src and a null-terminator
void fpConvertUnixToDosName (char* dst, const char* src, size_t bytes);
template<size_t bytes>
void fpConvertUnixToDosName (char (&dst)[bytes], const char* src) { fpConvertUnixToDosName(dst, src, bytes); }

// Combines the path and name strings, inserting a UNIX slash as required, and stores the result into the dst buffer
// path may be NULL, in which case name will be copied into the dst buffer, and the UNIX slash is NOT inserted
// Note: the dst buffer must have space to store: a copy of name, a copy of path (if not null), a UNIX slash (if path doesn't end with one) and a null-terminator
void fpUsePath (const char* name, const char* path, char* dst, size_t bytes);
template<size_t bytes>
void fpUsePath (const char* name, const char* path, char (&dst)[bytes]) { fpUsePath(name, path, dst, bytes); }

//=========================================================================================
//
// Normal timing.
//
#define ticks(Timer)   {Timer -= CryGetTicks(); }
#define unticks(Timer) {Timer += CryGetTicks() + 34; }

//=============================================================================

// the int 3 call for 32-bit version for .l-generated files.
#ifdef WIN64
#define LEX_DBG_BREAK
#else
#define LEX_DBG_BREAK DEBUG_BREAK
#endif

#include "Common/Defs.h"

#define FUNCTION_PROFILER_RENDERER FUNCTION_PROFILER_FAST(iSystem, PROFILE_RENDERER, g_bProfilerEnabled)

#define SCOPED_RENDERER_ALLOCATION_NAME_HINT(str)

#define SHADER_ASYNC_COMPILATION

#if !defined(_RELEASE)
//# define DETAILED_PROFILING_MARKERS
#endif
#if defined(DETAILED_PROFILING_MARKERS)
# define DETAILED_PROFILE_MARKER(x) DETAILED_PROFILE_MARKER((x))
#else
# define DETAILED_PROFILE_MARKER(x) (void)0
#endif

#define RAIN_OCC_MAP_SIZE           256


#if defined(WIN32)

#include <float.h>

struct ScopedSetFloatExceptionMask
{
    ScopedSetFloatExceptionMask(unsigned int disable = _EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW | _EM_DENORMAL | _EM_INVALID)
    {
        _clearfp();
        _controlfp_s(&oldMask, 0, 0);
        unsigned temp;
        _controlfp_s(&temp, disable, _MCW_EM);
    }
    ~ScopedSetFloatExceptionMask()
    {
        _clearfp();
        unsigned temp;
        _controlfp_s(&temp, oldMask, _MCW_EM);
    }
    unsigned oldMask;
};

#define SCOPED_ENABLE_FLOAT_EXCEPTIONS ScopedSetFloatExceptionMask scopedSetFloatExceptionMask(0)
#define SCOPED_DISABLE_FLOAT_EXCEPTIONS ScopedSetFloatExceptionMask scopedSetFloatExceptionMask

#endif  // defined(_MSC_VER)
#include "XRenderD3D9/DeviceManager/DeviceManagerInline.h"
#endif //__cplusplus

/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/
#endif

