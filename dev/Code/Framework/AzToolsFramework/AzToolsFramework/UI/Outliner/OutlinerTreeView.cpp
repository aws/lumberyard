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

#include <QDrag>
#include <QPainter>
#include <QHeaderView>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

namespace AzToolsFramework
{
    OutlinerTreeView::OutlinerTreeView(QWidget* pParent)
        : QTreeView(pParent)
        , m_mousePressedQueued(false)
        , m_mousePressedPos()
    {
        setUniformRowHeights(true);
        setHeaderHidden(true);
    }

    OutlinerTreeView::~OutlinerTreeView()
    {
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
        if(index.isValid() && index.column() == 0 && !selectionModel()->isSelected(index))
        {
            startDragForUnselectedItem(index, supportedActions);
        }
        else
        {
            QTreeView::startDrag(supportedActions);
        }
    }

    void OutlinerTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        QTreeView::drawBranches(painter, rect, index);

        painter->save();

        painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);

        // Model index used to walk up the tree to get information about ancestors
        // for determining where to draw branch lines and when to draw connecting lines
        // between parents and uncles.
        QModelIndex ancestorIndex = index.parent();

        // Compute the depth of the current entity in the hierarchy to determine where to draw the branch lines
        int depth = 1;
        while (ancestorIndex.isValid())
        {
            ancestorIndex = ancestorIndex.parent();
            ++depth;
        }

        // These two flags are used to determine how the hierarchy lines are drawn for each entry.
        // When an object has children, the horizontal line needs to be shorter to accomodate the drop-down icon.
        bool hasChildren = index.child(0, 0).isValid();
        // When an object is the first of it's siblings to be displayed, the vertical line associated with it needs to be drawn shorter
        // as to not eclipse its parent.
        bool isFirst = index.row() == 0;

        if (depth > 1)
        {
            // The pen we're going to use the draw the branch line
            // Pen color and style are dependent on the type of the entity and its parent.
            QPen pen;

            // Determine the color of the pen:
            // If the current entity is not part of a slice, it's line is always white.
            // If the current entity is part of a slice, the line color is determined by
            // it's parent type.
            OutlinerListModel::EntryType type = OutlinerListModel::EntityType;
            auto typeVariant = index.data(OutlinerListModel::EntityTypeRole);
            type = OutlinerListModel::EntryType(typeVariant.value<int>());

            if (type == OutlinerListModel::EntryType::EntityType)
            {
                QVariant data = index.data(Qt::ForegroundRole);
                if (data.canConvert<QBrush>())
                {
                    QBrush brush = data.value<QBrush>();
                    pen.setColor(brush.color());
                    pen.setWidthF(2.5f);
                    painter->setPen(pen);
                }
            }
            else
            {
                // Determine the color of the pen to use to draw the item
                // If no determination can be made, use the existing, style-defined pen
                auto parentNode = index.parent();


                QVariant data = parentNode.data(Qt::ForegroundRole);
                if (data.canConvert<QBrush>())
                {
                    QBrush brush = data.value<QBrush>();
                    pen.setColor(brush.color());
                    pen.setWidthF(2.5f);
                    painter->setPen(pen);
                }
            }

            style()->standardPalette().background();

            QRect rect = visualRect(index);

            int totalIndent = indentation() * (depth - 1) - (indentation() / 2) - 2;

            // If an entity has children, then there will be a drop-arrow drawn to the left of the entity name.
            // So we only draw the left half the horizontal portion of the branch line to accomodate it.
            if (hasChildren)
            {
                painter->drawLine(totalIndent + 1, rect.y() + (rect.height() / 2), totalIndent + (indentation() / 2), rect.y() + (rect.height() / 2));
            }
            else
            {
                painter->drawLine(totalIndent + 1, rect.y() + (rect.height() / 2), totalIndent + (indentation()), rect.y() + (rect.height() / 2));
            }

            // When an entity is the very first sibling at it's level, we only draw the bottom half of the vertical portion of it's branch line.
            if (isFirst)
            {
                painter->drawLine(totalIndent, rect.y(), totalIndent, rect.y() + (rect.height() / 2));
            }
            else
            {
                painter->drawLine(totalIndent, rect.y() - (rect.height() / 2), totalIndent, rect.y() + (rect.height() / 2));
            }

            // Now we draw vertical lines connecting all our our ancestors and their siblings.
            // We can improve performance by pre-computing and caching the depth information
            // required here.
            ancestorIndex = index.parent();
            while (ancestorIndex.parent().isValid())
            {
                // Depth determines where to draw the vertical lines.
                --depth;

                auto uncleIndex = ancestorIndex.sibling(ancestorIndex.row() + 1, ancestorIndex.column());
                // If that parent has a sibling, draw a line to them!
                if (uncleIndex.isValid())
                {
                    // Get the color used to draw this branch.
                    // If the sibling isn't a slice entity, we need to draw it using the
                    // parent's brush.
                    QPen pen;
                    pen.setWidthF(2.5);

                    // We can use the color of the foreground brush associated with the appropriate object to draw our lines.
                    auto uncleType = OutlinerListModel::EntryType(uncleIndex.data(OutlinerListModel::EntityTypeRole).value<int>());
                    if (uncleType == OutlinerListModel::EntityType)
                    {
                        QVariant brush = uncleIndex.data(Qt::ForegroundRole);
                        if (brush.canConvert<QBrush>())
                        {
                            pen.setColor(brush.value<QBrush>().color());
                        }
                    }
                    else
                    {
                        QVariant brush = ancestorIndex.parent().data(Qt::ForegroundRole);
                        if (brush.canConvert<QBrush>())
                        {
                            pen.setColor(brush.value<QBrush>().color());
                        }
                    }

                    painter->setPen(pen);

                    totalIndent = indentation() * (depth - 1) - (indentation() / 2) - 2;
                    // Vertical Line
                    painter->drawLine(totalIndent, rect.y() - (rect.height() / 2), totalIndent, rect.y() + (rect.height() / 2));
                }

                // Move onto the next ancestor
                ancestorIndex = ancestorIndex.parent();
            }
        }

        painter->restore();
    }

    bool OutlinerTreeView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
    {
        if (index.isValid() && index.column() == OutlinerListModel::ColumnName)
        {
            QModelIndex nameColumnIndex = index.sibling(index.row(), OutlinerListModel::ColumnName);
            if (nameColumnIndex.isValid())
            {
                return QAbstractItemView::edit(nameColumnIndex, trigger, event);
            }
        }

        return QTreeView::edit(index, trigger, event);
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

    void OutlinerTreeView::startDragForUnselectedItem(const QModelIndex& index, Qt::DropActions supportedActions)
    {
        //get the data for the unselected item(s)
        QMimeData* data = model()->mimeData({ index });
        if (data)
        {
            QRect rect = visualRect(index);

            //initiate drag/drop for the item
            QDrag* drag = new QDrag(this);
            drag->setPixmap(QPixmap::fromImage(createDragImageForUnselectedItem(index)));
            drag->setMimeData(data);
            drag->setHotSpot(m_mousePressedPos - rect.topLeft());
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

    QImage OutlinerTreeView::createDragImageForUnselectedItem(const QModelIndex& index)
    {
        //generate a drag image of the item icon and text, normally done internally, and inaccessible 
        QRect rect = visualRect(index);
        QImage dragImage(rect.size(), QImage::Format_ARGB32_Premultiplied);

        QPainter dragPainter(&dragImage);

        dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
        dragPainter.fillRect(dragImage.rect(), Qt::transparent);

        dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        dragPainter.drawPixmap(QPoint(0, 0),
            model()->data(index, Qt::DecorationRole).value<QIcon>().pixmap(QSize(16, 16)));
        dragPainter.setPen(
            model()->data(index, Qt::ForegroundRole).value<QBrush>().color());
        dragPainter.setFont(
            font());
        dragPainter.drawText(QRect(20, 0, rect.width() - 20, rect.height()),
            model()->data(index, Qt::DisplayRole).value<QString>());

        dragPainter.end();
        return dragImage;
    }
}

#include <UI/Outliner/OutlinerTreeView.moc>

