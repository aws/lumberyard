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

#pragma once

#include <AWSResourceManager.h>

#include "StackEventsModel.h"
#include "StackResourcesModel.h"

#include <QTime>

#include <AWSUtil.h>

template<typename BaseType>
class StackStatusModel
    : public BaseType
{
public:

    StackStatusModel(AWSResourceManager* resourceManager, int columnCount, StackResourcesModel* stackResourcesModel)
        : m_resourceManager{resourceManager}
        , m_stackResourcesModel{stackResourcesModel}
        , m_stackEventsModel{new StackEventsModel {m_resourceManager}}
    {
        setColumnCount(columnCount);

        for (int column = 0; column < columnCount; ++column)
        {
            setItem(0, column, new QStandardItem {});
        }

        setHorizontalHeaderItem(StackIdColumn, new QStandardItem {"ID"});
        setHorizontalHeaderItem(StackStatusColumn, new QStandardItem {"Status"});
        setHorizontalHeaderItem(CreationTimeColumn, new QStandardItem {"Created"});
        setHorizontalHeaderItem(LastUpdatedTimeColumn, new QStandardItem {"Updated"});
        setHorizontalHeaderItem(PendingActionColumn, new QStandardItem {"Pending"});

        connect(GetStackEventsModel().data(), &StackEventsModel::CommandStatusChanged, this, &StackStatusModel::OnCommandStatusChanged);
    }

    void OnCommandStatusChanged(StackEventsModel::CommandStatus commandStatus)
    {
        StackBusyStatusChanged(StackIsBusy());
    }

    QSharedPointer<IStackEventsModel> GetStackEventsModel() const override
    {
        return GetStackEventsModelInternal().staticCast<IStackEventsModel>();
    }

    QSharedPointer<StackEventsModel> GetStackEventsModelInternal() const
    {
        return m_stackEventsModel;
    }

    QSharedPointer<IStackResourcesModel> GetStackResourcesModel() const override
    {
        return GetStackResourcesModelInternal().staticCast<IStackResourcesModel>();
    }

    QSharedPointer<StackResourcesModel> GetStackResourcesModelInternal() const
    {
        return m_stackResourcesModel;
    }

    void ClearStackStatus()
    {
        m_isRefreshing = false;

        ClearStackStatusColumns();
        RefreshStatusChanged(m_isRefreshing);
    }

    void Refresh(bool force) override
    {
        if (m_stackResourcesModel)
        {
            m_stackResourcesModel->Refresh(force);
        }
        m_isRefreshing = true;
        RefreshStatusChanged(m_isRefreshing);
    }

    bool IsRefreshing() const override
    {
        return m_isRefreshing;
    }

    bool IsReady() const override
    {
        return m_isReady;
    }

    bool StackIsBusy() const override
    {
        return StackStatus().contains("IN_PROGRESS") || GetStackEventsModel()->IsCommandInProgress();
    }

    bool CanDeleteStack() const override
    {
        return true;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::TextAlignmentRole)
            {
                return Qt::AlignLeft;
            }
        }
        return BaseType::headerData(section, orientation, role);
    }

protected:

    AWSResourceManager* ResourceManager() const { return m_resourceManager; }

    bool IsRefreshTime()
    {
        return !m_lastRefreshTime.isValid() || m_lastRefreshTime.elapsed() > 2000;
    }

    void ProcessOutputStackDescription(const QVariantMap& map)
    {
        beginResetModel();
        if (map.contains("StackDescription"))
        {
            UpdateStackStatusColumns(map["StackDescription"].toMap());
        }
        else
        {
            ClearStackStatusColumns();
        }
        m_lastRefreshTime.start();
        m_isRefreshing = false;
        m_isReady = true;
        endResetModel();
        RefreshStatusChanged(m_isRefreshing);
        StackBusyStatusChanged(StackIsBusy());
    }

private:

    void UpdateStackStatusColumns(const QVariantMap& map)
    {

        // pending

        auto pendingAction = map["PendingAction"].toString();
        auto pendingActionItem = item(0, PendingActionColumn);
        if (pendingAction.isEmpty())
        {
            pendingActionItem->setText("--");
        }
        else
        {
            pendingActionItem->setText(AWSUtil::MakePrettyPendingActionText(pendingAction));
            pendingActionItem->setData(AWSUtil::MakePrettyPendingReasonTooltip(map["PendingReason"].toString()), Qt::ToolTipRole);
            pendingActionItem->setData(AWSUtil::MakePrettyPendingActionColor(pendingAction), Qt::TextColorRole);
        }


        // status

        auto stackStatus = map["StackStatus"].toString();
        auto stackStatusItem = item(0, StackStatusColumn);
        if (stackStatus.isEmpty())
        {
            stackStatusItem->setData("--", Qt::DisplayRole);
            stackStatusItem->setData("", ActualValueRole);
        }
        else
        {
            stackStatusItem->setData(AWSUtil::MakePrettyResourceStatusText(stackStatus), Qt::DisplayRole);
            stackStatusItem->setData(stackStatus, ActualValueRole);
            stackStatusItem->setData(AWSUtil::MakePrettyResourceStatusTooltip(stackStatus, map["StackStatusReason"].toString()), Qt::ToolTipRole);
            stackStatusItem->setData(AWSUtil::MakePrettyResourceStatusColor(stackStatus), Qt::TextColorRole);
        }

        // id

        auto id = map["StackId"].toString();
        auto idItem = item(0, StackIdColumn);
        if (id.isEmpty())
        {
            idItem->setData("--", Qt::DisplayRole);
            idItem->setData(QVariant {}, ActualValueRole);
        }
        else
        {
            idItem->setData(AWSUtil::MakeShortResourceId(id), Qt::DisplayRole);
            idItem->setData(id, ActualValueRole);
            idItem->setData(id, Qt::ToolTipRole);
        }

        // creation time text
        if (map["CreationTime"].isValid())
        {
            item(0, CreationTimeColumn)->setData(map["CreationTime"].toDateTime().toLocalTime(), Qt::DisplayRole);
        }
        else
        {
            item(0, CreationTimeColumn)->setData("--", Qt::DisplayRole);
        }

        // updated time text

        if (map["LastUpdatedTime"].isValid())
        {
            item(0, LastUpdatedTimeColumn)->setData(map["LastUpdatedTime"].toDateTime().toLocalTime(), Qt::DisplayRole);
        }
        else
        {
            item(0, LastUpdatedTimeColumn)->setData("--", Qt::DisplayRole);
        }
    }

    void ClearStackStatusColumns()
    {
        QVariant emptyDisplay {
            "--"
        };
        QVariant empty {};

        // status text

        item(0, StackStatusColumn)->setData(emptyDisplay, Qt::DisplayRole);

        // status actual value

        item(0, StackStatusColumn)->setData(empty, ActualValueRole);

        // status tooltip

        item(0, StackStatusColumn)->setData(empty, Qt::ToolTipRole);

        // status color

        item(0, StackStatusColumn)->setData(GetIEditor()->GetColorByName("TextColor"), Qt::TextColorRole);

        // id text

        item(0, StackIdColumn)->setData(emptyDisplay, Qt::DisplayRole);

        // id tool tip

        item(0, StackIdColumn)->setData(empty, Qt::ToolTipRole);

        // creation time text

        item(0, CreationTimeColumn)->setData(emptyDisplay, Qt::DisplayRole);

        // updated time text

        item(0, LastUpdatedTimeColumn)->setData(emptyDisplay, Qt::DisplayRole);
    }

    AWSResourceManager* m_resourceManager;
    QSharedPointer<StackResourcesModel> m_stackResourcesModel;
    QSharedPointer<StackEventsModel> m_stackEventsModel;
    bool m_isRefreshing {
        false
    };
    bool m_isReady {
        false
    };
    QTime m_lastRefreshTime;
};
