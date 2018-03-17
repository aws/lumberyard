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
#include "StaticData_precompiled.h"
#include <StaticDataMonitorEditorPlugin.h>
#include "IGemManager.h"
#include <StaticData/StaticDataBus.h>
#include <StaticDataMonitor.h>

#include "CryExtension/ICryFactoryRegistry.h"

StaticDataMonitorEditorPlugin::StaticDataMonitorEditorPlugin(IEditor* editor)
    : m_registered(false)
{
#if defined(USE_MONITOR)
    m_monitor = new CloudCanvas::StaticData::StaticDataMonitor;
#endif
}

void StaticDataMonitorEditorPlugin::Release()
{
#if defined(USE_MONITOR)
    delete m_monitor;
    delete this;
#endif
}

void StaticDataMonitorEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
#if defined (USE_MONITOR)
    switch (aEventId)
    {
    case eNotify_OnInit:
    {
        EBUS_EVENT(CloudCanvas::StaticData::StaticDataRequestBus, ReloadTagType, "");
    }
    break;
    case eNotify_OnQuit:
    {

    }
    break;
    default:
    {
    }
    }
#endif
}
