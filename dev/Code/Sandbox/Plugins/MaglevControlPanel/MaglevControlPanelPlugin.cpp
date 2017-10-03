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
#include "stdafx.h"
#include <MaglevControlPanelPlugin.h>

#include <InitializeCloudCanvasProject.h>
#include <QtViewPaneManager.h>
#include <QAWSCredentialsEditor.h>
#include <QBackgroundLambda.h>
#include <ResourceManagementView.h>
#include <ActiveDeployment.h>
#include <IEditor.h>
#include <CloudCanvas/ICloudCanvasEditor.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <WinWidget/WinWidget.h>
#include <CloudCanvas/CloudCanvasIdentityBus.h>

#include <QRect>

const char* TAG = "MaglevControlPanelPlugin";

MaglevControlPanelPlugin::MaglevControlPanelPlugin(IEditor* editor)
    : m_pluginSettings(QSettings::IniFormat, QSettings::UserScope, "Amazon", "Lumberyard")
    , m_awsResourceManager(editor)
{
    //Load our previously selected profile name and init the credentials provider
    auto profileName = m_pluginSettings.value(ProfileSelector::SELECTED_PROFILE_KEY, "default").toString().toStdString();

    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, SetCredentials, Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("AWSManager", profileName.c_str(), Aws::Auth::REFRESH_THRESHOLD));

    //allow qt to treat our strings like first class citizens.
    qRegisterMetaType<Aws::String>("Aws::String");
    qRegisterMetaType<AwsResultWrapper>("AwsResultWrapper");

    QtViewOptions opt;
    opt.isDeletable = true; 
    opt.canHaveMultipleInstances = false; 
    opt.showInMenu = true;
    opt.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    
    RegisterQtViewPane<ResourceManagementView>(editor, ResourceManagementView::GetWindowTitle(), LyViewPane::CategoryCloudCanvas, opt);

    WinWidget::RegisterWinWidget<InitializeCloudCanvasProject>();
    WinWidget::RegisterWinWidget<ActiveDeployment>();
    WinWidget::RegisterWinWidget<ProfileSelector>();
    WinWidget::RegisterWinWidget<AddProfileDialog>();
}

void MaglevControlPanelPlugin::Release()
{
    WinWidget::UnregisterWinWidget<AddProfileDialog>();
    WinWidget::UnregisterWinWidget<ActiveDeployment>();
    WinWidget::UnregisterWinWidget<ProfileSelector>();
    WinWidget::UnregisterWinWidget<InitializeCloudCanvasProject>();
}

void MaglevControlPanelPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
    switch (aEventId)
    {
    case eNotify_OnInit:
        m_awsResourceManager.OnInit();
        break;
    case eNotify_OnBeginGameMode:
        m_awsResourceManager.OnBeginGameMode();
        break;
    }
}

