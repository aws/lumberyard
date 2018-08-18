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

#include "IRCLog.h"
#include "ImageConvertor.h"
#include "ImageCompiler.h"

#define _TIFF_DATA_TYPEDEFS_                // because we defined uint32,... already
#include <libtiff/tiffio.h>  // TIFF library

static void TiffWarningHandler(const char* module, const char* fmt, va_list args)
{
    // unused tags, etc
    char str[2 * 1024];
    azvsnprintf(str, sizeof(str), fmt, args);
    RCLogWarning("LibTIFF: %s", str);
}

static void TiffErrorHandler(const char* module, const char* fmt, va_list args)
{
    char str[2 * 1024];
    azvsnprintf(str, sizeof(str), fmt, args);
    RCLogError("LibTIFF: %s", str);
}

static void LoadPresetAliases(
    const ICfgFile* const pIni,
    CImageConvertor::PresetAliases& presetAliases)
{
    presetAliases.clear();

    const char* const szPresetAliasesSectionName = "_presetAliases";

    // Load preset map entries
    if (pIni->FindSection(szPresetAliasesSectionName) >= 0)
    {
        class CMappingFiller
            : public IConfigSink
        {
        public:
            CMappingFiller(CImageConvertor::PresetAliases& presetAliases, const char* const szPresetAliasesSectionName)
                : m_presetAliases(presetAliases)
                , m_szPresetAliasesSectionName(szPresetAliasesSectionName)
                , m_bFailed(false)
            {
            }

            virtual void SetKeyValue(EConfigPriority ePri, const char* szKey, const char* szValue) override
            {
                const string key(szKey);
                if (m_presetAliases.find(key) != m_presetAliases.end())
                {
                    RCLogError("Preset name '%s' specified *multiple* times in section '%s' of rc.ini", szKey, m_szPresetAliasesSectionName);
                }
                m_presetAliases[key] = string(szValue);
            }

            CImageConvertor::PresetAliases& m_presetAliases;
            const char* const m_szPresetAliasesSectionName;
            bool m_bFailed;
        };

        CMappingFiller filler(presetAliases, szPresetAliasesSectionName);

        pIni->CopySectionKeysToConfig(eCP_PriorityRcIni, pIni->FindSection(szPresetAliasesSectionName), "", &filler);

        if (filler.m_bFailed)
        {
            presetAliases.clear();
            return;
        }
    }

    // Check that preset names actually exist in rc.ini
    for (CImageConvertor::PresetAliases::iterator it = presetAliases.begin(); it != presetAliases.end(); ++it)
    {
        const string& presetName = it->second;
        if (pIni->FindSection(presetName.c_str()) < 0)
        {
            RCLogError("Preset '%s' specified in section '%s' of rc.ini doesn't exist", presetName.c_str(), szPresetAliasesSectionName);
            presetAliases.clear();
            return;
        }
    }
}

CImageConvertor::CImageConvertor(IResourceCompiler* pRC)
{
    LoadPresetAliases(pRC->GetIniFile(), m_presetAliases);
}

CImageConvertor::~CImageConvertor()
{
}

void CImageConvertor::Release()
{
    delete this;
}

void CImageConvertor::Init(const ConvertorInitContext& context)
{
    TIFFSetErrorHandler(TiffErrorHandler);
    TIFFSetWarningHandler(TiffWarningHandler);
}

ICompiler* CImageConvertor::CreateCompiler()
{
    return new CImageCompiler(m_presetAliases);
}

bool CImageConvertor::SupportsMultithreading() const
{
    return true;
}

const char* CImageConvertor::GetExt(int index) const
{
    if (index == 0)
    {
        return "tif";
    }
    else if (index == 1)
    {
        return "hdr";
    }
    else if (index == 2)
    {
        return "bmp";
    }
    else if (index == 3)
    {
        return "gif";
    }
    else if (index == 4)
    {
        return "jpg";
    }
    else if (index == 5)
    {
        return "jpeg";
    }
    else if (index == 6)
    {
        return "jpe";
    }
    else if (index == 7)
    {
        return "tga";
    }
    else if (index == 8)
    {
        return "png";
    }
    else if (index == 9)
    {
        return "tiff";
    }

    return NULL;
}
