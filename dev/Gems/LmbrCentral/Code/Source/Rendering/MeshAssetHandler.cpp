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

#include "LmbrCentral_precompiled.h"

#include <AzCore/IO/GenericStreams.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzCore/std/parallel/binary_semaphore.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include "MeshAssetHandler.h"

#include <ICryAnimation.h>

namespace LmbrCentral
{   
    //////////////////////////////////////////////////////////////////////////
    const AZStd::string MeshAssetHandlerHelper::s_assetAliasToken = "@assets@/";

    // what mesh do we use as a placeholder when its currently busy compiling?
    static const char* g_meshCompilingSubstituteAsset = "engineassets/objects/default.cgf";

    MeshAssetHandlerHelper::MeshAssetHandlerHelper()
        : m_asyncLoadCvar(nullptr)
    {
    }

    void MeshAssetHandlerHelper::StripAssetAlias(const char*& assetPath)
    {
        const size_t assetAliasTokenLen = s_assetAliasToken.size() - 1;
        if (0 == strncmp(assetPath, s_assetAliasToken.c_str(), assetAliasTokenLen))
        {
            assetPath += assetAliasTokenLen;
        }
    }

    ICVar* MeshAssetHandlerHelper::GetAsyncLoadCVar()
    {
        if (!m_asyncLoadCvar)
        {
            m_asyncLoadCvar = gEnv->pConsole->GetCVar(s_meshAssetHandler_AsyncCvar);
        }

        return m_asyncLoadCvar;
    }

    //////////////////////////////////////////////////////////////////////////
    // Static Mesh Asset Handler
    //////////////////////////////////////////////////////////////////////////

    void AsyncStatObjLoadCallback(const AZ::Data::Asset<MeshAsset>& asset, _smart_ptr<IStatObj> statObj)
    {
        if (statObj)
        {
            asset.Get()->m_statObj = statObj;
        }
        else
        {
#if defined(AZ_ENABLE_TRACING)
            AZStd::string assetDescription = asset.ToString<AZStd::string>();
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
            AZ_Error("MeshAssetHandler", false, "Failed to load mesh asset %s", assetDescription.c_str());
#endif // AZ_ENABLE_TRACING
        }
    }

    MeshAssetHandler::~MeshAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr MeshAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;

        AZ_Assert(type == AZ::AzTypeInfo<MeshAsset>::Uuid(), "Invalid asset type! We handle only 'MeshAsset'");

        return aznew MeshAsset();
    }

    AZ::Data::AssetId MeshAssetHandler::AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        // if tracing is disabled, we are likely in a situation where we specifically don't want any diagnostic information or "errors" to appear
        // so in that case, don't load anything, don't substitute anything, don't escalate anything, just let the empty blank asset return.
#if defined(AZ_ENABLE_TRACING)
        if (asset.GetId().IsValid())
        {
            // find out whether its still compiling or it will never be available because its source file is missing.
            // this also escalates it, if found, to the top of the build queue:
            AzFramework::AssetSystem::AssetStatus statusResult = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(statusResult, &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatusById, asset.GetId());

            if ((statusResult == AzFramework::AssetSystem::AssetStatus_Compiling) || (statusResult == AzFramework::AssetSystem::AssetStatus_Queued))
            {
                // note that we can also check other codes and substitute other meshes if we want, here...
                // its currently compiling and will finish soon.
                // substitute a placeholder mesh:

                if (!m_missingMeshAssetId.IsValid())
                {
                    // substitute the missing mesh assetId so that there's at least something to render that indicates a problem
                    // in builds where there is no diagnostics or tracing, don't substitute anything, to prefer that there's no visual indication that
                    // something is wrong in shipped games.
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(m_missingMeshAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, g_meshCompilingSubstituteAsset, azrtti_typeid<MeshAsset>(), false);
                    AZ_Error("Mesh Asset Handler", m_missingMeshAssetId.IsValid(), "Attempted to substitute %s for a missing asset, but it is also missing!", g_meshCompilingSubstituteAsset);
                }

                if (m_missingMeshAssetId.IsValid())
                {
                    AZ_TracePrintf("MeshAssetHandler", "   - substituting with default asset ID %s\n", m_missingMeshAssetId.ToString<AZStd::string>().c_str());
                    // substitute the missing mesh asset.
                    return m_missingMeshAssetId;
                }
            }
        }
#endif // defined(AZ_ENABLE_TRACING)

        // otherwise, if we get here, it means that either it was truly missing, in which case let an error occur, or the missing default substitute asset
        // is also itself missing!
        return AZ::Data::AssetId();
    }


    bool MeshAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, AZ::IO::GenericStream* /*stream*/, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        // Load from preloaded stream.
        AZ_Assert(false, "Favor loading through custom stream override of LoadAssetData, in order to load through CryPak.");
        return false;
    }

    bool MeshAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<MeshAsset>::Uuid(), "Invalid asset type! We only load 'MeshAsset'");
        if (MeshAsset* meshAsset = asset.GetAs<MeshAsset>())
        {
            AZ_Assert(!meshAsset->m_statObj.get(), "Attempting to create static mesh without cleaning up the old one.");

            // Strip the alias. StatObj instances are stored in a dictionary by their path,
            // so to share instances with legacy cry entities, we need to use the same un-aliased format.
            StripAssetAlias(assetPath);

            // Temporary cvar guard while async loading of legacy mesh formats is stabilized.
            ICVar* cvar = GetAsyncLoadCVar();
            if (!cvar || cvar->GetIVal() == 0)
            {
                if (gEnv->mMainThreadId != CryGetCurrentThreadId())
                {
                    AZStd::binary_semaphore signaller;

                    auto callback = [&asset, &signaller](IStatObj* obj)
                    {
                        AsyncStatObjLoadCallback(asset, obj);
                        signaller.release();
                    };

                    gEnv->p3DEngine->LoadStatObjAsync(callback, assetPath);
                    signaller.acquire();
                }
                else
                {
                    AsyncStatObjLoadCallback(asset, gEnv->p3DEngine->LoadStatObjAutoRef(assetPath));
                }
            }
            else
            {
                _smart_ptr<IStatObj> statObj = gEnv->p3DEngine->LoadStatObjAutoRef(assetPath);

                if (statObj)
                {
                    meshAsset->m_statObj = statObj;
                }
                else
                {
#if defined(AZ_ENABLE_TRACING)
                    AZStd::string assetDescription = asset.GetId().ToString<AZStd::string>();
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
                    AZ_Error("MeshAssetHandler", false, "Failed to load mesh asset \"%s\".", assetDescription.c_str());
#endif // AZ_ENABLE_TRACING
                }
            }

            return true;
        }
        return false;
    }

    void MeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void MeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<MeshAsset>::Uuid());
    }

    void MeshAssetHandler::ProcessQueuedAssetRequests()
    {
        gEnv->p3DEngine->ProcessAsyncStaticObjectLoadRequests();
    }

    void MeshAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterLegacyHandler(this, AZ::AzTypeInfo<MeshAsset>::Uuid(), AZStd::this_thread::get_id());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MeshAsset>::Uuid());
    }

    void MeshAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<MeshAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType MeshAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<MeshAsset>::Uuid();
    }

    const char* MeshAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Static Mesh";
    }

    const char* MeshAssetHandler::GetGroup() const
    {
        return "Geometry";
    }

    const char* MeshAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/StaticMesh.svg";
    }

    AZ::Uuid MeshAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid("{FC315B86-3280-4D03-B4F0-5553D7D08432}");
    }

    void MeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(CRY_GEOMETRY_FILE_EXT);
    }

    //////////////////////////////////////////////////////////////////////////
    // Skinned Mesh Asset Handler
    //////////////////////////////////////////////////////////////////////////
    
    void AsyncCharacterInstanceLoadCallback(const AZ::Data::Asset<CharacterDefinitionAsset>& asset, ICharacterInstance* instance)
    {
        if (instance)
        {
            asset.Get()->m_characterInstance = instance;
        }
        else
        {
#if defined(AZ_ENABLE_TRACING)
            AZStd::string assetDescription = asset.ToString<AZStd::string>();
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
            AZ_Error("MeshAssetHandler", false, "Failed to load character instance asset %s", asset.GetId().ToString<AZStd::string>().c_str());
#endif // AZ_ENABLE_TRACING
        }
    }

    CharacterDefinitionAssetHandler::~CharacterDefinitionAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr CharacterDefinitionAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;

        AZ_Assert(type == AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid(), "Invalid asset type! We handle only 'CharacterDefinitionAsset'");
        
        return aznew CharacterDefinitionAsset();
    }

    bool CharacterDefinitionAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, AZ::IO::GenericStream* /*stream*/, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        // Load from preloaded stream.
        AZ_Assert(false, "Favor loading through custom stream override of LoadAssetData, in order to load through CryPak.");
        return false;
    }

    bool CharacterDefinitionAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        if (!gEnv->pCharacterManager)
        {
            return false;
        }

        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid(), "Invalid asset type! We only load 'CharacterDefinitionAsset'");
        if (CharacterDefinitionAsset* meshAsset = asset.GetAs<CharacterDefinitionAsset>())
        {
            AZ_Assert(!meshAsset->m_characterInstance.get(), "Attempting to create character instance without cleaning up the old one.");

            // Strip the alias. Character instances are stored in a dictionary by their path,
            // so to share instances with legacy cry entities, we need to use the same un-aliased format.
            StripAssetAlias(assetPath);
            
            // Temporary cvar guard while async loading of legacy mesh formats is stabilized.
            ICVar* cvar = GetAsyncLoadCVar();
            if (!cvar || cvar->GetIVal() == 0)
            {
                // only queue this if we're not on the main thread
                if (gEnv->mMainThreadId != CryGetCurrentThreadId())
                {
                    AZStd::binary_semaphore signaller;

                    auto callback = [&asset, &signaller](ICharacterInstance* instance)
                    {
                        AsyncCharacterInstanceLoadCallback(asset, instance);
                        signaller.release();
                    };

                    
                    gEnv->pCharacterManager->CreateInstanceAsync(callback, assetPath);
                    signaller.acquire();
                }
                else
                {
                    AsyncCharacterInstanceLoadCallback(asset, gEnv->pCharacterManager->CreateInstance(assetPath));
                }
            }
            else
            {
                ICharacterInstance* instance = gEnv->pCharacterManager->CreateInstance(assetPath);

                if (instance)
                {
                    meshAsset->m_characterInstance = instance;
                }
                else
                {
#if defined(AZ_ENABLE_TRACING)
                    AZStd::string assetDescription = asset.ToString<AZStd::string>();
                    EBUS_EVENT_RESULT(assetDescription, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());
                    AZ_Error("MeshAssetHandler", false, "Failed to load character instance asset %s", asset.GetId().ToString<AZStd::string>().c_str());
#endif // AZ_ENABLE_TRACING
                }
            }

            return true;
        }

        return false;
    }

    void CharacterDefinitionAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void CharacterDefinitionAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid());
    }

    void CharacterDefinitionAssetHandler::ProcessQueuedAssetRequests()
    {
        gEnv->pCharacterManager->ProcessAsyncLoadRequests();
    }

    void CharacterDefinitionAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterLegacyHandler(this, AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid(), AZStd::this_thread::get_id());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid());
    }

    void CharacterDefinitionAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType CharacterDefinitionAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid();
    }

    const char* CharacterDefinitionAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Character Definition";
    }

    const char * CharacterDefinitionAssetHandler::GetGroup() const
    {
        return "Geometry";
    }

    const char* CharacterDefinitionAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/SkinnedMesh.svg";
    }

    AZ::Uuid CharacterDefinitionAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid("{D3E1A9FC-56C9-4997-B56B-DA186EE2D62A}");
    }

    void CharacterDefinitionAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(CRY_CHARACTER_DEFINITION_FILE_EXT);
    }
    //////////////////////////////////////////////////////////////////////////

    GeomCacheAssetHandler::~GeomCacheAssetHandler()
    {
        Unregister();
    }

    AZ::Data::AssetPtr GeomCacheAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;

        AZ_Assert(type == AZ::AzTypeInfo<GeomCacheAsset>::Uuid(), "Invalid asset type! We handle only 'GeomCacheAsset'");

        return aznew GeomCacheAsset();
    }

    bool GeomCacheAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, AZ::IO::GenericStream* /*stream*/, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        // Load from preloaded stream.
        AZ_Assert(false, "Favor loading through custom stream override of LoadAssetData, in order to load through CryPak.");
        return false;
    }

    bool GeomCacheAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/)
    {
        AZ_Assert(asset.GetType() == AZ::AzTypeInfo<GeomCacheAsset>::Uuid(), "Invalid asset type! We only load 'GeomCacheAsset'");
        if (GeomCacheAsset* geomCacheAsset = asset.GetAs<GeomCacheAsset>())
        {
            AZ_Assert(!geomCacheAsset->m_geomCache.get(), "Attempting to create geom cache without cleaning up the old one.");

            // Strip the alias. GeomCache instances are stored in a dictionary by their path,
            // so to share instances with legacy cry entities, we need to use the same un-aliased format.
            StripAssetAlias(assetPath);

            //Load GeomCaches synchronously, there is no Async support currently in the 3DEngine.
            //Assets can stream asynchronously but this load step must be synchronous. 

            _smart_ptr<IGeomCache> geomCache = gEnv->p3DEngine->LoadGeomCache(assetPath);

            if (geomCache)
            {
                geomCache->SetProcessedByRenderNode(false);
                geomCacheAsset->m_geomCache = geomCache;
            }
            else
            {
#if defined(AZ_ENABLE_TRACING)
                AZStd::string assetDescription = asset.ToString<AZStd::string>();
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetDescription, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, asset.GetId());
                AZ_Error("GeomCacheAssetHandler", false, "Failed to load geom cache asset %s", assetDescription.c_str());
#endif // AZ_ENABLE_TRACING
            }

            return true;
        }
        return false;
    }

    void GeomCacheAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void GeomCacheAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<GeomCacheAsset>::Uuid());
    }

    void GeomCacheAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");
        AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<GeomCacheAsset>::Uuid());

        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<GeomCacheAsset>::Uuid());
    }

    void GeomCacheAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<GeomCacheAsset>::Uuid());

        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    AZ::Data::AssetType GeomCacheAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<GeomCacheAsset>::Uuid();
    }

    const char* GeomCacheAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Geom Cache";
    }

    const char* GeomCacheAssetHandler::GetGroup() const
    {
        return "Geometry";
    }

    AZ::Uuid GeomCacheAssetHandler::GetComponentTypeId() const
    {
        return AZ::Uuid("{045C0C58-C13E-49B0-A471-D4AC5D3FC6BD}");
    }

    void GeomCacheAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.push_back(CRY_GEOM_CACHE_FILE_EXT);
    }

} // namespace LmbrCentral
