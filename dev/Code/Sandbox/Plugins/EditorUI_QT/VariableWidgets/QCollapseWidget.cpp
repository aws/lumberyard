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
#include "stdafx.h"
#include "QCollapseWidget.h"
#include <VariableWidgets/ui_qcollapsewidget.h>
#include <QLabel>
#include "../ContextMenu.h"
#ifdef EDITOR_QT_UI_EXPORTS
    #include "AttributeItem.h"
    #include "AttributeView.h"
    #include <VariableWidgets/QCollapseWidget.moc>
#endif

#define STATE_COLLAPSED "collapsed"
#define STATE_UNCOLLAPSED "uncollapsed"

QCollapseWidget::QCollapseWidget(QWidget* parent, CAttributeItem* attributeItem)
    : QWidget(parent)
    , ui(new Ui::QCollapseWidget)
    , m_attributeItem(attributeItem)
{
    ui->setupUi(this);

    #ifdef EDITOR_QT_UI_EXPORTS
    if (m_attributeItem)
    {
        const QString path = m_attributeItem->getAttributePath(true);
        setCollapsed(m_attributeItem->getAttributeView()->getValue(path, STATE_COLLAPSED) == STATE_COLLAPSED);
    }
    #else
    setCollapsed(true);
#endif
    //disables tab focusing
    ui->collapseButton->setFocusPolicy(Qt::ClickFocus);
}

QCollapseWidget::~QCollapseWidget()
{
    delete ui;
}

void QCollapseWidget::addMainWidget(QCopyableWidget* widget)
{
    ui->layoutMainWidget->addWidget(widget);
}

void QCollapseWidget::addChildWidget(QCopyableWidget* widget)
{
    ui->layoutChildren->addWidget(widget);
}

QVBoxLayout* QCollapseWidget::getChildLayout()
{
    return ui->layoutChildren;
}

QHBoxLayout* QCollapseWidget::getMainLayout()
{
    return ui->layoutMainWidget;
}


void QCollapseWidget::setCollapsed(bool state)
{
    QIcon icon;
    if (state)
    {
        ui->widgetChildren->setHidden(true);
    }
    else
    {
        ui->widgetChildren->setHidden(false);
    }

    // make sure the parent widget knows that we have changed the geometry.

    ui->collapseButton->setProperty("collapsed", state);
    ui->collapseButton->style()->unpolish(ui->collapseButton);
    ui->collapseButton->style()->polish(ui->collapseButton);
    ui->collapseButton->update();
}

void QCollapseWidget::on_collapseButton_clicked()
{
    // Toggle

    const bool collapsed = !ui->widgetChildren->isHidden();

    setCollapsed(collapsed);

    #ifdef EDITOR_QT_UI_EXPORTS
    // Save state
    if (m_attributeItem)
    {
        const QString path = m_attributeItem->getAttributePath(true);
        m_attributeItem->getAttributeView()->setValue(path, collapsed ? STATE_COLLAPSED : STATE_UNCOLLAPSED);
    }
    #endif
}
