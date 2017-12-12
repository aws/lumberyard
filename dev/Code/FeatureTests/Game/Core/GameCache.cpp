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
#include "GameCache.h"
#include <ICryAnimation.h>

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

GameCache::GameCache()
    : m_textureCache()
    , m_materialCache()
    , m_statObjCache()
{
}

GameCache::~GameCache()
{
}

void GameCache::Init()
{
}

void GameCache::Reset()
{
    m_textureCache.clear();
    m_materialCache.clear();
    m_statObjCache.clear();

    for (int i = 0; i < CacheTypes::eCharacterFileModel_Count; ++i)
    {
        m_editorCharacterFileModelCaches[i].clear();
    }
}

void GameCache::GetMemoryUsage(ICrySizer* sizer) const
{
    sizer->AddContainer(m_textureCache);
    sizer->AddContainer(m_materialCache);
    sizer->AddContainer(m_statObjCache);

    for (int i = 0; i < CacheTypes::eCharacterFileModel_Count; ++i)
    {
        sizer->AddContainer(m_editorCharacterFileModelCaches[i]);
    }
}

bool GameCache::CacheTexture(const char* fileName, int textureFlags)
{
    bool addedToCache = false;

    if (IsFileNameValid(fileName))
    {
        const TextureKey textureKey(fileName, textureFlags);

        // Only add the texture if it is not already in the cache.
        if (m_textureCache.find(textureKey) == m_textureCache.end())
        {
            if (ITexture* texture = gEnv->pRenderer->EF_LoadTexture(fileName, textureFlags))
            {
                m_textureCache.emplace(std::make_pair(textureKey, ITexturePtr(texture)));
                texture->Release();
                addedToCache = true;
            }
        }
    }

    return addedToCache;
}

bool GameCache::CacheGeometry(const char* fileName)
{
    bool addedToCache = false;

    if (IsFileNameValid(fileName))
    {
        const stack_string fileExtension = PathUtil::GetExt(fileName);

        if ((fileExtension == CRY_CHARACTER_DEFINITION_FILE_EXT) || (fileExtension == CRY_SKEL_FILE_EXT) || (fileExtension == CRY_ANIM_GEOMETRY_FILE_EXT))
        {
            addedToCache = AddCachedCharacterFileModel(CacheTypes::eCharacterFileModel_Default, fileName);
        }
        else
        {
            const CCryNameCRC fileNameCRC = fileName;

            // Only add the static object if it is not already in the cache.
            if (m_statObjCache.find(fileNameCRC) == m_statObjCache.end())
            {
                if (_smart_ptr<IStatObj> staticObject = gEnv->p3DEngine->LoadStatObjAutoRef(fileName))
                {
                    m_statObjCache.emplace(std::make_pair(fileNameCRC, IStatObjPtr(staticObject)));
                    addedToCache = true;
                }
            }
        }
    }

    return addedToCache;
}

bool GameCache::CacheMaterial(const char* fileName)
{
    bool addedToCache = false;

    if (IsFileNameValid(fileName))
    {
        const CCryNameCRC fileNameCRC = fileName;

        // Only add the material if it is not already in the cache.
        if (m_materialCache.find(fileNameCRC) == m_materialCache.end())
        {
            if (_smart_ptr<IMaterial> material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(fileName))
            {
                m_materialCache.emplace(std::make_pair(fileNameCRC, material));
                addedToCache = true;
            }
        }
    }

    return addedToCache;
}

bool GameCache::AddCachedCharacterFileModel(CacheTypes::CharacterFileModel type, const char* fileName)
{
    bool addedToCache = false;

    if (IsFileNameValid(fileName))
    {
        if (gEnv->IsEditor())
        {
            if (!IsCharacterFileModelCached(fileName))
            {
                if (ICharacterInstance* cachedCharacterInstance = gEnv->pCharacterManager->CreateInstance(fileName))
                {
                    m_editorCharacterFileModelCaches[type].emplace(std::make_pair(CCryNameCRC(fileName), ICharacterInstancePtr(cachedCharacterInstance)));
                    addedToCache = true;
                }
            }
        }
        else
        {
            addedToCache = gEnv->pCharacterManager->LoadAndLockResources(fileName, 0);
        }
    }

    return addedToCache;
}

bool GameCache::IsCharacterFileModelCached(const char* fileName) const
{
    bool isCached = false;
    const CCryNameCRC fileNameCRC = fileName;

    for (int i = 0; !isCached && (i < CacheTypes::eCharacterFileModel_Count); ++i)
    {
        const auto& cache = m_editorCharacterFileModelCaches[i];

        if (cache.find(fileNameCRC) != cache.cend())
        {
            isCached = true;
        }
    }

    return isCached;
}