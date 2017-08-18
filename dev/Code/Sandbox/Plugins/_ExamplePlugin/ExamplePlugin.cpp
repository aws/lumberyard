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

#include "stdafx.h"
#include "ExamplePlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"

namespace PluginInfo
{
    const char* kName = "Example plug-in";
    const char* kGUID = "{DFA4AFF7-2C70-4B29-B736-54393C4ABADF}";
    const int kVersion = 1;
}

void CExamplePlugin::Release()
{
    delete this;
}

void CExamplePlugin::ShowAbout()
{
}

const char* CExamplePlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CExamplePlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CExamplePlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CExamplePlugin::CanExitNow()
{
    return true;
}

void CExamplePlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CExamplePlugin::ResetContent()
{
}

bool CExamplePlugin::CreateUIElements()
{
    return true;
}

void CExamplePlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}