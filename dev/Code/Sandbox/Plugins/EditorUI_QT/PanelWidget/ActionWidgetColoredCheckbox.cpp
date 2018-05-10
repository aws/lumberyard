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
#include <stdafx.h>
#include "ActionWidgetColoredCheckbox.h"
#include <PanelWidget/ui_ActionWidgetColoredCheckbox.h>
#include <PanelWidget/ActionWidgetColoredCheckbox.moc>

#include <QMenu>

ActionWidgetColoredCheckbox::ActionWidgetColoredCheckbox(QWidget* parent, ActionWidgetColoredCheckboxAction* action)
    : QWidget(parent)
    , ui(new Ui::ActionWidgetColoredCheckbox)
{
    setMouseTracking(true);
    ui->setupUi(this);

    ui->chkContextMenuCheckbock->setText(action->getCaption());
    m_colored = action->getColored();
    if (m_colored)
    {
        QPalette palette = ui->chkContextMenuCheckbock->palette();
        palette.setColor(QPalette::WindowText, QColor(0xff, 0x80, 0x00, 255));
        ui->chkContextMenuCheckbock->setPalette(palette);
    }
    ui->chkContextMenuCheckbock->setChecked(action->getChecked());

    //Trigger the menu action when the checkbox is clicked
    connect(ui->chkContextMenuCheckbock, &QCheckBox::clicked, action, [action]()
        {
            action->trigger();
        }
        );
}

ActionWidgetColoredCheckbox::~ActionWidgetColoredCheckbox()
{
    delete ui;
}

bool ActionWidgetColoredCheckbox::event(QEvent* ev)
{
    if (ev->type() == QEvent::Enter)
    {
        setFocus();
        return true;
    }

    return QWidget::event(ev);
}

void ActionWidgetColoredCheckbox::receiveFocusFromMenu()
{
    ui->chkContextMenuCheckbock->setFocus();
}

void ActionWidgetColoredCheckbox::focusInEvent(QFocusEvent* event)
{
    QPalette checkboxPalette = ui->chkContextMenuCheckbock->palette();
    if (m_colored)
    {
        checkboxPalette.setColor(QPalette::WindowText, QColor(0, 0, 0, 255));
    }
    else
    {
        checkboxPalette.setColor(QPalette::WindowText, QColor(0xff, 0xff, 0xff, 255));
    }
    ui->chkContextMenuCheckbock->setPalette(checkboxPalette);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0xff, 0x99, 0, 255));
    setPalette(pal);
    setAutoFillBackground(true);
    return QWidget::focusInEvent(event);
}
void ActionWidgetColoredCheckbox::focusOutEvent(QFocusEvent* event)
{
    QPalette checkboxPalette = ui->chkContextMenuCheckbock->palette();
    if (m_colored)
    {
        checkboxPalette.setColor(QPalette::WindowText, QColor(0xff, 0x80, 0x00, 255));
    }
    else
    {
        checkboxPalette.setColor(QPalette::WindowText, QColor(0xc0, 0xc0, 0xc0, 255));
    }
    ui->chkContextMenuCheckbock->setPalette(checkboxPalette);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0, 0, 0, 255));
    setPalette(pal);
    setAutoFillBackground(false);
    return QWidget::focusInEvent(event);
}

ActionWidgetColoredCheckboxAction::ActionWidgetColoredCheckboxAction(QWidget* parent)
    : QWidgetAction(parent)
    , m_caption("No caption set")
    , m_colored(false)
    , m_checked(false)
{
}

QWidget* ActionWidgetColoredCheckboxAction::createWidget(QWidget* parent)
{
    ActionWidgetColoredCheckbox* w = new ActionWidgetColoredCheckbox(parent, this);
    w->show();

    connect(this, &QAction::hovered, w, [w]{w->setFocus(); }); //When the action is highlighted (by mouse or keyboard) set focus to our widget.
    return w;
}

void ActionWidgetColoredCheckboxAction::deleteWidget(QWidget* widget)
{
    QWidgetAction::deleteWidget(widget);
}
