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

#include <AWSUtil.h>
#include <QTime>
#include <QDebug>
#include <QSortFilterProxyModel>

#include <IEditor.h>

class StackResourcesModel
    : public IStackResourcesModel
{
    Q_OBJECT

public:

    StackResourcesModel(AWSResourceManager* resourceManager)
        : m_resourceManager{resourceManager}
        , m_requestId{resourceManager->AllocateRequestId()}
    {
        InitializeModel(this);
        connect(m_resourceManager, &AWSResourceManager::ProcessOutputResourceList, this, &StackResourcesModel::ProcessOutputResourceList);
        appendEmptyRow();
    }

    void Refresh(bool force) override
    {
        if (force || IsRefreshTime())
        {
            QVariantMap args;
            const char* command = PrepareRefreshCommand(args);
            m_resourceManager->ExecuteAsyncWithRetry(m_requestId, command, args);
            m_isRefreshing = true;
            RefreshStatusChanged(m_isRefreshing);
        }
    }

    QString GetStackResourceRegion() const override
    {
        QSharedPointer<IAWSDeploymentModel> deploymentModel = m_resourceManager->GetDeploymentModel();
        QString region = deploymentModel->GetActiveDeploymentRegion();
        return region;
    }

    virtual const char* PrepareRefreshCommand(QVariantMap& args) = 0;

    bool IsRefreshTime()
    {
        return !m_lastRefreshTime.isValid() || m_lastRefreshTime.elapsed() > 2000;
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
        return IStackResourcesModel::headerData(section, orientation, role);
    }

private:

    AWSResourceManager* m_resourceManager;
    bool m_isRefreshing { false };
    bool m_isReady { false };
    QTime m_lastRefreshTime {};
    AWSResourceManager::RequestId m_requestId;

    bool m_containsChanges{ false };
    bool m_containsDeletionChanges{ false };
    bool m_containsSecurityChanges{ false };

    static void InitializeModel(QStandardItemModel* model)
    {
        model->setColumnCount(ColumnCount);
        model->setHorizontalHeaderItem(PendingActionColumn, new QStandardItem{ "Pending" });
        model->setHorizontalHeaderItem(ChangeImpactColumn, new QStandardItem{ "Impacts" });
        model->setHorizontalHeaderItem(NameColumn, new QStandardItem{ "Resource Name" });
        model->setHorizontalHeaderItem(ResourceTypeColumn, new QStandardItem{ "Type" });
        model->setHorizontalHeaderItem(ResourceStatusColumn, new QStandardItem{ "Status" });
        model->setHorizontalHeaderItem(TimestampColumn, new QStandardItem{ "Timestamp" });
        model->setHorizontalHeaderItem(PhysicalResourceIdColumn, new QStandardItem{ "ID" });
    }

    void ProcessOutputResourceList(AWSResourceManager::RequestId requestId, const QVariant& value)
    {

        if (requestId != m_requestId)
        {
            return;
        }

        auto map = value.toMap();

        auto resources = VariantListToMapList(map["Resources"].toList());
        Sort(resources, "Name");

        beginResetModel();

        m_containsChanges = false;
        m_containsSecurityChanges = false;
        m_containsDeletionChanges = false;

        removeRows(0, rowCount());

        for (auto it = resources.constBegin(); it != resources.constEnd(); ++it)
        {
            appendRow(MakeRow(*it));
        }

        if (rowCount() == 0)
        {
            appendEmptyRow();
        }

        m_lastRefreshTime.start();

        m_isRefreshing = false;
        m_isReady = true;

        endResetModel();

        RefreshStatusChanged(m_isRefreshing);

    }

    void appendEmptyRow()
    {
        QVariantMap map;
        appendRow(MakeRow(map));
    }

    static QList<QVariantMap> VariantListToMapList(const QList<QVariant>& list)
    {
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

        qSort(list.begin(), list.end(), compare);
    }

    enum
    {
        PendingChangeFilterRole = Qt::UserRole
    };

    QList<QStandardItem*> MakeRow(const QVariantMap& resource)
    {
        QList<QStandardItem*> row {};
        for (int column = 0; column < ColumnCount; ++column)
        {
            row.append(new QStandardItem {});
        }

        auto pendingAction = resource["PendingAction"].toString();
        if (pendingAction.isEmpty())
        {
            row[PendingActionColumn]->setText("--");
            row[PendingActionColumn]->setData("NO", PendingChangeFilterRole);
        }
        else
        {
            row[PendingActionColumn]->setText(AWSUtil::MakePrettyPendingActionText(pendingAction));
            row[PendingActionColumn]->setData(AWSUtil::MakePrettyPendingReasonTooltip(resource["PendingReason"].toString()), Qt::ToolTipRole);
            row[PendingActionColumn]->setData(AWSUtil::MakePrettyPendingActionColor(pendingAction), Qt::TextColorRole);
            row[PendingActionColumn]->setData("YES", PendingChangeFilterRole);
            m_containsChanges = true;
            if (pendingAction == "DELETE")
            {
                m_containsDeletionChanges = true;
            }
        }

        auto isPendingSecurityChange = resource["IsPendingSecurityChange"].toBool();
        if (isPendingSecurityChange)
        {
            row[ChangeImpactColumn]->setText(tr("Security"));
            row[ChangeImpactColumn]->setData(AWSUtil::MakePrettyPendingReasonTooltip(resource["PendingReason"].toString()), Qt::ToolTipRole);
            row[ChangeImpactColumn]->setData(GetIEditor()->GetColorByName("TextErrorColor"), Qt::TextColorRole);
            m_containsSecurityChanges = true;
        }
        else
        {
            row[ChangeImpactColumn]->setText("--");
        }

        auto name = resource["Name"].toString();
        if (name.isEmpty())
        {
            name = "--";
        }
        row[NameColumn]->setText(name);

        auto type = resource["ResourceType"].toString();
        if (type.isEmpty())
        {
            type = "--";
        }
        row[ResourceTypeColumn]->setText(type);

        auto resourceStatus = resource["ResourceStatus"].toString();
        if (resourceStatus.isEmpty())
        {
            row[ResourceStatusColumn]->setText("--");
        }
        else
        {
            row[ResourceStatusColumn]->setText(AWSUtil::MakePrettyResourceStatusText(resourceStatus));
            row[ResourceStatusColumn]->setData(AWSUtil::MakePrettyResourceStatusTooltip(resourceStatus, resource["ResourceStatusReason"].toString()), Qt::ToolTipRole);
            row[ResourceStatusColumn]->setData(AWSUtil::MakePrettyResourceStatusColor(resourceStatus), Qt::TextColorRole);
        }

        auto timestamp = resource["Timestamp"].toDateTime().toLocalTime().toString();
        if (timestamp.isEmpty())
        {
            timestamp = "--";
        }
        row[TimestampColumn]->setText(timestamp);

        auto id = resource["PhysicalResourceId"].toString();
        if (id.isEmpty())
        {
            row[PhysicalResourceIdColumn]->setText("--");
        }
        else
        {
            row[PhysicalResourceIdColumn]->setText(AWSUtil::MakeShortResourceId(id));
            row[PhysicalResourceIdColumn]->setData(id, Qt::ToolTipRole);
        }

        return row;
    }

    // This is... convoluted... when prompting the user to confirm changes we want to
    // show an empty row while waiting for the results of a forced refresh, then if 
    // there are no changes, we want to continue showing an empty row. 
    //
    // If we go with the QSortFilterProxyModel approach, we need a way to fake this
    // empty row. We do that by starting out as a proxy for an "empty" model, which
    // contains only the empty row, and then switching to be a proxy of the real 
    // model once it is reset, but only if it contains changes.
    //
    // We also depend on the view to release the proxy before releasing the underlying
    // model. If we had a way to get from "this" to a QSmartPointer that would not
    // be a problem.
    
    class EmptyPendingChangeProxyModel : public QStandardItemModel
    {

    public:

        EmptyPendingChangeProxyModel()
        {

            InitializeModel(this);

            QList<QStandardItem*> row{};
            for (int column = 0; column < ColumnCount; ++column)
            {
                row.append(new QStandardItem{});
            }

            row[PendingActionColumn]->setText("--");
            row[PendingActionColumn]->setData("YES", PendingChangeFilterRole);
            row[ChangeImpactColumn]->setText("--");
            row[NameColumn]->setText("--");
            row[ResourceTypeColumn]->setText("--");
            row[ResourceStatusColumn]->setText("--");
            row[TimestampColumn]->setText("--");
            row[PhysicalResourceIdColumn]->setText("--");

            appendRow(row);

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
            return QStandardItemModel::headerData(section, orientation, role);
        }

    };

    static EmptyPendingChangeProxyModel s_EmptyPendingChangeProxyModel;

    class PendingChangesProxyModel
        : public QSortFilterProxyModel
    {

    public:

        PendingChangesProxyModel(StackResourcesModel* proxiedModel)
            : QSortFilterProxyModel{ proxiedModel }
            , m_proxiedModel{ proxiedModel }
        {
            setFilterKeyColumn(PendingActionColumn);
            setFilterRole(PendingChangeFilterRole);
            setFilterFixedString("YES");
            setSourceModel(&s_EmptyPendingChangeProxyModel);
            connect(m_proxiedModel, &StackResourcesModel::modelReset, this, &PendingChangesProxyModel::OnProxiedModelReset);
            m_proxiedModel->Refresh(true); // force
        }

    private:

        StackResourcesModel* m_proxiedModel;

        void OnProxiedModelReset()
        {
            // we depend on setSourceModel to fire the modelReset signal for the view
            if (m_proxiedModel->ContainsChanges())
            {
                setSourceModel(m_proxiedModel);
            }
            else
            {
                setSourceModel(&s_EmptyPendingChangeProxyModel);
            }
        }

    };

    QSharedPointer<QSortFilterProxyModel> GetPendingChangeFilterProxy() override
    {
        return QSharedPointer<PendingChangesProxyModel>::create(this).dynamicCast<QSortFilterProxyModel>();
    }

    bool ContainsChanges() override
    {
        return m_containsChanges;
    }

    bool ContainsDeletionChanges()
    {
        return m_containsDeletionChanges;
    }

    bool ContainsSecurityChanges()
    {
        return m_containsSecurityChanges;
    }

};
