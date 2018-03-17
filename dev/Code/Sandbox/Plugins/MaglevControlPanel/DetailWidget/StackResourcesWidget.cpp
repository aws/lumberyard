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

#include "StackResourcesWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>
#include <QTreeView>
#include <QToolTip>
#include <ResourceManagementView.h>
#include <QApplication>
#include <QClipboard>

#include <IEditor.h>

#include <CloudCanvas/ICloudCanvasEditor.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

#include "MaximumSizedTableView.h"

#include <IAWSResourceManager.h>

#include <AWSProjectModel.h>

#include <DetailWidget/StackResourcesWidget.moc>

StackResourcesWidget::StackResourcesWidget(QSharedPointer<IStackResourcesModel> stackResourcesModel, ResourceManagementView* mainView)
    : m_stackResourcesModel{stackResourcesModel}
    , m_view{mainView}
{
    // root

    setObjectName("Resources");

    auto rootLayout = new QVBoxLayout {};
    rootLayout->setSpacing(0);
    rootLayout->setMargin(0);
    setLayout(rootLayout);

    // title row

    auto titleRowLayout = new QHBoxLayout {};
    rootLayout->addLayout(titleRowLayout);

    // title row - title

    auto titleLabel = new QLabel {
        "Stack resources"
    };
    titleLabel->setObjectName("Title");
    titleRowLayout->addWidget(titleLabel);

    // title row - stretch

    titleRowLayout->addStretch();

    // title row - refreshing

    m_refreshingLabel = new QLabel {
        "Refreshing..."
    };
    m_refreshingLabel->setObjectName("Refreshing");
    connect(m_stackResourcesModel.data(), &IStackResourcesModel::RefreshStatusChanged, this, &StackResourcesWidget::OnRefreshStatusChanged);
    titleRowLayout->addWidget(m_refreshingLabel);
    m_refreshingLabel->setVisible(m_stackResourcesModel->IsRefreshing());

    // message

    auto messageLabel = new QLabel {
        "The following table shows the status of the stack's resources in AWS."
    };
    messageLabel->setObjectName("Message");
    messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rootLayout->addWidget(messageLabel);

    // resources table

    m_resourcesTable = new MaximumSizedTableView {};
    m_resourcesTable->setObjectName("Table");
    m_resourcesTable->TableView()->setModel(m_stackResourcesModel.data());
    m_resourcesTable->TableView()->verticalHeader()->hide();
    m_resourcesTable->TableView()->setEditTriggers(QTableView::NoEditTriggers);
    m_resourcesTable->TableView()->setSelectionMode(QTableView::NoSelection);
    m_resourcesTable->TableView()->setFocusPolicy(Qt::NoFocus);
    m_resourcesTable->TableView()->setWordWrap(false);
    for (int column = 0; column < m_stackResourcesModel->columnCount(); ++column)
    {
        m_resourcesTable->TableView()->hideColumn(column);
    }
    m_resourcesTable->TableView()->showColumn(IStackResourcesModel::Columns::PendingActionColumn);
    m_resourcesTable->TableView()->showColumn(IStackResourcesModel::Columns::NameColumn);
    m_resourcesTable->TableView()->showColumn(IStackResourcesModel::Columns::PhysicalResourceIdColumn);
    m_resourcesTable->TableView()->showColumn(IStackResourcesModel::Columns::ResourceStatusColumn);
    m_resourcesTable->TableView()->showColumn(IStackResourcesModel::Columns::ResourceTypeColumn);
    m_resourcesTable->TableView()->showColumn(IStackResourcesModel::Columns::TimestampColumn);
    m_resourcesTable->Resize();

    connect(m_resourcesTable->TableView(), &QTableView::doubleClicked, this, &StackResourcesWidget::OnResourceTableDoubleClicked);
    connect(m_resourcesTable->TableView(), &MaximumSizeTableViewObject::OnRightClick, this, &StackResourcesWidget::OnResourceTableRightClicked);

    rootLayout->addWidget(m_resourcesTable, 1);

    // stretch

    rootLayout->addStretch();
}

void StackResourcesWidget::OnResourceTableDoubleClicked(const QModelIndex& thisIndex)
{
    if (thisIndex.isValid())
    {
        auto thisRow = thisIndex.row();
        auto thisCol = thisIndex.column();

        QItemSelectionModel* selectionModel = m_view->m_treeView->selectionModel();
        if (selectionModel)
        {
            auto curIndex = selectionModel->currentIndex();
            auto curData = m_view->m_treeView->model()->data(curIndex).toString();

            auto templateFileIndex = selectionModel->currentIndex().child(ResourceGroupNode::ChildNodes::ResourceGroupTemplateFilePathNode, 0);
            if (templateFileIndex.isValid())
            {
                auto resourceIndex = templateFileIndex.child(thisRow, 0);
                if (resourceIndex.isValid())
                {
                    selectionModel->setCurrentIndex(resourceIndex, QItemSelectionModel::ClearAndSelect);
                }
            }
        }
    }
}

QMenu* StackResourcesWidget::GetResourceContextMenu(QMouseEvent* mouseEvent)
{
    auto menu = new ToolTipMenu {};
    QModelIndex thisIndex = m_resourcesTable->TableView()->indexAt(mouseEvent->pos());

    int rowNm = thisIndex.row();

    QString resourceType = m_resourcesTable->TableView()->model()->data(m_resourcesTable->TableView()->model()->index(rowNm, IStackResourcesModel::ResourceTypeColumn)).toString();
    QString physicalResourceId = m_resourcesTable->TableView()->model()->data(m_resourcesTable->TableView()->model()->index(rowNm, IStackResourcesModel::PhysicalResourceIdColumn), Qt::ToolTipRole).toString();

    if (!physicalResourceId.length())
    {
        auto viewResource = menu->addAction("Resource not yet available");
        viewResource->setDisabled(true);
    }
    else
    {
        auto viewResource = menu->addAction("View resource in AWS console");
        connect(viewResource, &QAction::triggered, this, [this, physicalResourceId, resourceType]() { StackResourcesWidget::ViewConsoleResource(resourceType, physicalResourceId); });
        auto clipboardCopy = menu->addAction("Copy resource ID to clipboard");
        connect(clipboardCopy, &QAction::triggered, [physicalResourceId]() {QApplication::clipboard()->setText(physicalResourceId); });
    }
    return menu;
}

void StackResourcesWidget::ViewConsoleResource(const QString& resourceType, const QString& resourceName)
{
    QString region = m_stackResourcesModel->GetStackResourceRegion();
    QString destString = "https://console.aws.amazon.com/";
    if (resourceType == "AWS::S3::Bucket")
    {
        destString += "s3/?&";
        destString += "bucket=" + resourceName;
    }
    else if (resourceType == "AWS::DynamoDB::Table")
    {
        destString += "dynamodb/home?region=" + region + "#";
        destString += "tables:selected=" + resourceName;
    }
    else if (resourceType == "AWS::Lambda::Function")
    {
        destString += "lambda/home?region=" + region + "#";
        destString += "/functions/" + resourceName;
    }
    else if (resourceType == "AWS::IAM::Role")
    {
        destString += "iam/home?region=" + region + "#";
        destString += "/roles/" + resourceName;
    }
    else if (resourceType == "AWS::CloudFormation::Stack")
    {
        destString += "cloudformation/home?region=" + region + "#";
        destString += "/stack/detail?stackId=" + resourceName;
    }

    auto profileName = m_view->GetDefaultProfile();
    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, SetCredentials, Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("AWSManager", profileName.toStdString().c_str(), Aws::Auth::REFRESH_THRESHOLD));
    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, ApplyConfiguration);

    GetIEditor()->LaunchAWSConsole(destString);
}

void StackResourcesWidget::OnResourceTableRightClicked(QMouseEvent* mouseEvent)
{
    auto resourceMenu = GetResourceContextMenu(mouseEvent);

    if (resourceMenu)
    {
        resourceMenu->popup(mouseEvent->globalPos());
    }
}


void StackResourcesWidget::OnRefreshStatusChanged(bool refreshing)
{
    m_refreshingLabel->setVisible(refreshing);
    if (!refreshing)
    {
        m_resourcesTable->Resize();
    }
}

