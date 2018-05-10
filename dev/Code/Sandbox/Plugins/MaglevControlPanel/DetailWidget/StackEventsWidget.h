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

#include <IAWSResourceManager.h>

#include <QFrame>

class QVariant;
class QVBoxLayout;
class QTableView;
class QMovie;

class StackEventsWidget
    : public QFrame
{
    Q_OBJECT

public:

    StackEventsWidget(QSharedPointer<IStackEventsModel> stackEventsModel, QWidget* parent);

    void SetStackEventsModel(QSharedPointer<IStackEventsModel> stackEventsModel);

private:

    QSharedPointer<IStackEventsModel> m_stackEventsModel;

    QTableView* m_logTable;
    QWidget* m_statusInProgress;
    QWidget* m_statusSucceeded;
    QWidget* m_statusFailed;
    QMovie* m_inProgressMovie;

    QWidget* CreateTitleBar();

    void OnCommandStatusChanged(IStackEventsModel::CommandStatus commandStatus);
    void OnRowsInserted(const QModelIndex& parent, int first, int last);
    void OnCustomContextMenuRequested(const QPoint& pos);
};

