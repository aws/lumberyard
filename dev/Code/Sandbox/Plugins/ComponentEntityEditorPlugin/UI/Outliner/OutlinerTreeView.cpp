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

void OutlinerTreeView::mousePressEvent(QMouseEvent* /*event*/)
{
    //postponing normal mouse pressed logic until mouse is released or dragged
    //this means selection occurs on mouse released now
    //this is to support drag/drop of non-selected items
    m_mousePressedQueued = true;
}

void OutlinerTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_mousePressedQueued)
    {
        //treat this as a mouse pressed event to process selection etc
        processQueuedMousePressedEvent(event);
    }

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
    //cancel pending mouse press, which should be done already
    m_mousePressedQueued = false;

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

void OutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
    QTreeView::drawBranches(painter, rect, index);

    if (!index.parent().isValid())
    {
        return;
    }

    // determine the color of the pen based on entity type:
    // if the current entity is not part of a slice, it's line is always white.
    // if the current entity is part of a slice, the line color is determined by it's parent type.
    auto type = OutlinerListModel::EntryType(index.data(OutlinerListModel::EntityTypeRole).value<int>());
    bool isSliceEntity = (type != OutlinerListModel::EntityType);

    if (!isSliceEntity)
    {
        bool isDescendantOfSlice = false;
        for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
        {
            auto type = OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
            if (type != OutlinerListModel::EntityType)
            {
                isDescendantOfSlice = true;
                break;
            }
        }

        if (!isDescendantOfSlice)
        {
            // Don't draw the lines for entities, only for slices or entities that are children of slices
            return;

        }
    }

    painter->save();

    painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);

    static const QColor sliceEntityColor(GetIEditor()->GetColorByName("SliceEntityColor"));
    static const QColor sliceOverrideColor(GetIEditor()->GetColorByName("SliceOverrideColor"));

    // The pen we're going to use the draw the branch line
    // Pen color and style are dependent on the type of the entity and its parent.
    QPen pen;
    pen.setWidthF(2.5f);

    auto foregroundIndex = type != OutlinerListModel::EntryType::EntityType ? index.parent() : index;
    QVariant data = foregroundIndex.data(Qt::ForegroundRole);
    if (data.canConvert<QBrush>())
    {
        auto targetColor = data.value<QBrush>().color();
        if (isSliceEntity)
        {
            bool hasOverrides = index.data(OutlinerListModel::SliceEntityOverrideRole).value<bool>();
            targetColor = (hasOverrides ? sliceOverrideColor : sliceEntityColor);
        }
        pen.setColor(targetColor);
        painter->setPen(pen);
    }

    // draw a horizontal line from the parent branch to the item
    // if the item has children offset the drawn line to compensate for drawn expander buttons
    bool hasChildren = index.child(0, 0).isValid();
    int rectHalfHeight = rect.height() / 2;
    int horizontalLineY = rect.top() + rectHalfHeight;
    int horizontalLineLeft = rect.right() - indentation() * 1.5f;
    int horizontalLineRight = hasChildren ? (rect.right() - indentation()) : (rect.right() - indentation() * 0.5f);
    painter->drawLine(horizontalLineLeft, horizontalLineY, horizontalLineRight, horizontalLineY);

    // draw a vertical line segment connecting parent to child and child to child
    // if this is the last item, only draw half the line to terminate the segment
    bool hasNext = index.sibling(index.row() + 1, index.column()).isValid();
    int verticalLineX = rect.right() - indentation() * 1.5f;
    int verticalLineTop = rect.top();
    int verticalLineBottom = hasNext ? rect.bottom() : rect.bottom() - rectHalfHeight;
    painter->drawLine(verticalLineX, verticalLineTop, verticalLineX, verticalLineBottom);

    // for all ancestors with immediate/next siblings, draw vertical line segments offset from the current item
    // each individual segment will align to form connections between non-immediate parents and siblings

    // offset the line horizontally to account for parent depth
    float depthMultiplier = 2.5f;
    for (QModelIndex ancestorIndex = index.parent(); ancestorIndex.isValid(); ancestorIndex = ancestorIndex.parent())
    {
        // If that parent has a sibling, draw a line to them!
        auto uncleIndex = ancestorIndex.sibling(ancestorIndex.row() + 1, ancestorIndex.column());
        if (uncleIndex.isValid())
        {
            auto ancestorType = OutlinerListModel::EntryType(ancestorIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
            bool ancestorIsSliceEntity = (ancestorType != OutlinerListModel::EntryType::EntityType);

            auto uncleType = OutlinerListModel::EntryType(uncleIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
            bool uncleIsSliceEntity = (uncleType != OutlinerListModel::EntryType::EntityType);

            if (ancestorIsSliceEntity && uncleIsSliceEntity)
            {
                if (ancestorIndex.data(OutlinerListModel::SliceEntityOverrideRole).value<bool>())
                {
                    pen.setColor(sliceOverrideColor);
                }
                else
                {
                    pen.setColor(sliceEntityColor);
                }
                painter->setPen(pen);
            }
            else if (uncleIsSliceEntity)
            {
                auto parent = uncleIndex.parent();
                QVariant data = parent.data(Qt::ForegroundRole);
                if (data.canConvert<QBrush>())
                {
                    auto type = parent.data(OutlinerListModel::EntityTypeRole).value<int>();
                    if (type != OutlinerListModel::EntryType::EntityType)
                    {
                        bool parentHasOverrides = parent.data(OutlinerListModel::SliceEntityOverrideRole).value<bool>();
                        pen.setColor(parentHasOverrides ? sliceOverrideColor : sliceEntityColor);
                    }
                    else
                    {
                        pen.setColor(data.value<QBrush>().color());
                    }

                    painter->setPen(pen);
                }
            }
            else
            {
                QVariant data = uncleIndex.data(Qt::ForegroundRole);
                if (data.canConvert<QBrush>())
                {
                    pen.setColor(data.value<QBrush>().color());
                    painter->setPen(pen);
                }
            }

            // draw vertical line segment
            verticalLineX = rect.right() - indentation() * depthMultiplier;
            painter->drawLine(verticalLineX, rect.top(), verticalLineX, rect.bottom());
        }

        depthMultiplier += 1.0f;
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
    m_mousePressedQueued = false;
    m_mousePressedPos = event->pos();
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

