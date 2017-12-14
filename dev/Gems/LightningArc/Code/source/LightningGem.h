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

#include <GameEffectSystem/IGameEffectSystem.h>
#include <LightningArc/LightningArcBus.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace Lightning
{
    class LightningArcConverter;
    class LightningConverter;

    class LightningGem
        : public CryHooksModule
        , public LightningArcRequestBus::Handler
        , public GameEffectSystemNotificationBus::Handler
    {
    public:
        AZ_RTTI(LightningGem, "{89724952-ADBF-478A-AFFE-784BD0952E2D}", CryHooksModule);

        LightningGem();
        ~LightningGem();

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        void PostSystemInit();
        void Shutdown();
        
        CScriptBind_LightningArc* GetScriptBind() const override;
        CLightningGameEffect* GetGameEffect() const override;
        AZStd::shared_ptr<LightningGameEffectAZ> GetGameEffectAZ() const override;

        void LoadArcPresets() override;
        const AZStd::vector<AZStd::string>& GetArcPresetNames() const override;
        bool GetArcParamsForName(const AZStd::string& arcName, const LightningArcParams*& arcParams) const override;

    protected:
        AZStd::shared_ptr<LightningGameEffectAZ> m_gameEffectAZ; ///< LightningGameEffect that handles AZ entities and AZ types

        CLightningGameEffect* m_gameEffect = nullptr; ///< Legacy LightningGameEffect that handles Cry entities and Cry types
        CScriptBind_LightningArc* m_lightningArcScriptBind = nullptr; ///< Legacy script bind
        int g_gameFXLightningProfile; ///< Legacy cvar for toggling lightning profiling

        AZStd::vector<LightningArcParams> m_arcPresets; ///< List of available arc params from loaded asset library
        AZStd::vector<AZStd::string> m_arcPresetNames; ///< List of arc preset names; maps to m_arcPresets by index

        // GameEffectSystemNotificationBus
        void OnReleaseGameEffects() override;

#ifdef LIGHTNING_EDITOR
        AZStd::unique_ptr<Lightning::LightningArcConverter> m_lightningArcConverter;
        AZStd::unique_ptr<Lightning::LightningConverter> m_lightningConverter;
#endif //LIGHTNING_EDITOR
    };

} //namespace Lightning