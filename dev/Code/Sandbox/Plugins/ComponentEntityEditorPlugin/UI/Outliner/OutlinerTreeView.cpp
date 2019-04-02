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
#include "stdafx.h"

#include "OutlinerTreeView.hxx"
#include "OutlinerListModel.hxx"

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/algorithm.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzQtComponents/Components/Style.h>

#include <QDrag>
#include <QPainter>
#include <QHeaderView>
#include <QProxyStyle>

class OutlinerTreeViewProxyStyle : public QProxyStyle
{
public:
    OutlinerTreeViewProxyStyle()
        : QProxyStyle()
        , m_linePen(QColor("#FFFFFF"), 1)
        , m_rectPen(QColor("#B48BFF"), 1)
    {
        AzQtComponents::Style::fixProxyStyle(this, qApp->style());
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
    {
        // FIXME: Move to AzQtComponents Style.cpp
        //draw custom drop target art
        if (element == QStyle::PE_IndicatorItemViewItemDrop)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);

            if (option->rect.height() == 0)
            {
                //draw circle followed by line for drops between items
                painter->setPen(m_linePen);
                painter->drawEllipse(option->rect.topLeft(), 3, 3);
                painter->drawLine(QPoint(option->rect.topLeft().x() + 3, option->rect.topLeft().y()), option->rect.topRight());
            }
            else
            {
                //draw a rounded box for drops on items
                painter->setPen(m_rectPen);
                painter->drawRoundedRect(option->rect, 3, 3);
            }
            painter->restore();
            return;
        }

        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

private:
    QPen m_linePen;
    QPen m_rectPen;
};


OutlinerTreeView::OutlinerTreeView(QWidget* pParent)
    : QTreeView(pParent)
    , m_mousePressedQueued(false)
    , m_draggingUnselectedItem(false)
    , m_mousePressedPos()
{
    setStyle(new OutlinerTreeViewProxyStyle());

    setUniformRowHeights(true);
    setHeaderHidden(true);
}

OutlinerTreeView::~OutlinerTreeView()
{
}

void OutlinerTreeView::setAutoExpandDelay(int delay)
{
    m_expandOnlyDelay = delay;
}

void OutlinerTreeView::mousePressEvent(QMouseEvent* event)
{
    //postponing normal mouse pressed logic until mouse is released or dragged
    //this means selection occurs on mouse released now
    //this is to support drag/drop of non-selected items
    m_mousePressedQueued = true;

    m_mousePressedPos = event->pos();
}

void OutlinerTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_mousePressedQueued && !m_draggingUnselectedItem)
    {
        // mouseMoveEvent will set the state to be DraggingState, which will make Qt ignore 
        // mousePressEvent in QTreeViewPrivate::expandOrCollapseItemAtPos. So we manually 
        // and temporarily set it to EditingState.
        QAbstractItemView::State stateBefore = QAbstractItemView::state();
        QAbstractItemView::setState(QAbstractItemView::State::EditingState);

        //treat this as a mouse pressed event to process selection etc
        processQueuedMousePressedEvent(event);

        QAbstractItemView::setState(stateBefore);
    }
    m_mousePressedQueued = false;
    m_draggingUnselectedItem = false;

    QTreeView::mouseReleaseEvent(event);
}

void OutlinerTreeView::mouseDoubleClickEvent(QMouseEvent* event)
{
    //cancel pending mouse press
    m_mousePressedQueued = false;
    QTreeView::mouseDoubleClickEvent(event);
}

void OutlinerTreeView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_mousePressedQueued)
    {
        //disable selection for the pending click if the mouse moved so selection is maintained for dragging
        QAbstractItemView::SelectionMode selectionModeBefore = selectionMode();
        setSelectionMode(QAbstractItemView::NoSelection);

        //treat this as a mouse pressed event to process everything but selection
        processQueuedMousePressedEvent(event);

        //restore selection state
        setSelectionMode(selectionModeBefore);
    }

    //process mouse movement as normal, potentially triggering drag and drop
    QTreeView::mouseMoveEvent(event);
}

void OutlinerTreeView::focusInEvent(QFocusEvent* event)
{
    //cancel pending mouse press
    m_mousePressedQueued = false;
    QTreeView::focusInEvent(event);
}

void OutlinerTreeView::focusOutEvent(QFocusEvent* event)
{
    //cancel pending mouse press
    m_mousePressedQueued = false;
    QTreeView::focusOutEvent(event);
}

void OutlinerTreeView::startDrag(Qt::DropActions supportedActions)
{
    //if we are attempting to drag an unselected item then we must special case drag and drop logic
    //QAbstractItemView::startDrag only supports selected items
    QModelIndex index = indexAt(m_mousePressedPos);
    if (!index.isValid() || index.column() != 0)
    {
        return;
    }

    if (!selectionModel()->isSelected(index))
    {
        startCustomDrag({ index }, supportedActions);
        return;
    }

    if (!selectionModel()->selectedIndexes().empty())
    {
        startCustomDrag(selectionModel()->selectedIndexes(), supportedActions);
        return;
    }
}

void OutlinerTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    if (m_expandOnlyDelay >= 0)
    {
        m_expandTimer.start(m_expandOnlyDelay, this);
    }

    QTreeView::dragMoveEvent(event);
}

void OutlinerTreeView::dropEvent(QDropEvent* event)
{
    emit ItemDropped();
    QTreeView::dropEvent(event);
}

QColor GetHierarchyLineColor(bool isSliceEntity, bool isSelected)
{
    if (isSliceEntity && isSelected)
    {
        return GetIEditor()->GetColorByName("HierarchyLinesSlicesSelected");
    }
    else if(isSliceEntity)
    {
        return GetIEditor()->GetColorByName("HierarchyLinesSlices");
    }
    else if (isSelected)
    {
        return GetIEditor()->GetColorByName("HierarchyLinesNonSliceEntitiesSelected");
    }
    else
    {
        return GetIEditor()->GetColorByName("HierarchyLinesNonSliceEntities");
    }
}

void OutlinerTreeView::DrawLayerUI(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
    static const QColor outlinerHighlightColor(GetIEditor()->GetColorByName("LayerChildBGSelectionColor"));
    static const QColor layerChildBGColor(GetIEditor()->GetColorByName("LayerChildBackgroundColor"));
    bool isSelected = selectionModel()->isSelected(index);

    QColor layerBranchesBGColor = isSelected ? outlinerHighlightColor : layerChildBGColor;

    painter->save();
    painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);

    bool hasLayerAncestor = false;
    for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
    {
        auto ancestorType = OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
        if (ancestorType == OutlinerListModel::LayerType)
        {
            hasLayerAncestor = true;
            break;
        }
    }

    if (hasLayerAncestor)
    {
        QPainterPath layerBGPath;
        QRect layerBGRect(rect);
        layerBGRect.setLeft(layerBGRect.left() + indentation());
        layerBGPath.addRect(layerBGRect);
        painter->fillPath(layerBGPath, layerBranchesBGColor);
    }

    OutlinerListModel::EntryType indexEntryType = OutlinerListModel::EntryType(index.data(OutlinerListModel::EntityTypeRole).value<int>());
    if (indexEntryType == OutlinerListModel::LayerType)
    {
        QColor layerColor = index.data(OutlinerListModel::LayerColorRole).value<QColor>();
        QPainterPath layerIconPath;
        const int layerSquareSize = GetLayerSquareSize();
        QPoint layerBoxOffset(1 + OutlinerListModel::GetLayerStripeWidth()*2, (rect.height() - layerSquareSize) / 2);
        QRect layerIconRect(rect.topRight() + layerBoxOffset, QSize(layerSquareSize, layerSquareSize));
        layerIconPath.addRect(layerIconRect);
        painter->fillPath(layerIconPath, layerColor);
    }

    painter->restore();
}

void OutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
    DrawLayerUI(painter, rect, index);

    // Make sure the base class is called after the layer rect is drawn,
    // so that the foldout arrow draws on top of the layer color.
    QTreeView::drawBranches(painter, rect, index);

    // No need to draw connecting lines if this has no parent.
    if (!index.parent().isValid())
    {
        return;
    }

    QPen branchLinePen;
    branchLinePen.setWidthF(m_branchLineWidth);

    int lineBaseX = rect.right();
    QModelIndex previousIndex = index;
    for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
    {
        auto ancestorType = OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
        bool isSliceEntity = ancestorType == OutlinerListModel::SliceEntityType || ancestorType == OutlinerListModel::SliceHandleType;
        bool isSelected = selectionModel()->isSelected(index);
        branchLinePen.setColor(GetHierarchyLineColor(isSliceEntity, isSelected));

        // Layers don't have connecting lines drawn.
        if (ancestorType == OutlinerListModel::LayerType)
        {
            // Layers can only have other layers as parents, so once a layer is found in a hierarchy
            // the line drawing can stop.
            break;
        }
        painter->save();
        painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);
        painter->setPen(branchLinePen);
        int rectHalfHeight = rect.height() / 2;
        if (previousIndex == index)
        {
            // draw a horizontal line from the parent branch to the item
            // if the item has children offset the drawn line to compensate for drawn expander buttons
            bool hasChildren = previousIndex.child(0, 0).isValid();
            int horizontalLineY = rect.top() + rectHalfHeight;
            int horizontalLineLeft = rect.right() - indentation() * 1.5f;
            int horizontalLineRight = hasChildren ? (lineBaseX - indentation()) : (lineBaseX - indentation() * 0.5f);
            painter->drawLine(horizontalLineLeft, horizontalLineY, horizontalLineRight, horizontalLineY);
        }

        // draw a vertical line segment connecting parent to child and child to child
        // if this is the last item, only draw half the line to terminate the segment
        bool hasNext = previousIndex.sibling(previousIndex.row() + 1, previousIndex.column()).isValid();
        if (hasNext || previousIndex == index)
        {
            int verticalLineX = lineBaseX - indentation() * 1.5f;
            int verticalLineTop = rect.top();
            int verticalLineBottom = hasNext ? rect.bottom() : rect.bottom() - rectHalfHeight;
            painter->drawLine(verticalLineX, verticalLineTop, verticalLineX, verticalLineBottom);
        }

        painter->restore();

        lineBaseX -= indentation();
        previousIndex = ancestorIndex;
    }

    painter->restore();
}

void OutlinerTreeView::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_expandTimer.timerId())
    {
        //duplicates functionality from QTreeView, but won't collapse an already expanded item
        QPoint pos = this->viewport()->mapFromGlobal(QCursor::pos());
        if (state() == QAbstractItemView::DraggingState && this->rect().contains(pos))
        {
            QModelIndex index = indexAt(pos);
            if (!isExpanded(index))
            {
                setExpanded(index, true);
            }
        }
        m_expandTimer.stop();
    }

    QTreeView::timerEvent(event);
}

void OutlinerTreeView::processQueuedMousePressedEvent(QMouseEvent* event)
{
    //interpret the mouse event as a button press
    QMouseEvent mousePressedEvent(
        QEvent::MouseButtonPress,
        event->localPos(),
        event->windowPos(),
        event->screenPos(),
        event->button(),
        event->buttons(),
        event->modifiers(),
        event->source());
    QTreeView::mousePressEvent(&mousePressedEvent);
}

void OutlinerTreeView::startCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions)
{
    m_draggingUnselectedItem = true;

    //sort by container entity depth and order in hierarchy for proper drag image and drop order
    QModelIndexList indexListSorted = indexList;
    AZStd::unordered_map<AZ::EntityId, AZStd::list<AZ::u64>> locations;
    for (auto index : indexListSorted)
    {
        AZ::EntityId entityId(index.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
        AzToolsFramework::GetEntityLocationInHierarchy(entityId, locations[entityId]);
    }
    AZStd::sort(indexListSorted.begin(), indexListSorted.end(), [&locations](const QModelIndex& index1, const QModelIndex& index2) {
        AZ::EntityId e1(index1.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
        AZ::EntityId e2(index2.data(OutlinerListModel::EntityIdRole).value<AZ::u64>());
        const auto& locationsE1 = locations[e1];
        const auto& locationsE2 = locations[e2];
        return AZStd::lexicographical_compare(locationsE1.begin(), locationsE1.end(), locationsE2.begin(), locationsE2.end());
    });

    //get the data for the unselected item(s)
    QMimeData* data = model()->mimeData(indexListSorted);
    if (data)
    {
        //initiate drag/drop for the item
        QDrag* drag = new QDrag(this);
        drag->setPixmap(QPixmap::fromImage(createDragImage(indexListSorted)));
        drag->setMimeData(data);
        Qt::DropAction defDropAction = Qt::IgnoreAction;
        if (defaultDropAction() != Qt::IgnoreAction && (supportedActions & defaultDropAction()))
        {
            defDropAction = defaultDropAction();
        }
        else if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
        {
            defDropAction = Qt::CopyAction;
        }
        drag->exec(supportedActions, defDropAction);
    }
}

QImage OutlinerTreeView::createDragImage(const QModelIndexList& indexList)
{
    //generate a drag image of the item icon and text, normally done internally, and inaccessible 
    QRect rect(0, 0, 0, 0);
    for (auto index : indexList)
    {
        if (index.column() != 0)
        {
            continue;
        }
        QRect itemRect = visualRect(index);
        rect.setHeight(rect.height() + itemRect.height());
        rect.setWidth(AZStd::GetMax(rect.width(), itemRect.width()));
    }

    QImage dragImage(rect.size(), QImage::Format_ARGB32_Premultiplied);

    QPainter dragPainter(&dragImage);
    dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
    dragPainter.fillRect(dragImage.rect(), Qt::transparent);
    dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    dragPainter.setOpacity(0.35f);
    dragPainter.fillRect(rect, QColor("#222222"));
    dragPainter.setOpacity(1.0f);

    int imageY = 0;
    for (auto index : indexList)
    {
        if (index.column() != 0)
        {
            continue;
        }

        QRect itemRect = visualRect(index);
        dragPainter.drawPixmap(QPoint(0, imageY),
            model()->data(index, Qt::DecorationRole).value<QIcon>().pixmap(QSize(16, 16)));
        dragPainter.setPen(
            model()->data(index, Qt::ForegroundRole).value<QBrush>().color());
        dragPainter.setFont(
            font());
        dragPainter.drawText(QRect(20, imageY, rect.width() - 20, rect.height()),
            model()->data(index, Qt::DisplayRole).value<QString>());
        imageY += itemRect.height();
    }

    dragPainter.end();
    return dragImage;
}

#include <UI/Outliner/OutlinerTreeView.moc>

