/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#include "CloudGemDynamicContent_precompiled.h"
#include <DynamicContentEditorPlugin.h>
#include "IGemManager.h"
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include "CryExtension/ICryFactoryRegistry.h"
#include "QDynamicContentEditorMainWindow.h"
#include "QtViewPaneManager.h"

static const char* ViewPaneName = "Dynamic Content Manager";

DynamicContentEditorPlugin::DynamicContentEditorPlugin(IEditor* editor)
    : m_registered(false)
{
    AzToolsFramework::ViewPaneOptions opt;
    opt.isDeletable = true;
    opt.canHaveMultipleInstances = false;
    AzToolsFramework::RegisterViewPane<DynamicContent::QDynamicContentEditorMainWindow>(ViewPaneName, "Cloud Canvas", opt);
    m_registered = true;
}

void DynamicContentEditorPlugin::Release()
{
    if (m_registered)
    {
        AzToolsFramework::UnregisterViewPane(ViewPaneName);
    }
}

void DynamicContentEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
    switch (aEventId)
    {
    case eNotify_OnInit:
    {
    }
    break;
    case eNotify_OnQuit:
    {

    }
    break;
    default:
    {

    }
    }

}
