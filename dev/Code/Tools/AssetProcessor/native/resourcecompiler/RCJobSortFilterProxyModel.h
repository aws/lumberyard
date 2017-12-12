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

#include <QSortFilterProxyModel>



class RCJobSortFilterProxyModel
    : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit RCJobSortFilterProxyModel(QObject* parent = 0);


protected:
    //    bool lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

