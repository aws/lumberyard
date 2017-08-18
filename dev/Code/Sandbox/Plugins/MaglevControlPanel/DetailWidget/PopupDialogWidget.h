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

#include <QDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class PopupDialogWidget
    : public QDialog
{
    Q_OBJECT

public:
    PopupDialogWidget(QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    void AddWidgetPairRow(QWidget* leftWidget, QWidget* rightWidget, int stretch = 0);
    void AddSpanningWidgetRow(QWidget* widget, int stretch = 0);
    void AddLeftOnlyWidgetRow(QWidget* widget, int stretch = 0);
    void AddRightOnlyWidgetRow(QWidget* widget, int stretch = 0);
    void AddStretchRow(int stretch = 1);

    QPushButton* GetPrimaryButton();
    QPushButton* GetCancelButton();
    QLabel* GetBottomLabel();

protected:

    virtual void moveEvent(QMoveEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;

    QLabel m_bottomLabel;
    QPushButton m_primaryButton;
    QPushButton m_cancelButton;
    QGridLayout m_gridLayout;
    int m_currentRowNum;

signals:

    void DialogMoved();
    void DialogResized();
};