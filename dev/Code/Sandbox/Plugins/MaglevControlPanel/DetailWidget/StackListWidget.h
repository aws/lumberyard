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
class QLabel;
class MaximumSizedTableView;
class ButtonBarWidget;
class HeadingWidget;
class StackStatusWidget;

class IStackStatusListModel;

class StackListWidget
    : public QFrame
{
public:

    StackListWidget(QSharedPointer<IStackStatusListModel> stackStatusListModel, QWidget* parent);

    void AddButton(QPushButton* button);

protected:

    QSharedPointer<IStackStatusListModel> m_stackStatusListModel;

    QLabel* m_refreshingLabel;
    ButtonBarWidget* m_buttonBarWidget;
    MaximumSizedTableView* m_listTable;

    void CreateUI();
    void OnRefreshStatusChanged(bool refreshing);
};
