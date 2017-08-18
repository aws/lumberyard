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
#ifndef COMBOBOX_ITEMDELEGATE_H
#define COMBOBOX_ITEMDELEGATE_H

#include <QStyledItemDelegate>

class QVariant;
class QComboBox;

class ComboBoxItemDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ComboBoxItemDelegate(QObject* parent = nullptr);

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
protected:
    virtual QComboBox* createComboBox(QWidget* parent) const = 0;
private Q_SLOTS:
    void dismissComboBox();
};

class ActorComboBoxItemDelegate
    : public ComboBoxItemDelegate
{
    Q_OBJECT
public:
    explicit ActorComboBoxItemDelegate(QObject* parent = nullptr);
protected:
    QComboBox* createComboBox(QWidget* parent) const override;
};

class LookAtComboBoxItemDelegate
    : public ComboBoxItemDelegate
{
    Q_OBJECT
public:
    explicit LookAtComboBoxItemDelegate(QObject* parent = nullptr);
protected:
    QComboBox* createComboBox(QWidget* parent) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};

class TypeComboBoxItemDelegate
    : public ComboBoxItemDelegate
{
    Q_OBJECT
public:
    explicit TypeComboBoxItemDelegate(QObject* parent = nullptr);
protected:
    QComboBox* createComboBox(QWidget* parent) const override;
};

class LineEditWithDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LineEditWithDelegate(QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
Q_SIGNALS:
    void buttonClicked(int row);
};

#endif
