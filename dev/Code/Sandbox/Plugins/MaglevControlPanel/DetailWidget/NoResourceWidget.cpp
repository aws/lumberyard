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

#include "NoResourceWidget.h"
#include "ResourceManagementView.h"
#include <IAWSResourceManager.h>

#include <QPushButton>

#include "DetailWidget/NoResourceWidget.moc"

NoResourceWidget::NoResourceWidget(ResourceManagementView* view, const QSharedPointer<IResourceGroupStatusModel>& resourceGroupStatusModel, QWidget* parent)
    : ActionWidget{parent}
    , m_view{view}
    , m_resourceGroupStatusModel{resourceGroupStatusModel}
{
    SetTitleText(tr(
            "You have not yet added any resource definitions."
            ));

    SetMessageText(tr(
            "Resource groups are used to define the AWS resources you will use in your game. "
            "<p>"
            "Add resources to your resource group as needed to support gameplay features."
            ));

    SetActionText(
        QString::fromStdWString(L"Add resource \u25BC")
        );

    SetActionToolTip(tr(
            "Select an AWS resource to add to your resource group."
            ));

    connect(this, &ActionWidget::ActionClicked, this, &NoResourceWidget::OnAddResource);

    auto importResourceButton = new QPushButton{
        QString::fromStdWString(L"Import resource \u25BC")
    };
    importResourceButton->setObjectName("ImportExistingButton");
    importResourceButton->setToolTip("Import an existing AWS resource to your resource group.");
    importResourceButton->setDisabled(m_resourceGroupStatusModel->IsPendingDelete());
    connect(importResourceButton, &QPushButton::clicked, this, [this, importResourceButton]() {OnImportResource(importResourceButton); });
    AddButton(importResourceButton);

    auto deleteButton = new QPushButton {};
    deleteButton->setObjectName("DeleteButton");
    deleteButton->setText(m_resourceGroupStatusModel->GetDeleteButtonText());
    deleteButton->setToolTip(m_resourceGroupStatusModel->GetDeleteButtonToolTip());
    deleteButton->setDisabled(m_resourceGroupStatusModel->StackIsBusy() || m_resourceGroupStatusModel->IsPendingDelete());
    connect(deleteButton, &QPushButton::clicked, this, &NoResourceWidget::DeleteStack);
    AddButton(deleteButton);

    auto enableButton = new QPushButton{};
    enableButton->setObjectName("EnableButton");
    enableButton->setText(m_resourceGroupStatusModel->GetEnableButtonText());
    enableButton->setToolTip(m_resourceGroupStatusModel->GetEnableButtonToolTip());
    enableButton->setDisabled(m_resourceGroupStatusModel->StackIsBusy());
    connect(enableButton, &QPushButton::clicked, this, &NoResourceWidget::DeleteStack);
    AddButton(enableButton);
}

void NoResourceWidget::DeleteStack()
{
    m_view->DeleteStack(m_resourceGroupStatusModel);
}

void NoResourceWidget::EnableResourceGroup()
{
    m_resourceGroupStatusModel->EnableResourceGroup();
}

void NoResourceWidget::OnAddResource()
{
    auto buttonGeometry = GetActionButton()->geometry();
    auto popupPos = GetActionButton()->parentWidget()->mapToGlobal(buttonGeometry.bottomLeft());
    popupPos += QPoint {
        10, -1
    };
    m_view->CreateAddResourceMenu(m_resourceGroupStatusModel->GetTemplateModel(), this)->popup(popupPos);
}

void NoResourceWidget::OnImportResource(QPushButton* importResourceButton)
{
    auto buttonGeometry = importResourceButton->geometry();
    auto popupPos = importResourceButton->parentWidget()->mapToGlobal(buttonGeometry.bottomLeft());
    popupPos += QPoint{
        10, -1
    };
    m_view->CreateImportResourceMenu(this)->popup(popupPos);
}


