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

#include <QStandardItemModel>
#include "qstandarditemmodel.h"
// The abstract base class used for all AWSResourceManager QT models. This
// class is an extension of QStandardItemModel that lets you easily use
// scoped enums for the column index.

template<class ColumnEnumType>
class IAWSResourceManagerModel
    : public QStandardItemModel
{
public:

    typedef ColumnEnumType ColumnEnum;

    // QAbstractItemModel overrides

    using QStandardItemModel::index; // unhide

    virtual QModelIndex index(int row, ColumnEnum column, const QModelIndex& parent = QModelIndex()) const
    {
        return index(row, columnEnumToInt(column), parent);
    }

    using QStandardItemModel::sort;

    virtual void sort(ColumnEnum column, Qt::SortOrder order = Qt::SortOrder::AscendingOrder)
    {
        return QStandardItemModel::sort(columnEnumToInt(column), order);
    }

    using QStandardItemModel::data;

    virtual QVariant data(int row, ColumnEnum column) const
    {
        return data(index(row, column));
    }

    int findRow(ColumnEnum column, const QVariant& desiredValue) const
    {
        for (int row = 0; row < rowCount(); ++row)
        {
            if (data(row, column) == desiredValue)
            {
                return row;
            }
        }
        return -1;
    }

    // Helper Functions

    static int columnEnumToInt(ColumnEnum column)
    {
        return static_cast<int>(column);
    }

    static ColumnEnum intToColumnEnum(int column)
    {
        return static_cast<ColumnEnum>(column);
    }
};
