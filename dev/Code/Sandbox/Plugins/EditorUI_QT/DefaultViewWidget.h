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
#include "api.h"

#include <QWidget>
#include <QVector>
#include <functional>

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QSpacerItem;
class QAction;
class QMouseEvent;

class EDITOR_QT_UI_API DefaultViewWidget
    : public QWidget
{
    Q_OBJECT
public:
    DefaultViewWidget(QWidget* parent);
    ~DefaultViewWidget();

    void mouseReleaseEvent(QMouseEvent* mouseEvent);

    void SetSpaceAtTop(int height);
    void SetSpaceBetweenImageAndLabel(int height);
    void SetSpaceBetweenLabelsAndButtons(int height);

    //labels and images can be shown in any order
    void SetLabel(QString text);
    void SetImage(QString path, QSize size);

    //they are shown below the last image/label in a single row
    void AddButton(QString text, std::function<void()> onPressed);
    void AddButton(QString text, QAction* action);
    static const unsigned int STANDARD_TOP_SPACING = 100;
signals:
    void SignalLeftClicked(QPoint pos);
    void SignalRightClicked(QPoint pos);

private:
    virtual void showEvent(QShowEvent* e);
    void Invalidate();
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QVector<QPushButton*> m_buttons;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QLabel* m_label;
    QLabel* m_image;
    QVBoxLayout* m_layout;
    QHBoxLayout* m_buttonLayout;
    QSpacerItem* m_topSpace;
    QSpacerItem* m_spaceBetweenLabelAndImage;
    QSpacerItem* m_spaceBetweenLabelAndButtons;
    QSpacerItem* m_bottomStretch;
};
