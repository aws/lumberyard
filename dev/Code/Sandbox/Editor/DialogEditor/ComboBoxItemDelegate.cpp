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
#include "ComboBoxItemDelegate.h"
#include "DialogManager.h"

#include <QComboBox>

ComboBoxItemDelegate::ComboBoxItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* ComboBoxItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QComboBox* comboBox = createComboBox(parent);
    connect(comboBox, SIGNAL(activated(int)), this, SLOT(dismissComboBox()));
    return comboBox;
}

void ComboBoxItemDelegate::dismissComboBox()
{
    auto widget = qobject_cast<QWidget*>(sender());
    if (widget)
    {
        emit commitData(widget);
        emit closeEditor(widget);
    }
}

void ComboBoxItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
    {
        // get the index of the text in the combobox that matches the current value of the itenm
        QString currentText = index.data(Qt::EditRole).toString();
        int cbIndex = cb->findText(currentText);
        // if it is valid, adjust the combobox
        if (cbIndex >= 0)
        {
            cb->setCurrentIndex(cbIndex);
        }

        cb->showPopup();
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void ComboBoxItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
    {
        model->setData(index, cb->currentIndex(), Qt::EditRole);
    }
    else
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

ActorComboBoxItemDelegate::ActorComboBoxItemDelegate(QObject* parent)
    : ComboBoxItemDelegate(parent)
{
}

QComboBox* ActorComboBoxItemDelegate::createComboBox(QWidget* parent) const
{
    static QStringList items;
    if (items.isEmpty())
    {
        for (int i = 0; i < 8; ++i)
        {
            items.push_back(tr("Actor %1").arg(i + 1));
        }
    }

    QComboBox* combo = new QComboBox(parent);
    combo->addItems(items);
    return combo;
}

LookAtComboBoxItemDelegate::LookAtComboBoxItemDelegate(QObject* parent)
    : ComboBoxItemDelegate(parent)
{
}

QComboBox* LookAtComboBoxItemDelegate::createComboBox(QWidget* parent) const
{
    QComboBox* combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("---"), CEditorDialogScript::NO_ACTOR_ID);
    combo->addItem(QStringLiteral("#RESET#"), CEditorDialogScript::STICKY_LOOKAT_RESET_ID);
    for (int i = 0; i < 8; ++i)
    {
        combo->addItem(tr("Actor%1").arg(i + 1), i);
    }
    return combo;
}

void LookAtComboBoxItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
    {
        model->setData(index, cb->currentData(), Qt::EditRole);
    }
    else
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

TypeComboBoxItemDelegate::TypeComboBoxItemDelegate(QObject* parent)
    : ComboBoxItemDelegate(parent)
{
}

QComboBox* TypeComboBoxItemDelegate::createComboBox(QWidget* parent) const
{
    QComboBox* combo = new QComboBox(parent);
    combo->addItems({ "Action", "Signal" });
    return combo;
}

#include <DialogEditor/ComboBoxItemDelegate.moc>
