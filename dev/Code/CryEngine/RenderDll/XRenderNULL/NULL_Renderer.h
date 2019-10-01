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

#ifndef NULL_RENDERER_H
#define NULL_RENDERER_H

#if _MSC_VER > 1000
# pragma once
#endif

/*
===========================================
The NULLRenderer interface Class
===========================================
*/

#define MAX_TEXTURE_STAGES 4

#include "CryArray.h"
#include "NULLRenderAuxGeom.h"

//////////////////////////////////////////////////////////////////////
class CNULLRenderer
    : public CRenderer
{
public:

    ////---------------------------------------------------------------------------------------------------------------------
    virtual SRenderPipeline* GetRenderPipeline() override { return nullptr; }
    virtual SRenderThread* GetRenderThread() override { return nullptr; }
    virtual void FX_SetState(int st, int AlphaRef = -1, int RestoreState = 0) override;
    void SetCull(ECull eCull, bool bSkipMirrorCull = false) override {}
    virtual SDepthTexture* GetDepthBufferOrig() override { return nullptr; }
    virtual uint32 GetBackBufferWidth() override { return 0; }
    virtual uint32 GetBackBufferHeight() override { return 0; };
    virtual const SRenderTileInfo* GetRenderTileInfo() const override { return nullptr; }

    virtual void FX_CommitStates(const SShaderTechnique* pTech, const SShaderPass* pPass, bool bUseMaterialState) override {}
    virtual void FX_Commit(bool bAllowDIP = false) override {}
    virtual long FX_SetVertexDeclaration(int StreamMask, const AZ::Vertex::Format& vertexFormat) override { return 0; }
    virtual void FX_DrawIndexedPrimitive(const eRenderPrimitiveType eType, const int nVBOffset, const int nMinVertexIndex, const int nVerticesCount, const int nStartIndex, const int nNumIndices, bool bInstanced = false) override {}
    virtual SDepthTexture* FX_GetDepthSurface(int nWidth, int nHeight, bool bAA, bool shaderResourceView = false) override { return nullptr; }
    virtual long FX_SetIStream(const void* pB, uint32 nOffs, RenderIndexType idxType) override { return -1; }
    virtual long FX_SetVStream(int nID, const void* pB, uint32 nOffs, uint32 nStride, uint32 nFreq = 1) override { return -1; }
    virtual void FX_DrawPrimitive(const eRenderPrimitiveType eType, const int nStartVertex, const int nVerticesCount, const int nInstanceVertices = 0) {}
    virtual void DrawQuad3D(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const ColorF& color, float ftx0, float fty0, float ftx1, float fty1) override {}
    virtual void FX_ClearTarget(ITexture* pTex) override;
    virtual void FX_ClearTarget(SDepthTexture* pTex) override;

    virtual bool FX_SetRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) override;
    virtual bool FX_PushRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount = 1) override;
    virtual bool FX_SetRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, bool bPush = false, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) override;
    virtual bool FX_PushRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, int nCMSide = -1, bool bScreenVP = false, uint32 nTileCount = 1) override;
    virtual bool FX_RestoreRenderTarget(int nTarget) override;
    virtual bool FX_PopRenderTarget(int nTarget) override;
    virtual void EF_Scissor(bool bEnable, int sX, int sY, int sWdt, int sHgt) override {};
    virtual void FX_ResetPipe() override {};

    ////---------------------------------------------------------------------------------------------------------------------

    CNULLRenderer();
    virtual ~CNULLRenderer();

    virtual WIN_HWND Init(int x, int y, int width, int height, unsigned int cbpp, int zbpp, int sbits, bool fullscreen, bool isEditor, WIN_HINSTANCE hinst, WIN_HWND Glhwnd = 0, bool bReInit = false, const SCustomRenderInitArgs* pCustomArgs = 0, bool bShaderCacheGen = false);
    virtual WIN_HWND GetHWND();
    virtual bool SetWindowIcon(const char* path);

    /////////////////////////////////////////////////////////////////////////////////
    // Render-context management
    /////////////////////////////////////////////////////////////////////////////////
    virtual bool SetCurrentContext(WIN_HWND hWnd);
    virtual bool CreateContext(WIN_HWND hWnd, bool bAllowMSAA, int SSX, int SSY);
    virtual bool DeleteContext(WIN_HWND hWnd);
    virtual void MakeMainContextActive();

    virtual int GetCurrentContextViewportWidth() const { return -1; }
    virtual int GetCurrentContextViewportHeight() const { return -1; }
    /////////////////////////////////////////////////////////////////////////////////

    virtual int  CreateRenderTarget(const char* name, int nWidth, int nHeight, const ColorF& cClear, ETEX_Format eTF = eTF_R8G8B8A8);
    virtual bool DestroyRenderTarget(int nHandle);
    virtual bool ResizeRenderTarget(int nHandle, int nWidth, int nHeight);
    virtual bool SetRenderTarget(int nHandle, SDepthTexture* pDepthSurf = nullptr);
    virtual SDepthTexture* CreateDepthSurface(int nWidth, int nHeight, bool shaderResourceView = false);
    virtual void DestroyDepthSurface(SDepthTexture* pDepthSurf);

    virtual int GetOcclusionBuffer(uint16* pOutOcclBuffer, Matrix44* pmCamBuffe);
    virtual void WaitForParticleBuffer(threadID nThreadId);

    virtual void GetVideoMemoryUsageStats(size_t& vidMemUsedThisFrame, size_t& vidMemUsedRecently, bool bGetPoolsSizes = false) {}

    virtual   void SetRenderTile(f32 nTilesPosX, f32 nTilesPosY, f32 nTilesGridSizeX, f32 nTilesGridSizeY) {}

    virtual void EF_InvokeShadowMapRenderJobs(const int nFlags){}
    //! Fills array of all supported video formats (except low resolution formats)
    //! Returns number of formats, also when called with NULL
    virtual int   EnumDisplayFormats(SDispFormat* Formats);

    //! Return all supported by video card video AA formats
    virtual int   EnumAAFormats(SAAFormat* Formats) { return 0; }

    //! Changes resolution of the window/device (doen't require to reload the level
    virtual bool  ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForce);

    virtual Vec2 SetViewportDownscale(float xscale, float yscale) { return Vec2(0, 0); }
    virtual void SetCurDownscaleFactor(Vec2 sf) {};

    virtual EScreenAspectRatio GetScreenAspect(int nWidth, int nHeight) { return eAspect_4_3; }

    virtual void SwitchToNativeResolutionBackbuffer() {}

    virtual void  ShutDown(bool bReInit = false);
    virtual void  ShutDownFast();

    virtual void  BeginFrame();
    virtual void  RenderDebug(bool bRernderStats = true);
    virtual void    EndFrame();
    virtual void    LimitFramerate(const int maxFPS, const bool bUseSleep) {}

    virtual void    TryFlush();

    virtual void  Reset (void) {};
    virtual void    RT_ReleaseCB(void*){}

    virtual void DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType);
    virtual void DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices);

    virtual void  DrawBuffer(CVertexBuffer* pVBuf, CIndexBuffer* pIBuf, int nNumIndices, int nOffsIndex, const PublicRenderPrimitiveType nPrmode, int nVertStart = 0, int nVertStop = 0);

    virtual   void    CheckError(const char* comment);

    virtual void DrawLine(const Vec3& vPos1, const Vec3& vPos2) {};
    virtual void Graph(byte* g, int x, int y, int wdt, int hgt, int nC, int type, const char* text, ColorF& color, float fScale) {};

    virtual   void    SetCamera(const CCamera& cam);
    virtual   void    SetViewport(int x, int y, int width, int height, int id = 0);
    virtual   void    SetScissor(int x = 0, int y = 0, int width = 0, int height = 0);
    virtual void  GetViewport(int* x, int* y, int* width, int* height) const;

    virtual void  SetCullMode (int mode = R_CULL_BACK);
    virtual bool  EnableFog   (bool enable);
    virtual void  SetFogColor(const ColorF& color);
    virtual   void    EnableVSync(bool enable);

    virtual void  DrawPrimitivesInternal(CVertexBuffer* src, int vert_num, const eRenderPrimitiveType prim_type);

    virtual void  PushMatrix();
    virtual void  RotateMatrix(float a, float x, float y, float z);
    virtual void  RotateMatrix(const Vec3& angels);
    virtual void  TranslateMatrix(float x, float y, float z);
    virtual void  ScaleMatrix(float x, float y, float z);
    virtual void  TranslateMatrix(const Vec3& pos);
    virtual void  MultMatrix(const float* mat);
    virtual   void    LoadMatrix(const Matrix34* src = 0);
    virtual void  PopMatrix();

    virtual   void    EnableTMU(bool enable);
    virtual void  SelectTMU(int tnum);

    virtual bool ChangeDisplay(unsigned int width, unsigned int height, unsigned int cbpp);
    virtual void ChangeViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool bMainViewport = false);

    virtual   bool SaveTga(unsigned char* sourcedata, int sourceformat, int w, int h, const char* filename, bool flip) const { return false; }

    //download an image to video memory. 0 in case of failure
    virtual void CreateResourceAsync(SResourceAsync* Resource) {};
    virtual void ReleaseResourceAsync(SResourceAsync* Resource) {};
    virtual   unsigned int DownLoadToVideoMemory(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) { return 0; }
    virtual unsigned int DownLoadToVideoMemoryCube(const byte* data, int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) { return 0; }
    virtual unsigned int DownLoadToVideoMemory3D(const byte* data, int w, int h, int d, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat = true, int filter = FILTER_BILINEAR, int Id = 0, const char* szCacheName = NULL, int flags = 0, EEndian eEndian = eLittleEndian, RectI* pRegion = NULL, bool bAsynDevTexCreation = false) { return 0; }
    virtual void UpdateTextureInVideoMemory(uint32 tnum, const byte* newdata, int posx, int posy, int w, int h, ETEX_Format eTFSrc = eTF_R8G8B8A8, int posz = 0, int sizez = 1){}

    virtual   bool SetGammaDelta(const float fGamma);
    virtual void RestoreGamma(void) {};

    virtual   void RemoveTexture(unsigned int TextureId) {}
    virtual   void DeleteFont(IFFont* font) {}

    virtual void Draw2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1);
    virtual void Push2dImage(float xpos, float ypos, float w, float h, int texture_id, float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1, float angle = 0, float r = 1, float g = 1, float b = 1, float a = 1, float z = 1, float stereoDepth = 0);
    virtual void Draw2dImageList();
    virtual void  Draw2dImageStretchMode(bool stretch) {};
    virtual void DrawImage(float xpos, float ypos, float w, float h, int texture_id, float s0, float t0, float s1, float t1, float r, float g, float b, float a, bool filtered = true);
    virtual void DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float s[4], float t[4], float r, float g, float b, float a, bool filtered = true);

    virtual void PushWireframeMode(int mode);
    virtual void PopWireframeMode();
    virtual void FX_PushWireframeMode(int mode);
    virtual void FX_PopWireframeMode();
    virtual void FX_SetWireframeMode(int mode);

    virtual void FX_PreRender(int Stage) override {}
    virtual void FX_PostRender() override {}

    virtual void ResetToDefault();
    virtual void SetDefaultRenderStates() {}

    virtual int  GenerateAlphaGlowTexture(float k);

    virtual void ApplyViewParameters(const CameraViewParameters&) override {}
    virtual void SetMaterialColor(float r, float g, float b, float a);

    virtual void GetMemoryUsage(ICrySizer* Sizer);

    // Project/UnProject.  Returns true if successful.
    virtual bool ProjectToScreen(float ptx, float pty, float ptz,
        float* sx, float* sy, float* sz);
    virtual int UnProject(float sx, float sy, float sz,
        float* px, float* py, float* pz,
        const float modelMatrix[16],
        const float projMatrix[16],
        const int    viewport[4]);
    virtual int UnProjectFromScreen(float  sx, float  sy, float  sz,
        float* px, float* py, float* pz);

    // Shadow Mapping
    virtual bool PrepareDepthMap(ShadowMapFrustum* SMSource, int nFrustumLOD = 0, bool bClearPool = false);
    virtual void DrawAllShadowsOnTheScreen();
    virtual void OnEntityDeleted(IRenderNode* pRenderNode) {};

    virtual void FX_SetClipPlane (bool bEnable, float* pPlane, bool bRefract);

    virtual void SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa) {};
    virtual void EF_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa) {};

    virtual void SetSrgbWrite(bool srgbWrite) {};
    virtual void EF_SetSrgbWrite(bool sRGBWrite) {};

    //for editor
    virtual void  GetModelViewMatrix(float* mat);
    virtual void  GetProjectionMatrix(float* mat);

    //for texture
    virtual ITexture* EF_LoadTexture(const char* nameTex, const uint32 flags = 0);
    virtual ITexture* EF_LoadDefaultTexture(const char* nameTex);

    virtual void DrawQuad(const Vec3& right, const Vec3& up, const Vec3& origin, int nFlipMode = 0);
    virtual void DrawQuad(float dy, float dx, float dz, float x, float y, float z);
    // NOTE: deprecated
    virtual void ClearTargetsImmediately(uint32 nFlags);
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth);
    virtual void ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors);
    virtual void ClearTargetsImmediately(uint32 nFlags, float fDepth);

    virtual void ClearTargetsLater(uint32 nFlags);
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth);
    virtual void ClearTargetsLater(uint32 nFlags, const ColorF& Colors);
    virtual void ClearTargetsLater(uint32 nFlags, float fDepth);

    virtual void ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX = -1, int nScaledY = -1);
    virtual void ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight, bool BGRA = true);

    virtual bool CaptureFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight);
    virtual bool CopyFrameBufferFast(unsigned char* pDstRGBA8, int destinationWidth, int destinationHeight);
    virtual bool RegisterCaptureFrame(ICaptureFrameListener* pCapture);
    virtual bool UnRegisterCaptureFrame(ICaptureFrameListener* pCapture);
    virtual bool InitCaptureFrameBufferFast(uint32 bufferWidth, uint32 bufferHeight);
    virtual void CloseCaptureFrameBufferFast(void);
    virtual void CaptureFrameBufferCallBack(void);


    virtual void ReleaseHWShaders() {}
    virtual void PrintResourcesLeaks() {}

    //misc
    virtual bool ScreenShot(const char* filename = NULL, int width = 0);

    virtual void Set2DMode(uint32 orthoX, uint32 orthoY, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f) {}
    virtual void Unset2DMode(const TransformationMatrices& restoringMatrices) {}
    virtual void Set2DModeNonZeroTopLeft(float orthoLeft, float orthoTop, float orthoWidth, float orthoHeight, TransformationMatrices& backupMatrices, float znear = -1e10f, float zfar = 1e10f) {}

    virtual int ScreenToTexture(int nTexID);

    virtual void DrawPoints(Vec3 v[], int nump, ColorF& col, int flags) {};
    virtual void DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround) {};

    virtual void    RefreshSystemShaders() {}

    // Shaders/Shaders support
    // RE - RenderElement

    virtual void EF_Release(int nFlags);
    virtual void FX_PipelineShutdown(bool bFastShutdown = false);

    //==========================================================
    // external interface for shaders
    //==========================================================

    virtual bool EF_SetLightHole(Vec3 vPos, Vec3 vNormal, int idTex, float fScale = 1.0f, bool bAdditive = true);

    // Draw all shaded REs in the list
    virtual void EF_EndEf3D (const int nFlags, const int nPrecacheUpdateId, const int nNearPrecacheUpdateId, const SRenderingPassInfo& passInfo);

    // 2d interface for shaders
    virtual void EF_EndEf2D(const bool bSort);
    virtual bool EF_PrecacheResource(SShaderItem* pSI, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId, int nCounter);
    virtual bool EF_PrecacheResource(ITexture* pTP, float fDist, float fTimeToReady, int Flags, int nUpdateId, int nCounter);
    virtual void PrecacheResources();
    virtual void PostLevelLoading() {}
    virtual void PostLevelUnload() {}

    virtual ITexture* EF_CreateCompositeTexture(int type, const char* szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, ETEX_Format eTF, const STexComposition* pCompositions, size_t nCompositions, int8 nPriority = -1);

    void EF_Init();

    virtual IDynTexture* MakeDynTextureFromShadowBuffer(int nSize, IDynTexture* pDynTexture);
    virtual void MakeSprite(IDynTexture*& rTexturePtr, float _fSpriteDistance, int nTexSize, float angle, float angle2, IStatObj* pStatObj, const float fBrightnessMultiplier, SRendParams& rParms);
    virtual uint32 RenderOccludersIntoBuffer(const CCamera& viewCam, int nTexSize, PodArray<IRenderNode*>& lstOccluders, float* pBuffer) { return 0; }

    virtual IRenderAuxGeom* GetIRenderAuxGeom(void* jobID = 0)
    {
        return m_pNULLRenderAuxGeom;
    }

    virtual IColorGradingController* GetIColorGradingController();
    virtual IStereoRenderer* GetIStereoRenderer();

    virtual ITexture* Create2DTexture(const char* name, int width, int height, int numMips, int flags, unsigned char* data, ETEX_Format format);

    //////////////////////////////////////////////////////////////////////
    // Replacement functions for the Font engine ( vlad: for font can be used old functions )
    virtual bool FontUploadTexture(class CFBitmap*, ETEX_Format eTF = eTF_R8G8B8A8);
    virtual   int  FontCreateTexture(int Width, int Height, byte* pData, ETEX_Format eTF = eTF_R8G8B8A8, bool genMips = FontCreateTextureGenMipsDefaultValue, const char* textureName = nullptr);
    virtual   bool FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte* pData);
    virtual void FontReleaseTexture(class CFBitmap* pBmp);
    virtual void FontSetTexture(class CFBitmap*, int nFilterMode);
    virtual void FontSetTexture(int nTexId, int nFilterMode);
    virtual void FontSetRenderingState(bool overrideViewProjMatrices, TransformationMatrices& backupMatrices) {}
    virtual void FontSetBlending(int src, int dst, int baseState) {}
    virtual void FontRestoreRenderingState(bool overrideViewProjMatrices, const TransformationMatrices& restoringMatrices) {}

    virtual void GetLogVBuffers(void) {}

    virtual void RT_PresentFast() {}

    virtual void RT_ForceSwapBuffers() {}
    virtual void RT_SwitchToNativeResolutionBackbuffer(bool resolveBackBuffer) {}

    virtual void RT_BeginFrame() {}
    virtual void RT_EndFrame() {}
    virtual void RT_Init() {}
    virtual void RT_ShutDown(uint32 nFlags) {}
    virtual bool RT_CreateDevice() { return true; }
    virtual void RT_Reset() {}
    virtual void RT_SetCull(int nMode) {}
    virtual void RT_SetScissor(bool bEnable, int x, int y, int width, int height){}
    virtual void RT_RenderScene(int nFlags, SThreadInfo& TI, RenderFunc pRenderFunc) {}
    virtual void RT_PrepareStereo(int mode, int output) {}
    virtual void RT_CopyToStereoTex(int channel) {}
    virtual void RT_UpdateTrackingStates() {}
    virtual void RT_DisplayStereo() {}
    virtual void RT_SetCameraInfo() {}
    virtual void RT_SetStereoCamera() {}
    virtual void RT_ReadFrameBuffer(unsigned char* pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY) {}
    virtual void RT_RenderScene(int nFlags, SThreadInfo& TI, int nR, RenderFunc pRenderFunc) {};
    virtual void RT_CreateResource(SResourceAsync* Res) {};
    virtual void RT_ReleaseResource(SResourceAsync* Res) {};
    virtual void RT_ReleaseRenderResources() {};
    virtual void RT_UnbindResources() {};
    virtual void RT_UnbindTMUs() {};
    virtual void RT_PrecacheDefaultShaders() {};
    virtual void RT_CreateRenderResources() {};
    virtual void RT_ClearTarget(ITexture* pTex, const ColorF& color) {};
    virtual void RT_RenderDebug(bool bRenderStats = true) {};

    virtual HRESULT RT_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle) { return S_OK; }
    virtual HRESULT RT_CreateIndexBuffer(UINT Length, DWORD Usage, DWORD Format, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle) { return S_OK; };
    virtual HRESULT RT_CreateVertexShader(DWORD* pBuf, void** pShader, void* pInst) { return S_OK; };
    virtual HRESULT RT_CreatePixelShader(DWORD* pBuf, void** pShader) { return S_OK; };
    virtual void RT_ReleaseVBStream(void* pVB, int nStream) {};
    virtual void RT_DrawDynVB(int Pool, uint32 nVerts) {}
    virtual void RT_DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType) {}
    virtual void RT_DrawDynVBUI(SVF_P2F_C4B_T2F_F4B* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType) {}
    virtual void RT_DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) const {}
    virtual void RT_DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround) {}
    virtual void RT_Draw2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z) {}
    virtual void RT_Push2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z, float stereoDepth) {}
    virtual void RT_Draw2dImageList() {}
    virtual void RT_Draw2dImageStretchMode(bool bStretch) {}
    virtual void RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, int texture_id, float* s, float* t, DWORD col, bool filtered = true) {}
    virtual void EF_ClearTargetsImmediately(uint32 nFlags) {}
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil) {}
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors) {}
    virtual void EF_ClearTargetsImmediately(uint32 nFlags, float fDepth, uint8 nStencil) {}

    virtual void EF_ClearTargetsLater(uint32 nFlags) {}
    virtual void EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil) {}
    virtual void EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors) {}
    virtual void EF_ClearTargetsLater(uint32 nFlags, float fDepth, uint8 nStencil) {}

    virtual void RT_PushRenderTarget(int nTarget, CTexture* pTex, SDepthTexture* pDS, int nS)  {};
    virtual void RT_PopRenderTarget(int nTarget) {};
    virtual void RT_SetViewport(int x, int y, int width, int height, int id) {};

    virtual void RT_SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode = false) {};
    virtual void SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode = false) {};

    virtual void SetMatrices(float* pProjMat, float* pViewMat) {}

    virtual void PushProfileMarker(const char* label) {}
    virtual void PopProfileMarker(const char* label) {}

    virtual void RT_InsertGpuCallback(uint32 context, GpuCallbackFunc callback) {}
    virtual void EnablePipelineProfiler(bool bEnable) {}

    virtual IOpticsElementBase* CreateOptics(EFlareType type) const { return NULL;        }

    virtual bool BakeMesh(const SMeshBakingInputParams* pInputParams, SMeshBakingOutput* pReturnValues) { return false; }
    virtual PerInstanceConstantBufferPool* GetPerInstanceConstantBufferPoolPointer() override { return nullptr; }

    IDynTexture* CreateDynTexture2(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource, ETexPool eTexPool) override;

#ifdef SUPPORT_HW_MOUSE_CURSOR
    virtual IHWMouseCursor* GetIHWMouseCursor() { return NULL; }
#endif

    virtual void StartLoadtimePlayback(ILoadtimeCallback* pCallback) {}
    virtual void StopLoadtimePlayback() {}

private:
    CNULLRenderAuxGeom* m_pNULLRenderAuxGeom;
    IColorGradingController* m_pNULLColorGradingController;
    IStereoRenderer* m_pNULLStereoRenderer;
};

//=============================================================================

extern CNULLRenderer* gcpNULL;



#endif //NULL_RENDERER
