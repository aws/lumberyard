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

// Description : Declaration of the type CDevice and the functions to
//               initialize OpenGL contexts and detect hardware
//               capabilities.


#ifndef __GLDEVICE__
#define __GLDEVICE__

#include "GLCommon.hpp"
#include "GLContext.hpp"

namespace NCryOpenGL
{
    // Optional device context features
    enum EFeature
    {
        eF_ComputeShader,
        eF_IndexedBoolState,
        eF_StencilOnlyFormat,
        eF_MultiSampledTextures,
        eF_DrawIndirect,
        eF_StencilTextures,
        eF_AtomicCounters,
        eF_DispatchIndirect,
        eF_ShaderImages,
        eF_VertexAttribBinding,
        eF_TextureViews,
        eF_DepthClipping,
        eF_SeparablePrograms,
        eF_TextureBorderClamp,
        eF_TextureAnisotropicFiltering,
        eF_BufferStorage,
        eF_MultiBind,
        eF_DebugOutput,
        eF_DualSourceBlending,
        eF_NUM // Must be last one
    };

    struct SResourceUnitCapabilities
    {
        GLint m_aiMaxTotal;
        GLint m_aiMaxPerStage[eST_NUM];
    };

    // Hardware capabilities of a device context
    struct SCapabilities
    {
        GLint m_iMaxSamples;
        GLint m_iMaxVertexAttribs;

        SResourceUnitCapabilities m_akResourceUnits[eRUT_NUM];

        GLint m_iUniformBufferOffsetAlignment;
        GLint m_iMaxUniformBlockSize;

#if DXGL_SUPPORT_SHADER_STORAGE_BLOCKS
        GLint m_iShaderStorageBufferOffsetAlignment;
#endif

        // vertex attrib binding
        GLint m_iMaxVertexAttribBindings;
        GLint m_iMaxVertexAttribRelativeOffset;

        // The supported usage for each GI format (union of D3D11_FORMAT_SUPPORT flags)
        uint32 m_auFormatSupport[eGIF_NUM];

#if DXGL_SUPPORT_COPY_IMAGE
        // Some drivers implementation of glCopyImageSubData does not work on cube map faces as specified by the standard
        bool m_bCopyImageWorksOnCubeMapFaces;
#endif
    };

    struct SVersion
    {
        uint32 m_uMajorVersion;
        uint32 m_uMinorVersion;

        uint32 ToUint() const
        {
            return m_uMajorVersion * 100 + m_uMinorVersion * 10;
        }
    };

    struct SPixelFormatSpec
    {
        const SUncompressedLayout* m_pLayout;
        uint32 m_uNumSamples;
        bool m_bSRGB;
    };

    struct SFrameBufferSpec
        : SPixelFormatSpec
    {
        uint32 m_uWidth;
        uint32 m_uHeight;
    };

    typedef SBitMask<eF_NUM, SUnsafeBitMaskWord> TFeatures;

    struct SFeatureSpec
    {
        TFeatures m_kFeatures;
        SVersion m_kVersion;
    };

    struct SDisplayMode
    {
        uint32 m_uWidth;
        uint32 m_uHeight;
        uint32 m_uFrequency;
#if defined(DXGL_USE_SDL)
        EGIFormat m_ePixelFormat;
#elif defined(WIN32)
        uint32 m_uBitsPerPixel;
#endif
    };

    struct SResourceUnitPartitionBound
    {
        uint32 m_uFirstUnit; // Lowest unit index used
        uint32 m_uNumUnits; // Number of contiguous unit indices used
    };

    typedef SResourceUnitPartitionBound TPipelineResourceUnitPartitionBound[eST_NUM];

#if defined(DXGL_USE_SDL)
    // Interface for the different type of window contexts.
    // The window context is the platform target where openGL will draw.
    struct IWindowContext
    {
    public:
        virtual ~IWindowContext() = default;

        // Make the provided context and the window's surface current for the calling thread. 
        virtual bool MakeCurrent(const TRenderingContext context) const = 0;
        
        // Swap the window's surface buffers.
        virtual bool SwapBuffers() const = 0;

        // Creates a new OpenGL context that is shared with the provided context using the current window. 
        virtual TRenderingContext CreateGLContext(const TRenderingContext sharedContext) const = 0;
    };

    // Implementation of the window context using SDL.
    // It basically forward all calls to the SDL library.
    struct SDLWindowContext : public IWindowContext
    {
    public:
        SDLWindowContext(SDL_Window* window);
        ~SDLWindowContext() override;
        
        // Returns the SDL_Window
        SDL_Window* GetWindow() const { return m_window; }

        bool MakeCurrent(const TRenderingContext context) const override;
        bool SwapBuffers() const override;
        TRenderingContext CreateGLContext(const TRenderingContext sharedContext) const override;

    protected:
        SDL_Window* m_window;
    };

#ifdef AZ_PLATFORM_ANDROID
    // Implementation of the window context as a PBuffer surface using EGL.
    struct EGLPBufferWindowContext : public IWindowContext
    {
    public:
        // Initialize the surface and display using the provided dimensions and pixel format specification.
        EGLPBufferWindowContext(EGLint width, EGLint height, const SPixelFormatSpec& pixelFormat);
        ~EGLPBufferWindowContext() override;

        bool MakeCurrent(const TRenderingContext context) const override;
        bool SwapBuffers() const override;
        TRenderingContext CreateGLContext(const TRenderingContext sharedContext) const override;

    protected:
        EGLSurface m_surface;
        EGLDisplay m_display;
        EGLConfig m_config;
    };
#endif //AZ_PLATFORM_ANDROID

#if defined(DXGL_SINGLEWINDOW)

    struct SMainWindow
    {
        TWindowContext m_pSDLWindow;
        uint32 m_uWidth;
        uint32 m_uHeight;
        string m_strTitle;
        bool m_bFullScreen;

        static SMainWindow ms_kInstance;
    };

#endif // DXGL_SINGLEWINDOW
#endif // DXGL_USE_SDL

#if DXGL_USE_ES_EMULATOR

    DXGL_DECLARE_REF_COUNTED(struct, SDisplayConnection)
    {
        EGLDisplay m_kDisplay;
        EGLSurface m_kSurface;
        EGLConfig m_kConfig;
    };

#endif //DXGL_USE_ES_EMULATOR

    DXGL_DECLARE_REF_COUNTED(struct, SOutput)
    {
        string m_strDeviceID;
        string m_strDeviceName;
        std::vector<SDisplayMode> m_kModes;
        SDisplayMode m_kDesktopMode;
    };

    DXGL_DECLARE_REF_COUNTED(struct, SAdapter)
    {
        string m_strRenderer;
        string m_strVendor;
        string m_strVersion;
        SVersion m_sVersion;

        SCapabilities m_kCapabilities;
        std::vector<SOutputPtr> m_kOutputs;

        TFeatures m_kFeatures;
        size_t m_uVRAMBytes;
        unsigned int m_eDriverVendor;
        AZStd::unordered_set<size_t> m_kExtensions;

        void AddExtension(const AZStd::string& kExtension)
        {
            m_kExtensions.insert(AZStd::hash<AZStd::string>()(kExtension));
        }

        bool HasExtension(const AZStd::string& kExtension) const
        {
            return m_kExtensions.find(AZStd::hash<AZStd::string>()(kExtension)) != m_kExtensions.end();
        }
    };

#if DXGL_FULL_EMULATION
    struct SDummyWindow;
#endif //DXGL_FULL_EMULATION

#if DXGL_USE_ES_EMULATOR
    typedef EGLNativeDisplayType TNativeDisplay;
#elif defined(DXGL_USE_SDL)
    typedef TWindowContext* TNativeDisplay;
#else
    typedef TWindowContext TNativeDisplay;
#endif

    DXGL_DECLARE_REF_COUNTED(class, CDevice)
    {
    public:

        CDevice(SAdapter * pAdapter, const SFeatureSpec&kFeatureSpec, const SPixelFormatSpec&kPixelFormatSpec);
        ~CDevice();

#if !DXGL_FULL_EMULATION
        static void Configure(uint32 uNumSharedContexts);
#endif //!DXGL_FULL_EMULATION
#if defined(DXGL_USE_SDL)
        static bool CreateSDLWindow(const char* szTitle, uint32 uWidth, uint32 uHeight, bool bFullScreen, HWND * pHandle);
        static void DestroySDLWindow(HWND kHandle);
#endif //defined(DXGL_USE_SDL)

        bool Initialize(const TNativeDisplay&kDefaultNativeDisplay);
        void Shutdown();
        bool Present(const TWindowContext&kTargetWindowContext);

        CContext* ReserveContext();
        void ReleaseContext();
        CContext* AllocateContext(CContext::ContextType type = CContext::ResourceType);
        void FreeContext(CContext * pContext);
        void BindContext(CContext * pContext);
        void UnbindContext(CContext * pContext);
        void SetCurrentContext(CContext * pContext);
        CContext* GetCurrentContext();
        uint32 GetMaxContextCount();

        void IssueFrameFences();
        bool FlushFrameFence(uint32 uContext) { return m_kContextFenceIssued.Set(uContext, false); }

        SResourceNamePool& GetTextureNamePool() { return m_kTextureNamePool; }
        SResourceNamePool& GetBufferNamePool()  { return m_kBufferNamePool; }
        SResourceNamePool& GetFrameBufferNamePool()  { return m_kFrameBufferNamePool; }

        const SIndexPartition& GetResourceUnitPartition(EResourceUnitType eType, uint32 uID) { return m_kResourceUnitPartitions[eType][uID]; }
        uint32 GetNumResourceUnitPartitions(EResourceUnitType eType) { return (uint32)m_kResourceUnitPartitions[eType].size(); }

        bool SetFullScreenState(const SFrameBufferSpec&kFrameBufferSpec, bool bFullScreen, SOutput * pOutput);
        bool ResizeTarget(const SDisplayMode&kTargetMode);
        void SetBackBufferTexture(SDefaultFrameBufferTexture * pBackTexture);
        SAdapter* GetAdapter() { return m_spAdapter; }
        const TWindowContext& GetDefaultWindowContext() { return m_kDefaultWindowContext; }
        const SFeatureSpec& GetFeatureSpec() { return m_kFeatureSpec; }
        const SPixelFormatSpec& GetPixelFormatSpec() { return m_kPixelFormatSpec; }
        bool IsFeatureSupported(EFeature feature) const;
        static CDevice* GetCurrentDevice() { return ms_pCurrentDevice; }

    protected:

        typedef std::vector<CContext*> TContexts;
        typedef std::vector<SIndexPartition> TPartitions;

        void InitializeResourceUnitPartitions();

        void PartitionResourceIndices(
            EResourceUnitType eUnitType,
            const TPipelineResourceUnitPartitionBound * akPartitionBounds,
            uint32 uNumPartitions);

        static bool CreateRenderingContexts(
            TWindowContext & kWindowContext,
            std::vector<TRenderingContext>&kRenderingContexts,
            const SFeatureSpec&kFeatureSpec,
            const SPixelFormatSpec&kPixelFormat,
            const TNativeDisplay &kNativeDisplay);

        static bool MakeCurrent(const TWindowContext&kWindowContext, TRenderingContext kRenderingContext);

        static uint32 ms_uNumContextsPerDevice;
        static CDevice* ms_pCurrentDevice;

        SOutputPtr m_spFullScreenOutput;
        SAdapterPtr m_spAdapter;
        SFeatureSpec m_kFeatureSpec;
        SPixelFormatSpec m_kPixelFormatSpec;
        TWindowContext m_kDefaultWindowContext;
        TNativeDisplay m_kDefaultNativeDisplay;
#if DXGL_FULL_EMULATION
        SDummyWindow* m_pDummyWindow;
#endif //DXGL_FULL_EMULATION
        TContexts m_kContexts;
        SList m_kFreeContexts[CContext::NumContextTypes];
        void* m_pCurrentContextTLS;

        SBitMask<MAX_NUM_CONTEXT_PER_DEVICE, SSpinlockBitMaskWord> m_kContextFenceIssued;

        SResourceNamePool m_kTextureNamePool;
        SResourceNamePool m_kBufferNamePool;
        SResourceNamePool m_kFrameBufferNamePool;

        TPartitions m_kResourceUnitPartitions[eRUT_NUM];
    };

    bool FeatureLevelToFeatureSpec(SFeatureSpec& kContextSpec, D3D_FEATURE_LEVEL eFeatureLevel, NCryOpenGL::SAdapter* pGLAdapter);
    void GetStandardPixelFormatSpec(SPixelFormatSpec& kPixelFormatSpec);
    bool SwapChainDescToFrameBufferSpec(SFrameBufferSpec& kFrameBufferSpec, const DXGI_SWAP_CHAIN_DESC& kSwapChainDesc);
    bool GetNativeDisplay(TNativeDisplay& kNativeDisplay, HWND kWindowHandle);
    bool CreateWindowContext(TWindowContext& kWindowContext, const SFeatureSpec& kFeatureSpec, const SPixelFormatSpec& kPixelFormatSpec, const TNativeDisplay& kNativeDisplay);
    void ReleaseWindowContext(TWindowContext& kWindowContext);
#if defined(DXGL_USE_SDL)
    const SGIFormatInfo* SDLFormatToGIFormatInfo(int format);
#endif //defined(DXGL_USE_SDL)

    uint32 DetectGIFormatSupport(EGIFormat eGIFormat);
    bool DetectFeaturesAndCapabilities(TFeatures& kFeatures, SCapabilities& kCapabilities, const SVersion& version);
    bool DetectAdapters(std::vector<SAdapterPtr>& kAdapters);
    bool CheckAdapterCapabilities(const SAdapter& kAdapter, AZStd::string* pErrorMsg = nullptr);
    bool DetectOutputs(const SAdapter& kAdapter, std::vector<SOutputPtr>& kOutputs);
    bool CheckFormatMultisampleSupport(SAdapter* pAdapter, EGIFormat eFormat, uint32 uNumSamples);
    void GetDXGIModeDesc(DXGI_MODE_DESC* pDXGIModeDesc, const SDisplayMode& kDisplayMode);
    bool GetDisplayMode(SDisplayMode* pDisplayMode, const DXGI_MODE_DESC& kDXGIModeDesc);
}


#endif //__GLDEVICE__
