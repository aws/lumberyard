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
#include <stdafx.h>

#include <ActiveDeployment.h>

#include <QAWSCredentialsEditor.h>
#include <QDesktopServices>
#include <QUrl>
#include <QErrorMessage>
#include <ActiveDeployment.moc>
#include <EditorCoreAPI.h>
#include <IAWSResourceManager.h>
#include <MaglevControlPanelPlugin.h>

using namespace AZStd;

// The URL embedded here isn't expected to be used.  See ActiveDeployment::OnSetupDeploymentClicked.
static const char* NO_DEPLOYMENTS_LABEL_TEXT = "There aren't any deployments associated with this Lumberyard project. AWS account admins can add new deployments using the Resource Manager."
    " Learn how to <a href=\"https://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-ui-rm-deployments.html\" style=\"color: #6496e5\">create a deployment</a>.  ";
static const char* HAS_DEPLOYMENTS_LABEL_TEXT = "AWS account admins can add new deployments using the Resource Manager. Otherwise, authorized users can switch the active deployment here.";
static const char* SELECT_DEPLOYMENT_LABEL = "Select deployment";

static const char* PROTECTED_LABEL = " (protected)";

ActiveDeployment::ActiveDeployment(QWidget* parent)
    : ScrollSelectionDialog(parent)
    , m_model(GetIEditor()->GetAWSResourceManager()->GetDeploymentModel())
{
    InitializeWindow();
}

void ActiveDeployment::SetupConnections()
{
    connect(&*m_model, &IAWSDeploymentModel::modelReset, this, &ActiveDeployment::OnModelReset);
    if (GetNoDataLabel())
    {
        connect(GetNoDataLabel(), &QLabel::linkActivated, this, &ActiveDeployment::OnSetupDeploymentClicked);
    }
}

void ActiveDeployment::SetupNoDataConnection()
{
    if (GetNoDataLabel())
    {
        connect(GetNoDataLabel(), &QLabel::linkActivated, this, &ActiveDeployment::OnSetupDeploymentClicked);
    }
}

void ActiveDeployment::AddScrollHeadings(QVBoxLayout* boxLayout)
{
    QLabel* selectLabel = new QLabel(SELECT_DEPLOYMENT_LABEL);
    boxLayout->addWidget(selectLabel);
}

void ActiveDeployment::SetCurrentSelection(QString& selectedText) const
{
    if (selectedText.endsWith(PROTECTED_LABEL))
    {
        int snipIndex = selectedText.lastIndexOf(PROTECTED_LABEL);
        selectedText = selectedText.left(snipIndex);
    }
    m_model->SetUserDefaultDeployment(selectedText);
}

bool ActiveDeployment::IsSelected(int rowNum) const
{
    return m_model->data(rowNum, AWSDeploymentColumn::Default).value<bool>();
}

QString ActiveDeployment::GetDataForRow(int rowNum) const
{
    QString returnString;
    if (rowNum < GetRowCount())
    {
        returnString = m_model->data(rowNum, AWSDeploymentColumn::Name).value<QString>();
        if (m_model->data(rowNum, AWSDeploymentColumn::Protected).value<bool>())
        {
            returnString += PROTECTED_LABEL;
        }
    }
    return returnString;
}

int ActiveDeployment::GetRowCount() const
{
    return m_model->rowCount();
}

const char* ActiveDeployment::GetNoDataText() const
{
    return NO_DEPLOYMENTS_LABEL_TEXT;
}

const char* ActiveDeployment::GetHasDataText() const
{
    return HAS_DEPLOYMENTS_LABEL_TEXT;
}

void ActiveDeployment::OnSetupDeploymentClicked()
{
    QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetDeploymentHelpLink())));
}