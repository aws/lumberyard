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
#include <IEditor.h>
#include <Include/IPlugin.h>

///
/// This class implements the Editor Plugin API (IPlugin)
/// This plugin is dead and has been replaced by LyMetrics, but since we don't have a safe deprecation process for plugins
/// it's still here as an empty module
///
class MetricsPlugin
    : public IPlugin
{
public:
    MetricsPlugin();

    void Release() override;
    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{66630c3b-9d1f-4318-a209-45df6f968fd2}"; }
    DWORD GetPluginVersion() override { return s_pluginVersion; }
    const char* GetPluginName() override { return "Metrics"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override;

private:
    static const DWORD s_pluginVersion = 1;
};
