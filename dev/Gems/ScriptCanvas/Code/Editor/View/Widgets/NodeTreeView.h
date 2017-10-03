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
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class NodeTreeView
            : public AzToolsFramework::QTreeViewWithStateSaving
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(NodeTreeView, AZ::SystemAllocator, 0);
            explicit NodeTreeView(QWidget* parent = nullptr);
			
            void resizeEvent(QResizeEvent* event) override;

        protected:
            void mousePressEvent(QMouseEvent* ev) override;
            void mouseMoveEvent(QMouseEvent* ev) override;
            void mouseReleaseEvent(QMouseEvent* ev) override;

        private:
            void UpdatePointer(const QModelIndex &modelIndex, bool isMousePressed);
        };
    }
}
