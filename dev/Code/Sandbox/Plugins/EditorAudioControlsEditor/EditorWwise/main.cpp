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

#include <platform.h>
#include <platform_impl.h>
#include <IEditor.h>
#include <IPlugin.h>
#include "AudioSystemEditor_wwise.h"


AudioControls::CAudioSystemEditor_wwise* g_pWwiseInterface = nullptr;

//-----------------------------------------------------------------------------------------------//
extern "C" PLUGIN_API void QueryPluginSettings(SPluginSettings& settings)
{
    // Lumberyard:
    // mark the EditorWwise plugin as non-autoLoad because we don't want this to be loaded
    // with all the other dlls in EditorPlugins/.
    // EditorAudioControlsEditor.dll will load this or another middleware dll on it's own.
    settings.pluginVersion = 0x1;
    settings.autoLoad = false;
}

//-----------------------------------------------------------------------------------------------//
// Call this to signal that the parent (EditorAudioControlsEditor) plugin is being released.
// This will allow this plugin (EditorWwise) to do some cleanup.
extern "C" PLUGIN_API void OnPluginRelease()
{
    delete g_pWwiseInterface;
    g_pWwiseInterface = nullptr;
}

//-----------------------------------------------------------------------------------------------//
extern "C" PLUGIN_API AudioControls::IAudioSystemEditor * GetAudioInterface(IEditor * pEditor)
{
    ModuleInitISystem(pEditor->GetSystem(), "EditorWwise");
    if (!g_pWwiseInterface)
    {
        g_pWwiseInterface = new AudioControls::CAudioSystemEditor_wwise();
    }
    return g_pWwiseInterface;
}

