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

#include "StdAfx.h"

#include "TextureManager.h"
#include "Texture.h"
#include "ISystem.h"
#include <AzFramework/Asset/AssetSystemBus.h>


//////////////////////////////////////////////////////////////////////////
CTextureManager* CTextureManager::s_Instance = nullptr;
//------------------------------------------------------------------------------
void CTextureManager::ReleaseTextureSemantics()
{
    for (int slot = 0; slot < EFTT_MAX; slot++)
    {
        delete m_TexSlotSemantics[slot].suffix;
        m_TexSlotSemantics[slot].suffix = nullptr;
        m_TexSlotSemantics[slot].def = nullptr;
        m_TexSlotSemantics[slot].neutral = nullptr;
    }
}

void CTextureManager::ReleaseTextures()
{
    AZ_Warning("[Shaders System]", false, "Textures Manager - releasing all textures");

    // Remove fixed default textures
    for (auto& iter : m_DefaultTextures)
    {
        SAFE_RELEASE_FORCE(iter.second);
    }
    m_DefaultTextures.clear();

    // Remove fixed engine textures
    for (auto& iter : m_EngineTextures)
    {
        SAFE_RELEASE_FORCE(iter.second);
    }
    m_EngineTextures.clear();

    // Material textures should be released by releasing the materials themselves.
    m_MaterialTextures.clear();
}

//------------------------------------------------------------------------------
// [Shaders System]
// The following method should be enhanced to load the user defined default textures
// from an XML file data 'defaultTextures.xml'
// Sample code for the load is provided below to be followed in the future.
//------------------------------------------------------------------------------
void CTextureManager::LoadDefaultTextures()
{
    struct textureEntry
    {
        const char* szTextureName;
        const char* szFileName;
        uint32 flags;
    }

    // The following texture names are the name by which the texture pointers will 
    // be indexed.    Notice that they match to the previously stored textures with
    // name pattern removing 's_ptex'[Name]. Example: 's_ptexWhite' will now be 'White'
    texturesFromFile[] =
    {
        {"NoTextureCM",                 "EngineAssets/TextureMsg/ReplaceMeCM.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"White",                       "EngineAssets/Textures/White.dds",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"Gray",                        "EngineAssets/Textures/Grey.dds",                          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"Black",                       "EngineAssets/Textures/Black.dds",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"BlackAlpha",                  "EngineAssets/Textures/BlackAlpha.dds",                    FT_DONT_RELEASE | FT_DONT_STREAM },
        {"BlackCM",                     "EngineAssets/Textures/BlackCM.dds",                       FT_DONT_RELEASE | FT_DONT_STREAM },
        {"DefaultProbeCM",              "EngineAssets/Shading/defaultProbe_cm.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"FlatBump",                    "EngineAssets/Textures/White_ddn.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM | FT_TEX_NORMAL_MAP },
        {"PaletteDebug",                "EngineAssets/Textures/palletteInst.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"PaletteTexelsPerMeter",       "EngineAssets/Textures/TexelsPerMeterGrad.dds",            FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconShaderCompiling",         "EngineAssets/Icons/ShaderCompiling.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconStreaming",               "EngineAssets/Icons/Streaming.dds",                        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconStreamingTerrainTexture", "EngineAssets/Icons/StreamingTerrain.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconNullSoundSystem",         "EngineAssets/Icons/NullSoundSystem.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconNavigationProcessing",    "EngineAssets/Icons/NavigationProcessing.dds",             FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ShadowJitterMap",             "EngineAssets/Textures/rotrandom.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"EnvironmentBRDF",             "EngineAssets/Shading/environmentBRDF.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ScreenNoiseMap",              "EngineAssets/Textures/JumpNoiseHighFrequency_x27y19.dds", FT_DONT_RELEASE | FT_DONT_STREAM | FT_NOMIPS },
        {"DissolveNoiseMap",            "EngineAssets/Textures/noise.dds",                         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"GrainFilterMap",              "EngineAssets/ScreenSpace/grain_bayer_mul.dds",            FT_DONT_RELEASE | FT_DONT_STREAM | FT_NOMIPS },
        {"FilmGrainMap",                "EngineAssets/ScreenSpace/film_grain.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM | FT_NOMIPS },
        {"VignettingMap",               "EngineAssets/Shading/vignetting.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"AOJitter",                    "EngineAssets/ScreenSpace/PointsOnSphere4x4.dds",          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"AOVOJitter",                  "EngineAssets/ScreenSpace/PointsOnSphereVO4x4.dds",        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"NormalsFitting",              "EngineAssets/ScreenSpace/NormalsFitting.dds",             FT_DONT_RELEASE | FT_DONT_STREAM },
        {"AverageMemoryUsage",          "EngineAssets/Icons/AverageMemoryUsage.dds",               FT_DONT_RELEASE | FT_DONT_STREAM },
        {"LowMemoryUsage",              "EngineAssets/Icons/LowMemoryUsage.dds",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"HighMemoryUsage",             "EngineAssets/Icons/HighMemoryUsage.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"LivePreview",                 "EngineAssets/Icons/LivePreview.dds",                      FT_DONT_RELEASE | FT_DONT_STREAM },
#if !defined(_RELEASE)
        {"NoTexture",                   "EngineAssets/TextureMsg/ReplaceMe.dds",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling",        "EngineAssets/TextureMsg/TextureCompiling.dds",            FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_a",      "EngineAssets/TextureMsg/TextureCompiling_a.dds",          FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_cm",     "EngineAssets/TextureMsg/TextureCompiling_cm.dds",         FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_ddn",    "EngineAssets/TextureMsg/TextureCompiling_ddn.dds",        FT_DONT_RELEASE | FT_DONT_STREAM },
        {"IconTextureCompiling_ddna",   "EngineAssets/TextureMsg/TextureCompiling_ddna.dds",       FT_DONT_RELEASE | FT_DONT_STREAM },
        {"DefaultMergedDetail",         "EngineAssets/Textures/GreyAlpha.dds",                     FT_DONT_RELEASE | FT_DONT_STREAM },
        {"MipMapDebug",                 "EngineAssets/TextureMsg/MipMapDebug.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorBlue",                   "EngineAssets/TextureMsg/color_Blue.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorCyan",                   "EngineAssets/TextureMsg/color_Cyan.dds",                  FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorGreen",                  "EngineAssets/TextureMsg/color_Green.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorPurple",                 "EngineAssets/TextureMsg/color_Purple.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorRed",                    "EngineAssets/TextureMsg/color_Red.dds",                   FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorWhite",                  "EngineAssets/TextureMsg/color_White.dds",                 FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorYellow",                 "EngineAssets/TextureMsg/color_Yellow.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorOrange",                 "EngineAssets/TextureMsg/color_Orange.dds",                FT_DONT_RELEASE | FT_DONT_STREAM },
        {"ColorMagenta",                "EngineAssets/TextureMsg/color_Magenta.dds",               FT_DONT_RELEASE | FT_DONT_STREAM },
#else
        {"NoTexture",                   "EngineAssets/TextureMsg/ReplaceMeRelease.dds",            FT_DONT_RELEASE | FT_DONT_STREAM },
#endif
    };

    for ( textureEntry& entry : texturesFromFile)
    {
        CTexture*       pNewTexture = CTexture::ForName(entry.szFileName, entry.flags, eTF_Unknown);
        if (pNewTexture)
        {
            CCryNameTSCRC   texEntry(entry.szTextureName);
            m_DefaultTextures[texEntry] = pNewTexture;
        }
        else
        {
            AZ_Assert(false, "Error - CTextureManager failed to load default texture" );
            AZ_Warning("[Shaders System]", false, "Error - CTextureManager failed to load default texture" );
        }
    }

    m_texNoTexture = GetDefaultTexture("NoTexture");
    m_texNoTextureCM = GetDefaultTexture("NoTextureCM");
    m_texWhite = GetDefaultTexture("White");
    m_texBlack = GetDefaultTexture("Black");
    m_texBlackCM = GetDefaultTexture("BlackCM");
}

void CTextureManager::LoadMaterialTexturesSemantics()
{
    // Shaders System] - To Do - replace the following with the actual pointers
    CTexture*       pNoTexture = CTextureManager::Instance()->GetNoTexture();
    CTexture*       pWhiteTexture = CTextureManager::Instance()->GetWhiteTexture();
    CTexture*       pGrayTexture = CTextureManager::Instance()->GetDefaultTexture("Gray");
    CTexture*       pFlatBumpTexture = CTextureManager::Instance()->GetDefaultTexture("FlatBump");

    // [Shaders System] - this needs to move to shader code and be reflected, hence data driven per 
    // texture usage / declaration in the shader.
    MaterialTextureSemantic     texSlotSemantics[] =
    {
        // NOTE: must be in order with filled holes to allow direct lookup
        MaterialTextureSemantic( EFTT_DIFFUSE,          4, pNoTexture,         pWhiteTexture,    "_diff" ),
        MaterialTextureSemantic( EFTT_NORMALS,          2, pFlatBumpTexture,   pFlatBumpTexture, "_ddn" ),
        MaterialTextureSemantic( EFTT_SPECULAR,         1, pWhiteTexture,      pWhiteTexture,    "_spec" ),
        MaterialTextureSemantic( EFTT_ENV,              0, pWhiteTexture,      pWhiteTexture,    "_cm" ),
        MaterialTextureSemantic( EFTT_DETAIL_OVERLAY,   3, pGrayTexture,       pWhiteTexture,    "_detail" ),
        MaterialTextureSemantic( EFTT_SECOND_SMOOTHNESS, 2, pWhiteTexture,     pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_HEIGHT,           2, pWhiteTexture,      pWhiteTexture,    "_displ" ),
        MaterialTextureSemantic( EFTT_DECAL_OVERLAY,    3, pGrayTexture,       pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_SUBSURFACE,       3, pWhiteTexture,      pWhiteTexture,    "_sss" ),
        MaterialTextureSemantic( EFTT_CUSTOM,           4, pWhiteTexture,      pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_CUSTOM_SECONDARY, 2, pFlatBumpTexture,   pFlatBumpTexture, "" ),
        MaterialTextureSemantic( EFTT_OPACITY,          4, pWhiteTexture,      pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_SMOOTHNESS,       2, pWhiteTexture,      pWhiteTexture,    "_ddna" ),
        MaterialTextureSemantic( EFTT_EMITTANCE,        1, pWhiteTexture,      pWhiteTexture,    "_em" ),
        MaterialTextureSemantic( EFTT_OCCLUSION,        4, pWhiteTexture,      pWhiteTexture,    "" ),
        MaterialTextureSemantic( EFTT_SPECULAR_2,       4, pWhiteTexture,      pWhiteTexture,    "_spec" ),

        // This is the terminator for the name-search
        MaterialTextureSemantic( EFTT_UNKNOWN,          0, CTexture::s_pTexNULL,      CTexture::s_pTexNULL,     "" ),
    };

    for (int i = 0; i < EFTT_MAX; i++)
    {
        SAFE_DELETE(m_TexSlotSemantics[i].suffix);      // make sure we are not leaving snail marks
        m_TexSlotSemantics[i] = texSlotSemantics[i];
    }
}

/*
//==============================================================================
// The following two method should be implemented in order to finish rip off the rest
// of the horrible CTexture slots management via static declarations in class CTexture!
// The first one is the initialization method for all engine textures slots and
// the second is a better candidate than LoadDefajultTextures that can replace it
// in order to have it data driven.
//------------------------------------------------------------------------------
void CTextureManager::CreateEngineTextures()
{
    ...
    Replace the code in CTexture::LoadDefaultSystemTextures() with a proper code to be 
    placed here.
    ...
}

void CTextureManager::LoadDefaultTexturesFromFile()
{
#if !defined(NULL_RENDERER)
    if (m_DefaultTextures.size())
    {
        return;
    }

    uint32 nDefaultFlags = FT_DONT_STREAM;

    XmlNodeRef root = GetISystem()->LoadXmlFromFile("EngineAssets/defaulttextures.xml");
    if (root)
    {
        // we loop over this twice.
        // we are looping from the back to the front to make sure that the order of the assets are correct ,when we try to load them the second time.
        for (int i = root->getChildCount() - 1; i >= 0; i--)
        {
            XmlNodeRef      textureEntry = root->getChild(i);
            if (!textureEntry->isTag("TextureEntry"))
            {
                continue;
            }

            // make an ASYNC request to move it to the top of the queue:
            XmlString   fileName;
            if (textureEntry->getAttr("FileName", fileName))
            {
                EBUS_EVENT(AzFramework::AssetSystemRequestBus, GetAssetStatus, fileName.c_str() );
            }
        }

        for (int i = 0; i < root->getChildCount(); i++)
        {
            XmlNodeRef entry = root->getChild(i);
            if (!entry->isTag("entry"))
            {
                continue;
            }

            if (entry->getAttr("nomips", nNoMips) && nNoMips)

                uint32 nFlags = nDefaultFlags;

            // check attributes to modify the loading flags
            int nNoMips = 0;
            if (entry->getAttr("nomips", nNoMips) && nNoMips)
            {
                nFlags |= FT_NOMIPS;
            }

            // default textures should be compiled synchronously:
            const char*     texName = entry->getContent();
            if (!gEnv->pCryPak->IsFileExist(texName))
            {
                // make a SYNC request to block until its ready.
                EBUS_EVENT(AzFramework::AssetSystemRequestBus, CompileAssetSync, texName);
            }

            CTexture* pTexture = CTexture::ForName(texName, nFlags, eTF_Unknown);
            if (pTexture)
            {
                CCryNameTSCRC nameID(texName);
                m_DefaultTextures[nameID] = pTexture;
            }
        }
    }
#endif
}
*/

