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
#ifndef QCOLLAPSEPANEL_H
#define QCOLLAPSEPANEL_H

#include <QWidget>
#include <QObjectUserData>
#include "QCopyableWidget.h"

namespace Ui {
    class QCollapsePanel;
}

class QVBoxLayout;
class CAttributeItem;
class QMenu;

class QCollapsePanel
    : public QCopyableWidget
    , public QObjectUserData
{
    Q_OBJECT

public:
    explicit QCollapsePanel(QWidget* parent = nullptr, CAttributeItem* attributeItem = nullptr);
    virtual  ~QCollapsePanel();

    void setCollapsed(bool state);
    bool isCollapsed() const;
    void addWidget(QCopyableWidget* widget);
    void setTitle(const QString& title);
    QVBoxLayout* getChildLayout();

private slots:
    void on_CaptionButton_clicked();

private:
    virtual void mousePressEvent(QMouseEvent* e);

private:
    Ui::QCollapsePanel* ui;
    CAttributeItem* m_attributeItem;
    QAction* m_optionalCollapseAction;
    QMenu* m_menu;
};

#endif // QCOLLAPSEPANEL_H
