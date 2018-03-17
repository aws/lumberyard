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
#ifndef CRYINCLUDE__PARTICLEEDITOR_PLUGIN_H
#define CRYINCLUDE__PARTICLEEDITOR_PLUGIN_H
#include <Include/IPlugin.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

class CParticleEditorPlugin
    : public IPlugin
    , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
{
public:
    CParticleEditorPlugin(IEditor* editor);

    void Release() override;
    void ShowAbout() override;
    const char* GetPluginGUID() override;
    DWORD GetPluginVersion() override;
    const char* GetPluginName() override;
    bool CanExitNow() override;
    void OnEditorNotify(EEditorNotifyEvent aEventId) override;
    static const char* m_RegisteredQtViewPaneName;

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& /*sourceUUID*/, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
};
#endif // CRYINCLUDE__PARTICLEEDITOR_EXAMPLEPLUGIN_H
