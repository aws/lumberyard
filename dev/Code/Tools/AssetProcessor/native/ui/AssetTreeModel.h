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

#include <AssetDatabase/AssetDatabase.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QItemSelectionModel>

namespace AssetProcessor
{
    class AssetTreeItem;
    class AssetTreeModel : public QAbstractItemModel
    {
    public:
        AssetTreeModel(QObject *parent = nullptr);
        virtual ~AssetTreeModel();

        QModelIndex index(int row, int column, const QModelIndex &parent) const override;

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        QModelIndex parent(const QModelIndex &index) const override;
        bool hasChildren(const QModelIndex &parent) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;

        void Reset();

        static QFlags<QItemSelectionModel::SelectionFlag> GetAssetTreeSelectionFlags();

    protected:
        // Called by reset, while a Qt model reset is active.
        virtual void ResetModel() = 0;

        AZStd::unique_ptr<AssetTreeItem> m_root;
        QFileIconProvider m_fileProvider; // Cache the icon provider, it's expensive to construct.
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection m_dbConnection;
    };
}
