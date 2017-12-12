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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"


#ifndef EXCLUDE_OCULUS_SDK

#include "OculusResources.h"
#include "OculusLog.h"
#include "OculusAllocator.h"


//////////////////////////////////////////////////////////////////////////
// OculusResources

OculusResources* OculusResources::ms_pRes = 0;

OculusResources& OculusResources::GetAccess()
{
#if !defined(_RELEASE)
    if (!ms_pRes)
    {
        __debugbreak();
    }
#endif

    return *ms_pRes;
}


OculusResources::OculusResources()
    : m_pDevMan(0)
    , m_pDeviceWrapped(0)
{
    ScopedSwitchToGlobalHeap globalHeap;

    gEnv->pLog->Log("Using OculusVR SDK " OVR_VERSION_STRING);

    OVR::Log::SetGlobalLog(&OculusLog::GetAccess());

    OVR::System::Init(&OculusLog::GetAccess(), &OculusAllocator::GetAccess());

    m_pDevMan = *OVR::DeviceManager::Create();
    if (m_pDevMan)
    {
        m_pDeviceWrapped = OculusDevice::CreateInstance(m_pDevMan);
        m_pDevMan->SetMessageHandler(this);
    }
}


OculusResources::~OculusResources()
{
#if !defined(_RELEASE)
    if (m_pDeviceWrapped && m_pDeviceWrapped->GetRefCount() != 1)
    {
        __debugbreak();
    }
#endif
    if (m_pDevMan)
    {
        m_pDevMan->SetMessageHandler(0);
    }

    SAFE_RELEASE(m_pDeviceWrapped);
    m_pDevMan = 0;

    OVR::System::Destroy();
}


void OculusResources::OnMessage(const OVR::Message& msg)
{
    // Note: for more involved state manipulation thread safety measures are needed since we're called from Oculus device thread
    if (msg.pDevice && m_pDeviceWrapped)
    {
        if (msg.Type == OVR::Message_DeviceAdded)
        {
            if (msg.pDevice->GetType() == OVR::Device_HMD && !m_pDeviceWrapped->HasHMDDevice())
            {
                m_pDeviceWrapped->CreateDevices();
            }
        }
    }
}


void OculusResources::Init()
{
    assert(!ms_pRes);
    static char s_OculusResourcestorage[sizeof(OculusResources)] = {0};
    if (!ms_pRes)
    {
        ms_pRes = new(s_OculusResourcestorage) OculusResources;
    }
}


void OculusResources::Shutdown()
{
    assert(ms_pRes);
    if (ms_pRes)
    {
        ms_pRes->~OculusResources();
        ms_pRes = 0;
    }
}


#endif // #ifndef EXCLUDE_OCULUS_SDK
