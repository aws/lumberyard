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

#include <QFrame>

class QPushButton;
class QHBoxLayout;
class QLabel;
class QTableView;

class IStackStatusModel;

class StackStatusWidget
    : public QFrame
{
    Q_OBJECT

public:

    StackStatusWidget(QSharedPointer<IStackStatusModel> stackStatusModel, QWidget* parent);

private:

    QSharedPointer<IStackStatusModel> m_stackStatusModel;

    QLabel* m_titleLabel;
    QLabel* m_refreshingLabel;
    QTableView* m_statusTable;

    void CreateUI();
    void UpdateStatusTableSize();
    void OnRefreshStatusChanged(bool refreshing);
};
