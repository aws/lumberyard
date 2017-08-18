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
        setColumnCount(columnCount);

        setHorizontalHeaderItem(NameColumn, new QStandardItem {"Name"});
        setHorizontalHeaderItem(ResourceStatusColumn, new QStandardItem {"Status"});
        setHorizontalHeaderItem(TimestampColumn, new QStandardItem {"Timestamp"});
        setHorizontalHeaderItem(PhysicalResourceIdColumn, new QStandardItem {"ID"});
        setHorizontalHeaderItem(PendingActionColumn, new QStandardItem{ "Pending" });
    }

    void Refresh(bool force) override
    {
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
    QTime m_lastRefreshTime;

    bool IsRefreshTime()
    {
        return !m_isRefreshing && (!m_lastRefreshTime.isValid() || m_lastRefreshTime.elapsed() > 2000);
    }

    void ProcessStatusList(QVariant& variantList)
    {
        auto list = VariantToMapList(variantList);
        Sort(list, "Name");

        beginResetModel();

        removeRows(0, rowCount());

        for (auto it = list.constBegin(); it != list.constEnd(); ++it)
        {
            appendRow(MakeRow(*it));
        }

        m_lastRefreshTime.start();

        m_isRefreshing = false;
        m_isReady = true;

        endResetModel();

        RefreshStatusChanged(m_isRefreshing);
    }

    QList<QStandardItem*> MakeRow(const QVariantMap& map)
    {
        QList<QStandardItem*> row;
        for (int column = 0; column < ColumnCount; ++column)
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
            row[PendingActionColumn]->setText("--");
        }
        else
        {
            row[PendingActionColumn]->setText(AWSUtil::MakePrettyPendingActionText(pendingAction));
            row[PendingActionColumn]->setData(AWSUtil::MakePrettyPendingReasonTooltip(map["PendingReason"].toString()), Qt::ToolTipRole);
            row[PendingActionColumn]->setData(AWSUtil::MakePrettyPendingActionColor(pendingAction), Qt::TextColorRole);
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
                return v1[column] < v2[column];
            };

        qSort(list.begin(), list.end(), compare);
    }
};
