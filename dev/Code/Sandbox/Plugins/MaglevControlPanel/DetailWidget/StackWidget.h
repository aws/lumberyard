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
class QCheckBox;
class ButtonBarWidget;
class IStackStatusModel;
class ResourceManagementView;
class StackStatusWidget;

class StackWidget
    : public QFrame
{
    Q_OBJECT

public:

    StackWidget(ResourceManagementView* view, QSharedPointer<IStackStatusModel> stackStatusModel, QWidget* parent);

    void AddButton(QPushButton* button);
    void AddCheckBox(QCheckBox* checkBox);
    void AddUpdateButton();
    void AddDeleteButton();

private:

    ResourceManagementView* m_view;
    QSharedPointer<IStackStatusModel> m_stackStatusModel;

    StackStatusWidget* m_statusWidget {
        nullptr
    };
    ButtonBarWidget* m_buttonBarWidget {
        nullptr
    };
    QPushButton* m_updateButton {
        nullptr
    };
    QPushButton* m_deleteButton {
        nullptr
    };

    void CreateUI();
    void UpdateUI();
    void UpdateStack();
    void DeleteStack();
};
