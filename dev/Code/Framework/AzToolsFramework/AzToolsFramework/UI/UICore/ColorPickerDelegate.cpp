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
#include <QtCore/QAbstractItemModel>
#include "ColorPickerDelegate.hxx"
#include <QtWidgets/QColorDialog>

namespace AzToolsFramework
{
    ColorPickerDelegate::ColorPickerDelegate(QObject* pParent)
        : QStyledItemDelegate(pParent)
    {
    }

    QWidget* ColorPickerDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        (void)index;
        (void)option;
        QColorDialog* ptrDialog = new QColorDialog(parent);
        ptrDialog->setWindowFlags(Qt::Tool);
        ptrDialog->setOptions(QColorDialog::NoButtons);
        return ptrDialog;
    }

    void ColorPickerDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        QColorDialog* colorEditor = qobject_cast<QColorDialog*>(editor);

        if (!editor)
        {
            return;
        }

        QVariant colorResult = index.data(COLOR_PICKER_ROLE);
        if (colorResult == QVariant())
        {
            return;
        }

        QColor pickedColor = qvariant_cast<QColor>(colorResult);

        colorEditor->setCurrentColor(pickedColor);
    }

    void ColorPickerDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        QColorDialog* colorEditor = qobject_cast<QColorDialog*>(editor);

        if (!editor)
        {
            return;
        }

        QVariant colorVariant = colorEditor->currentColor();
        model->setData(index, colorVariant, COLOR_PICKER_ROLE);
    }

    void ColorPickerDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        (void)index;
        QRect pickerpos = option.rect;

        pickerpos.setTopLeft(editor->parentWidget()->mapToGlobal(pickerpos.topLeft()));
        pickerpos.adjust(64, 0, 0, 0);
        editor->setGeometry(pickerpos);
    }
} // namespace AzToolsFramework

#include <UI/UICore/ColorPickerDelegate.moc>