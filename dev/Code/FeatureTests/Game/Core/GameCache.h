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
#pragma once

namespace LYGame
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Different cache types.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    namespace CacheTypes
    {
        /// Types of cached resources.
        enum Resource
        {
            eResource_Texture                   = 0,
            eResource_TextureDeferredCubemap    = 1,
            eResource_StaticObject              = 2,
            eResource_Material                  = 3
        };

        /// Character file model cache types.
        enum CharacterFileModel
        {
            eCharacterFileModel_Default,
            eCharacterFileModel_Client,
            eCharacterFileModel_Shadow,
            eCharacterFileModel_Count,
        };
    }

    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Caches game resources.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class GameCache
    {
        struct TextureKey
        {
            TextureKey()
                : NameCRC(0U)
                , TextureFlags(0) {}

            TextureKey(const char* name, int textureFlags)
                : NameCRC(name)
                , TextureFlags(textureFlags) {}

            struct Compare
            {
                bool operator()(const TextureKey& key1, const TextureKey& key2) const
                {
                    return
                        (key1.NameCRC < key2.NameCRC) ||
                        ((key1.NameCRC == key2.NameCRC) && (key1.TextureFlags < key2.TextureFlags));
                }
            };

            CCryNameCRC NameCRC;
            int         TextureFlags;
        };

        // Texture type definitions.
        typedef _smart_ptr<ITexture>                                    ITexturePtr;
        typedef std::map<TextureKey, ITexturePtr, TextureKey::Compare>  TextureCacheMap;

        // Static object type definitions.
        typedef _smart_ptr<IStatObj>                IStatObjPtr;
        typedef std::map<CCryNameCRC, IStatObjPtr>  StatObjCacheMap;

        // Material type definitions.
        typedef _smart_ptr<IMaterial>               IMaterialPtr;
        typedef std::map<CCryNameCRC, IMaterialPtr> MaterialCacheMap;

        // Character instance type definitions.
        typedef _smart_ptr<ICharacterInstance>                  ICharacterInstancePtr;
        typedef std::map<CCryNameCRC, ICharacterInstancePtr>    CharacterFileModelCacheMap;

    public:
        GameCache();
        virtual ~GameCache();

        void Init();
        void Reset();
        void GetMemoryUsage(ICrySizer* sizer) const;

        bool CacheTexture(const char* fileName, int textureFlags);
        bool CacheGeometry(const char* fileName);
        bool CacheMaterial(const char* fileName);

    private:
        // Character file model cache helpers.
        bool AddCachedCharacterFileModel(CacheTypes::CharacterFileModel type, const char* fileName);
        bool IsCharacterFileModelCached(const char* fileName) const;
        bool IsFileNameValid(const char* fileName) const { return fileName && fileName[0]; }

    private:
        // Multiple resource cache, for game elements which might not have proper resource management.
        TextureCacheMap     m_textureCache;
        MaterialCacheMap    m_materialCache;
        StatObjCacheMap     m_statObjCache;

        // Character file model caches.
        CharacterFileModelCacheMap m_editorCharacterFileModelCaches[CacheTypes::eCharacterFileModel_Count];
    };
} // namespace LYGame