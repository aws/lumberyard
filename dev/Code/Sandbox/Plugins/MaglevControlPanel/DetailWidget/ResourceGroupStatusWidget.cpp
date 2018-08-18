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

#include "ResourceGroupStatusWidget.h"

#include "StackWidget.h"

#include <QPushButton>

#include <ResourceManagementView.h>
#include <IAWSResourceManager.h>

#include "DetailWidget/ResourceGroupStatusWidget.moc"

ResourceGroupStatusWidget::ResourceGroupStatusWidget(ResourceManagementView* view, QSharedPointer<IResourceGroupStatusModel> resourceGroupStatusModel, QWidget* parent)
    : StackEventsSplitter{resourceGroupStatusModel->GetStackEventsModel(), parent}
    , m_view{view}
    , m_resourceGroupStatusModel{resourceGroupStatusModel}
{
    CreateUI();
    connect(m_resourceGroupStatusModel.data(), &IResourceGroupStatusModel::modelReset, this, &ResourceGroupStatusWidget::UpdateUI);
    connect(this, &StackEventsSplitter::RefreshTime, this, [this](){ m_resourceGroupStatusModel->Refresh(); } );
}

void ResourceGroupStatusWidget::CreateUI()
{
    auto stackWidget = new StackWidget {
        m_view, m_resourceGroupStatusModel, this
    };
    SetTopWidget(stackWidget);

    m_addResourceButton = new QPushButton{
        QString::fromStdWString(L"Add resource \u25BC")
    };
    m_addResourceButton->setToolTip("Select an AWS resource to add to your resource group.");
    m_addResourceButton->setDisabled(m_resourceGroupStatusModel->IsPendingDelete());
    connect(m_addResourceButton, &QPushButton::clicked, this, &ResourceGroupStatusWidget::OnAddResource);
    stackWidget->AddButton(m_addResourceButton);

    m_importResourceButton = new QPushButton{
        QString::fromStdWString(L"Import resource \u25BC")
    };
    m_importResourceButton->setObjectName("ImportExistingButton");
    m_importResourceButton->setToolTip("Import an existing AWS resource to your resource group.");
    m_importResourceButton->setDisabled(m_resourceGroupStatusModel->IsPendingDelete());
    connect(m_importResourceButton, &QPushButton::clicked, this, &ResourceGroupStatusWidget::OnImportResource);
    stackWidget->AddButton(m_importResourceButton);

    m_uploadLambdaCodeButton = new QPushButton{
        QString::fromStdWString(L"Upload Lambda code")
    };
    m_uploadLambdaCodeButton->setToolTip("Upload the Lambda function code without a full feature stack update.");
    m_uploadLambdaCodeButton->setDisabled(m_resourceGroupStatusModel->IsPendingDelete());
    connect(m_uploadLambdaCodeButton, &QPushButton::clicked, this, &ResourceGroupStatusWidget::OnUploadLambdaCode);
    stackWidget->AddButton(m_uploadLambdaCodeButton);

    stackWidget->AddUpdateButton();
    stackWidget->AddDeleteButton();

    m_enableResourceGroupButton = new QPushButton{
        m_resourceGroupStatusModel->GetEnableButtonText()
    };
    m_enableResourceGroupButton->setToolTip(m_resourceGroupStatusModel->GetEnableButtonToolTip());
    m_enableResourceGroupButton->setDisabled(m_resourceGroupStatusModel->IsPendingDelete());
    connect(m_enableResourceGroupButton, &QPushButton::clicked, this, &ResourceGroupStatusWidget::EnableResourceGroup);
    stackWidget->AddButton(m_enableResourceGroupButton);

    UpdateUI();
}

void ResourceGroupStatusWidget::UpdateUI()
{
    m_addResourceButton->setEnabled(!m_resourceGroupStatusModel->IsPendingDelete());
    m_uploadLambdaCodeButton->setEnabled(!m_resourceGroupStatusModel->IsPendingCreate() && !m_resourceGroupStatusModel->IsPendingDelete());
}

void ResourceGroupStatusWidget::OnAddResource()
{
    auto buttonGeometry = m_addResourceButton->geometry();
    auto popupPos = m_addResourceButton->parentWidget()->mapToGlobal(buttonGeometry.bottomLeft());
    popupPos += QPoint {
        10, -1
    };
    m_view->CreateAddResourceMenu(m_resourceGroupStatusModel->GetTemplateModel(), this)->popup(popupPos);
}

void ResourceGroupStatusWidget::OnImportResource()
{
    auto buttonGeometry = m_importResourceButton->geometry();
    auto popupPos = m_importResourceButton->parentWidget()->mapToGlobal(buttonGeometry.bottomLeft());
    popupPos += QPoint {
        10, -1
    };
    m_view->CreateImportResourceMenu(this)->popup(popupPos);
}

void ResourceGroupStatusWidget::OnUploadLambdaCode()
{
    m_view->UploadLambdaCode(m_resourceGroupStatusModel, "");
}

void ResourceGroupStatusWidget::EnableResourceGroup()
{
    m_resourceGroupStatusModel->EnableResourceGroup();
}