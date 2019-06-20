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
#include <AzFramework/Application/Application.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <QLibrary>

using namespace AudioControls;

#if defined(AZ_PLATFORM_WINDOWS)
#define WWISE_IMPL_DLL      "EditorWwise.dll"
#define NOSOUND_IMPL_DLL    "EditorNoSound.dll"
#else
#define WWISE_IMPL_DLL      "libEditorWwise.dylib"
#define NOSOUND_IMPL_DLL    "libEditorNoSound.dylib"
#endif

static const char* g_sImplementationCVarName = "s_AudioSystemImplementationName";

typedef IAudioSystemEditor* (* TPfnGetAudioInterface)(IEditor*);

//-----------------------------------------------------------------------------------------------//
CImplementationManager::CImplementationManager()
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

        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        AZ_Assert(engineRoot != nullptr, "Unable to communicate with AzFramework::ApplicationRequests::Bus");
        string sDLLName(engineRoot);

        AZStd::string_view binFolderName;
        AZ::ComponentApplicationBus::BroadcastResult(binFolderName, &AZ::ComponentApplicationRequests::GetBinFolder);

        AZStd::string middlewareDllPath = AZStd::string::format("%s" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "EditorPlugins" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, binFolderName.data());

        sDLLName += middlewareDllPath.c_str();
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

        m_middlewarePlugin = AZ::DynamicModuleHandle::Create(sDLLName.c_str());
        if (!m_middlewarePlugin->Load(false))
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(Audio Controls Editor) Couldn't load the middleware specific editor dll (%s).", sDLLName.c_str());
            return false;
        }

        TPfnGetAudioInterface pfnAudioInterface = m_middlewarePlugin->GetFunction<TPfnGetAudioInterface>("GetAudioInterface");
        if (!pfnAudioInterface)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(Audio Controls Editor) Couldn't get middleware interface from loaded dll (%s).", sDLLName.c_str());
            m_middlewarePlugin->Unload();
            m_middlewarePlugin.reset();
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
    if ((m_middlewarePlugin) && (m_middlewarePlugin->IsLoaded()))
    {
        typedef void(* TPfnOnPluginRelease)();
        auto fnOnPluginRelease = m_middlewarePlugin->GetFunction<TPfnOnPluginRelease>("OnPluginRelease");
        if (fnOnPluginRelease)
        {
            fnOnPluginRelease();
        }
        m_middlewarePlugin->Unload();
        m_middlewarePlugin.reset();
        ms_pAudioSystemImpl = nullptr;
    }
}

//-----------------------------------------------------------------------------------------------//
AudioControls::IAudioSystemEditor* CImplementationManager::GetImplementation()
{
    return ms_pAudioSystemImpl;
}

#include <ImplementationManager.moc>