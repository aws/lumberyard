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

#include <QAction>
#include <QTimer>
#include <qlabel.h>
#include <qitemselectionmodel.h>
#include <qstyleditemdelegate.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include "Editor/View/Windows/MainWindowBus.h"

class QSortFilterProxyModel;

namespace Ui
{
    class NodePalette;
}

namespace GraphCanvas
{
    class GraphCanvasMimeEvent;
}

namespace ScriptCanvasEditor
{
    namespace Model
    {
        class NodePaletteSortFilterProxyModel;
    }

    namespace Widget
    {        
        class NodeTreeView;

        // This class will provide a label that auto truncates itself to the size that it is given.
        class AutoElidedText
            : public QLabel
        {
        public:
            AZ_CLASS_ALLOCATOR(AutoElidedText, AZ::SystemAllocator, 0);

            AutoElidedText(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
            ~AutoElidedText();

            QString fullText() const;
            void setFullText(const QString& text);

            void resizeEvent(QResizeEvent* resizeEvent) override;

            QSize minimumSizeHint() const override;
            QSize sizeHint() const override;

            void RefreshLabel();

        private:

            QString m_fullText;
        };

        class NodePaletteTreeDelegate : public QStyledItemDelegate
        {
        public:
            AZ_CLASS_ALLOCATOR(NodePaletteTreeDelegate, AZ::SystemAllocator, 0);

            NodePaletteTreeDelegate(QWidget* parent = nullptr);
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        };

        class NodePalette
            : public AzQtComponents::StyledDockWidget
            , public MainWindowNotificationBus::Handler
            , public GraphCanvas::GraphCanvasTreeModelRequestBus::Handler
        {
            Q_OBJECT
        public:
            static const char* GetMimeType() { return "scriptcanvas/node-palette-mime-event"; }

            NodePalette(const QString& windowLabel, QWidget* parent, bool inContextMenu = false);
            ~NodePalette();

            void FocusOnSearchFilter();

            void ResetModel();
            void ResetDisplay();
            GraphCanvas::GraphCanvasMimeEvent* GetContextMenuEvent() const;

            // MainWindowNotificationBus
            void PreOnActiveSceneChanged() override;
            void PostOnActiveSceneChanged() override;
            ////

            // GraphCanvas::GraphCavnasTreeModelRequestBus::Handler
            void ClearSelection() override;
            ////

        public slots:
            void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void OnScrollChanged(int scrollPosition);

        signals:
            void OnContextMenuSelection();

        private:

            bool eventFilter(QObject* object, QEvent* event) override;

            void RefreshFloatingHeader();
            void OnQuickFilterChanged();
            void UpdateFilter();
            void ClearFilter();

            bool m_sliding;

            qreal m_scrollRange;
            qreal m_scrollbarHeight;

            QPointF m_topRight;
            QPointF m_bottomRight;
            QPointF m_lastPosition;
            qreal m_offsetCounter;

            AZStd::unique_ptr<Ui::NodePalette> ui;
            
            GraphCanvas::GraphCanvasMimeEvent* m_contextMenuCreateEvent;

            QTimer        m_filterTimer;
            AZStd::vector< AutoElidedText* > m_floatingLabels;
            NodeTreeView* m_view;
            Model::NodePaletteSortFilterProxyModel* m_model;
        };
    }
}
