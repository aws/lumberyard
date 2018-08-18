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

// Description : Implementation of the NULL renderer API


#include "StdAfx.h"
#include "NULL_Renderer.h"
#include <IColorGradingController.h>
#include "IStereoRenderer.h"
#include "../Common/Textures/TextureManager.h"

#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>
// init memory pool usage

#include "GraphicsPipeline/FurBendData.h"

// Included only once per DLL module.
#include <platform_impl.h>

CCryNameTSCRC CTexture::s_sClassName = CCryNameTSCRC("CTexture");
CCryNameTSCRC CHWShader::s_sClassNameVS = CCryNameTSCRC("CHWShader_VS");
CCryNameTSCRC CHWShader::s_sClassNamePS = CCryNameTSCRC("CHWShader_PS");
CCryNameTSCRC CShader::s_sClassName = CCryNameTSCRC("CShader");

CNULLRenderer* gcpNULL = NULL;

//////////////////////////////////////////////////////////////////////

class CNullColorGradingController
    : public IColorGradingController
{
public:
    virtual int LoadColorChart(const char* pChartFilePath) const { return 0; }
    virtual int LoadDefaultColorChart() const { return 0; }
    virtual void UnloadColorChart(int texID) const {}
    virtual void SetLayers(const SColorChartLayer* pLayers, uint32 numLayers) {}
};

//////////////////////////////////////////////////////////////////////

class CNullStereoRenderer
    : public IStereoRenderer
{
public:
    virtual EStereoDevice GetDevice() { return STEREO_DEVICE_NONE; }
    virtual EStereoDeviceState GetDeviceState() { return STEREO_DEVSTATE_UNSUPPORTED_DEVICE; }
    virtual void GetInfo(EStereoDevice* device, EStereoMode* mode, EStereoOutput* output, EStereoDeviceState* state) const
    {
        if (device)
        {
            *device = STEREO_DEVICE_NONE;
        }
        if (mode)
        {
            *mode = STEREO_MODE_NO_STEREO;
        }
        if (output)
        {
            *output = STEREO_OUTPUT_STANDARD;
        }
        if (state)
        {
            *state = STEREO_DEVSTATE_OK;
        }
    }
    virtual bool GetStereoEnabled() { return false; }
    virtual float GetStereoStrength() { return 0; }
    virtual float GetMaxSeparationScene(bool half = true) { return 0; }
    virtual float GetZeroParallaxPlaneDist() { return 0; }
    virtual void GetNVControlValues(bool& stereoEnabled, float& stereoStrength) {};
    virtual void OnHmdDeviceChanged() {}
    virtual bool IsRenderingToHMD() override { return false; }
    Status GetStatus() const override { return IStereoRenderer::Status::kIdle; }
};

//////////////////////////////////////////////////////////////////////
CNULLRenderer::CNULLRenderer()
{
    gcpNULL = this;
    m_pNULLRenderAuxGeom = CNULLRenderAuxGeom::Create(*this);
    m_pNULLColorGradingController = new CNullColorGradingController();
    m_pNULLStereoRenderer = new CNullStereoRenderer();
    m_pixelAspectRatio = 1.0f;
}

//////////////////////////////////////////////////////////////////////////
bool QueryIsFullscreen()
{
    return false;
}


#include <stdio.h>
//////////////////////////////////////////////////////////////////////
CNULLRenderer::~CNULLRenderer()
{
    ShutDown();
    delete m_pNULLRenderAuxGeom;
    delete m_pNULLColorGradingController;
    delete m_pNULLStereoRenderer;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::EnableTMU(bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::CheckError(const char* comment)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::BeginFrame()
{
    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_nFrameUpdateID++;
    m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();

    m_pNULLRenderAuxGeom->BeginFrame();
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ChangeDisplay(unsigned int width, unsigned int height, unsigned int bpp)
{
    return false;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport)
{
}

void CNULLRenderer::RenderDebug(bool bRenderStats)
{
}

void CNULLRenderer::EndFrame()
{
    //m_pNULLRenderAuxGeom->Flush(true);
    m_pNULLRenderAuxGeom->EndFrame();
    m_pRT->RC_EndFrame(!m_bStartLevelLoading);
}

void CNULLRenderer::TryFlush()
{
}

void CNULLRenderer::GetMemoryUsage(ICrySizer* Sizer)
{
}

WIN_HWND CNULLRenderer::GetHWND()
{
#if defined(WIN32)
    return GetDesktopWindow();
#else
    return NULL;
#endif
}

bool CNULLRenderer::SetWindowIcon(const char* path)
{
    return false;
}

void TexBlurAnisotropicVertical(CTexture* pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//IMAGES DRAWING
////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Draw2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Push2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float angle, float r, float g, float b, float a, float z, float stereoDepth)
{
}

void CNULLRenderer::Draw2dImageList()
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered)
{
}

void CNULLRenderer::DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], float r, float g, float b, float a, bool filtered)
{
}

///////////////////////////////////////////
void CNULLRenderer::DrawBuffer(CVertexBuffer* pVBuf, CIndexBuffer* pIBuf, int nNumIndices, int nOffsIndex, const PublicRenderPrimitiveType nPrmode, int nVertStart, int nVertStop)
{
}

void CNULLRenderer::DrawPrimitivesInternal(CVertexBuffer* src, int vert_num, const eRenderPrimitiveType prim_type)
{
}

void CRenderMesh::DrawImmediately()
{
}

///////////////////////////////////////////
void CNULLRenderer::SetCullMode(int mode)
{
}

///////////////////////////////////////////
bool CNULLRenderer::EnableFog(bool enable)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC EXTENSIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::EnableVSync(bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::SelectTMU(int tnum)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::PushMatrix()
{
}

///////////////////////////////////////////
void CNULLRenderer::RotateMatrix(float a, float x, float y, float z)
{
}

void CNULLRenderer::RotateMatrix(const Vec3& angles)
{
}

///////////////////////////////////////////
void CNULLRenderer::TranslateMatrix(float x, float y, float z)
{
}

void CNULLRenderer::MultMatrix(const float* mat)
{
}

void CNULLRenderer::TranslateMatrix(const Vec3& pos)
{
}

///////////////////////////////////////////
void CNULLRenderer::ScaleMatrix(float x, float y, float z)
{
}

///////////////////////////////////////////
void CNULLRenderer::PopMatrix()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void CNULLRenderer::LoadMatrix(const Matrix34* src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::PushWireframeMode(int mode){}
void CNULLRenderer::PopWireframeMode(){}
void CNULLRenderer::FX_PushWireframeMode(int mode){}
void CNULLRenderer::FX_PopWireframeMode(){}
void CNULLRenderer::FX_SetWireframeMode(int mode){}

///////////////////////////////////////////
void CNULLRenderer::SetCamera(const CCamera& cam)
{
    int nThreadID = m_pRT->GetThreadList();
    m_RP.m_TI[nThreadID].m_cam = cam;
}

void CNULLRenderer::GetViewport(int* x, int* y, int* width, int* height) const
{
    *x = 0;
    *y = 0;
    *width = m_width;
    *height = m_height;
}

void CNULLRenderer::SetViewport(int x, int y, int width, int height, int id)
{
}

void CNULLRenderer::SetScissor(int x, int y, int width, int height)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetModelViewMatrix(float* mat)
{
    memcpy(mat, &m_IdentityMatrix, sizeof(m_IdentityMatrix));
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetProjectionMatrix(float* mat)
{
    memcpy(mat, &m_IdentityMatrix, sizeof(m_IdentityMatrix));
}

//////////////////////////////////////////////////////////////////////
ITexture* CNULLRenderer::EF_LoadTexture(const char * nameTex, const uint32 flags)
{
    return CTextureManager::Instance()->GetNoTexture();
}

//////////////////////////////////////////////////////////////////////
ITexture* CNULLRenderer::EF_LoadDefaultTexture(const char * nameTex)
{
    return CTextureManager::Instance()->GetDefaultTexture(nameTex);
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawQuad(const Vec3& right, const Vec3& up, const Vec3& origin, int nFlipmode /*=0*/)
{
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy, float* sz)
{
    return false;
}

int CNULLRenderer::UnProject(float sx, float sy, float sz,
    float* px, float* py, float* pz,
    const float modelMatrix[16],
    const float projMatrix[16],
    const int    viewport[4])
{
    return 0;
}

//////////////////////////////////////////////////////////////////////
int CNULLRenderer::UnProjectFromScreen(float  sx, float  sy, float  sz,
    float* px, float* py, float* pz)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ScreenShot(const char* filename, int width)
{
    return true;
}

int CNULLRenderer::ScreenToTexture(int nTexID)
{
    return 0;
}

void CNULLRenderer::ResetToDefault()
{
}

///////////////////////////////////////////
void CNULLRenderer::SetMaterialColor(float r, float g, float b, float a)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ClearTargetsImmediately(uint32 nFlags) {}
void CNULLRenderer::ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth) {}
void CNULLRenderer::ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors) {}
void CNULLRenderer::ClearTargetsImmediately(uint32 nFlags, float fDepth) {}

void CNULLRenderer::ClearTargetsLater(uint32 nFlags) {}
void CNULLRenderer::ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth) {}
void CNULLRenderer::ClearTargetsLater(uint32 nFlags, const ColorF& Colors) {}
void CNULLRenderer::ClearTargetsLater(uint32 nFlags, float fDepth) {}

void CNULLRenderer::ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY)
{
}

void CNULLRenderer::ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight, bool BGRA)
{
}

bool CNULLRenderer::CaptureFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight)
{
    return false;
}
bool CNULLRenderer::CopyFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight)
{
    return false;
}

bool CNULLRenderer::InitCaptureFrameBufferFast(uint32 bufferWidth, uint32 bufferHeight)
{
    return(false);
}

void CNULLRenderer::CloseCaptureFrameBufferFast(void)
{
}

bool CNULLRenderer::RegisterCaptureFrame(ICaptureFrameListener* pCapture)
{
    return(false);
}
bool CNULLRenderer::UnRegisterCaptureFrame(ICaptureFrameListener* pCapture)
{
    return(false);
}

void CNULLRenderer::CaptureFrameBufferCallBack(void)
{
}


void CNULLRenderer::SetFogColor(const ColorF& color)
{
}

void CNULLRenderer::DrawQuad(float dy, float dx, float dz, float x, float y, float z)
{
}

//////////////////////////////////////////////////////////////////////

int CNULLRenderer::CreateRenderTarget(const char* name, int nWidth, int nHeight, const ColorF& cClear, ETEX_Format eTF)
{
    return 0;
}

bool CNULLRenderer::DestroyRenderTarget(int nHandle)
{
    return true;
}

bool CNULLRenderer::SetRenderTarget(int nHandle, SDepthTexture* pDepthSurf)
{
    return true;
}

SDepthTexture* CNULLRenderer::CreateDepthSurface(int nWidth, int nHeight)
{
    return nullptr;
}

void CNULLRenderer::DestroyDepthSurface(SDepthTexture* pDepthSurf)
{
}

void CNULLRenderer::WaitForParticleBuffer(threadID nThreadId)
{
}

int CNULLRenderer::GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffe)
{
    return 0;
}

IColorGradingController* CNULLRenderer::GetIColorGradingController()
{
    return m_pNULLColorGradingController;
}

IStereoRenderer* CNULLRenderer::GetIStereoRenderer()
{
    return m_pNULLStereoRenderer;
}

//=========================================================================================


ILog* iLog;
IConsole* iConsole;
ITimer* iTimer;
ISystem* iSystem;

extern "C" DLL_EXPORT IRenderer * CreateCryRenderInterface(ISystem * pSystem)
{
    ModuleInitISystem(pSystem, "CryRenderer");

    gbRgb = false;

    iConsole    = gEnv->pConsole;
    iLog            = gEnv->pLog;
    iTimer      = gEnv->pTimer;
    iSystem     = gEnv->pSystem;

    CRenderer* rd = new CNULLRenderer();
    if (rd)
    {
        rd->InitRenderer();
    }

#ifdef LINUX
    srand(clock());
#else
    srand(GetTickCount());
#endif

    return rd;
}

class CEngineModule_CryRenderer
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryRenderer, "EngineModule_CryRenderer", 0x540c91a7338e41d3, 0xaceeac9d55614450)

    virtual const char* GetName() const {
        return "CryRenderer";
    }
    virtual const char* GetCategory() const {return "CryEngine"; }

    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;
        env.pRenderer = CreateCryRenderInterface(pSystem);
        return env.pRenderer != 0;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryRenderer)

CEngineModule_CryRenderer::CEngineModule_CryRenderer()
{
};

CEngineModule_CryRenderer::~CEngineModule_CryRenderer()
{
};

void COcclusionQuery::Create()
{
}

void COcclusionQuery::Release()
{
}

void COcclusionQuery::BeginQuery()
{
}

void COcclusionQuery::EndQuery()
{
}

uint32 COcclusionQuery::GetVisibleSamples(bool bAsynchronous)
{
    return 0;
}

/*static*/ FurBendData& FurBendData::Get()
{
    static FurBendData s_instance;
    return s_instance;
}

void FurBendData::InsertNewElements()
{
}

void FurBendData::FreeData()
{
}

void FurBendData::OnBeginFrame()
{
}

TArray<SRenderLight>* CRenderer::EF_GetDeferredLights(const SRenderingPassInfo& passInfo, const eDeferredLightType eLightType)
{
    static TArray<SRenderLight> lights;
    return &lights;
}

SRenderLight* CRenderer::EF_GetDeferredLightByID(const uint16 nLightID, const eDeferredLightType eLightType)
{
    return nullptr;
}


void CRenderer::BeginSpawningGeneratingRendItemJobs(int nThreadID)
{
}

void CRenderer::BeginSpawningShadowGeneratingRendItemJobs(int nThreadID)
{
}

void CRenderer::EndSpawningGeneratingRendItemJobs(int nThreadID)
{
}

void CNULLRenderer::PrecacheResources()
{
}

bool CNULLRenderer::EF_PrecacheResource(SShaderItem* pSI, float fMipFactorSI, float fTimeToReady, int Flags, int nUpdateId, int nCounter)
{
    return true;
}

ITexture* CNULLRenderer::EF_CreateCompositeTexture(int type, const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, ETEX_Format eTF, const STexComposition* pCompositions, size_t nCompositions, int8 nPriority)
{
    return CTextureManager::Instance()->GetNoTexture();
}

void CNULLRenderer::FX_ClearTarget(ITexture* pTex)
{
}

void CNULLRenderer::FX_ClearTarget(SDepthTexture* pTex)
{
}

bool CNULLRenderer::FX_SetRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount)
{
    return true;
}

bool CNULLRenderer::FX_PushRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount)
{
    return true;
}

bool CNULLRenderer::FX_SetRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, bool bPush, int nCMSide, bool bScreenVP, uint32 nTileCount)
{
    return true;
}

bool CNULLRenderer::FX_PushRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, int nCMSide, bool bScreenVP, uint32 nTileCount)
{
    return true;
}
bool CNULLRenderer::FX_RestoreRenderTarget(int nTarget)
{
    return true;
}
bool CNULLRenderer::FX_PopRenderTarget(int nTarget)
{
    return true;
}

IDynTexture* CNULLRenderer::CreateDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool)
{
    return nullptr;
}