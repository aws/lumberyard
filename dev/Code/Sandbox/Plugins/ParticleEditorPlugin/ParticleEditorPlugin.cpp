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
#include "stdafx.h"
#include "ParticleEditorPlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include "Include/IEditorClassFactory.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <LyViewPaneNames.h>
#include <QT/MainWindow.h>

static bool const c_EnableParticleEditorMenuEntry = true;
const char* CParticleEditorPlugin::m_RegisteredQtViewPaneName = "Particle Editor";

namespace PluginInfo
{
    const char* kName = "Particle Editor plug-in";
    const char* kGUID = "{98C1DC36-5D0E-4CF6-90CE-AFA1107CE80F}";
    const int kVersion = 1;
}

CParticleEditorPlugin::CParticleEditorPlugin(IEditor* editor)
{
	if (c_EnableParticleEditorMenuEntry)
	{
		AzToolsFramework::ViewPaneOptions options;
		options.canHaveMultipleInstances = true;
		options.isPreview = true;
		/*
		 * We disabled the deletion on close for following reason:
		 * 1. Optimize the editor work-flow. There is no need to create a brand new editor
		 * every time user open it.
		 * 2. Prevent the crash on race condition. If we delete the editor on close, while other
		 * operations are not finished, it could crash the editor.
		*/
		options.isDeletable = false;
		options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

		AzToolsFramework::RegisterViewPane<CMainWindow>(m_RegisteredQtViewPaneName, LyViewPane::CategoryTools, options);
	}
}

void CParticleEditorPlugin::Release()
{
    if (c_EnableParticleEditorMenuEntry)
    {
        AzToolsFramework::UnregisterViewPane(m_RegisteredQtViewPaneName);
    }
    delete this;
}

void CParticleEditorPlugin::ShowAbout()
{
}

const char* CParticleEditorPlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CParticleEditorPlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CParticleEditorPlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CParticleEditorPlugin::CanExitNow()
{
    return true;
}

void CParticleEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}
