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
#include "StdAfx.h"

#include "ColumnGroupItemDelegate.h"

#include <QPainter>

ColumnGroupItemDelegate::ColumnGroupItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QSize ColumnGroupItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    if (index.model()->hasChildren(index))
    {
        s.setHeight(s.height() * 2);
    }
    return s;
}

void ColumnGroupItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.model()->hasChildren(index))
    {
        if (index.column() > 0)
        {
            return;
        }
        painter->setClipping(false);
        painter->setPen(option.palette.text().color());
        if (option.state & QStyle::State_Selected)
        {
            painter->setPen(option.palette.highlightedText().color());
        }
        QRect textRect = option.rect;
        textRect.setRight(qobject_cast<QWidget*>(parent())->width());
        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(textRect, option.palette.highlight());
        }
        // there's just one text - somewhere in the model row, show it!
        QString content;
        const int columnCount = index.model()->columnCount(index.parent());
        for (int column = 0; column < columnCount; ++column)
        {
            content += index.sibling(index.row(), column).data().toString();
        }
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignBottom | Qt::ElideRight, content);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}
