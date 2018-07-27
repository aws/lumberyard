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

#include "stdafx.h"
#include <assert.h>                         // assert()

#include "IRCLog.h"                         // IRCLog
#include "IConfig.h"                        // IConfig

#include "ImageCompiler.h"                  // CImageCompiler
#include "ImageUserDialog.h"                // CImageUserDialog

#include "PixelFormats.h"                   // g_pixelformats

#include "ImageObject.h"                                        // ImageToProcess

#include "Cry_Math.h"                       // Vec3
#include "Cry_Color.h"                      // ColorF
#include "IResCompiler.h"
#include "CryEndian.h"                         // Endian converting (for ImageExtensionHelper)
#include "ITexture.h"                       // eTEX enum
#include "tools.h"                          // GetBoolParam()
#include "CryPtrArray.h"
#include "Util.h"                           // getMin(), getMax()
#include "UpToDateFileHelpers.h"
#include "CryPath.h"                        // PathUtil::

#include "Filtering/Cubemap.h"
#include "Formats/TIFF.h"
#include "Formats/HDR.h"
#include "Formats/DDS.h"
#include "Formats/QtImage.h"
#include "Operations/ColorChart/ColorChart.h"

#include "SuffixUtil.h"                     // SuffixUtil
#include "ImageExtensionHelper.h"           // CImageExtensionHelper

#include "Streaming/TextureSplitter.h"

#include "FileUtil.h"                       // GetFileSize()

#include "StringHelpers.h"

#include "IResourceCompilerHelper.h"

#include <iterator>

#include <ThreadUtils.h>

#include <QApplication>
#include <QLabel>
#include <QMessageBox>

static ThreadUtils::CriticalSection s_initDiallogSystemLock;


const char* stristr(const char* szString, const char* szSubstring)
{
    int nSuperstringLength = (int)strlen(szString);
    int nSubstringLength = (int)strlen(szSubstring);

    for (int nSubstringPos = 0; nSubstringPos <= nSuperstringLength - nSubstringLength; ++nSubstringPos)
    {
        if (_strnicmp(szString + nSubstringPos, szSubstring, nSubstringLength) == 0)
        {
            return szString + nSubstringPos;
        }
    }

    return NULL;
}

// checks if preset exists, reports warning or error if doesn't
bool CImageCompiler::CheckForExistingPreset(const ConvertContext& CC, const string& presetName, bool errorInsteadOfWarning)
{
    if (CC.pRC->GetIniFile()->FindSection(presetName.c_str()) < 0)
    {
        if (errorInsteadOfWarning)
        {
            RCLogError("Preset '%s' doesn't exist in rc.ini. Please contact a lead technical artist.", presetName.c_str());
        }
        else
        {
            RCLogWarning("Preset '%s' doesn't exist in rc.ini.", presetName.c_str());
        }
        return false;
    }
    return true;
}


// constructor
CImageCompiler::CImageCompiler(const CImageConvertor::PresetAliases& presetAliases)
    : m_Progress(*this)
    , m_presetAliases(presetAliases)
{
    m_pInputImage = 0;
    m_pFinalImage = 0;
    m_bDialogSystemInitialized = false;
    m_bInternalPreview = false;
    m_pImageUserDialog = 0;
    m_pTextureSplitter = 0;

    ClearInputImageFlags();
}


// destructor
CImageCompiler::~CImageCompiler()
{
    FreeImages();

    ReleaseDialogSystem();

    if (m_pTextureSplitter)
    {
        m_pTextureSplitter->Release();
        m_pTextureSplitter = 0;
    }
}


void CImageCompiler::FreeImages()
{
    assert((m_pInputImage != m_pFinalImage) || (m_pInputImage == 0 && m_pFinalImage == 0));
    delete m_pFinalImage;
    m_pFinalImage = 0;
    delete m_pInputImage;
    m_pInputImage = 0;
}


void CImageCompiler::GetInputFromMemory(ImageObject* pInput)
{
    FreeImages();

    m_pInputImage = pInput;

    ClearInputImageFlags();

    if (m_pInputImage)
    {
        m_Props.m_bPreserveAlpha = m_pInputImage->HasNonOpaqueAlpha();
    }
}


bool CImageCompiler::SaveOutput(const ImageObject* pImageObject, const char* szType, const char* lpszPathName)
{
    assert(lpszPathName);
    assert(pImageObject);

    if (!pImageObject)
    {
        RCLogError("CImageCompiler::Save() failed (conversion failed?)");
        return false;
    }

    // Force remove of the read only flag.
    QFile(lpszPathName).setPermissions(QFile::ReadOther | QFile::WriteOther );

    if (m_Props.GetImageCompressor() == CImageProperties::eImageCompressor_Tif)
    {
        if (!ImageTIFF::SaveByUsingTIFFSaver(lpszPathName, GetSettingsFile(lpszPathName).c_str(), &m_Props, pImageObject))
        {
            RCLogError("CImageCompiler::SaveAsTIFF() failed");
            return false;
        }
    }
    else if (m_Props.GetImageCompressor() == CImageProperties::eImageCompressor_Hdr)
    {
        if (!ImageHDR::SaveByUsingHDRSaver(lpszPathName, &m_Props, pImageObject))
        {
            RCLogError("CImageCompiler::SaveAsHDR() failed");
            return false;
        }
    }
    else
    {
        if (!pImageObject->SaveImage(lpszPathName, false))
        {
            RCLogError("CImageCompiler::Save failed (file IO?)");
            return false;
        }
    }

    const uint32 sizeTotal = CalcTextureMemory(pImageObject);

    const string filename = PathHelpers::GetFilename(lpszPathName);
    if (pImageObject->GetAttachedImage())
    {
        const uint32 sizeAttached = CalcTextureMemory(pImageObject->GetAttachedImage());
        RCLog("   Processed %s (%d bytes, including %d bytes attached): %s", szType, sizeTotal, sizeAttached, filename.c_str());
    }
    else
    {
        RCLog("   Processed %s (%d bytes): %s", szType, sizeTotal, filename.c_str());
    }

    return true;
}


bool CImageCompiler::InitDialogSystem()
{
    ThreadUtils::AutoLock lock(s_initDiallogSystemLock);

    if (m_bDialogSystemInitialized)
    {
        return true;
    }

    m_bDialogSystemInitialized = true;

    return true;
}


void CImageCompiler::ReleaseDialogSystem()
{
    ThreadUtils::AutoLock lock(s_initDiallogSystemLock);

    if (m_bDialogSystemInitialized)
    {
        m_bDialogSystemInitialized = false;
    }
}


ImageObject* CImageCompiler::LoadImageFromFile(const char* lpszPathName, const char* lpszExtension, string& res_specialInstructions)
{
    res_specialInstructions.clear();

    if (!azstricmp(lpszExtension, "tif") || !azstricmp(lpszExtension, "tiff"))
    {
        return ImageTIFF::LoadByUsingTIFFLoader(lpszPathName, GetSettingsFile(lpszPathName).c_str(), &m_Props, res_specialInstructions);
    }

    if (!azstricmp(lpszExtension, "hdr"))
    {
        return ImageHDR::LoadByUsingHDRLoader(lpszPathName, &m_Props, res_specialInstructions);
    }

    if (!azstricmp(lpszExtension, "dds"))
    {
        return ImageDDS::LoadByUsingDDSLoader(lpszPathName, &m_Props, res_specialInstructions);
    }

    if (ImageQImage::IsExtensionSupported(lpszExtension))
    {
        return ImageQImage::LoadByUsingQImageLoader(lpszPathName, GetSettingsFile(lpszPathName).c_str(), &m_Props, res_specialInstructions);
    }

    RCLogError("%s: Unsupported extension:'%s'", __FUNCTION__, lpszExtension);
    return 0;
}


bool CImageCompiler::SaveImageToFile(const char* lpszPathName, const char* lpszExtension, const ImageObject* pImageObject)
{
    if (!azstricmp(lpszExtension, "tif"))
    {
        return ImageTIFF::SaveByUsingTIFFSaver(lpszPathName, GetSettingsFile(lpszPathName).c_str(), &m_Props, pImageObject);
    }

    if (!azstricmp(lpszExtension, "hdr"))
    {
        return ImageHDR::SaveByUsingHDRSaver(lpszPathName, &m_Props, pImageObject);
    }

    if (!azstricmp(lpszExtension, "dds"))
    {
        return ImageDDS::SaveByUsingDDSSaver(lpszPathName, &m_Props, pImageObject);
    }

    RCLogError("%s: Unsupported extension:'%s'", __FUNCTION__, lpszExtension);
    return 0;
}


bool CImageCompiler::LoadInput(
    const char* lpszPathName,
    const char* lpszExtension,
    const bool bLoadConfig,
    string& specialInstructions)
{
    specialInstructions.clear();

    FreeImages();

    assert(!m_pInputImage);
    if (m_pInputImage)
    {
        RCLogError("%s: Unexpected failure", __FUNCTION__);
    }

    m_pInputImage = LoadImageFromFile(lpszPathName, lpszExtension, specialInstructions);

    if (!m_pInputImage)
    {
        return false;
    }

    if (bLoadConfig)
    {
        const int platformCount = m_CC.multiConfig->getPlatformCount();
        for (int i = 0; i < platformCount; ++i)
        {
            m_CC.multiConfig->getConfig(i).ClearPriorityUsage(eCP_PriorityFile);
            m_CC.multiConfig->getConfig(i).SetFromString(eCP_PriorityFile, specialInstructions.c_str());
        }
    }

    // test cases for SuffixUtil::HasSuffix()
    assert(!SuffixUtil::HasSuffix("dirt_ddn.dds",        '_', "ddndif"));
    assert(SuffixUtil::HasSuffix("dirt_ddndif.dds",     '_', "ddndif"));
    assert(!SuffixUtil::HasSuffix("dirt.dds",            '_', "ddndif"));
    assert(SuffixUtil::HasSuffix("dirt_ddn_ddndif.dds", '_', "ddndif"));
    assert(SuffixUtil::HasSuffix("dirt_ddndif_ddn.dds", '_', "ddndif"));
    assert(SuffixUtil::HasSuffix("dirt_ddndif",         '_', "ddndif"));
    assert(!SuffixUtil::HasSuffix("dirt_ddndif2",        '_', "ddndif"));
    assert(!SuffixUtil::HasSuffix("dirt_ddndif2_",       '_', "ddndif"));

    if (SuffixUtil::HasSuffix(m_CC.sourceFileNameOnly, '_', "ddndif"))
    {
        m_Props.m_bPreserveZ = true;
    }

    return true;
}


bool CImageCompiler::IsFinalPixelFormatValidForNormalmaps(int platform) const
{
    const EPixelFormat format = m_Props.GetDestPixelFormat(m_Props.m_bPreserveAlpha, false, platform);

    // TODO: use data-driven approach - valid combinations should be specified in rc.ini, not hardcoded here

    if (format == ePixelFormat_3DC ||
        format == ePixelFormat_EAC_RG11 ||
        format == ePixelFormat_BC5 ||
        format == ePixelFormat_BC5s ||
        format == ePixelFormat_DXT1)
    {
        return true;
    }

    return false;
}


static void printColor(const char* msg, const float* c)
{
    RCLog("%s:", msg);
    RCLog("%.10f %.10f %.10f %.10f", c[0], c[1], c[2], c[3]);
    RCLog("%u %u %u %u", (uint)((c[0] * 255.0f) + 0.5f), (uint)((c[1] * 255.0f) + 0.5f), (uint)((c[2] * 255.0f) + 0.5f), (uint)((c[3] * 255.0f) + 0.5f));
}


uint32 CImageCompiler::_CalcTextureMemory(const EPixelFormat pixelFormat, const uint32 dwWidth, const uint32 dwHeight, const bool bCubemap, const bool bMips)
{
    if (pixelFormat < 0)
    {
        return 0;
    }
    const PixelFormatInfo* const pFormatInfo = CPixelFormats::GetPixelFormatInfo(pixelFormat);
    assert(pFormatInfo);

    const uint32 width = bCubemap ? dwWidth / 6 : dwWidth;
    const uint32 height = dwHeight;

    const uint32 mipCount = bMips ? CPixelFormats::ComputeMaxMipCount(pixelFormat, dwWidth, dwHeight, bCubemap) : 1;

    uint32 totalSize = 0;

    for (uint32 mip = 0; mip < mipCount; ++mip)
    {
        const uint32 w = Util::getMax(width >> mip, (uint32)1);
        const uint32 h = Util::getMax(height >> mip, (uint32)1);

        if (pFormatInfo->bCompressed)
        {
            const uint32 widthInBlocks = (w + (pFormatInfo->blockWidth - 1)) / pFormatInfo->blockWidth;
            const uint32 heightInBlocks = (h + (pFormatInfo->blockHeight - 1)) / pFormatInfo->blockHeight;
            static const uint32 pixelsPerBlock = pFormatInfo->blockWidth * pFormatInfo->blockHeight;
            totalSize += (widthInBlocks * heightInBlocks * pFormatInfo->bitsPerBlock) / 8;
        }
        else
        {
            totalSize += (w * h * pFormatInfo->bitsPerBlock) / 8;
        }
    }

    return bCubemap ? totalSize * 6 : totalSize;
}


void CImageCompiler::Release()
{
    delete this;
}


void CImageCompiler::SetCC(const ConvertContext& CC)
{
    m_CC = CC;
    m_Props.SetPropsCC(&m_CC, false);
}


//#include <crtdbg.h>       // to find memory leaks

void CImageCompiler::BeginProcessing(const IConfig* config)
{
    FreeImages();
    if (config->GetAsBool("userdialog", false, true))
    {
        InitDialogSystem();
    }
}

void CImageCompiler::EndProcessing()
{
    FreeImages();
    ReleaseDialogSystem();
}


static string AutoselectPreset(const ConvertContext& CC, const uint32 width, const uint32 height, const bool hasAlpha)
{
    const char* const defaultColorchart = "ColorChart";
    const char* const defaultBump       = "NormalsFromDisplacement";
    const char* const defaultNormalmap  = "Normals";
    const char* const defaultNormalmapGloss  = "NormalsWithSmoothness";
    const char* const defaultReflectance = "Reflectance";
    const char* const defaultCubemap    = "EnvironmentProbeHDR";
    const char* const defaultHDRCubemap = "EnvironmentProbeHDR";
    const char* const defaultPow2       = "Albedo";
    const char* const defaultPow2Alpha  = "AlbedoWithGenericAlpha";
    const char* const defaultNonpow2    = "ReferenceImage";

    const string fileName = CC.config->GetAsString("overwritefilename", CC.sourceFileNameOnly, CC.sourceFileNameOnly);

    string presetName;

    // TODO: make auto-preset assignment configurable in the rc.ini
    if (SuffixUtil::HasSuffix(fileName.c_str(), '_', "cch"))
    {
        presetName = defaultColorchart;
    }
    else if (SuffixUtil::HasSuffix(fileName.c_str(), '_', "bump"))
    {
        presetName = defaultBump;
    }
    else if (SuffixUtil::HasSuffix(fileName.c_str(), '_', "spec"))
    {
        presetName = defaultReflectance;
    }
    else if (SuffixUtil::HasSuffix(fileName.c_str(), '_', "ddn"))
    {
        presetName = defaultNormalmap;
    }
    else if (SuffixUtil::HasSuffix(fileName.c_str(), '_', "ddna"))
    {
        presetName = defaultNormalmapGloss;
    }
    else if (SuffixUtil::HasSuffix(fileName.c_str(), '_', "cm") || (StringHelpers::EndsWithIgnoreCase(CC.sourceFileNameOnly, ".hdr")))
    {
        uint32 face = 0;
        if (width * 1 == height * 6)
        {
            face = (height / 1);
        }
        else if (width * 2 == height * 4)
        {
            face = (height / 2);
        }
        else if (width * 3 == height * 4)
        {
            face = (height / 3);
        }
        else if (width * 4 == height * 3)
        {
            face = (height / 4);
        }

        if (face > 0)
        {
            if (StringHelpers::EndsWithIgnoreCase(CC.sourceFileNameOnly, ".hdr"))
            {
                presetName = defaultHDRCubemap;
            }
            else
            {
                presetName = defaultCubemap;
            }

            if (face > 256)
            {
                RCLogWarning(
                    "The cubemap %s is too large (%d x %d). This will take a lot of memory. Reduce the size of the cubemap.",
                    CC.sourceFileNameOnly.c_str(), width, height);
            }
        }
        else
        {
            presetName = defaultNonpow2;
        }
    }
    else if (Util::isPowerOfTwo(width) && Util::isPowerOfTwo(height))
    {
        presetName = hasAlpha ? defaultPow2Alpha : defaultPow2;
    }
    else
    {
        presetName = defaultNonpow2;
    }

    return presetName;
}


bool CImageCompiler::ProcessImplementation()
{
    const string sourceFile = m_CC.GetSourcePath();

    string settings;
    if (!LoadInput(sourceFile.c_str(), m_CC.convertorExtension.c_str(), true, settings))
    {
        RCLogError("%s: LoadInput(file: '%s', ext: '%s') failed", __FUNCTION__, sourceFile.c_str(), m_CC.convertorExtension.c_str());
        return false;
    }

    assert(m_pInputImage);

    const uint32 width  = m_pInputImage->GetWidth(0);
    const uint32 height = m_pInputImage->GetHeight(0);

    if ((width < 1) || (height < 1))
    {
        RCLogError("%s: Image file %s is broken: it has bad dimensions (%d x %d).",
            __FUNCTION__, sourceFile.c_str(), width, height);
        return false;
    }

    if (m_CC.pRC->GetVerbosityLevel() > 2)
    {
        RCLog("Image size: %d x %d", (int)width, (int)height);

        if (m_pInputImage->GetPixelFormat() == ePixelFormat_A32B32G32R32F)
        {
            ColorF c;
            m_pInputImage->GetComponentMaxima(c);
            RCLog("Max values of components: %g %g %g %g", c.r, c.g, c.b, c.a);
        }
    }

    if (m_CC.config->GetAsBool("showsettings", false, true))
    {
        RCLog("Settings read from '%s':", sourceFile.c_str());
        RCLog("  '%s'", settings.c_str());
        return true;
    }

    if (m_CC.config->HasKey("savesettings"))
    {
        settings = m_CC.config->GetAsString("savesettings", "", "");
        RCLog("Saving settings to '%s':", GetSettingsFile(sourceFile.c_str()).c_str());
        RCLog("  '%s'", settings.c_str());

        // Tiff
        if (StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "tif"))
        {
            return ImageTIFF::UpdateAndSaveSettingsToTIF(GetSettingsFile(sourceFile.c_str()).c_str(), settings);
        }
        else if (ImageQImage::IsExtensionSupported(m_CC.convertorExtension.c_str())) // New formats
        {
            return ImageQImage::UpdateAndSaveSettings(GetSettingsFile(sourceFile.c_str()).c_str(), settings);
        }
        else
        {
            RCLogError("Option \"savesettings\" applies only to certain file formats.");
            return false;
        }
    }

    if (m_CC.config->HasKey("savepreset"))
    {
        const string savePreset = m_CC.config->GetAsString("savepreset", "", "");
        RCLog("Saving preset to '%s'", GetOutputPath().c_str());
        if (savePreset.empty())
        {
            RCLogError("Option \"savepreset\" must specify the name of the preset to be set.");
            return false;
        }

        const string presetName = GetUnaliasedPresetName(savePreset);

        if (!StringHelpers::EqualsIgnoreCase(presetName, savePreset))
        {
            RCLog("Using preset name '%s' instead of '%s'", presetName.c_str(), savePreset.c_str());
        }

        if (!CheckForExistingPreset(m_CC, presetName, true))
        {
            return false;
        }

        bool bSavedConfig = false;
        if (StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "tif"))
        {
            bSavedConfig = ImageTIFF::UpdateAndSaveSettingsToTIF(GetSettingsFile(sourceFile.c_str()).c_str(), &m_Props, &settings, true);
        }
        else if (ImageQImage::IsExtensionSupported(m_CC.convertorExtension.c_str()))  // New formats
        {
            bSavedConfig = ImageQImage::UpdateAndSaveSettings(GetSettingsFile(sourceFile.c_str()).c_str(), &m_Props, &settings, true);
        }
        else
        {
            RCLogError("Option \"savepreset\" applies only to TIF files.");
            return false;
        }

        // change the preset
        SetPreset(presetName);
        SetPresetSettings();

        return bSavedConfig;
    }

    bool autopreset = false;
    string presetName = m_CC.config->GetAsString("preset", "", "", eCP_PriorityFile);
    if (StringHelpers::Equals(presetName, "*"))
    {
        presetName.clear();
    }
    else
    {
        presetName = GetUnaliasedPresetName(presetName);
    }

    if (presetName.empty() || !CheckForExistingPreset(m_CC, presetName, false))
    {
        autopreset = true;

        presetName = AutoselectPreset(m_CC, width, height, m_Props.m_bPreserveAlpha);
        presetName = GetUnaliasedPresetName(presetName);

        if (m_CC.pRC->GetVerbosityLevel() >= 0)
        {
            RCLog("Auto-selecting preset '%s' for file '%s' (%d x %d)",
                presetName.c_str(), sourceFile.c_str(), width, height);
        }

        if (!CheckForExistingPreset(m_CC, presetName, true))
        {
            return false;
        }
    }

    SetPreset(presetName);
    SetPresetSettings();

    // Getting rid of obsolete autooptimization feature.
    // We will modify "reduce" (if needed) and set "autooptimizefile=0" in
    // the file settings.
    {
        // Prevent *new* files from having autooptimization turned on
        // by setting "autooptimizefile=1".
        //
        // We detect that a file is new by checking if the settings string is empty.
        // Note that some of our *existing* files have no settings, so our detection
        // will treat them as *new*. It may produce image dimensions that are
        // different from the ones that were produced in the past. But, luckily
        // for us we don't have many files without settings and majority of such files
        // contains generated data tables which don't use mipmaps.
        // Anyway, our vision is that every image file should have settings.
        // In rare situations when settings are missing a manual user action
        // is recommended anyway ("rc <filename.tif> -userdialog", for example).
        if (settings.empty())
        {
            m_Props.SetAutoOptimizeFile(false);
        }

        // Get rid of "autooptimize" in *file* settings (note that "autooptimize" may
        // appear in file settings only because of a bug)
        if (m_CC.config->HasKey("autooptimize", eCP_PriorityFile))
        {
            // Let's replace "autooptimize=0" by "autooptimizefile=0" (result is the same)
            // or delete "autooptimize=1" (result is the same)
            if (!m_CC.config->GetAsBool("autooptimize", true, true, eCP_PriorityFile))
            {
                m_CC.multiConfig->setKeyValue(eCP_PriorityFile, "autooptimizefile", "0");
            }
            m_CC.multiConfig->setKeyValue(eCP_PriorityFile, "autooptimize", 0);
        }

        const bool bAutooptimize = m_CC.config->GetAsBool("autooptimize", true, true);
        if (m_CC.config->HasKey("autooptimize") && bAutooptimize)
        {
            RCLogError("Setting 'autooptimize' to 1 in rc.ini or command-line is not allowed");
            return false;
        }

        const bool bAutooptimizeFile = m_CC.config->GetAsBool("autooptimizefile", true, true);
        if (m_CC.config->HasKey("autooptimizefile", eCP_PriorityAll & (~eCP_PriorityFile)))
        {
            RCLogError("Specifying 'autooptimizefile' in rc.ini or command-line is not allowed");
            return false;
        }

        // Make sure that switching autooptimization off does not change dimensions of the
        // produced images. We modify 'reduce' setting to compensate it, if needed.
        if (bAutooptimize && bAutooptimizeFile && (width >= 1024 || height >= 1024))
        {
            CImageProperties::ReduceMap reduceMap;

            m_Props.GetReduceResolutionFile(reduceMap);

            for (CImageProperties::ReduceMap::iterator it = reduceMap.begin(); it != reduceMap.end(); ++it)
            {
                CImageProperties::ReduceItem& ri = it->second;
                if (ri.value <= 0)
                {
                    ++ri.value;
                }
            }

            m_Props.SetReduceResolutionFile(reduceMap);
        }

        // Turn off autooptimization
        m_Props.SetAutoOptimizeFile(false);
    }

    if (m_CC.config->GetAsBool("cleansettings", false, true))
    {
        RCLog("Settings read from '%s':", sourceFile.c_str());
        RCLog("  '%s'", settings.c_str());

        RCLog("Saving clean settings to '%s'...", GetOutputPath().c_str());

        if (StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "tif"))
        {
            return ImageTIFF::UpdateAndSaveSettingsToTIF(GetSettingsFile(sourceFile.c_str()).c_str(), &m_Props, &settings, true);
        }
        else if (ImageQImage::IsExtensionSupported(m_CC.convertorExtension.c_str())) // New formats
        {
            return ImageQImage::UpdateAndSaveSettings(GetSettingsFile(sourceFile.c_str()).c_str(), &m_Props, &settings, true);
        }
        else
        {
            RCLogError("Option \"cleansettings\" applies only to certain types of files.");
            return false;
        }
    }

    if (m_CC.config->GetAsBool("userdialog", false, true))
    {
        // we only require SCC if we get here.
        bool enableSourceControl = !(m_CC.config->GetAsBool("nosourcecontrol", false, false));

        if (enableSourceControl)
        {
            using SCConnectionBus = AzToolsFramework::SourceControlConnectionRequestBus;
            SCConnectionBus::Broadcast(&SCConnectionBus::Events::EnableSourceControl, true);
        }

        CImageUserDialog dialog;
        m_pImageUserDialog = &dialog;
        const bool bSuccess = dialog.DoModal(this);
        m_pImageUserDialog = NULL;
        return bSuccess;
    }
    else if (m_CC.config->GetAsBool("analyze", false, true))
    {
        return AnalyzeWithProperties(autopreset);
    }
    else
    {
        return RunWithProperties(true);
    }

    assert(0);
    return false;
}

bool CImageCompiler::ProcessWithAzFramework()
{
    AZ::IO::FileIOBase* prevIO = AZ::IO::FileIOBase::GetInstance();
    AZ::IO::FileIOBase::SetInstance(nullptr);
    AZ::IO::FileIOBase* fileIO = m_CC.pRC->GetSystemEnvironment()->pFileIO;
    AZ::IO::FileIOBase::SetInstance(fileIO);

    RCImageCompilerApplication app;
    app.Start(AZ::ComponentApplication::Descriptor());

    bool success = ProcessImplementation();

    app.Stop();

    AZ::IO::FileIOBase::SetInstance(nullptr);
    AZ::IO::FileIOBase::SetInstance(prevIO);

    return success;
}

bool CImageCompiler::Process()
{
    if (m_CC.config->GetAsBool("skipmissing", false, true) && !FileUtil::FileExists(m_CC.GetSourcePath()))
    {
        // If source file does not exist, ignore it.
        return true;
    }

    // Note: if the user asks for just showing/changing settings, then we:
    //   1) don't check filetime,
    //   2) don't set filetime of the output file only if the output file exists,
    //   3) don't register file output.
    const bool bSettings =
        m_CC.config->GetAsBool("showsettings", false, true) ||
        m_CC.config->GetAsBool("cleansettings", false, true) ||
        m_CC.config->HasKey("savesettings") ||
        m_CC.config->HasKey("savepreset");

    // Note: if the user uses dialog mode, then we don't do anything
    // regarding checking/setting filetime, registering file output etc.
    // (even if user clicked [Generate output] we will not set dates
    // of generated files - the button is just for debug (and it
    // cannot generate files for ALL PLATFORMS anyway)).
    const bool bDialog = m_CC.config->GetAsBool("userdialog", false, true);
    const bool bAnalyze = m_CC.config->GetAsBool("analyze", false, true);

    if (!bDialog && !bAnalyze && !bSettings &&
        !m_CC.bForceRecompiling &&
        UpToDateFileHelpers::FileExistsAndUpToDate(GetOutputPath(), m_CC.GetSourcePath()))
    {
        // The file is up-to-date
        m_CC.pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
        return true;
    }

    m_Props.SetToDefault(true);
    {
        bool bDisablePreview = false;
        if (bDialog)
        {
            const bool bPressed = (Qt::ShiftModifier & QApplication::queryKeyboardModifiers()) == Qt::ShiftModifier;

            if (m_CC.pRC->GetVerbosityLevel() >= 2)
            {
                RCLog("Shift key status: (%s)", (bPressed ? "pressed" : "not pressed"));
            }

            bDisablePreview = bPressed;
        }

        m_Props.SetPropsCC(&m_CC, bDisablePreview);
    }

    const bool bSuccess = ProcessWithAzFramework();

    if (!bDialog && !bAnalyze && !bSettings)
    {
        if (bSuccess)
        {
            if (!UpToDateFileHelpers::SetMatchingFileTime(GetOutputPath(), m_CC.GetSourcePath()))
            {
                return false;
            }
            m_CC.pRC->AddInputOutputFilePair(m_CC.GetSourcePath(), GetOutputPath());
        }
    }

    FreeImages();

    return bSuccess;
}

string CImageCompiler::GetOutputFileNameOnly() const
{
    //get the conversion contexts sourcefilename unless overwritefilename is set then use that
    const string sourceFileFinal = m_CC.config->GetAsString("overwritefilename", m_CC.sourceFileNameOnly.c_str(), m_CC.sourceFileNameOnly.c_str());

    //if its a source image and a save or clean setting is set
    if (IResourceCompilerHelper::IsSourceImageFormatSupported(m_CC.sourceFileNameOnly) &&
        (m_CC.config->GetAsBool("cleansettings", false, true) ||
         m_CC.config->HasKey("savesettings") ||
         m_CC.config->HasKey("savepreset")))
    {
        //then return the sourcefinal name with the conversion contexts ext
        return PathHelpers::ReplaceExtension(sourceFileFinal, PathUtil::GetExt(m_CC.sourceFileNameOnly));
    }
    else
    {
        //otherwise return sourcefinal name with dds for non streaming and $dds for streaming
        return PathHelpers::ReplaceExtension(sourceFileFinal, (m_CC.config->GetAsBool("streaming", false, true) ? "$dds" : "dds"));
    }
}

string CImageCompiler::GetOutputPath() const
{
    return PathHelpers::Join(m_CC.GetOutputFolder(), GetOutputFileNameOnly());
}

string CImageCompiler::GetSettingsFile(const char* currentFile) const
{
    const string sourceFileFinal = m_CC.config->GetAsString("settingsFile", currentFile, currentFile);
    return sourceFileFinal;
}

void CImageCompiler::AutoPreset()
{
    if (!m_CC.config->GetAsBool("autopreset", true, true))
    {
        return;
    }

    // Rule: textures in Textures/Terrain/Detail that are no _DDN should become TerrainDiffuseHighPassed
    {
        const char* szPattern = "Textures\\Terrain\\Detail\\";
        int iPattern = strlen(szPattern);
        string sPath = PathUtil::GetPath(m_CC.GetSourcePath());
        int iPath = sPath.length();

        if (iPath >= iPattern)
        {
            if (strnicmp(&sPath[iPath - iPattern], szPattern, iPattern) == 0)
            {
                if (!SuffixUtil::HasSuffix(m_CC.sourceFileNameOnly, '_', "ddn") &&
                    !SuffixUtil::HasSuffix(m_CC.sourceFileNameOnly, '_', "ddna"))
                {
                    string presetName = GetUnaliasedPresetName(m_Props.GetPreset());

                    if (presetName == "" || presetName == GetUnaliasedPresetName("Albedo"))
                    {
                        presetName = GetUnaliasedPresetName("Terrain_Albedo_HighPassed");

                        SetPreset(presetName);
                        SetPresetSettings();

                        RCLog("/autopreset applied to '%s': '%s'", m_CC.sourceFileNameOnly.c_str(), presetName.c_str());
                    }
                }
            }
        }
    }
}


bool CImageCompiler::AnalyzeWithProperties(bool autopreset, const char* szExtendFileName)
{
    const string sourceFile = m_CC.GetSourcePath();
    string sOutputFileName = GetOutputPath();

    // add a string to the filename (before the extension)
    if (szExtendFileName)
    {
        const string ext = PathHelpers::FindExtension(sOutputFileName);
        sOutputFileName = PathHelpers::RemoveExtension(sOutputFileName) + szExtendFileName + string(".") + ext;
    }

    {
        if (!m_pFinalImage)
        {
            const string source_ext = PathHelpers::FindExtension(sourceFile.c_str());

            string settings;
            if (!(m_pFinalImage = CImageCompiler::LoadImageFromFile(sOutputFileName.c_str(), source_ext.c_str(), settings)))
            {
                RCLogError("CImageCompiler::LoadImageFromFile() failed for '%s'", sOutputFileName.c_str());
                return false;
            }
        }

        ImageToProcess iimage(m_pInputImage->CopyImage());
        assert(iimage.get() != m_pInputImage);
        iimage.ConvertFormat(&m_Props, ePixelFormat_A32B32G32R32F);

        ImageToProcess oimage(m_pFinalImage->CopyImage());
        assert(oimage.get() != m_pFinalImage);
        oimage.ConvertFormat(&m_Props, ePixelFormat_A32B32G32R32F);

        // dynscaled analysis
        ImageObject::EAlphaContent eA = oimage.get()->ClassifyAlphaContent();
        ColorF cDevI;
        iimage.get()->IsPerfectGreyscale(&cDevI);
        bool bGI = (cDevI == ColorF(0.0f));
        ColorF cDevO;
        oimage.get()->IsPerfectGreyscale(&cDevO);
        bool bGO = (cDevO == ColorF(0.0f));
        Vec4 cMin, cMax;
        oimage.get()->GetColorRange(cMin, cMax);              //cMin.w = cMax.w = 0; bool bC = (cMin == cMax);

        // TODO: will fail for Long/Lat transformed images
        const uint32 mipI = CPixelFormats::ComputeMaxMipCount(m_pFinalImage->GetPixelFormat(), m_pInputImage->GetWidth(0), m_pInputImage->GetHeight(0), m_pFinalImage->GetCubemap() == ImageObject::eCubemap_Yes);
        const uint32 mipO = CPixelFormats::ComputeMaxMipCount(m_pFinalImage->GetPixelFormat(), m_pFinalImage->GetWidth(0), m_pFinalImage->GetHeight(0), false);
        const uint32 mipS = m_pFinalImage->GetMipCount();
        const int channel = CPixelFormats::GetPixelFormatInfo(m_pFinalImage->GetPixelFormat())->nChannels;
        const int hasalph = CPixelFormats::GetPixelFormatInfo(m_pFinalImage->GetPixelFormat())->bHasAlpha;
        const bool walpha = (channel >= 4 && hasalph) || m_pFinalImage->GetAttachedImage();

        static bool header = false;
        if (!header)
        {
            header = true;

            fprintf(stderr, "O,");
            fprintf(stderr, "Preset,");
            fprintf(stderr, "Type,");
            fprintf(stderr, "A/O sRGB,"); // auto sRGB vs. out sRGB
            fprintf(stderr, "In Fmt,");
            fprintf(stderr, "In W,In H,In M,");
            fprintf(stderr, "Out Fmt,");
            fprintf(stderr, "Out W,Out H,Out M,");
            fprintf(stderr, "Pers M,");
            fprintf(stderr, "Red OI,Red SO,");
            fprintf(stderr, "Dev I,Dev O,Band,"); // max intra color range in/out, greyscale banding in out
            fprintf(stderr, "Alpha,");
            fprintf(stderr, "Ren,");
            fprintf(stderr, "R Min,G Min,B Min,A Min,");
            fprintf(stderr, "R Max,G Max,B Max,A Max,");
            fprintf(stderr, "Gain,");
            fprintf(stderr, "Name\n");
        }

        bool optimizable = autopreset;

        optimizable = optimizable | (eA != ImageObject::eAlphaContent_Greyscale ? (walpha ? true : false) : false);

        fprintf(stderr, "%d,", optimizable ? 1 : 0);

        if (autopreset)
        {
            fprintf(stderr, "%s (Auto),", m_Props.GetPreset().c_str());
        }
        else
        {
            fprintf(stderr, "%s,", m_Props.GetPreset().c_str());
        }

        fprintf(stderr, "%s,", m_pFinalImage->GetCubemap() == ImageObject::eCubemap_Yes ? "2D Cube" : "2D");
        fprintf(stderr, "%d,", (m_Props.GetColorSpace().second != CImageProperties::eOutputColorSpace_Linear) * (m_pFinalImage->GetImageFlags() & CImageExtensionHelper::EIF_SRGBRead ? 1 : -1));
        fprintf(stderr, "%s,", CPixelFormats::GetPixelFormatInfo(m_pInputImage->GetPixelFormat())->szName);
        fprintf(stderr, "%d,%d,%d,", m_pInputImage->GetWidth(0), m_pInputImage->GetHeight(0), mipI);

        if (m_pFinalImage->GetAttachedImage())
        {
            fprintf(stderr, "%s/%s,", CPixelFormats::GetPixelFormatInfo(m_pFinalImage->GetPixelFormat())->szName, CPixelFormats::GetPixelFormatInfo(m_pFinalImage->GetAttachedImage()->GetPixelFormat())->szName);
        }
        else
        {
            fprintf(stderr, "%s,", CPixelFormats::GetPixelFormatInfo(m_pFinalImage->GetPixelFormat())->szName);
        }

        fprintf(stderr, "%d,%d,%d,", m_pFinalImage->GetWidth(0), m_pFinalImage->GetHeight(0), mipO);
        fprintf(stderr, "%d,", m_pFinalImage->GetNumPersistentMips());
        fprintf(stderr, "%d,%d,", mipO - mipI, mipS - mipO);
        fprintf(stderr, "%g,%g,%d,", channel >= 3 ? cDevI.Max() : -1.0f, channel >= 3 ? cDevO.Max() : -1.0f, (bGI && !bGO) ? 1 : 0);
        fprintf(stderr, "%s,", (eA == ImageObject::eAlphaContent_OnlyWhite ? (walpha ? "White" : "Void") : (eA == ImageObject::eAlphaContent_OnlyBlack ? (walpha ? "Black" : "Void") : (eA == ImageObject::eAlphaContent_OnlyBlackAndWhite ? "B/W" : "Grey"))));

        if (m_pFinalImage->GetAttachedImage())
        {
            fprintf(stderr, "%s/%s,", m_pFinalImage->GetImageFlags() & CImageExtensionHelper::EIF_RenormalizedTexture ? "No" : "Ra", m_pFinalImage->GetAttachedImage()->GetImageFlags() & CImageExtensionHelper::EIF_RenormalizedTexture ? "No" : "Ra");
        }
        else
        {
            fprintf(stderr, "%s,", m_pFinalImage->GetImageFlags() & CImageExtensionHelper::EIF_RenormalizedTexture ? "Norm" : "Raw");
        }

#define log2(x) (log(x) / log(2))
        fprintf(stderr, "%g,%g,%g,%g,", cMin.x, cMin.y, cMin.z, cMin.w);
        fprintf(stderr, "%g,%g,%g,%g,", cMax.x, cMax.y, cMax.z, cMax.w);
        fprintf(stderr, "%g,", -log2(cMax.x - cMin.x) + -log2(cMax.y - cMin.y) + -log2(cMax.z - cMin.z) + -log2(cMax.w - cMin.w));
        fprintf(stderr, "%s\n", sOutputFileName.c_str());
    }

    return true;
}

bool CImageCompiler::RunWithProperties(bool inbSave, const char* szExtendFileName)
{
    m_Progress.Start();

    if (m_CC.pRC->GetVerbosityLevel() > 5)
    {
        RCLog("RunWithProperties(%s, %s)", (inbSave ? "true" : "false"), (szExtendFileName ? szExtendFileName : "<null>"));
    }

    const bool bSplitForStreaming = m_CC.config->GetAsBool("streaming", false, true);
    if (bSplitForStreaming)
    {
        if (!m_pTextureSplitter)
        {
            m_pTextureSplitter = new CTextureSplitter;
        }
    }

    m_bInternalPreview = (!inbSave && m_pImageUserDialog);

    AutoPreset();

    {
        const EPixelFormat srcFormat = m_pInputImage->GetPixelFormat();
        if (!CPixelFormats::IsPixelFormatUncompressed(srcFormat))
        {
            RCLogWarning("Skipping RunWithProperties() for '%s' because it is compressed", m_CC.sourceFileNameOnly.c_str());
            return true;    // do not process
        }
    }

    if (m_pInputImage->HasImageFlags(CImageExtensionHelper::EIF_Volumetexture))
    {
        return true;    // do not process
    }

    if (SuffixUtil::HasSuffix(m_CC.sourceFileNameOnly, '_', "ddn") ||
        SuffixUtil::HasSuffix(m_CC.sourceFileNameOnly, '_', "ddna") ||
        SuffixUtil::HasSuffix(m_CC.sourceFileNameOnly, '_', "bump"))
    {
        const int platform = m_CC.multiConfig->getActivePlatform();
        if (!IsFinalPixelFormatValidForNormalmaps(platform))
        {
            RCLogWarning("'%s' (ddn/bump) used not normal-map texture format (bad lighting on DX10 and console)", m_CC.sourceFileNameOnly.c_str());
        }
    }

    string sOutputFileName = GetOutputPath();

    // add a string to the filename (before the extension)
    if (szExtendFileName)
    {
        const string ext = PathHelpers::FindExtension(sOutputFileName);
        sOutputFileName = PathHelpers::RemoveExtension(sOutputFileName) + szExtendFileName + string(".") + ext;
    }

    if (m_pFinalImage)
    {
        assert(m_pFinalImage != m_pInputImage);
        delete m_pFinalImage;
        m_pFinalImage = 0;
    }

    if (inbSave && m_CC.config->GetAsBool("nooutput", false, true))
    {
        RCLogError("%s: Process save Ext:'%s' /nooutput was specified", __FUNCTION__, m_CC.convertorExtension.c_str());
        inbSave = false;
    }

    if (m_Props.GetMipRenormalize() &&
        (m_Props.GetColorSpace().first != CImageProperties::eInputColorSpace_Linear ||
         m_Props.GetColorSpace().second != CImageProperties::eOutputColorSpace_Linear))
    {
        RCLogError("%s: Incompatible settings: mip renormalization is enabled together with sRGB mode", __FUNCTION__);
        return false;
    }

    if (m_Props.GetCubemap() && !m_pInputImage->HasCubemapCompatibleSizes())
    {
        RCLogError("%s: Image can't be converted to a cubemap: %dx%d", __FUNCTION__, m_pInputImage->GetWidth(0), m_pInputImage->GetHeight(0));
        return false;
    }

    if (m_Props.GetPowOf2() && !(m_pInputImage->HasPowerOfTwoSizes() || m_Props.GetCubemap()))
    {
        RCLogError("%s: Image sizes are not power-of-two: %dx%d (width and height needs to be one of ..,16,32,64,128,256,..)", __FUNCTION__, m_pInputImage->GetWidth(0), m_pInputImage->GetHeight(0));
        return false;
    }

    ImageToProcess image(m_pInputImage->CopyImage());
    uint32 inputWidth  = m_pInputImage->GetWidth(0);
    uint32 inputHeight = m_pInputImage->GetHeight(0);

    // rescan input for alpha
    m_Props.m_bPreserveAlpha = image.get()->HasNonOpaqueAlpha();

    // initial checks for source image
    if (m_pImageUserDialog)
    {
        AnalyzeImageAndSuggest(image.getConst());
    }

    // multi-stage
    {
        if (m_CC.config->GetAsBool("colorchart", false, true))
        {
            IColorChart* pCC = Create3dLutColorChart();
            if (!pCC)
            {
                return false;
            }

            if (!pCC->GenerateFromInput(image.get()))
            {
                pCC->GenerateDefault();
            }

            if (!image.set(pCC->GenerateChartImage()))
            {
                return false;
            }

            SAFE_RELEASE(pCC);
        }

        // note: the function also expands all supported raw formats to ARGB32F
        image.GammaToLinearRGBA32F(m_Props.GetColorSpace().first == CImageProperties::eInputColorSpace_Srgb);
        assert(image.get()->GetPixelFormat() == ePixelFormat_A32B32G32R32F);

        if (!image.get()->Swizzle(m_CC.config->GetAsString("swizzle", "", "").c_str()))
        {
            return false;
        }

        // Set alpha to 1.0 if "discardalpha" is requested.
        // We do it to prevent input alpha data appearing in the output (when the
        // destination pixel format does not support complete alpha removal).
        if (m_Props.GetDiscardAlpha())
        {
            if (!image.get()->Swizzle("rgb1"))
            {
                return false;
            }
        }

        if (m_Props.GetGlossLegacyDistribution())
        {
            image.get()->ConvertLegacyGloss();
        }

        // Convert probe if needed
        // Set images' property to be cubemap from here on
        if (m_Props.GetCubemap())
        {
            assert(inputWidth  == image.get()->GetWidth(0));
            assert(inputHeight == image.get()->GetHeight(0));

            image.ConvertProbe(m_Props.GetPowOf2());

            inputWidth  = image.get()->GetWidth(0);
            inputHeight = image.get()->GetHeight(0);
        }
        else
        {
            image.get()->SetCubemap(ImageObject::eCubemap_No);
        }

        assert(image.get()->GetCubemap() != ImageObject::eCubemap_UnknownYet);

        // Make image square if needed
        if (image.get()->GetCubemap() != ImageObject::eCubemap_Yes)
        {
            const EPixelFormat dstFormat = m_Props.GetDestPixelFormat(m_Props.m_bPreserveAlpha, m_bInternalPreview, m_CC.platform);

            if (dstFormat >= 0 && CPixelFormats::GetPixelFormatInfo(dstFormat)->bSquarePow2)
            {
                uint32 width, height, mips;
                image.get()->GetExtent(width, height, mips);
                assert(mips == 1);

                if (!Util::isPowerOfTwo(width) || !Util::isPowerOfTwo(height))
                {
                    RCLogError("%s Image sizes are not power-of-two: %dx%d (width and height needs to be one of ..,16,32,64,128,256,..)", __FUNCTION__, width, height);
                    return false;
                }

                // TODO: make a command-line option to choose between upscaling and downscaling
                const bool bDownscale = false;
                while (width != height)
                {
                    if (bDownscale)
                    {
                        (width > height) ? image.DownscaleTwiceHorizontally() : image.DownscaleTwiceVertically();
                    }
                    else
                    {
                        (width < height) ? image.UpscalePow2TwiceHorizontally() : image.UpscalePow2TwiceVertically();
                    }

                    image.get()->GetExtent(width, height, mips);
                }
            }
        }

        {
            const int minAlpha = Util::getClamped(m_Props.GetMinimumAlpha(), 0, 255);
            if (minAlpha > 0)
            {
                image.get()->ClampMinimumAlpha(minAlpha / 255.0f);
            }
        }

        // BumpToNormal postprocess
        if (m_Props.m_BumpToNormal.GetBumpToNormalFilterIndex() > 0)
        {
            if (image.get()->GetCubemap() == ImageObject::eCubemap_Yes)
            {
                RCLogError("CImageCompiler::BumpToNormal failed (cubemaps are not supported)");
                image.set(0);
                return false;
            }

            if (m_Props.GetColorSpace().second != CImageProperties::eOutputColorSpace_Linear)
            {
                RCLogWarning("%s: 'bump to normal' writing sRGB image will produce bad normals. check/fix presets.", m_CC.sourceFileNameOnly.c_str());
            }

            image.BumpToNormalMap(m_Props.m_BumpToNormal, false); // value from RGB luminance

            if (!image.get())
            {
                return false;
            }
        }

        // Alpha as bump
        if (m_Props.m_AlphaAsBump.GetBumpToNormalFilterIndex() > 0)
        {
            if (image.get()->GetCubemap() == ImageObject::eCubemap_Yes)
            {
                RCLogError("CImageCompiler::BumpToNormal failed (cubemaps are not supported)");
                image.set(0);
                return false;
            }

            ImageToProcess image2(image.get()->CopyImage());
            image2.BumpToNormalMap(m_Props.m_AlphaAsBump, true);   // value from alpha

            if (!image.get())
            {
                return false;
            }

            // alpha2bump suppress the alpha channel export - this behavior saves memory
            m_Props.m_bPreserveAlpha = false;

            // both need to be in range 0..1
            image.AddNormalMap(image2.get());
        }

        // Compute luminance and put it into alpha channel
        if (m_CC.config->GetAsBool("lumintoalpha", false, true))
        {
            image.get()->GetLuminanceInAlpha();

            m_Props.m_bPreserveAlpha = true;
        }

        if (m_Props.GetGlossFromNormals() && m_Props.GetMipRenormalize())
        {
            // Normalize the base mip map. This has to be done explicitly because we need to disable mip renormalization to
            // preserve the normal length when deriving the normal variance
            image.get()->NormalizeVectors(0, 1);
        }

        // generate mipmaps and/or reduce resolution and/or compute average color
        if (image.get()->GetCubemap() == ImageObject::eCubemap_Yes)
        {
            SCubemapFilterParams params;

            params.FilterType = m_Props.GetCubemapFilterType();
            params.BaseFilterAngle = m_Props.GetCubemapFilterAngle();
            params.InitialMipAngle = m_Props.GetCubemapMipFilterAngle();
            params.MipAnglePerLevelScale = m_Props.GetCubemapMipFilterSlope();
            params.FixupWidth = m_Props.GetCubemapEdgeFixupWidth();
            params.FixupType = (params.FixupWidth > 0) ? CP_FIXUP_PULL_LINEAR : CP_FIXUP_NONE;
            params.SampleCountGGX = m_Props.GetCubemapGGXSampleCount();
            params.BRDFGlossScale = m_Props.GetBRDFGlossScale();
            params.BRDFGlossBias = m_Props.GetBRDFGlossBias();

            CreateCubemapMipMaps(
                image,
                params,
                m_Props.GetRequestedResolutionReduce(inputWidth, inputHeight),
                !m_Props.GetMipMaps());
        }
        else
        {
            bool mipRenormalize = m_Props.GetMipRenormalize() && !m_Props.GetGlossFromNormals();

            CreateMipMaps(
                image,
                m_Props.GetRequestedResolutionReduce(inputWidth, inputHeight),
                !m_Props.GetMipMaps(),
                mipRenormalize);
        }

        if (!image.get())
        {
            return false;
        }

        //{
        //printColor("post mipmap", (float*)&image.get()->GetPixel<Color4<float> >(0, 149, 287));
        //}

#if 0
        if (m_CC.config->GetAsBool("normaltoheight", false, true))
        {
            image.NormalToHeight();
        }
#endif

        if (m_Props.GetGlossFromNormals())
        {
            image.get()->GlossFromNormals(m_Props.m_bPreserveAlpha);

            m_Props.m_bPreserveAlpha = true;

            if (m_Props.GetMipRenormalize())
            {
                image.get()->NormalizeVectors(1, 100);
            }
        }

        // high pass subtract mip level is subtracted when applying the [cheap] high pass filter - this prepares assets to allow this in the shader
        {
            const int mipDownCount = m_Props.GetHighPass();
            if (mipDownCount > 0)
            {
                image.CreateHighPass(mipDownCount);
            }
        }

        // calculate brightness before RGBK compression
        const float avgBrightness = image.get()->CalculateAverageBrightness();
        image.get()->SetAverageBrightness(avgBrightness);

        // force alpha channel for RGBK compression
        if (m_Props.GetRGBKCompression() > 0)
        {
            m_Props.m_bPreserveAlpha = true;
        }

        EPixelFormat iDestPixelFormat = m_Props.GetDestPixelFormat(m_Props.m_bPreserveAlpha, m_bInternalPreview, m_CC.platform);

        // convert HDR texture to RGBK
        if (m_Props.GetRGBKCompression() > 0)
        {
            image.CompressRGB32FToRGBK8(&m_Props, m_Props.GetRGBKMaxValue(), iDestPixelFormat == ePixelFormat_DXT5, m_Props.GetRGBKCompression());
        }
        else
        {
            image.ConvertModel(&m_Props, m_Props.GetColorModel());
        }

        // normalize image's range if necessary
        // this must be done before conversion to gamma, or the image would be sampled wrong by the hardware
        if (!m_Props.GetRGBKCompression() && (m_Props.GetNormalizeRange() || m_Props.GetNormalizeRangeAlpha()))
        {
            // expand range from [0,1] to [0,2^(2^4-1)] for the floating-point formats
            // to maximize the use of the coding-space, and to prevent discarding
            // mantissa-bits of values with low magnitude
            int nExponentBits = 0;
            if (CPixelFormats::IsFormatFloatingPoint(iDestPixelFormat, false))
            {
                nExponentBits = 4;
            }

            // in case alpha isn't used as a weight for compression we can normalize it just fine
            ImageObject::EColorNormalization eColorNorm = ImageObject::eColorNormalization_Normalize;
            ImageObject::EAlphaNormalization eAlphaNorm = ImageObject::eAlphaNormalization_Normalize;

            if (!m_Props.GetNormalizeRangeAlpha() || CPixelFormats::IsFormatWithWeightingAlpha(iDestPixelFormat))
            {
                eAlphaNorm = ImageObject::eAlphaNormalization_PassThrough;
            }

            if (!m_Props.GetNormalizeRange())
            {
                eColorNorm = ImageObject::eColorNormalization_PassThrough;
            }

            if (!m_Props.m_bPreserveAlpha || m_Props.GetDiscardAlpha() || CPixelFormats::IsFormatWithoutAlpha(iDestPixelFormat))
            {
                eAlphaNorm = ImageObject::eAlphaNormalization_SetToZero;
            }

            image.get()->NormalizeImageRange(eColorNorm, eAlphaNorm, false, nExponentBits);
        }

        // convert linear to sRGB, if needed
        switch (m_Props.GetColorSpace().second)
        {
        case CImageProperties::eOutputColorSpace_Linear:
            // linear->linear: nothing to do
            break;
        case CImageProperties::eOutputColorSpace_Srgb:
            image.LinearRGBAAnyFToGammaRGBAAnyF();
            break;
        case CImageProperties::eOutputColorSpace_Auto:
            // Let's convert to sRGB dark images only (converting bright images decreases image quality)
        {
            bool bThresholded = false;
            {
                Histogram<256> histogram;
                if (image.get()->ComputeLuminanceHistogramForAnyRGB(histogram))
                {
                    const size_t medianBinIndex = 116;
                    const float percentage = histogram.getPercentage(medianBinIndex, 255);

                    // The image has significant amount of dark pixels, it's good to use sRGB
                    bThresholded = (percentage < 50.0f);
                }
            }

            if (bThresholded)
            {
                bool convertToSRGB = true;

                // if the image is DXT1 compressable (width and height are a multiple of 4), additionally estimate the conversion error
                // to only convert if it doesn't introduce error
                if (((image.get()->GetWidth(0) & 3) == 0) && ((image.get()->GetHeight(0) & 3) == 0))
                {
                    float fErrorLinearDXT1 = 0.0f;
                    float fErrorSRGBDXT1 = 0.0f;

                    image.get()->GetDXT1GammaCompressionError(&m_Props, &fErrorLinearDXT1, &fErrorSRGBDXT1);

                    if (m_CC.pRC->GetVerbosityLevel() > 2)
                    {
                        RCLog("sRGB vs. Linear is %f vs. %f => %d,%d => %s", m_CC.sourceFileNameOnly.c_str(), fErrorSRGBDXT1, fErrorLinearDXT1, fErrorSRGBDXT1 < fErrorLinearDXT1, bThresholded, fErrorSRGBDXT1 < fErrorLinearDXT1 ? "sRGB" : "Linear");
                    }

                    // Don't convert if it would lower the image quality when saved as sRGB according to GetDXT1GammaCompressionError()
                    if (fErrorSRGBDXT1 >= fErrorLinearDXT1)
                    {
                        convertToSRGB = false;
                    }
                }

                // our final conclusion: if the texture had a significant percentage of dark pixels and, if applicable, it was DXT1 compressable and gamma compression wouldn't introduce error,
                // then we convert it to sRGB
                if (convertToSRGB)
                {
                    image.LinearRGBAAnyFToGammaRGBAAnyF();
                }
            }
        }
        break;
        default:
            assert(0);
            break;
        }

        // compress result
        if (iDestPixelFormat >= 0)
        {
            EPixelFormat format = iDestPixelFormat;

            if (m_CC.config->GetAsBool("outputuncompressed", false, true))
            {
                switch (format)
                {
                case ePixelFormat_DXT1:
                case ePixelFormat_BC1:
                    format = ePixelFormat_X8R8G8B8;
                    break;
                case ePixelFormat_DXT1a:
                case ePixelFormat_DXT3:
                case ePixelFormat_DXT3t:
                case ePixelFormat_DXT5:
                case ePixelFormat_DXT5t:
                case ePixelFormat_BC1a:
                case ePixelFormat_BC2:
                case ePixelFormat_BC2t:
                case ePixelFormat_BC3:
                case ePixelFormat_BC3t:
                case ePixelFormat_BC7:
                case ePixelFormat_BC7t:
                    format = ePixelFormat_A8R8G8B8;
                    break;
                case ePixelFormat_3DCp:
                case ePixelFormat_BC4:
                case ePixelFormat_BC4s:
                    format = ePixelFormat_R8;
                    break;
                case ePixelFormat_BC6UH:
                    format = ePixelFormat_A16B16G16R16F;
                    break;
                case ePixelFormat_3DC:
                case ePixelFormat_BC5:
                case ePixelFormat_BC5s:
                    format = ePixelFormat_G16R16F;
                    break;
                default:
                    break;
                }
            }

            if (m_Props.GetAutoDetectLuminance())
            {
                // check if alpha can be dropped completely, white is returned by texture samplers by default
                const bool bAlphaChannelIsMissingOrWhite =
                    CPixelFormats::IsPixelFormatWithoutAlpha(format) ||
                    m_Props.GetDiscardAlpha() ||
                    !image.get()->HasNonOpaqueAlpha();
                const bool bRGBChannelsAreEqual =
                    image.get()->IsPerfectGreyscale();

                if (bRGBChannelsAreEqual)
                {
                    // new generation consoles and PC support BC7 textures (but not A8L8/L8)
                    if (CPixelFormats::IsPixelFormatForExtendedDDSOnly(format))
                    {
                        RCLog("CImageCompiler: Image with all equal channels and white alpha detected - using BC7-format");
                        format = ePixelFormat_BC7;
                    }
                    // otherwise we don't have an appropriate format to store single channel (+ alpha) textures
                }
            }

            if (m_Props.GetAutoDetectBlackAndWhiteAlpha())
            {
                const ImageObject::EAlphaContent eAC = image.get()->ClassifyAlphaContent();

                // check if alpha can be dropped completely, white is returned by texture samplers by default
                const bool bAlphaChannelIsMissingOrWhite =
                    CPixelFormats::IsPixelFormatWithoutAlpha(format) ||
                    m_Props.GetDiscardAlpha() ||
                    !(eAC == ImageObject::eAlphaContent_Greyscale ||
                      eAC == ImageObject::eAlphaContent_OnlyBlack ||
                      eAC == ImageObject::eAlphaContent_OnlyBlackAndWhite);
                // check if alpha can be put into a 1bit black and white mask
                const bool bAlphaChannelCanBeStoredWith1bit =
                    CPixelFormats::IsFormatWithThresholdAlpha(format) ||
                    !(eAC == ImageObject::eAlphaContent_Greyscale);

                EPixelFormat formatBefore = format;
                switch (format)
                {
                case ePixelFormat_DXT1:
                case ePixelFormat_DXT3:
                case ePixelFormat_DXT5:
                    format =
                        (bAlphaChannelIsMissingOrWhite    ? ePixelFormat_DXT1 : format);
                    break;
                case ePixelFormat_DXT1a:
                case ePixelFormat_DXT3t:
                case ePixelFormat_DXT5t:
                    format =
                        (bAlphaChannelIsMissingOrWhite    ? ePixelFormat_DXT1 :
                         (bAlphaChannelCanBeStoredWith1bit ? ePixelFormat_DXT1a : format));
                    break;
                case ePixelFormat_BC1:
                case ePixelFormat_BC2:
                case ePixelFormat_BC3:
                    format =
                        (bAlphaChannelIsMissingOrWhite    ? ePixelFormat_BC1 : format);
                    break;
                case ePixelFormat_BC1a:
                case ePixelFormat_BC2t:
                case ePixelFormat_BC3t:
                    format =
                        (bAlphaChannelIsMissingOrWhite    ? ePixelFormat_BC1 :
                         (bAlphaChannelCanBeStoredWith1bit ? ePixelFormat_BC1a : format));
                    break;
                }

                if (formatBefore != format)
                {
                    const char* const szAlphaContent = (bAlphaChannelIsMissingOrWhite ? "white or no" : "black and white");
                    const char* const szFormatGeneration = (!CPixelFormats::IsPixelFormatForExtendedDDSOnly(format) ? "DXT1" : "BC1");
                    const char* const szFormatSuffix = (bAlphaChannelIsMissingOrWhite ? "" : "a");

                    RCLog("CImageCompiler: Image with %s alpha detected - using %s%s-format", szAlphaContent, szFormatGeneration, szFormatSuffix);
                }
            }

            image.ConvertFormat(&m_Props, format);

            if (!image.get())
            {
                return false;
            }
        }

        // Drop mips of attached alpha if requested to save memory
        // Be cautious with this option when generating gloss from normals as close surfaces might appear too rough
        {
            ImageObject* const pAttachedImage = image.get()->GetAttachedImage();
            if (pAttachedImage)
            {
                for (int i = 0; i < m_Props.GetReduceAlpha(); ++i)
                {
                    pAttachedImage->DropTopMip();
                }
            }
        }
    }

    if (image.get()->GetAttachedImage())
    {
        image.get()->AddImageFlags(CImageExtensionHelper::EIF_AttachedAlpha);
    }

    if (image.get()->GetCubemap() == ImageObject::eCubemap_Yes)
    {
        image.get()->AddImageFlags(CImageExtensionHelper::EIF_Cubemap);
    }

    ImageObject* walk = image.get();
    while (walk)
    {
        Vec4 minColor, maxColor;
        walk->GetColorRange(minColor, maxColor);

        bool renormed =
            (minColor.x != 0.0f && minColor.x != 1.0f) ||
            (minColor.y != 0.0f && minColor.y != 1.0f) ||
            (minColor.z != 0.0f && minColor.z != 1.0f) ||
            (minColor.w != 0.0f && minColor.w != 1.0f) ||
            (maxColor.x != 0.0f && maxColor.x != 1.0f) ||
            (maxColor.y != 0.0f && maxColor.y != 1.0f) ||
            (maxColor.z != 0.0f && maxColor.z != 1.0f) ||
            (maxColor.w != 0.0f && maxColor.w != 1.0f);
        if (renormed)
        {
            walk->AddImageFlags(CImageExtensionHelper::EIF_RenormalizedTexture);
        }
        else
        {
            walk->RemoveImageFlags(CImageExtensionHelper::EIF_RenormalizedTexture);
        }

        walk = walk->GetAttachedImage();
    }

    {
        const uint32 nNumStreamableMips = m_CC.config->GetAsInt("numstreamablemips", 100, 100);
        const uint32 nNumMips = image.get()->GetMipCount();

        // The engine will treat small textures as non streamable. In which case there's
        // no point splitting the texture.
        const uint32 nMaxSize = 1 << (CTextureSplitter::kNumLastMips + 2);
        const bool bStreamable = (image.get()->GetWidth(0) > nMaxSize) && (image.get()->GetHeight(0) > nMaxSize);

        const uint32 nNumPersistentMips = bStreamable
            ? (nNumMips >= nNumStreamableMips ? nNumMips - nNumStreamableMips : 0)
            : nNumMips;

        image.get()->SetNumPersistentMips(nNumPersistentMips);
    }

    assert(m_pFinalImage == 0);
    m_pFinalImage = image.get();
    image.forget();

    if (inbSave)
    {
        const char* pExt = "DDS";
        if (m_Props.GetImageCompressor() == CImageProperties::eImageCompressor_Tif)
        {
            pExt = "TIF";
        }
        else if (m_Props.GetImageCompressor() == CImageProperties::eImageCompressor_Hdr)
        {
            pExt = "HDR";
        }

        // destination
        if (!SaveOutput(m_pFinalImage, pExt, sOutputFileName.c_str()))
        {
            RCLogError("CImageCompiler::SaveOutput() failed for '%s'", sOutputFileName.c_str());
            return false;
        }
    }

    // for reflection cubemaps - generate the second diffuse cubemap
    if ((m_pFinalImage->GetCubemap() == ImageObject::eCubemap_Yes) && !m_CC.config->GetAsBool("userdialog", false, true))
    {
        string presetName = m_Props.GetCubemapGenerateDiffuse();
        presetName = GetUnaliasedPresetName(presetName);

        if (!presetName.empty() && (!szExtendFileName || strcmp(szExtendFileName, "_diff") != 0))
        {
            RCLog("Generating diffuse cubemap (_diff)");
            m_CC.multiConfig->setKeyValue(eCP_PriorityHighest, "autooptimize", "0");

            SetPreset(presetName);
            SetPresetSettings();

            ClearInputImageFlags();
            RunWithProperties(inbSave, "_diff");
        }
    }

    if (bSplitForStreaming)
    {
        // When streaming splitting requested run texture splitter in place.
        const string outFile = GetOutputPath();
        ConvertContext cc_split(m_CC);

        string sourceFileFinal;
        if (szExtendFileName)
        {
            // Faking source filename to generate output filename with respect to postfix
            const string outFilename = PathHelpers::GetFilename(outFile);
            sourceFileFinal = PathHelpers::RemoveExtension(outFilename);
            sourceFileFinal += szExtendFileName;

            const string ext = PathHelpers::FindExtension(outFilename);
            if (!ext.empty())
            {
                sourceFileFinal += '.';
                sourceFileFinal += ext;
            }
        }
        else
        {
            sourceFileFinal = PathHelpers::GetFilename(outFile);
        }

        cc_split.SetSourceFileNameOnly(sourceFileFinal);
        cc_split.SetSourceFolder(PathHelpers::GetDirectory(outFile));

        ConvertContext* const p = (ConvertContext*)m_pTextureSplitter->GetConvertContext();
        *p = cc_split;

        m_pTextureSplitter->SetOverrideSourceFileName(m_CC.GetSourcePath().c_str());
        m_pTextureSplitter->Process();
    }

    m_Progress.Finish();

    return true;
}


void CImageCompiler::ClearInputImageFlags()
{
    m_Props.ClearInputImageFlags();
}


void CImageCompiler::GetFinalImageInfo(int platform, EPixelFormat& finalFormat, bool& bCubemap, uint32& mipCount, uint32& mipReduce, uint32& width, uint32& height) const
{
    finalFormat = (EPixelFormat) - 1;
    bCubemap = false;
    mipCount = 0;
    mipReduce = 0;
    width = 0;
    height = 0;

    EPixelFormat inputFormat;

    const PixelFormatInfo* pInputFormatInfo = 0;
    const PixelFormatInfo* pFinalFormatInfo = 0;

    uint32 dwInputWidth;
    uint32 dwInputHeight;

    {
        inputFormat = m_pInputImage->GetPixelFormat();
        if (inputFormat < 0)
        {
            return;
        }

        pInputFormatInfo = CPixelFormats::GetPixelFormatInfo(inputFormat);
        assert(pInputFormatInfo);

        uint32 dwMips;

        m_pInputImage->GetExtentTransformed(dwInputWidth, dwInputHeight, dwMips, m_Props);
    }
   
    {
        finalFormat = m_Props.GetDestPixelFormat(m_Props.m_bPreserveAlpha, false, platform);
        if (finalFormat < 0)
        {
            return;
        }

        pFinalFormatInfo = CPixelFormats::GetPixelFormatInfo(finalFormat);
        assert(pFinalFormatInfo);
    }

    bCubemap = m_Props.GetCubemap(platform) && m_pInputImage->HasCubemapCompatibleSizes();

    const uint32 dwMaxMipCount = CPixelFormats::ComputeMaxMipCount(finalFormat, dwInputWidth, dwInputHeight, bCubemap);
    const uint32 dwWantedReduce = m_Props.GetRequestedResolutionReduce(dwInputWidth, dwInputHeight, platform);
    uint32 dwFinalReduce = Util::getMin(dwWantedReduce, dwMaxMipCount - 1U);
    uint32 dwFinalMipCount = dwMaxMipCount - dwFinalReduce;

    uint32 dwFinalWidth = Util::getMax(dwInputWidth >> dwFinalReduce, 1U);
    uint32 dwFinalHeight = Util::getMax(dwInputHeight >> dwFinalReduce, 1U);

    //Apply maxtexturesize if it is specified
    const int maxTextureSize = m_Props.GetMaxTextureSize();
    const int minTextureSize = m_Props.GetMinTextureSize();
   
    if (maxTextureSize > 0)
    {
        while ((dwFinalWidth > (uint32)maxTextureSize) || (dwFinalHeight > (uint32)maxTextureSize))
        {
            dwFinalWidth = Util::getMax(dwFinalWidth >> 1, 1U);
            dwFinalHeight = Util::getMax(dwFinalHeight >> 1, 1U);

            --dwFinalMipCount;
            ++dwFinalReduce;
        }
    }

    if (minTextureSize > 0)
    {
        while ((dwFinalWidth < (uint32)minTextureSize) || (dwFinalHeight < (uint32)minTextureSize))
        {
            dwFinalWidth = Util::getMax(dwFinalWidth << 1, 1U);
            dwFinalHeight = Util::getMax(dwFinalHeight << 1, 1U);

            ++dwFinalMipCount;
            --dwFinalReduce;
        }
    }

    dwFinalMipCount = (m_Props.GetMipMaps(platform) ? Util::getMax(dwFinalMipCount, 1U) : 1U);

    mipCount = dwFinalMipCount;
    mipReduce = dwFinalReduce;
    width = dwFinalWidth;
    height = dwFinalHeight;
}


string CImageCompiler::GetInfoStringUI(const bool inbOrig) const
{
    if (!m_pInputImage)
    {
        return "Input image is not loaded";
    }

    QString str;

    if (inbOrig)
    {
        const EPixelFormat format = m_pInputImage->GetPixelFormat();
        if (format < 0)
        {
            return "Format not recognized";
        }

        const PixelFormatInfo* const pFormatInfo = CPixelFormats::GetPixelFormatInfo(format);
        assert(pFormatInfo);

        uint32 dwWidth, dwHeight, dwMips;
        m_pInputImage->GetExtentTransformed(dwWidth, dwHeight, dwMips, m_Props);

        const uint32 dwMem = CalcTextureMemory(m_pInputImage);

        str = QString("%1x%2 Fmt:%3 Alpha:%4\nMem:%5kB").arg(dwWidth).arg(dwHeight).arg(pFormatInfo->szName).arg(pFormatInfo->szAlpha).arg(dwMem / 1024.0f, 0, 'f', 1);
    }
    else if (!m_pFinalImage)
    {
        str = QString();
    }
    else
    {
        EPixelFormat finalFormat;
        bool bFinalCubemap;
        uint32 dwFinalMipCount;
        uint32 dwFinalReduce;
        uint32 dwFinalWidth;
        uint32 dwFinalHeight;

        GetFinalImageInfo(m_CC.platform, finalFormat, bFinalCubemap, dwFinalMipCount, dwFinalReduce, dwFinalWidth, dwFinalHeight);

        if (finalFormat < 0)
        {
            return "Format not recognized";
        }

        const PixelFormatInfo* const pFinalFormatInfo = CPixelFormats::GetPixelFormatInfo(finalFormat);
        assert(pFinalFormatInfo);

        uint32 dwMem = _CalcTextureMemory(finalFormat, dwFinalWidth, dwFinalHeight, bFinalCubemap, (dwFinalMipCount > 1));

        const char* szAlpha = pFinalFormatInfo->szAlpha;
        const char* szAttachedStr = "";

        if (m_pFinalImage->GetAttachedImage())
        {
            const EPixelFormat alphaFormat = m_pFinalImage->GetAttachedImage()->GetPixelFormat();
            const PixelFormatInfo* const pAlphaFormatInfo = CPixelFormats::GetPixelFormatInfo(alphaFormat);
            assert(pAlphaFormatInfo);
            szAlpha = pAlphaFormatInfo->szName;
            szAttachedStr = "+";
            dwMem += _CalcTextureMemory(alphaFormat, dwFinalWidth, dwFinalHeight, bFinalCubemap, (dwFinalMipCount > 1));
        }

        const uint32 dwFinalImageFlags = m_pFinalImage->GetImageFlags();
        const char* const szModel =
            ((dwFinalImageFlags& CImageExtensionHelper::EIF_Colormodel) == CImageExtensionHelper::EIF_Colormodel_CIE ? "CIE" :
             ((dwFinalImageFlags& CImageExtensionHelper::EIF_Colormodel) == CImageExtensionHelper::EIF_Colormodel_YCC ? "YCC" :
              ((dwFinalImageFlags& CImageExtensionHelper::EIF_Colormodel) == CImageExtensionHelper::EIF_Colormodel_YFF ? "YFF" :
               ((dwFinalImageFlags& CImageExtensionHelper::EIF_Colormodel) == CImageExtensionHelper::EIF_Colormodel_IRB ? "IRB" : "RGB"))));
        const char* const szSRGB =
            (dwFinalImageFlags & CImageExtensionHelper::EIF_SRGBRead ? "s" : "");
        const char* const szNorm =
            (dwFinalImageFlags & CImageExtensionHelper::EIF_RenormalizedTexture ? "n" : "");

        str = QString("%1x%2 Mips:%3 Fmt:%4 Mdl:%5%6%7 Alpha:%8%9\n"
            "Mem:%10kB reduce:%11 Flags:%12  08x")
            .arg(dwFinalWidth).arg(dwFinalHeight).arg(dwFinalMipCount).arg(pFinalFormatInfo->szName).arg(szSRGB).arg(szModel).arg(szNorm).arg(szAttachedStr).arg(szAlpha)
            .arg(dwMem / 1024.0f, 0, 'f', 1).arg(dwFinalReduce).arg(dwFinalImageFlags, 8, 16, QChar('0'));
    }

    return str.toUtf8().data();
}


string CImageCompiler::GetDestInfoString()
{
    uint32 dwWidth, dwHeight, dwMips;
    m_pInputImage->GetExtentTransformed(dwWidth, dwHeight, dwMips, m_Props);

    const EPixelFormat format = (m_pFinalImage) ? m_pFinalImage->GetPixelFormat() : (EPixelFormat) - 1;
    const char* const szName = (format < 0) ? "?" : CPixelFormats::GetPixelFormatInfo(format)->szName;
    const char* const szAlpha = (format < 0) ? "?" : CPixelFormats::GetPixelFormatInfo(format)->szAlpha;

    const uint32 dwReduce = m_Props.GetRequestedResolutionReduce(dwWidth, dwHeight);
    const uint32 dwW = Util::getMax(dwWidth >> dwReduce, (uint32)1);
    const uint32 dwH = Util::getMax(dwHeight >> dwReduce, (uint32)1);
    const bool bCubemap = m_Props.GetCubemap() && m_pInputImage->HasCubemapCompatibleSizes();
    const uint32 dwNumMips = (m_Props.GetMipMaps())
        ? Util::getMax((int)CPixelFormats::ComputeMaxMipCount(format, dwWidth, dwHeight, bCubemap) - (int)dwReduce, 1)
        : 1;

    const uint32 dwMem = CalcTextureMemory(m_pFinalImage);
    const uint32 dwAttachedMem = (m_pFinalImage && m_pFinalImage->GetAttachedImage())
        ? CalcTextureMemory(m_pFinalImage->GetAttachedImage())
        : 0;

    QString str = QString(
        "%1"        // Width
        "\t%2"      // Height
        "\t%3"      // Alpha
        "\t%4"      // Mips
        "\t%5"      // MemInKBMem
        "\t%6"      // Format
        "\t%7"      // reduce
        "\t%8")     // AttachedMemInKB
        .arg(dwW).arg(dwH).arg(szAlpha).arg(dwNumMips).arg(dwMem / 1024.0f, 0, 'f', 1).arg(szName).arg(dwReduce).arg(dwAttachedMem / 1024.0f, 0, 'f', 1);

    return str.toUtf8().data();
}


uint32 CImageCompiler::CalcTextureMemory(const ImageObject* pImage)
{
    if (!pImage)
    {
        return 0;
    }

    uint32 dwWidth, dwHeight, dwMips;
    pImage->GetExtent(dwWidth, dwHeight, dwMips);

    uint32 size = _CalcTextureMemory(pImage->GetPixelFormat(), dwWidth, dwHeight, pImage->HasCubemapCompatibleSizes(), (dwMips > 1));

    if (pImage->GetAttachedImage())
    {
        size += CalcTextureMemory(pImage->GetAttachedImage());
    }

    return size;
}


string CImageCompiler::GetUnaliasedPresetName(const string& presetName) const
{
    const CImageConvertor::PresetAliases::const_iterator it = m_presetAliases.find(presetName);
    if (it == m_presetAliases.end())
    {
        return presetName;
    }
    return it->second;
}

void CImageCompiler::SetPreset(const string& preset)
{
    if (m_CC.pRC->GetVerbosityLevel() > 2)
    {
        RCLog("Setting preset: %s", preset.c_str());
    }

    m_Props.SetPreset(preset);
}

void CImageCompiler::SetPresetSettings()
{
    uint32 dwImageFlags = 0;

    if (m_Props.GetDecal())
    {
        dwImageFlags |= CImageExtensionHelper::EIF_Decal;
    }

    if (m_Props.GetSupressEngineReduce())
    {
        dwImageFlags |= CImageExtensionHelper::EIF_SupressEngineReduce;
    }

    m_pInputImage->SetImageFlags(dwImageFlags);
}

// set the stored properties to the current file and save it
bool CImageCompiler::UpdateAndSaveConfig()
{
    if (StringHelpers::EqualsIgnoreCase(m_CC.convertorExtension, "tif"))
    {
        return ImageTIFF::UpdateAndSaveSettingsToTIF(GetSettingsFile(m_CC.GetSourcePath()).c_str(), &m_Props, 0, false);
    }
    else if (ImageQImage::IsExtensionSupported(m_CC.convertorExtension.c_str())) // New formats
    {
        return ImageQImage::UpdateAndSaveSettings(GetSettingsFile(m_CC.GetSourcePath()).c_str(), &m_Props, 0, false);
    }

    return false;
}


void CImageCompiler::AnalyzeImageAndSuggest(const ImageObject* pSourceImage)
{
    // works only within CryTIF
    if (!m_pImageUserDialog || !pSourceImage)
    {
        return;
    }

    // check only diffuse textures
    const string presetName = m_Props.GetPreset();

    // only if preset has changed
    if (m_sLastPreset != presetName)
    {
        m_sLastPreset = presetName;
        QLabel* label = m_pImageUserDialog->GetWindow()->findChild<QLabel*>("warningLine");
        label->clear();

        // check if normal map highQ compression is not a waste of memory
        if (StringHelpers::EqualsIgnoreCase(presetName, "Normals"))
        {
            // force full image conversion (even if zoomed or clipped)
            const bool bInternalPreview = m_bInternalPreview;
            m_bInternalPreview = false;
            const float fDXTError = pSourceImage->GetDXT1NormalsCompressionError(&m_Props);
            m_bInternalPreview = bInternalPreview;

            // empirical threshold
            if (fDXTError < 0.02f)
            {
                label->setText("For this normal map you can use Normalmap_lowQ preset which has nearly the same quality and saves memory!");
            }
        }
    }

    if (m_Props.GetMipRenormalize())
    {
        static bool bWarnAboutBadNormalFormat = true;
        if (bWarnAboutBadNormalFormat && StringHelpers::ContainsIgnoreCase(presetName, "highQ"))
        {
            if (pSourceImage->GetPixelFormat() != ePixelFormat_A16B16G16R16 &&
                pSourceImage->GetPixelFormat() != ePixelFormat_A16B16G16R16F &&
                pSourceImage->GetPixelFormat() != ePixelFormat_A32B32G32R32F)
            {
                // Set to false to not warn multiple times for the same issue.
                bWarnAboutBadNormalFormat = false;

                static const char* const pOutMessage =
                    "Texture contains low precision normal vectors and will likely cause imprecise lighting and reflection.\n"
                    "Switch to 16bit and normalize, or re-export from source-material.\n"
                    "Do you want to come back to Photoshop and fix it yourself?";
                if (QMessageBox::warning(nullptr, "The texture is low quality", pOutMessage, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
                {
                    exit(eRcExitCode_UserFixing);
                }
            }
        }

        static bool bWarnAboutBadNormalDistribution = true;
        if (bWarnAboutBadNormalDistribution && StringHelpers::ContainsIgnoreCase(presetName, "highQ"))
        {
            Histogram<256> histogram;
            if (!pSourceImage->ComputeUnitSphereDeviationHistogramForAnyRGB(histogram))
            {
                return;
            }

            const float meanBin = histogram.getMeanBin();
            if ((meanBin < (128.0f - 4)) || (meanBin > (128.0f + 4)))
            {
                // Set to false to not warn multiple times for the same issue.
                bWarnAboutBadNormalDistribution = false;

                static const char* const pOutMessage =
                    "Texture contains severely denormal vectors and will likely be normalized illformed.\n"
                    "Switch to 16bit and normalize, or re-export from source-material.\n"
                    "Do you want to come back to Photoshop and fix it yourself?";
                if (QMessageBox::warning(nullptr, "The texture is low quality", pOutMessage, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
                {
                    exit(eRcExitCode_UserFixing);
                }
            }
        }
    }

    if (StringHelpers::StartsWithIgnoreCase(presetName, "Albedo"))
    {
        return;
    }

    Histogram<256> histogram;
    if (!pSourceImage->ComputeLuminanceHistogramForAnyRGB(histogram))
    {
        return;
    }

    static bool bWarnAboutLowMeanLuminance = false;  // 2013/02/15: this warning is disabled due to Tom Deerberg's request to Sandbox team made on 2013/02/14
    if (bWarnAboutLowMeanLuminance)
    {
        const float meanBin = histogram.getMeanBin();
        if (meanBin < 48.0f)
        {
            bWarnAboutLowMeanLuminance = false; // we warn only once per session
            QString outMessage = QString(
                "Texture is too dark and will cause banding, adjust levels.\n"
                "Mean luminance: %1 / 255.\n"
                "Utilize the color range more efficiently.\n"
                "Do you want to come back to Photoshop and fix it yourself?")
                .arg(meanBin, 0, 'f', 1);
            if (QMessageBox::warning(nullptr, "The texture is too dark", outMessage, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                exit(eRcExitCode_UserFixing);
            }
            else
            {
                return;
            }
        }
    }

    static bool bWarnAboutManyDarkPixels = true;
    if (bWarnAboutManyDarkPixels)
    {
        const int darkBin = 31;
        const float darkPercentage = histogram.getPercentage(0, darkBin);
        if (darkPercentage >= 50.0f)
        {
            bWarnAboutManyDarkPixels = false; // we warn only once per session
            QString outMessage = QString(
                "Texture is too dark and will cause banding, adjust levels.\n"
                "More than half of the image's pixels are dark: %1% pixels have luminance less than %2 / 255.\n"
                "Utilize the color range more efficiently.\n"
                "Do you want to come back to Photoshop and fix it yourself?")
                .arg(darkPercentage, 0, 'f', 1).arg(darkBin);
            if (QMessageBox::warning(nullptr, "The texture is too dark", outMessage, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                exit(eRcExitCode_UserFixing);
            }
            else
            {
                return;
            }
        }
    }
}
