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
#include <QTime>
#include "AWSUtil.h"

template<typename BaseType>
class StackStatusListModel
    : public BaseType
{
public:

    StackStatusListModel(AWSResourceManager* resourceManager, int columnCount)
        : m_resourceManager{resourceManager}
    {
        BaseType::setColumnCount(columnCount);

        BaseType::setHorizontalHeaderItem(BaseType::NameColumn, new QStandardItem {"Name"});
        BaseType::setHorizontalHeaderItem(BaseType::ResourceStatusColumn, new QStandardItem {"Status"});
        BaseType::setHorizontalHeaderItem(BaseType::TimestampColumn, new QStandardItem {"Timestamp"});
        BaseType::setHorizontalHeaderItem(BaseType::PhysicalResourceIdColumn, new QStandardItem {"ID"});
        BaseType::setHorizontalHeaderItem(BaseType::PendingActionColumn, new QStandardItem{ "Pending" });
    }

    void Refresh(bool force) override
    {
        m_isRefreshing = true;
        BaseType::RefreshStatusChanged(m_isRefreshing);
    }

    bool IsRefreshing() const override
    {
        return m_isRefreshing;
    }

    bool IsReady() const override
    {
        return m_isReady;
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

    AWSResourceManager* m_resourceManager;
    bool m_isRefreshing {
        false
    };
    bool m_isReady {
        false
    };
    QElapsedTimer m_lastRefreshTime;

    bool IsRefreshTime()
    {
        return !m_isRefreshing && (!m_lastRefreshTime.isValid() || m_lastRefreshTime.elapsed() > 2000);
    }

    void ProcessStatusList(QVariant& variantList)
    {
        auto list = VariantToMapList(variantList);
        Sort(list, "Name");

        BaseType::beginResetModel();

        BaseType::removeRows(0, BaseType::rowCount());

        for (auto it = list.constBegin(); it != list.constEnd(); ++it)
        {
            BaseType::appendRow(MakeRow(*it));
        }

        m_lastRefreshTime.start();

        m_isRefreshing = false;
        m_isReady = true;

        BaseType::endResetModel();

        BaseType::RefreshStatusChanged(m_isRefreshing);
    }

    QList<QStandardItem*> MakeRow(const QVariantMap& map)
    {
        QList<QStandardItem*> row;
        for (int column = 0; column < BaseType::ColumnCount; ++column)
        {
            row.append(new QStandardItem {});
        }

        UpdateRow(row, map);

        return row;
    }

    virtual void UpdateRow(const QList<QStandardItem*>& row, const QVariantMap& map)
    {

        auto pendingAction = map["PendingAction"].toString();
        if (pendingAction.isEmpty())
        {
            row[BaseType::PendingActionColumn]->setText("--");
        }
        else
        {
            row[BaseType::PendingActionColumn]->setText(AWSUtil::MakePrettyPendingActionText(pendingAction));
            row[BaseType::PendingActionColumn]->setData(AWSUtil::MakePrettyPendingReasonTooltip(map["PendingReason"].toString()), Qt::ToolTipRole);
            row[BaseType::PendingActionColumn]->setData(AWSUtil::MakePrettyPendingActionColor(pendingAction), Qt::ForegroundRole);
        }
        
    }

    static QList<QVariantMap> VariantToMapList(const QVariant& variant)
    {
        auto list = variant.toList();
        QList<QVariantMap> result;
        for (auto it = list.constBegin(); it != list.constEnd(); ++it)
        {
            result.append(it->toMap());
        }
        return result;
    }

    static void Sort(QList<QVariantMap>& list, const QString& column)
    {
        auto compare = [column](const QVariantMap& v1, const QVariantMap& v2)
            {
                return v1[column].toString() < v2[column].toString();
            };

        std::sort(list.begin(), list.end(), compare);
    }
};
