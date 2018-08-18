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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/atomic.h>

#include <CrySystemBus.h>
#include <LoadScreenBus.h>

#if AZ_LOADSCREENCOMPONENT_ENABLED

/*
* This component is responsible for managing the load screen.
*/
class LoadScreenComponent
    : public AZ::Component
    , public CrySystemEventBus::Handler
    , public LoadScreenBus::Handler
{
public:

    AZ_COMPONENT(LoadScreenComponent, "{97CDBD6C-C621-4427-87C8-10E1B8F947FF}");

    LoadScreenComponent();
    ~LoadScreenComponent() override;

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    //////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // CrySystemEvents
    void OnCrySystemInitialized(ISystem&, const SSystemInitParams& params) override;
    void OnCrySystemShutdown(ISystem&) override;
    ////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // LoadScreenBus interface implementation
    void UpdateAndRender() override;
    void GameStart() override;
    void LevelStart() override;
    void Pause() override;
    void Resume() override;
    void Stop() override;  
    bool IsPlaying() override;
    //////////////////////////////////////////////////////////////////////////

protected:
    // Workaround for VS2013
    // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
    LoadScreenComponent(const LoadScreenComponent&) = delete;

    static void Reflect(AZ::ReflectContext* context);

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LoadScreenService", 0x901b031c));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LoadScreenService", 0x901b031c));
    }

private:

    void Reset();
    void ClearCVars(const std::list<const char*>& varNames);
    AZ::EntityId loadFromCfg(const char* pathVarName, const char* autoPlayVarName, const char* fixedFpsVarName, const char* maxFpsVarName);

    //////////////////////////////////////////////////////////////////////////
    float m_fixedDeltaTimeInSeconds;
    float m_maxDeltaTimeInSeconds;
    CTimeValue m_previousCallTimeForUpdateAndRender;
    bool m_isPlaying;
    AZ::EntityId m_gameCanvasEntityId;
    AZ::EntityId m_levelCanvasEntityId;
    AZStd::atomic_bool m_processingLoadScreen;
    //////////////////////////////////////////////////////////////////////////
};

#include "LoadScreenComponent.cpp"

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
