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

#include <QTreeView>
#include <QMouseEvent>

class MannDeselectableTreeView : public QTreeView
{
public:
    MannDeselectableTreeView(QWidget *parent) : QTreeView(parent) {}
    virtual ~MannDeselectableTreeView() {}

private:
    virtual void mousePressEvent(QMouseEvent *event)
    {
        QModelIndex currentItem = indexAt(event->pos());

        if (!currentItem.isValid())
        {
            clearSelection();
            selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Select);
        }

        QTreeView::mousePressEvent(event);
    }
};