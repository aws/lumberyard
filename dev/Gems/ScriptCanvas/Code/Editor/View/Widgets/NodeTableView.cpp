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

#include <precompiled.h>
#include "NodeTableView.h"
#include <QHeaderView>

#include <Editor/View/Widgets/ui_NodeTableView.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        NodeTableView::NodeTableView(QWidget* parent /*= nullptr*/)
            : QTableView(parent)
            , ui(new Ui::NodeTableView)

        {
            ui->setupUi(this);
        }

        void NodeTableView::Init()
        {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setDragEnabled(true);
            setAutoScroll(true);
            setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
            setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
            setDragDropMode(QAbstractItemView::DragDropMode::DragOnly);

            verticalHeader()->hide();
            horizontalHeader()->hide();
        }

        #include <Editor/View/Widgets/NodeTableView.moc>
    }
}

