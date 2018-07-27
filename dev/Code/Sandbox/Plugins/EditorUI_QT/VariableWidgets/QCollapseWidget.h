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
#ifndef QCOLLAPSEWIDGET_H
#define QCOLLAPSEWIDGET_H

#include <QWidget>
#include "QMenu"
#include "QCopyableWidget.h"

class QVBoxLayout;
class QHBoxLayout;
class CAttributeItem;

namespace Ui {
    class QCollapseWidget;
}

class QCollapseWidget
    : public QWidget
{
    Q_OBJECT

public:
    explicit QCollapseWidget(QWidget* parent = 0, CAttributeItem* attributeItem = nullptr);
    virtual ~QCollapseWidget();

    void addMainWidget(QCopyableWidget* widget);
    void addChildWidget(QCopyableWidget* widget);
    void setCollapsed(bool state);
    QVBoxLayout* getChildLayout();
    QHBoxLayout* getMainLayout();
private slots:
    void on_collapseButton_clicked();

private:
    Ui::QCollapseWidget* ui;
    CAttributeItem* m_attributeItem;
};

#endif // QCOLLAPSEWIDGET_H
