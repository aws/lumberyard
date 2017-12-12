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
#include "AssetTaggingPlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include "Include/IEditorClassFactory.h"

namespace PluginInfo
{
    const char* kName = "AssetTagging";
    const char* kGUID = "{DBB805D2-B302-4B84-8986-82C4A8061C4D}";
    const int kVersion = 1;
}

void CAssetTaggingPlugin::Release()
{
    GetIEditor()->GetClassFactory()->UnregisterClass("Asset Tagging");
    delete this;
}

void CAssetTaggingPlugin::ShowAbout()
{
}

const char* CAssetTaggingPlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CAssetTaggingPlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CAssetTaggingPlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CAssetTaggingPlugin::CanExitNow()
{
    return true;
}

void CAssetTaggingPlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CAssetTaggingPlugin::ResetContent()
{
}

bool CAssetTaggingPlugin::CreateUIElements()
{
    return true;
}

void CAssetTaggingPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}