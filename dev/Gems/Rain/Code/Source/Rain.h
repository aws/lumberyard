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
#ifndef CRYINCLUDE_GAMEDLL_ENVIRONMENT_RAIN_H
#define CRYINCLUDE_GAMEDLL_ENVIRONMENT_RAIN_H
#pragma once

#include <IGameObject.h>

namespace LYGame
{
    class CRain
        : public CGameObjectExtensionHelper < CRain, IGameObjectExtension >
    {
    public:

        DECLARE_COMPONENT_TYPE("ComponentRain", 0xD822390820164C00, 0xAD3B490D08E7F9A6);

        CRain();
        virtual ~CRain();

        // IGameObjectExtension
        bool Init(IGameObject* pGameObject) override;
        void InitClient(ChannelId channelId) override {}
        void PostInit(IGameObject* pGameObject) override;
        void PostInitClient(ChannelId channelId) override {}
        bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
        void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override { }
        bool GetEntityPoolSignature(TSerialize signature) override;
        bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override { return true; }
        void FullSerialize(TSerialize ser) override;
        void PostSerialize() override { }
        void SerializeSpawnInfo(TSerialize ser, IEntity*) override { }
        ISerializableInfoPtr GetSpawnInfo() override { return 0; }
        void Update(SEntityUpdateContext& ctx, int updateSlot) override;
        void PostUpdate(float frameTime) override {}
        void PostRemoteSpawn() override {}
        void HandleEvent(const SGameObjectEvent&) override;
        void ProcessEvent(SEntityEvent&) override;
        void SetChannelId(ChannelId id) override { }
        void SetAuthority(bool auth) override;
        void GetMemoryUsage(ICrySizer* pSizer) const override { pSizer->Add(*this); }

        //~IGameObjectExtension

        bool Reset();

    protected:

        void PreloadTextures();

        SRainParams m_params;
        bool        m_bEnabled;

        typedef std::vector<ITexture*> TTextureList;
        TTextureList m_Textures;

    private:
        CRain(const CRain&);
        CRain& operator = (const CRain&);
    };

    // Extracted from macro REGISTER_GAME_OBJECT();
    struct CRainCreator
        : public IGameObjectExtensionCreatorBase
    {
        IGameObjectExtensionPtr Create()
        {
            return gEnv->pEntitySystem->CreateComponent<CRain>();
        }

        void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
        {
            CRain::GetGameObjectExtensionRMIData(ppRMI, nCount);
        }
    };
} // LYGame

#endif // CRYINCLUDE_GAMEDLL_ENVIRONMENT_RAIN_H
