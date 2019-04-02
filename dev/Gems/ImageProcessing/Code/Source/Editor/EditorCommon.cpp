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

#include <ImageProcessing_precompiled.h>
#include <Editor/EditorCommon.h>
#include <Processing/ImageToProcess.h>
#include <ImageLoader/ImageLoaders.h>
#include <BuilderSettings/TextureSettings.h>
#include <BuilderSettings/PresetSettings.h>
#include <BuilderSettings/PixelFormats.h>
#include <Processing/ImageConvert.h>
#include <AzCore/std/string/conversions.h>

namespace ImageProcessingEditor
{
    using namespace ImageProcessing;

    bool EditorHelper::s_IsPixelFormatStringInited = false;
    const char* EditorHelper::s_PixelFormatString[ImageProcessing::EPixelFormat::ePixelFormat_Count];
    
    void EditorHelper::InitPixelFormatString()
    {
        if (!s_IsPixelFormatStringInited)
        {
            s_IsPixelFormatStringInited = true;
        }

        CPixelFormats& pixelFormats = CPixelFormats::GetInstance();
        for (int format = 0; format < EPixelFormat::ePixelFormat_Count; format ++)
        {
            const PixelFormatInfo* info = pixelFormats.GetPixelFormatInfo((EPixelFormat)format);
            s_PixelFormatString[(EPixelFormat)format] = "";
            if (info)
            {
                s_PixelFormatString[(EPixelFormat)format] = info->szName;
            }
            else
            {
                AZ_Error("Texture Editor", false, "Cannot find name of EPixelFormat %i", format);
            }
        }
    }

    const AZStd::string EditorHelper::GetFileSizeString(AZ::u32 fileSizeInBytes)
    {
        AZStd::string fileSizeStr;

        static double kb = 1024.0f;
        static double mb = kb * 1024.0;
        static double gb = mb * 1024.0;

        static AZStd::string byteStr = "B";
        static AZStd::string kbStr = "KB";
        static AZStd::string mbStr = "MB";
        static AZStd::string gbStr = "GB";

#if defined(AZ_PLATFORM_APPLE)
        kb = 1000.0;
        mb = kb * 1000.0;
        gb = mb * 1000.0;

        kbStr = "kB";
        mbStr = "mB";
        gbStr = "gB";
#endif //AZ_PLATFORM_APPLE

        if (fileSizeInBytes < kb)
        {
            fileSizeStr = AZStd::string::format("%d%s", fileSizeInBytes, byteStr.c_str());
        }
        else if (fileSizeInBytes < mb)
        {
            double size = fileSizeInBytes / kb;
            fileSizeStr = AZStd::string::format("%.2f%s", size, kbStr.c_str());
        }
        else if (fileSizeInBytes < gb)
        {
            double size = fileSizeInBytes / mb;
            fileSizeStr = AZStd::string::format("%.2f%s", size, mbStr.c_str());
        }
        else
        {
            double size = fileSizeInBytes / gb;
            fileSizeStr = AZStd::string::format("%.2f%s", size, gbStr.c_str());
        }
        return fileSizeStr;
    }

    const AZStd::string EditorHelper::ToReadablePlatformString(const AZStd::string& platformRawStr)
    {
        AZStd::string readableString;
        AZStd::string platformStrLowerCase = platformRawStr;
        AZStd::to_lower(platformStrLowerCase.begin(), platformStrLowerCase.end());
        if (platformStrLowerCase == "pc")
        {
            readableString = "PC";
        }
        else if (platformStrLowerCase == "es3")
        {
            readableString = "Android";
        }
        else if (platformStrLowerCase == "osx_gl")
        {
            readableString = "macOS";
        }
        else if (platformStrLowerCase == "xboxone")
        {
            readableString = "Xbox One";
        }
        else if (platformStrLowerCase == "ps4")
        {
            readableString = "PS4";
        }
        else if (platformStrLowerCase == "ios")
        {
            readableString = "iOS";
        }
        else
        {
            return platformRawStr;
        }

        return readableString;
    }


    EditorTextureSetting::EditorTextureSetting(const AZ::Uuid& sourceTextureId)
    {
        m_sourceTextureId = sourceTextureId;
        const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* fullDetails = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(m_sourceTextureId);
        m_textureName = fullDetails->GetName();
        m_fullPath = fullDetails->GetFullPath();
        bool generatedDefaults = false;
        m_settingsMap = TextureSettings::GetMultiplatformTextureSetting(m_fullPath, generatedDefaults);
        m_img = IImageObjectPtr(LoadImageFromFile(m_fullPath));

        //! Editor Sanity Check: We need to make sure the preset id is valid in texture setting
        //! if not, we need to assign a default one
        for (auto& settingIter: m_settingsMap)
        {
            const AZ::Uuid presetId = settingIter.second.m_preset;
            const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(presetId);
            if (!preset)
            {
                settingIter.second.ApplyPreset(BuilderSettingManager::Instance()->GetSuggestedPreset(m_textureName, m_img));
                AZ_Error("Texture Editor", false, "Cannot find preset %s! Will assign a suggested one for the texture.", presetId.ToString<AZStd::string>().c_str());
            }
        }
    }

    EditorTextureSetting::~EditorTextureSetting()
    {

    }

    void EditorTextureSetting::SetIsOverrided()
    {
        for (auto& it : m_settingsMap)
        {
            m_overrideFromPreset = false;
            TextureSettings& textureSetting = it.second;
            const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(textureSetting.m_preset);
            if (presetSetting != nullptr)
            {
                if ((textureSetting.m_sizeReduceLevel != presetSetting->m_sizeReduceLevel) ||
                    (textureSetting.m_suppressEngineReduce != presetSetting->m_suppressEngineReduce) ||
                    (presetSetting->m_mipmapSetting != nullptr && textureSetting.m_mipGenType != presetSetting->m_mipmapSetting->m_type))
                {
                    m_overrideFromPreset = true;
                }
            }
            else
            {
                AZ_Error("Texture Editor", false, "Texture Preset %s is not found!", textureSetting.m_preset.ToString<AZStd::string>().c_str());
            }
        }
    }

    void EditorTextureSetting::SetToPreset(const AZStd::string presetName)
    {
        m_overrideFromPreset = false;

        for (auto& it : m_settingsMap)
        {
            TextureSettings& textureSetting = it.second;

            AZ::Uuid presetId = BuilderSettingManager::Instance()->GetPresetIdFromName(presetName);
            if (presetId.IsNull())
            {
                AZ_Error("Texture Editor", false, "Texture Preset %s has no associated UUID.", presetName.c_str());
                return;
            }

            textureSetting.ApplyPreset(presetId);
        }
    }


    //Get the texture setting on certain platform
    TextureSettings& EditorTextureSetting::GetMultiplatformTextureSetting(const AZStd::string& platform)
    {
        AZ_Assert(m_settingsMap.size() > 0, "Texture Editor", "There is no texture settings for texture %s", m_textureName.c_str());
        PlatformName platformName = platform;
        if (platform.empty())
        {
            platformName = BuilderSettingManager::s_defaultPlatform;
        }
        if (m_settingsMap.find(platformName) != m_settingsMap.end())
        {
            return m_settingsMap[platformName];
        }
        else
        {
            AZ_Error("Texture Editor", false, "Cannot find texture setting on platform %s", platformName.c_str());
        }
        return m_settingsMap.begin()->second;
    }

    //Gets the final resolution/reduce/mip count for a texture on a certain platform
    //@param wantedReduce indicates the reduce level that's preferred
    //@return successfully get the value or not
    bool EditorTextureSetting::GetFinalInfoForTextureOnPlatform(const AZStd::string& platform, AZ::u32& finalWidth, AZ::u32& finalHeight, AZ::u32 wantedReduce, AZ::u32& finalReduce, AZ::u32& finalMipCount)
    {
        if (m_settingsMap.find(platform) == m_settingsMap.end())
        {
            return false;
        }

        // Copy current texture setting and set to desired reduce
        TextureSettings textureSetting = m_settingsMap[platform]; 
        textureSetting.m_sizeReduceLevel = wantedReduce;

        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(textureSetting.m_preset, platform);
        if (presetSetting)
        {
            EPixelFormat pixelFormat = presetSetting->m_pixelFormat;
            CPixelFormats& pixelFormats = CPixelFormats::GetInstance();

            AZ::u32 inputWidth = m_img->GetWidth(0);
            AZ::u32 inputHeight = m_img->GetHeight(0);

            GetOutputExtent(inputWidth, inputHeight, finalWidth, finalHeight, finalReduce, &textureSetting, presetSetting);

            AZ::u32 mipMapCount = pixelFormats.ComputeMaxMipCount(pixelFormat, finalWidth, finalHeight);
            finalMipCount = presetSetting->m_mipmapSetting != nullptr && textureSetting.m_enableMipmap ? mipMapCount : 1;
            finalReduce = AZStd::min<int>(AZStd::max<int>(s_MinReduceLevel, finalReduce), s_MaxReduceLevel);

            return true;
        }
        else
        {
            return false;
        }
    }

    bool EditorTextureSetting::RefreshMipSetting(bool enableMip)
    {
        bool enabled = true;
        for (auto& it : m_settingsMap)
        {
            const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(it.second.m_preset);
            if (enableMip)
            {
                if (preset && preset->m_mipmapSetting)
                {
                    it.second.m_enableMipmap = true;
                    it.second.m_mipGenType = preset->m_mipmapSetting->m_type;
                }
                else
                {
                    it.second.m_enableMipmap = false;
                    enabled = false;
                    AZ_Error("Texture Editor", false, "Preset %s does not support mipmap!", preset->m_name.c_str());
                }
            }
            else
            {
                it.second.m_enableMipmap = false;
                enabled = false;
            }
        }
        return enabled;
    }

    void EditorTextureSetting::PropagateCommonSettings()
    {
        if (m_settingsMap.size() <= 1)
        {
            //Only one setting available, no need to propagate
            return;
        }

        TextureSettings& texSetting = GetMultiplatformTextureSetting();
        for (auto& it = ++ m_settingsMap.begin(); it != m_settingsMap.end(); ++it)
        {
            const PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;
            if (it->first != defaultPlatform)
            {
                it->second.m_enableMipmap = texSetting.m_enableMipmap;
                it->second.m_maintainAlphaCoverage = texSetting.m_maintainAlphaCoverage;
                it->second.m_mipGenEval = texSetting.m_mipGenEval;
                it->second.m_mipGenType = texSetting.m_mipGenType;
                for (size_t i = 0; i < TextureSettings::s_MaxMipMaps; i++)
                {
                    it->second.m_mipAlphaAdjust[i] = texSetting.m_mipAlphaAdjust[i];
                }
            }
        }
    }

    AZStd::list<ResolutionInfo> EditorTextureSetting::GetResolutionInfo(AZStd::string platform, AZ::u32& minReduce, AZ::u32& maxReduce)
    {
        AZStd::list<ResolutionInfo> resolutionInfos;
        // Set the min/max reduce to the global value range first
        minReduce = s_MaxReduceLevel;
        maxReduce = s_MinReduceLevel;
        for (int i = s_MinReduceLevel; i <= s_MaxReduceLevel; i++)
        {
            ResolutionInfo resolutionInfo;
            GetFinalInfoForTextureOnPlatform(platform, resolutionInfo.width, resolutionInfo.height, i, resolutionInfo.reduce, resolutionInfo.mipCount);
            // Finds out the final min/max reduce based on range in different platforms
            minReduce = AZStd::min<unsigned int>(resolutionInfo.reduce, minReduce);
            maxReduce = AZStd::max<unsigned int>(resolutionInfo.reduce, maxReduce);
            resolutionInfos.push_back(resolutionInfo);
        }
        return resolutionInfos;
    }

    AZStd::list<ResolutionInfo> EditorTextureSetting::GetResolutionInfoForMipmap(AZStd::string platform)
    {
        AZStd::list<ResolutionInfo> resolutionInfos;
        unsigned int baseReduce = m_settingsMap[platform].m_sizeReduceLevel;
        ResolutionInfo baseInfo;
        GetFinalInfoForTextureOnPlatform(platform, baseInfo.width, baseInfo.height, baseReduce, baseInfo.reduce, baseInfo.mipCount);
        resolutionInfos.push_back(baseInfo);
        for (unsigned int i = 1; i < baseInfo.mipCount; i++)
        { 
            ResolutionInfo resolutionInfo;
            GetFinalInfoForTextureOnPlatform(platform, resolutionInfo.width, resolutionInfo.height, baseReduce + i, resolutionInfo.reduce, resolutionInfo.mipCount);
            resolutionInfos.push_back(resolutionInfo);
        }
        return resolutionInfos;
    }

} //namespace ImageProcessingEditor

