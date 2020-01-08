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

#pragma once

#include "IConvertor.h"             // IConvertor
#include "ImageConvertor.h"         // ImageConvertor::PresetAliases
#include "ImageProperties.h"        // CImageProperties
#include "Filtering/CubeMapGen-1.4-Source/CCubeMapProcessor.h"
#include <IResCompiler.h>

#include "GenerationProgress.h"

#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>

enum EConfigPriority;
class ImageObject;
class ImageToProcess;
struct tiff;
class CImageUserDialog;
class CTextureSplitter;
struct SCubemapFilterParams;


class CImageCompiler
    : public ICompiler
{
    friend class CGenerationProgress;

public:

    /**
    * AZToolsFramework application override for RC Image Compiler
    * Adds the minimal set of system components required for Source control operations.
    */
    class RCImageCompilerApplication
        : public AzToolsFramework::ToolsApplication
    {
    public:
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components = AZ::ComponentApplication::GetRequiredSystemComponents();

            components.insert(components.end(), {
                azrtti_typeid<AZ::MemoryComponent>(),
                azrtti_typeid<AZ::JobManagerComponent>(),
                azrtti_typeid<AzToolsFramework::PerforceComponent>(),
            });

            return components;
        }
    };

    CImageCompiler(const CImageConvertor::PresetAliases& presetAliases);
    virtual ~CImageCompiler();

    // Arguments:
    //  lpszExtension - must not be 0, must be lower case
    bool LoadInput(const char* lpszPathName, const char* lpszExtension, const bool bLoadConfig, string& specialInstructions);

    // Arguments:
    //   pImageObject - must not be 0
    //   szType must not be 0, for log output
    bool SaveOutput(const ImageObject* pImageObject, const char* szType, const char* lpszPathName);

    // used to pass existing image as input
    // Arguments:
    //   pInput - data will be stored and freed automatically
    void GetInputFromMemory(ImageObject* pInput);

    //
    void FreeImages();

    // Arguments:
    //   *ppInputImage - must not be 0
    //   fGamma >0, 1.0 means no gamma correction is applied
    void CreateMipMaps(ImageToProcess& image, uint32 indwReduceResolution, const bool inbRemoveMips, const bool bRenormalize);

    // Arguments:
    //   *ppInputImage - must not be 0, aspect 6:1
    //   fGamma >0, 1.0 means no gamma correction is applied
    //   cubemap filter params - see ATICubemapGen lib
    void CreateCubemapMipMaps(ImageToProcess& image, const SCubemapFilterParams& params, uint32 indwReduceResolution, const bool inbRemoveMips);

    // text for the info below the preview
    string GetInfoStringUI(const bool inbOrig) const;

    // text for the info below the preview
    void GetFinalImageInfo(int platform, EPixelFormat& finalFormat, bool& bCubemap, uint32& mipCount, uint32& mipReduce, uint32& width, uint32& height) const;

    // text for file statistics (not multiline, tab separated)
    string GetDestInfoString();

    const ImageObject* GetInputImage() const
    {
        return m_pInputImage;
    }

    const ImageObject* GetFinalImage() const
    {
        return m_pFinalImage;
    }

    // run with the user specified user properties in m_Prop
    // Arguments:
    //   inbSave - true=save the result, false=update only the internal structures for previewing
    //   szExtendFileName - e.g. "_DDN" or "_DDNDIF" is inserted before the file extension
    // Return:
    //   return true=success, false otherwise(e.g. compression failed because of non power of two)
    bool RunWithProperties(bool inbSave, const char* szExtendFileName = 0);
    bool AnalyzeWithProperties(bool autopreset, const char* szExtendFileName = 0);

    //
    void AutoPreset();

    //! set the stored properties to the current file and save it
    //! @return true=success, false otherwise
    bool UpdateAndSaveConfig();

    //
    void SetCC(const ConvertContext& CC);

    //
    string GetUnaliasedPresetName(const string& presetName) const;
    void SetPreset(const string& presetName);
    void SetPresetSettings();

    // interface ICompiler ----------------------------------------------------

    virtual void Release();
    virtual void BeginProcessing(const IConfig* config);
    virtual void EndProcessing();
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

    // ------------------------------------------------------------------------

    // depending on extension this calls a different loading function
    // Arguments:
    //   lpszExtension - extension passed as parameter to provide processing of files with wrong extension
    //   res_specialInstructions - after return contains special instructions (if any) from the image file
    ImageObject* LoadImageFromFile(const char* lpszPathName, const char* lpszExtension, string& res_specialInstructions);
    bool SaveImageToFile(const char* lpszPathName, const char* lpszExtension, const ImageObject* pImageObject);

    static bool CheckForExistingPreset(const ConvertContext& CC, const string& presetName, bool errorInsteadOfWarning);

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;
    string GetSettingsFile(const char* currentFile) const;

    // actual implementation of Process method
    bool ProcessImplementation();
    bool ProcessWithAzFramework();

    //TODO: Need a non windows specific dialog system
#if defined(AZ_PLATFORM_WINDOWS) || AZ_TRAIT_OS_PLATFORM_APPLE
    // does export-time checks
    void AnalyzeImageAndSuggest(const ImageObject* pSourceImage);
    
    bool InitDialogSystem();
    void ReleaseDialogSystem();
#endif
    
    bool IsFinalPixelFormatValidForNormalmaps(int platform) const;

    //!
    void ClearInputImageFlags();

    // Returns:
    //   size in bytes, 0 if pImage is 0
    static uint32 CalcTextureMemory(const ImageObject* pImage);
    
    static uint32 _CalcTextureMemory(const EPixelFormat pixelFormat, const uint32 dwWidth, const uint32 dwHeight, const bool bCubemap, const bool bMips);

public:  // ------------------------------------------------------------------
    CImageProperties          m_Props;                          // user settings
    ConvertContext            m_CC;

    CGenerationProgress       m_Progress;

    CTextureSplitter*         m_pTextureSplitter;

private: // ------------------------------------------------------------------

    const CImageConvertor::PresetAliases& m_presetAliases;

    ImageObject*              m_pInputImage;                    // input image
    ImageObject*              m_pFinalImage;                    // changed destination image

    // cubemap processing
    CCubeMapProcessor         m_AtiCubemanGen;                  // for cubemap filtering

    string                    m_sLastPreset;                    // last preset used for conversion (used only in AnalyzeImageAndSuggest)

    bool                      m_bDialogSystemInitialized;       // true when dialog subsystem is initialized
#if defined(AZ_PLATFORM_WINDOWS) || AZ_TRAIT_OS_PLATFORM_APPLE
    CImageUserDialog*         m_pImageUserDialog;               // backlink to user dialog for preview calculation handling
#endif
    bool                      m_bInternalPreview;               // indicates whether currently an internal preview is calculated (true) or results are stored (false)
};
