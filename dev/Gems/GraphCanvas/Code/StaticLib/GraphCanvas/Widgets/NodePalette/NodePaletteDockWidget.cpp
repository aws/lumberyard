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
#include <QBoxLayout>
#include <QEvent>
#include <QCompleter>
#include <QCoreApplication>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QSignalBlocker>
#include <QScrollBar>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <StaticLib/GraphCanvas/Widgets/NodePalette/ui_NodePaletteDockWidget.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>

namespace GraphCanvas
{
    ////////////////////////////
    // NodePaletteTreeDelegate
    ////////////////////////////
    NodePaletteTreeDelegate::NodePaletteTreeDelegate(QWidget* parent)
        : GraphCanvas::IconDecoratedNameDelegate(parent)
    {
    }

    void NodePaletteTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        painter->save();

        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options, index);

        // paint the original node item
        GraphCanvas::IconDecoratedNameDelegate::paint(painter, option, index);

        const int textMargin = options.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, options.widget) + 1;
        QRect textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options);
        textRect = textRect.adjusted(textMargin, 0, -textMargin, 0);

        QModelIndex sourceIndex = static_cast<const NodePaletteSortFilterProxyModel*>(index.model())->mapToSource(index);
        GraphCanvas::NodePaletteTreeItem* treeItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceIndex.internalPointer());
        if (treeItem && treeItem->HasHighlight())
        {
            // pos, len
            const AZStd::pair<int, int>& highlight = treeItem->GetHighlight();
            QString preSelectedText = options.text.left(highlight.first);
            int preSelectedTextLength = options.fontMetrics.width(preSelectedText);
            QString selectedText = options.text.mid(highlight.first, highlight.second);
            int selectedTextLength = options.fontMetrics.width(selectedText);

            QRect highlightRect(textRect.left() + preSelectedTextLength, textRect.top(), selectedTextLength, textRect.height());

            // paint the highlight rect
            painter->fillRect(highlightRect, options.palette.highlight());
        }

        painter->restore();
    }

    //////////////////////////
    // NodePaletteDockWidget
    //////////////////////////
    NodePaletteDockWidget::NodePaletteDockWidget(GraphCanvasTreeItem* treeItem ,const EditorId& editorId, const QString& windowLabel, QWidget* parent, bool inContextMenu)
        : AzQtComponents::StyledDockWidget(parent)
        , m_ui(new Ui::NodePaletteDockWidget())
        , m_editorId(editorId)
        , m_contextMenuCreateEvent(nullptr)
        , m_model(nullptr)
    {
        m_model = aznew NodePaletteSortFilterProxyModel(this);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &NodePaletteDockWidget::UpdateFilter);

        setWindowTitle(windowLabel);
        m_ui->setupUi(this);

        QObject::connect(m_ui->m_quickFilter, &QLineEdit::textChanged, this, &NodePaletteDockWidget::OnQuickFilterChanged);

        QAction* clearAction = m_ui->m_quickFilter->addAction(QIcon(":/GraphCanvasEditorResources/lineedit_clear.png"), QLineEdit::TrailingPosition);
        QObject::connect(clearAction, &QAction::triggered, this, &NodePaletteDockWidget::ClearFilter);

        GraphCanvas::GraphCanvasTreeModel* sourceModel = aznew GraphCanvas::GraphCanvasTreeModel(treeItem, this);
        sourceModel->setMimeType(GetMimeType());

        GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

        m_model->setSourceModel(sourceModel);
        m_model->PopulateUnfilteredModel();

        m_ui->treeView->setModel(m_model);
        m_ui->treeView->setItemDelegate(aznew NodePaletteTreeDelegate(this));

        if (!inContextMenu)
        {
            QObject::connect(m_ui->m_quickFilter, &QLineEdit::returnPressed, this, &NodePaletteDockWidget::UpdateFilter);
        }
        else
        {
            QObject::connect(m_ui->m_quickFilter, &QLineEdit::returnPressed, this, &NodePaletteDockWidget::TrySpawnItem);
        }

        if (inContextMenu)
        {
            setTitleBarWidget(new QWidget());
            setFeatures(NoDockWidgetFeatures);
            setContentsMargins(15, 0, 0, 0);
            m_ui->dockWidgetContents->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

            QObject::connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePaletteDockWidget::OnSelectionChanged);
        }

        QObject::connect(m_ui->treeView->verticalScrollBar(), &QScrollBar::valueChanged, this, &NodePaletteDockWidget::OnScrollChanged);

        if (!inContextMenu)
        {
            GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(m_editorId);
            m_ui->treeView->InitializeTreeViewSaving(AZ_CRC("NodePalette_ContextView", 0x3cfece67));
            m_ui->treeView->ApplyTreeViewSnapshot();
        }
        else
        {
            m_ui->treeView->InitializeTreeViewSaving(AZ_CRC("NodePalette_TreeView", 0x9d6844cd));
        }

        m_ui->treeView->PauseTreeViewSaving();

        m_ui->m_categoryLabel->SetElideMode(Qt::ElideLeft);
    }

    NodePaletteDockWidget::~NodePaletteDockWidget()
    {
        GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusDisconnect();
        delete m_contextMenuCreateEvent;
    }

    void NodePaletteDockWidget::FocusOnSearchFilter()
    {
        m_ui->m_quickFilter->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void NodePaletteDockWidget::ResetDisplay()
    {
        delete m_contextMenuCreateEvent;
        m_contextMenuCreateEvent = nullptr;

        {
            QSignalBlocker blocker(m_ui->m_quickFilter);
            m_ui->m_quickFilter->clear();

            m_model->ClearFilter();
            m_model->invalidate();
        }

        {
            QSignalBlocker blocker(m_ui->treeView->selectionModel());
            m_ui->treeView->clearSelection();
        }

        m_ui->treeView->collapseAll();
        m_ui->m_categoryLabel->setFullText("");

        setVisible(true);
    }

    GraphCanvas::GraphCanvasMimeEvent* NodePaletteDockWidget::GetContextMenuEvent() const
    {
        return m_contextMenuCreateEvent;
    }

    void NodePaletteDockWidget::ResetSourceSlotFilter()
    {
        m_model->ResetSourceSlotFilter();
        m_ui->m_quickFilter->setCompleter(m_model->GetCompleter());
    }

    void NodePaletteDockWidget::FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId)
    {
        m_model->FilterForSourceSlot(sceneId, sourceSlotId);
        m_ui->m_quickFilter->setCompleter(m_model->GetCompleter());
    }

    void NodePaletteDockWidget::PreOnActiveGraphChanged()
    {
        ClearSelection();

        if (!m_model->HasFilter())
        {
            m_ui->treeView->UnpauseTreeViewSaving();
            m_ui->treeView->CaptureTreeViewSnapshot();
            m_ui->treeView->PauseTreeViewSaving();
        }

        m_ui->treeView->model()->layoutAboutToBeChanged();
    }

    void NodePaletteDockWidget::PostOnActiveGraphChanged()
    {
        m_ui->treeView->model()->layoutChanged();


        if (!m_model->HasFilter())
        {
            m_ui->treeView->UnpauseTreeViewSaving();
            m_ui->treeView->ApplyTreeViewSnapshot();
            m_ui->treeView->PauseTreeViewSaving();
        }
        else
        {
            UpdateFilter();
        }
    }

    void NodePaletteDockWidget::ClearSelection()
    {
        m_ui->treeView->selectionModel()->clearSelection();
    }

    const GraphCanvas::GraphCanvasTreeItem* NodePaletteDockWidget::GetTreeRoot() const
    {
        return static_cast<GraphCanvas::GraphCanvasTreeModel*>(m_model->sourceModel())->GetTreeRoot();
    }

    GraphCanvasTreeItem* NodePaletteDockWidget::CreatePaletteRoot() const
    {
        return nullptr;
    }

    void NodePaletteDockWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (selected.indexes().empty())
        {
            // Nothing to do.
            return;
        }

        auto index = selected.indexes().first();

        if (!index.isValid())
        {
            // Nothing to do.
            return;
        }

        // IMPORTANT: mapToSource() is NECESSARY. Otherwise the internalPointer
        // in the index is relative to the proxy model, NOT the source model.
        QModelIndex sourceModelIndex = m_model->mapToSource(index);

        GraphCanvas::NodePaletteTreeItem* nodePaletteItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(sourceModelIndex.internalPointer());
        HandleSelectedItem(nodePaletteItem);
    }

    void NodePaletteDockWidget::OnScrollChanged(int scrollPosition)
    {
        RefreshFloatingHeader();
    }

    void NodePaletteDockWidget::RefreshFloatingHeader()
    {
        // TODO: Fix slight visual hiccup with the sizing of the header when labels vanish.
        // Seems to remain at size for one frame.
        QModelIndex proxyIndex = m_ui->treeView->indexAt(QPoint(0, 0));
        QModelIndex modelIndex = m_model->mapToSource(proxyIndex);
        GraphCanvas::NodePaletteTreeItem* currentItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(modelIndex.internalPointer());

        QString fullPathString;

        bool needsSeparator = false;

        while (currentItem && currentItem->GetParent() != nullptr)
        {
            currentItem = static_cast<GraphCanvas::NodePaletteTreeItem*>(currentItem->GetParent());

            // This is the root element which is invisible. We don't want to display that.
            if (currentItem->GetParent() == nullptr)
            {
                break;
            }

            if (needsSeparator)
            {
                fullPathString.prepend("/");
            }

            fullPathString.prepend(currentItem->GetName());
            needsSeparator = true;
        }

        m_ui->m_categoryLabel->setFullText(fullPathString);
    }

    void NodePaletteDockWidget::OnQuickFilterChanged()
    {
        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void NodePaletteDockWidget::UpdateFilter()
    {
        if (!m_model->HasFilter())
        {
            m_ui->treeView->UnpauseTreeViewSaving();
            m_ui->treeView->CaptureTreeViewSnapshot();
            m_ui->treeView->PauseTreeViewSaving();
        }

        QString text = m_ui->m_quickFilter->userInputText();

        m_model->SetFilter(text);
        m_model->invalidate();

        if (!m_model->HasFilter())
        {
            m_ui->treeView->UnpauseTreeViewSaving();
            m_ui->treeView->ApplyTreeViewSnapshot();
            m_ui->treeView->PauseTreeViewSaving();

            m_ui->treeView->clearSelection();
        }
        else
        {
            m_ui->treeView->expandAll();
        }
    }

    void NodePaletteDockWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(m_ui->m_quickFilter);
            m_ui->m_quickFilter->setText("");
        }

        UpdateFilter();
    }

    void NodePaletteDockWidget::TrySpawnItem()
    {
        QCompleter* completer = m_ui->m_quickFilter->completer();
        QModelIndex modelIndex = completer->currentIndex();

        if (modelIndex.isValid())
        {
            // The docs say this is fine. So here's hoping.
            QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(completer->completionModel());

            if (proxyModel)
            {
                QModelIndex sourceIndex = proxyModel->mapToSource(modelIndex);

                if (sourceIndex.isValid())
                {
                    NodePaletteAutoCompleteModel* autoCompleteModel = qobject_cast<NodePaletteAutoCompleteModel*>(proxyModel->sourceModel());

                    const GraphCanvas::GraphCanvasTreeItem* treeItem = autoCompleteModel->FindItemForIndex(sourceIndex);

                    if (treeItem)
                    {
                        HandleSelectedItem(treeItem);
                    }
                }
            }
        }
        else
        {
            UpdateFilter();
        }
    }

    void NodePaletteDockWidget::HandleSelectedItem(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        m_contextMenuCreateEvent = treeItem->CreateMimeEvent();

        if (m_contextMenuCreateEvent)
        {
            emit OnContextMenuSelection();
        }
    }

    void NodePaletteDockWidget::ResetModel()
    {
        GraphCanvasTreeModelRequestBus::Handler::BusDisconnect(); 

        GraphCanvas::GraphCanvasTreeModel* sourceModel = aznew GraphCanvas::GraphCanvasTreeModel(CreatePaletteRoot(), this);
        sourceModel->setMimeType(GetMimeType());
            
        delete m_model;
        m_model = aznew NodePaletteSortFilterProxyModel(this);
        m_model->setSourceModel(sourceModel);
        m_model->PopulateUnfilteredModel();
            
        GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

        m_ui->treeView->setModel(m_model);

        ResetDisplay();
    }

    #include <StaticLib/GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.moc>
}
