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
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <QMetaType>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <QSettings>
#include <Util/Variable.h>
#include <AWSResourceManager.h>

class MaglevControlPanelPlugin
    : public IPlugin
{
public:
    MaglevControlPanelPlugin(IEditor* editor);

    void Release() override;
    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{E76FB5E9-9E94-4BAF-AF9D-08F308E33572}"; }
    DWORD GetPluginVersion() override { return s_pluginVersion; }
    const char* GetPluginName() override { return "MaglevControlPanel"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override;

    static const char* GetHelpLink() { return "https://docs.aws.amazon.com/console/lumberyard/developerguide/cloud-canvas"; }
    static const char* GetDeploymentHelpLink() { return "https://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-ui-rm-deployments.html"; }
private:
    static const DWORD s_pluginVersion = 1;
    QSettings m_pluginSettings;
    AWSResourceManager m_awsResourceManager;
};
