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
#include "ImplementationManager.h"
#include <IConsole.h>
#include <IAudioSystemEditor.h>
#include <IEditor.h>
#include "ATLControlsModel.h"
#include "AudioControlsEditorPlugin.h"

using namespace AudioControls;

#define WWISE_IMPL_DLL      "EditorAudioControlsEditorWwise.dll"
#define NOSOUND_IMPL_DLL    "EditorNoSound.dll"

const string g_sImplementationCVarName = "s_AudioSystemImplementationName";

const string g_sMiddlewareDllPath = BINFOLDER_NAME "\\EditorPlugins\\";

typedef IAudioSystemEditor* (* TPfnGetAudioInterface)(IEditor*);

//-----------------------------------------------------------------------------------------------//
CImplementationManager::CImplementationManager()
    : ms_hMiddlewarePlugin(nullptr)
{
}

//-----------------------------------------------------------------------------------------------//
bool CImplementationManager::LoadImplementation()
{
    CATLControlsModel* pATLModel = CAudioControlsEditorPlugin::GetATLModel();
    ICVar* pCVar = gEnv->pConsole->GetCVar(g_sImplementationCVarName);
    if (pCVar && pATLModel)
    {
        pATLModel->ClearAllConnections();

        // release the loaded implementation (if any)
        Release();

        string sDLLName = g_sMiddlewareDllPath;
        string middlewareName(pCVar->GetString());

        if (middlewareName == "CryAudioImplWwise")
        {
            sDLLName += WWISE_IMPL_DLL;
        }
        // else if (other implementations...)
        else
        {
            sDLLName += NOSOUND_IMPL_DLL;
        }

        ms_hMiddlewarePlugin = LoadLibraryA(sDLLName);
        if (!ms_hMiddlewarePlugin)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(Audio Controls Editor) Couldn't load the middleware specific editor dll.");
            return false;
        }

        TPfnGetAudioInterface pfnAudioInterface = (TPfnGetAudioInterface)GetProcAddress(ms_hMiddlewarePlugin, "GetAudioInterface");
        if (!pfnAudioInterface)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(Audio Controls Editor) Couldn't get middleware interface from loaded dll.");
            FreeLibrary(ms_hMiddlewarePlugin);
            return false;
        }

        ms_pAudioSystemImpl = pfnAudioInterface(GetIEditor());
        if (ms_pAudioSystemImpl)
        {
            ms_pAudioSystemImpl->Reload();
            pATLModel->ReloadAllConnections();
            pATLModel->ClearDirtyFlags();
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(Audio Controls Editor) Data from middleware is empty.");
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(Audio Controls Editor) CVar %s not defined. Needed to derive the Editor plugin name.", g_sImplementationCVarName);
        return false;
    }
    ImplementationChanged();
    return true;
}

//-----------------------------------------------------------------------------------------------//
void CImplementationManager::Release()
{
    if (ms_hMiddlewarePlugin)
    {
        typedef void(* TPfnOnPluginRelease)();
        auto fnOnPluginRelease = (TPfnOnPluginRelease)GetProcAddress(ms_hMiddlewarePlugin, "OnPluginRelease");
        if (fnOnPluginRelease)
        {
            fnOnPluginRelease();
        }
        FreeLibrary(ms_hMiddlewarePlugin);
        ms_hMiddlewarePlugin = nullptr;
        ms_pAudioSystemImpl = nullptr;
    }
}

//-----------------------------------------------------------------------------------------------//
AudioControls::IAudioSystemEditor* CImplementationManager::GetImplementation()
{
    return ms_pAudioSystemImpl;
}

#include <ImplementationManager.moc>