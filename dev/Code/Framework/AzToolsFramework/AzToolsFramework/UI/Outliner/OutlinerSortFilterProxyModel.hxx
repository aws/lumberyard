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

#ifndef OUTLINER_SORT_FILTER_PROXY_MODEL_H
#define OUTLINER_SORT_FILTER_PROXY_MODEL_H

#include <AzCore/base.h>
#include <QtCore/QSortFilterProxyModel>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

namespace AzToolsFramework
{
    /*!
     * Enables the Outliner to filter entries based on search string.
     * Enables the Outliner to do custom sorting on entries.
     */
    class OutlinerSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(OutlinerSortFilterProxyModel, AZ::SystemAllocator, 0)

        OutlinerSortFilterProxyModel(QObject* pParent = nullptr);

        void UpdateFilter();

        //////////////////////////////////////////////////////////////
        // A series of punch-through translators to allow the model
        // to exchange information with the view.
        //////////////////////////////////////////////////////////////
    public Q_SLOTS:
        void OnItemExpanded(const QModelIndex& index);
        void OnItemCollapsed(const QModelIndex& index);
        void OnExpandItem(const QModelIndex& index);

    Q_SIGNALS:
        void ItemExpanded(const QModelIndex& index);
        void ItemCollapsed(const QModelIndex& index);
        void ExpandItem(const QModelIndex& index);

    protected:
        // Qt overrides
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
        void sort(int column, Qt::SortOrder order) override;

        QString m_filterName;
    };
}

#endif
