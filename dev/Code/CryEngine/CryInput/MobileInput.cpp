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

#include "MobileInput.h"

#include <ILog.h>
#include <ISystem.h>

#include "MotionSensorInputDevice.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMobileInput Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    static const float s_defaultMotionSensorUpdateIntervalSeconds = 1.0f / 30.0f;
    static float s_motionSensorUpdateIntervalSeconds = s_defaultMotionSensorUpdateIntervalSeconds;
    static void OnMotionSensorUpdateIntervalChange(ICVar* pVar)
    {
        // This is called after s_motionSensorUpdateIntervalSeconds
        // (referenced directly by the cvar) is set, so we have to force the refresh.
        gEnv->pInput->SetMotionSensorUpdateInterval(pVar->GetFVal(), true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMobileInput::CMobileInput(ISystem* pSystem)
    : CBaseInput()
    , m_sensorListeners()
    , m_manuallyActivatedSensorFlags(eMSF_None)
    , m_sensorPausedCount(0)
    , m_sensorDevice(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMobileInput::~CMobileInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMobileInput::Init()
{
    gEnv->pLog->Log("Initializing MobileInput");

    REGISTER_CVAR_CB(s_motionSensorUpdateIntervalSeconds,
        s_defaultMotionSensorUpdateIntervalSeconds,
        VF_NULL,
        "Sets the desired motion sensor update interval\n",
        &OnMotionSensorUpdateIntervalChange);

    if (!CBaseInput::Init())
    {
        gEnv->pLog->Log("Error: CBaseInput::Init failed");
        return false;
    }

    m_sensorDevice = new CMotionSensorInputDevice(*this);
    if (!AddInputDevice(m_sensorDevice))
    {
        // CBaseInput::AddInputDevice calls delete on it's argument if it fails.
        gEnv->pLog->Log("Error: Initializing Mobile Sensor");
        m_sensorDevice = nullptr;
        return false;
    }

    AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::ShutDown()
{
    AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();

    gEnv->pLog->Log("MobileInput Shutdown");
    if (m_sensorDevice)
    {
        m_sensorDevice->RefreshSensors(eMSF_None, s_motionSensorUpdateIntervalSeconds);
        m_sensorDevice = nullptr; // This gets deleted in CBaseInput::ShutDown
    }

    CBaseInput::ShutDown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::PauseMotionSensorInput()
{
    if (++m_sensorPausedCount == 1)
    {
        if (m_sensorDevice)
        {
            m_sensorDevice->RefreshSensors(eMSF_None, s_motionSensorUpdateIntervalSeconds);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::UnpauseMotionSensorInput()
{
    if (--m_sensorPausedCount == 0)
    {
        RefreshSensors();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::OnApplicationSuspended(Event /*lastEvent*/)
{
    PauseMotionSensorInput();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::OnApplicationResumed(Event /*lastEvent*/)
{
    UnpauseMotionSensorInput();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMobileInput::IsMotionSensorDataAvailable(EMotionSensorFlags sensorFlags) const
{
    return m_sensorDevice ? m_sensorDevice->IsSensorDataAvailable(sensorFlags) : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMobileInput::AddMotionSensorEventListener(IMotionSensorEventListener* pListener,
    EMotionSensorFlags sensorFlags)
{
    m_sensorListeners[pListener] |= sensorFlags;
    RefreshSensors();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::RemoveMotionSensorEventListener(IMotionSensorEventListener* pListener,
    EMotionSensorFlags sensorFlags)
{
    m_sensorListeners[pListener] &= ~sensorFlags;
    if (m_sensorListeners[pListener] == eMSF_None)
    {
        m_sensorListeners.erase(pListener);
    }
    RefreshSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::ActivateMotionSensors(EMotionSensorFlags sensorFlags)
{
    m_manuallyActivatedSensorFlags |= sensorFlags;
    RefreshSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::DeactivateMotionSensors(EMotionSensorFlags sensorFlags)
{
    m_manuallyActivatedSensorFlags &= ~sensorFlags;
    RefreshSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::SetMotionSensorUpdateInterval(float seconds, bool force)
{
    if (s_motionSensorUpdateIntervalSeconds == seconds && !force)
    {
        return;
    }

    s_motionSensorUpdateIntervalSeconds = seconds;

    // It's unclear (on either Android or iOS) whether changing the update interval will effect
    // already active sensors, so deactivate them first to ensure the new update interval sticks.
    if (m_sensorDevice)
    {
        m_sensorDevice->RefreshSensors(eMSF_None, s_motionSensorUpdateIntervalSeconds);
    }

    RefreshSensors();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::SetMotionSensorFilter(IMotionSensorFilter* filter)
{
    if (m_sensorDevice)
    {
        m_sensorDevice->SetMotionSensorFilter(filter);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const SMotionSensorData* CMobileInput::GetMostRecentMotionSensorData() const
{
    return m_sensorDevice ? &(m_sensorDevice->GetMostRecentData()) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::PostMotionSensorEvent(const SMotionSensorEvent& event, bool bForce /*= false*/)
{
    if (!bForce && !IsEventPostingEnabled())
    {
        return;
    }

    for (const auto& keyValue : m_sensorListeners)
    {
        if (keyValue.second & event.updatedFlags)
        {
            keyValue.first->OnMotionSensorEvent(event);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileInput::RefreshSensors()
{
    if (!m_sensorDevice)
    {
        return;
    }

    int activeSensorFlags = m_manuallyActivatedSensorFlags;
    for (const auto& keyValue : m_sensorListeners)
    {
        activeSensorFlags |= keyValue.second;
    }

    m_sensorDevice->RefreshSensors((EMotionSensorFlags)activeSensorFlags, s_motionSensorUpdateIntervalSeconds);
}
