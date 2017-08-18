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
#include "Sprite.h"
#include <ITexture.h>
#include <CryPath.h>
#include <IRenderer.h>
#include <ISerialize.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace
{
    const char* const spriteExtension = "sprite";

    // Increment this when the Sprite Serialize(TSerialize) function
    // changes to be incompatible with previous data
    uint32 spriteFileVersionNumber = 1;
    const char* spriteVersionNumberTag = "versionNumber";

    const char* allowedSpriteTextureExtensions[] = {
        "tif", "jpg", "jpeg", "tga", "bmp", "png", "gif", "dds"
    };
    const int numAllowedSpriteTextureExtensions = AZ_ARRAY_SIZE(allowedSpriteTextureExtensions);

    bool IsValidSpriteTextureExtension(const char* extension)
    {
        for (int i = 0; i < numAllowedSpriteTextureExtensions; ++i)
        {
            if (strcmp(extension, allowedSpriteTextureExtensions[i]) == 0)
            {
                return true;
            }
        }

        return false;
    }

    bool ReplaceSpriteExtensionWithTextureExtension(const string& spritePath, string& texturePath)
    {
        string testPath;
        for (int i = 0; i < numAllowedSpriteTextureExtensions; ++i)
        {
            testPath = PathUtil::ReplaceExtension(spritePath, allowedSpriteTextureExtensions[i]);
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO && fileIO->Exists(testPath.c_str()))
            {
                texturePath = testPath;
                return true;
            }
        }

        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// STATIC MEMBER DATA
////////////////////////////////////////////////////////////////////////////////////////////////////

CSprite::CSpriteHashMap* CSprite::s_loadedSprites;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite::CSprite()
    : m_texture(nullptr)
{
    AddRef();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite::~CSprite()
{
    if (m_texture)
    {
        // In order to avoid the texture being deleted while there are still commands on the render
        // thread command queue that use it, we queue a command to delete the texture onto the
        // command queue.
        SResourceAsync* pInfo = new SResourceAsync();
        pInfo->eClassName = eRCN_Texture;
        pInfo->pResource = m_texture;
        gEnv->pRenderer->ReleaseResourceAsync(pInfo);
    }

    s_loadedSprites->erase(m_pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const string& CSprite::GetPathname() const
{
    return m_pathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const string& CSprite::GetTexturePathname() const
{
    return m_texturePathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite::Borders CSprite::GetBorders() const
{
    return m_borders;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::SetBorders(Borders borders)
{
    m_borders = borders;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ITexture* CSprite::GetTexture() const
{
    // If the sprite is associated with a render target, the sprite does not own the texture.
    // In this case, find the texture by name when it is requested.
    if (!m_texture && !m_pathname.empty())
    {
        return gEnv->pRenderer->EF_GetTextureByName(m_pathname.c_str());
    }
    return m_texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::Serialize(TSerialize ser)
{
    ser.BeginGroup("Sprite");

    ser.Value("m_left", m_borders.m_left);
    ser.Value("m_right", m_borders.m_right);
    ser.Value("m_top", m_borders.m_top);
    ser.Value("m_bottom", m_borders.m_bottom);

    ser.EndGroup();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::SaveToXml(const string& pathname)
{
    // NOTE: The input pathname has to be a path that can used to save - so not an Asset ID
    // because of this we do not store the pathname

    XmlNodeRef root =  GetISystem()->CreateXmlNode("Sprite");
    std::unique_ptr<IXmlSerializer> pSerializer(GetISystem()->GetXmlUtils()->CreateXmlSerializer());
    ISerialize* pWriter = pSerializer->GetWriter(root);
    TSerialize ser = TSerialize(pWriter);

    ser.Value(spriteVersionNumberTag, spriteFileVersionNumber);
    Serialize(ser);

    return root->saveToFile(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::AreBordersZeroWidth() const
{
    return (m_borders.m_left == 0 && m_borders.m_right == 1 && m_borders.m_top == 0 && m_borders.m_bottom == 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CSprite::GetSize() const
{
    ITexture* texture = GetTexture();
    if (texture)
    {
        return AZ::Vector2(static_cast<float>(texture->GetWidth()), static_cast<float>(texture->GetHeight()));
    }
    else
    {
        return AZ::Vector2(0.0f, 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::Initialize()
{
    s_loadedSprites = new CSpriteHashMap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::Shutdown()
{
    delete s_loadedSprites;
    s_loadedSprites = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite* CSprite::LoadSprite(const string& pathname)
{
    // the input string could be in any form. So make it normalized
    // NOTE: it should not be a full path at this point. If called from the UI editor it will
    // have been transformed to a game path. If being called with a hard coded path it should be a
    // game path already - it is not good for code to be using full paths.
    AZStd::string assetPath(pathname.c_str());
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, assetPath);
    string gamePath = assetPath.c_str();

    // check the extension and work out the pathname of the sprite file and the texture file
    // currently it works if the input path is either a sprite file or a texture file
    const char* extension = PathUtil::GetExt(gamePath.c_str());

    string spritePath;
    string texturePath;
    if (strcmp(extension, spriteExtension) == 0)
    {
        spritePath = gamePath;

        // look for a texture file with the same name
        if (!ReplaceSpriteExtensionWithTextureExtension(spritePath, texturePath))
        {
            gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
                gamePath.c_str(), "No texture file found for sprite: %s, no sprite will be used", gamePath.c_str());
            return nullptr;
        }
    }
    else if (IsValidSpriteTextureExtension(extension))
    {
        texturePath = gamePath;
        spritePath = PathUtil::ReplaceExtension(gamePath, spriteExtension);
    }
    else
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            gamePath.c_str(), "Invalid file extension for sprite: %s, no sprite will be used", gamePath.c_str());
        return nullptr;
    }

    // test if the sprite is already loaded, if so return loaded sprite
    auto result = s_loadedSprites->find(spritePath);
    CSprite* loadedSprite = (result == s_loadedSprites->end()) ? nullptr : result->second;

    if (loadedSprite)
    {
        loadedSprite->AddRef();
        return loadedSprite;
    }

    // load the texture file
    uint32 loadTextureFlags = (FT_USAGE_ALLOWREADSRGB | FT_DONT_STREAM);
    ITexture* texture = gEnv->pRenderer->EF_LoadTexture(texturePath.c_str(), loadTextureFlags);

    if (!texture || !texture->IsTextureLoaded())
    {
        gEnv->pSystem->Warning(
            VALIDATOR_MODULE_SHINE,
            VALIDATOR_WARNING,
            VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            texturePath.c_str(),
            "No texture file found for sprite: %s, no sprite will be used. "
            "NOTE: File must be in current project or a gem.",
            spritePath.c_str());
        return nullptr;
    }

    // create Sprite object
    CSprite* sprite = new CSprite;

    sprite->m_texture = texture;
    sprite->m_pathname = spritePath;
    sprite->m_texturePathname = texturePath;

    // try to load the sprite side-car file if it exists, it is optional and if it does not
    // exist we just stay with default values
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (fileIO && fileIO->Exists(sprite->m_pathname.c_str()))
    {
        sprite->LoadFromXmlFile();
    }

    // add sprite to list of loaded sprites
    (*s_loadedSprites)[sprite->m_pathname] = sprite;

    return sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSprite* CSprite::CreateSprite(const string& renderTargetName)
{
    // test if the sprite is already loaded, if so return loaded sprite
    auto result = s_loadedSprites->find(renderTargetName);
    CSprite* loadedSprite = (result == s_loadedSprites->end()) ? nullptr : result->second;

    if (loadedSprite)
    {
        loadedSprite->AddRef();
        return loadedSprite;
    }

    // create Sprite object
    CSprite* sprite = new CSprite;

    sprite->m_texture = nullptr;
    sprite->m_pathname = renderTargetName;
    sprite->m_texturePathname.clear();

    // add sprite to list of loaded sprites
    (*s_loadedSprites)[sprite->m_pathname] = sprite;

    return sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSprite::ReplaceSprite(ISprite** baseSprite, ISprite* newSprite)
{
    if (baseSprite)
    {
        if (newSprite)
        {
            newSprite->AddRef();
        }

        SAFE_RELEASE(*baseSprite);

        *baseSprite = newSprite;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSprite::LoadFromXmlFile()
{
    XmlNodeRef root = GetISystem()->LoadXmlFromFile(m_pathname.c_str());

    if (!root)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            m_pathname.c_str(),
            "No sprite file found for sprite: %s, default sprite values will be used", m_pathname.c_str());
        return false;
    }

    std::unique_ptr<IXmlSerializer> pSerializer(GetISystem()->GetXmlUtils()->CreateXmlSerializer());
    ISerialize* pReader = pSerializer->GetReader(root);

    TSerialize ser = TSerialize(pReader);

    uint32 versionNumber = spriteFileVersionNumber;
    ser.Value(spriteVersionNumberTag, versionNumber);
    if (versionNumber != spriteFileVersionNumber)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE | VALIDATOR_FLAG_TEXTURE,
            m_pathname.c_str(),
            "Unsupported version number found for sprite file: %s, default sprite values will be used",
            m_pathname.c_str());
        return false;
    }

    Serialize(ser);

    return true;
}
