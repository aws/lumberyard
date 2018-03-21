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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <QDialog>

class DeepFilterProxyModel;
class QLabel;
class QLineEdit;
class QModelIndex;
class QStandardItemModel;
class QString;
class QTreeView;
class QWidget;
struct IDefaultSkeleton;

class JointSelectionDialog
    : public QDialog
{
    Q_OBJECT
public:
    JointSelectionDialog(QWidget* parent);
    QSize sizeHint() const override;

    bool chooseJoint(QString& name, IDefaultSkeleton* skeleton);
protected slots:
    void onActivated(const QModelIndex& index);
    void onFilterChanged(const QString&);
protected:
    bool eventFilter(QObject* obj, QEvent* event);
private:
    QTreeView* m_tree;
    QStandardItemModel* m_model;
    DeepFilterProxyModel* m_filterModel;
    QLineEdit* m_filterEdit;
    QLabel* m_skeletonLabel;
};

