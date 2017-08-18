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

#pragma once

#include <IEditor.h>
#include <Include/IPlugin.h>
#include "ATLControlsModel.h"
#include "QATLControlsTreeModel.h"
#include <IAudioSystemEditor.h>
#include <IAudioInterfacesCommonData.h>

#include <QStandardItem>

namespace Audio
{
    struct IAudioProxy;
} // namespace Audio


class CImplementationManager;

//-------------------------------------------------------------------------------------------//
class CAudioControlsEditorPlugin
    : public IPlugin
    , public ISystemEventListener
{
public:
    explicit CAudioControlsEditorPlugin(IEditor* editor);

    void Release() override;
    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{DFA4AFF7-2C70-4B29-B736-GRH00040314}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "AudioControlsEditor"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override {}

    static void SaveModels();
    static void ReloadModels();
    static void ReloadScopes();
    static AudioControls::CATLControlsModel* GetATLModel();
    static AudioControls::QATLTreeModel* GetControlsTree();
    static CImplementationManager* GetImplementationManager();
    static AudioControls::IAudioSystemEditor* GetAudioSystemEditorImpl();
    static void ExecuteTrigger(const string& sTriggerName);
    static void StopTriggerExecution();

    ///////////////////////////////////////////////////////////////////////////
    // ISystemEventListener
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    ///////////////////////////////////////////////////////////////////////////

private:
    static AudioControls::CATLControlsModel ms_ATLModel;
    static AudioControls::QATLTreeModel ms_layoutModel;
    static std::set<string> ms_currentFilenames;
    static Audio::IAudioProxy* ms_pIAudioProxy;
    static Audio::TAudioControlID ms_nAudioTriggerID;

    static CImplementationManager ms_implementationManager;
};
