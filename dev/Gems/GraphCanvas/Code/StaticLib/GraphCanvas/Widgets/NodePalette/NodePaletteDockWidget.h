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
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>

class QSortFilterProxyModel;

namespace Ui
{
    class NodePaletteDockWidget;
}

namespace GraphCanvas
{    
    class GraphCanvasMimeEvent;
    class NodePaletteSortFilterProxyModel;

    class NodePaletteTreeDelegate : public GraphCanvas::IconDecoratedNameDelegate
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteTreeDelegate, AZ::SystemAllocator, 0);

        NodePaletteTreeDelegate(QWidget* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    };

    class NodePaletteDockWidget
        : public AzQtComponents::StyledDockWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public GraphCanvas::GraphCanvasTreeModelRequestBus::Handler
    {
        Q_OBJECT
    public:
        static const char* GetMimeType() { return "scriptcanvas/node-palette-mime-event"; }

        NodePaletteDockWidget(GraphCanvasTreeItem* treeItem, const EditorId& editorId, const QString& windowLabel, QWidget* parent, bool inContextMenu);
        ~NodePaletteDockWidget();

        void FocusOnSearchFilter();

        void ResetModel();
        void ResetDisplay();

        GraphCanvas::GraphCanvasMimeEvent* GetContextMenuEvent() const;

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId);

        // GraphCanvas::AssetEditorNotificationBus
        void PreOnActiveGraphChanged() override;
        void PostOnActiveGraphChanged() override;
        ////

        // GraphCanvas::GraphCanvasTreeModelRequestBus::Handler
        void ClearSelection() override;
        ////

        // NodeTreeView
        const GraphCanvasTreeItem* GetTreeRoot() const;
        ////

    protected:

        // This method here is to help facilitate resetting the model. This will not be called during
        // the initial construction(because yay virtual functions).
        virtual GraphCanvasTreeItem* CreatePaletteRoot() const;

    public slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnScrollChanged(int scrollPosition);

    signals:
        void OnContextMenuSelection();

    private:

        void RefreshFloatingHeader();
        void OnQuickFilterChanged();
        void UpdateFilter();
        void ClearFilter();

        // Will try and spawn the item specified by the QCompleter
        void TrySpawnItem();

        void HandleSelectedItem(const GraphCanvas::GraphCanvasTreeItem* treeItem);

        AZStd::unique_ptr<Ui::NodePaletteDockWidget> m_ui;
            
        EditorId m_editorId;
        GraphCanvas::GraphCanvasMimeEvent* m_contextMenuCreateEvent;

        QTimer        m_filterTimer;;
        NodePaletteSortFilterProxyModel* m_model;
    };
}
