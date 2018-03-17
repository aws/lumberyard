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
#include "LightningArc_precompiled.h"
#include <platform_impl.h>

#include <IEntityClass.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "LightningGem.h"
#include "LightningArc.h"
#include "LightningGameEffectCry.h"
#include "LightningGameEffectAz.h"
#include "ScriptBind_LightningArc.h"

#include "LightningArcComponent.h"
#include "LightningComponent.h"
#include "SkyHighlightComponent.h"
#ifdef LIGHTNING_EDITOR
#include "EditorLightningArcComponent.h"
#include "EditorLightningComponent.h"
#include "EditorSkyHighlightComponent.h"
#endif // LIGHTNING_EDITOR

#include <AzCore/std/smart_ptr/make_shared.h>

namespace Lightning
{
    LightningGem::LightningGem()
    {
        m_descriptors.push_back(Lightning::LightningArcComponent::CreateDescriptor());
        m_descriptors.push_back(Lightning::LightningComponent::CreateDescriptor());
        m_descriptors.push_back(Lightning::SkyHighlightComponent::CreateDescriptor());

#ifdef LIGHTNING_EDITOR
        m_descriptors.push_back(Lightning::EditorLightningArcComponent::CreateDescriptor());
        m_descriptors.push_back(Lightning::EditorLightningComponent::CreateDescriptor());
        m_descriptors.push_back(Lightning::EditorSkyHighlightComponent::CreateDescriptor());

        m_lightningArcConverter = AZStd::make_unique<Lightning::LightningArcConverter>();
        m_lightningConverter = AZStd::make_unique<Lightning::LightningConverter>();

        m_lightningArcConverter->BusConnect();
        m_lightningConverter->BusConnect();
#endif // LIGHTNING_EDITOR

        LightningArcRequestBus::Handler::BusConnect();
    }

    LightningGem::~LightningGem()
    {
#ifdef LIGHTNING_EDITOR
        m_lightningArcConverter->BusDisconnect();
        m_lightningConverter->BusDisconnect();
#endif // LIGHTNING_EDITOR

        LightningArcRequestBus::Handler::BusDisconnect();
    }

    void LightningGem::PostSystemInit()
    {
        REGISTER_CVAR(g_gameFXLightningProfile, 0, 0, "Toggles game effects system lightning arc profiling");

        LoadArcPresets();

        m_gameEffectAZ = AZStd::make_shared<LightningGameEffectAZ>();
        m_gameEffectAZ->Initialize();

        // Init GameEffect
        m_gameEffect = new CLightningGameEffect();
        m_gameEffect->Initialize();

        // Init ScriptBind
        m_lightningArcScriptBind = new CScriptBind_LightningArc(GetISystem());

        // Init GameObjectExtension
        // Originally registered with REGISTER_GAME_OBJECT(pFramework, LightningArc, "Scripts/Entities/Environment/LightningArc.lua");
        // If more objects need registered, consider bringing the macro back along with the GameFactory wrapper.
        IEntityClassRegistry::SEntityClassDesc clsDesc;
        clsDesc.sName = "LightningArc";
        clsDesc.sScriptFile = "Scripts/Entities/Environment/LightningArc.lua";
        static CLightningArcCreator _creator;
        GetISystem()->GetIGame()->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension("LightningArc", &_creator, &clsDesc);
    }

    void LightningGem::Shutdown()
    {
        m_gameEffectAZ = nullptr;

        if (m_gameEffect)
        {
            m_gameEffect->Release();
        }
        SAFE_DELETE(m_gameEffect);
        SAFE_DELETE(m_lightningArcScriptBind);
    }

    void LightningGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_GAME_POST_INIT:
            IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*gEnv->pEntitySystem->GetComponentFactoryRegistry());
            GameEffectSystemNotificationBus::Handler::BusConnect();
            break;

        case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
            RegisterExternalFlowNodes();
            break;

            // Called on ESYSTEM_EVENT_GAME_POST_INIT_DONE instead of ESYSTEM_EVENT_GAME_POST_INIT because the GameEffectSystem Gem
            // uses ESYSTEM_EVENT_GAME_POST_INIT to initialize, and this requires that has happened already.
        case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
            PostSystemInit();
            break;

        case ESYSTEM_EVENT_FULL_SHUTDOWN:
        case ESYSTEM_EVENT_FAST_SHUTDOWN:
            GameEffectSystemNotificationBus::Handler::BusDisconnect();
            Shutdown();
            break;
        }
    }

    CScriptBind_LightningArc* LightningGem::GetScriptBind() const
    {
        return m_lightningArcScriptBind;
    }

    CLightningGameEffect* LightningGem::GetGameEffect() const
    {
        return m_gameEffect;
    }

    AZStd::shared_ptr<LightningGameEffectAZ> LightningGem::GetGameEffectAZ() const
    {
        return m_gameEffectAZ;
    }

    void LightningGem::LoadArcPresets()
    {
        const char* fileLocation = "Libs/LightningArc/LightningArcEffects.xml";
        XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFromFile(fileLocation);

        if (!rootNode)
        {
            AZ_Warning("LightningGameEffectAZ", "Could not load lightning data. Invalid XML file '%s'! ", fileLocation);
            return;
        }

        m_arcPresets.clear();
        m_arcPresetNames.clear();

        size_t numPresets = static_cast<size_t>(rootNode->getChildCount());
        m_arcPresets.resize(numPresets);

        for (size_t i = 0; i < numPresets; ++i)
        {
            XmlNodeRef preset = rootNode->getChild(i);
            const char* presetName = preset->getAttr("name");
            m_arcPresets[i].m_azCrc = AZ::Crc32(presetName);
            m_arcPresets[i].Reset(preset);

            m_arcPresetNames.push_back(presetName);
        }
    }

    const AZStd::vector<AZStd::string>& LightningGem::GetArcPresetNames() const
    {
        return m_arcPresetNames;
    }

    bool LightningGem::GetArcParamsForName(const AZStd::string& arcName, const LightningArcParams*& arcParams) const
    {
        for (size_t i = 0; i < m_arcPresets.size(); ++i)
        {
            if (m_arcPresets[i].m_azCrc == AZ::Crc32(arcName.c_str()))
            {
                arcParams = &m_arcPresets[i];
                return true;
            }
        }

        return false;
    }

    void LightningGem::OnReleaseGameEffects()
    {
        Shutdown();
    }
} // namespace Lightning 

AZ_DECLARE_MODULE_CLASS(LightningArc_4c28210b23544635aa15be668dbff15d, Lightning::LightningGem)
