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
#ifndef _GEM_LIGHTNINGARC_H_
#define _GEM_LIGHTNINGARC_H_

#include <GameEffectSystem/IGameEffectSystem.h>
#include <LightningArc/LightningArcBus.h>

class LightningArcGem
    : public CryHooksModule
    , public LightningArcRequestBus::Handler
    , public GameEffectSystemNotificationBus::Handler
{
public:
    AZ_RTTI(LightningArcGem, "{89724952-ADBF-478A-AFFE-784BD0952E2D}", CryHooksModule);

    LightningArcGem();
    ~LightningArcGem() override;

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    void PostSystemInit();
    void Shutdown();

public:
    CScriptBind_LightningArc* GetScriptBind() const override;
    CLightningGameEffect* GetGameEffect() const override;

protected:
    CLightningGameEffect* m_gameEffect = nullptr;
    CScriptBind_LightningArc* m_lightningArcScriptBind = nullptr;
    int g_gameFXLightningProfile;

    //////////////////////////////////////////////////////////////////////////
    // GameEffectSystemNotificationBus
    void OnReleaseGameEffects() override;
    //////////////////////////////////////////////////////////////////////////
};

#endif //_GEM_LIGHTNINGARC_H_