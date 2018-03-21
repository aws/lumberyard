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
#include "QPresetWidget.h"
#include <QLabel>
#include <QPushButton>
#include <qcoreevent.h>
#include <QEvent>
#include <QBoxLayout>


#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QPresetWidget.moc>
#endif
#include "IEditorParticleUtils.h"


QPresetWidget::QPresetWidget(QString _name, QWidget* _value, QWidget* parent)
    : QWidget(parent)
    , value(_value)
{
    name = new QLabel(this);
    name->setText(_name);
    background = new QWidget(this);
    value->installEventFilter(this);
    SetBackgroundStyle(tr("background-image:url(:/particleQT/icons/color_btn_bkgn.png); border: none;"));
    layout = new QGridLayout(this);
    layout->addWidget(background, 0, 0);
    layout->addWidget(value, 0, 0);
    layout->addWidget(name, 0, 1);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    m_tooltip = new QToolTipWidget(this);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QPresetWidget::~QPresetWidget()
{
    if (name)
    {
        delete name;
    }
    if (background)
    {
        delete background;
    }
    if (value)
    {
        delete value;
    }
}

bool QPresetWidget::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == value)
    {
        if (e->type() == QEvent::ToolTip)
        {
            QHelpEvent* event = (QHelpEvent*)e;

            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preset Library", "Preset", GetName());

            m_tooltip->TryDisplay(event->globalPos(), this, QToolTipWidget::ArrowDirection::ARROW_LEFT);

            return true;
        }
        if (e->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
        if (e->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                if ((bool)callback_on_click)
                {
                    callback_on_click(name->text());
                }
            }
            if (mouseEvent->button() == Qt::RightButton)
            {
                if ((bool)callback_on_right_click)
                {
                    callback_on_right_click(name->text());
                }
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

void QPresetWidget::SetBackgroundStyle(QString ss)
{
    background->setStyleSheet(ss);
}

void QPresetWidget::SetLabelStyle(QString ss)
{
    name->setStyleSheet(ss);
}

void QPresetWidget::SetForegroundStyle(QString ss)
{
    value->setStyleSheet(ss);
}

void QPresetWidget::SetOnRightClickCallback(std::function<void(QString)> callback)
{
    callback_on_right_click = callback;
}

void QPresetWidget::SetOnClickCallback(std::function<void(QString)> callback)
{
    callback_on_click = callback;
}

QWidget* QPresetWidget::GetWidget()
{
    return value;
}

void QPresetWidget::setPresetSize(int w, int h)
{
    value->setFixedSize(w, h);
    value->setMinimumSize(w, h);
    value->setMaximumSize(w, h);
    background->setFixedSize(w, h);
    background->setMinimumSize(w, h);
    background->setMaximumSize(w, h);
}

void QPresetWidget::setLabelWidth(int w)
{
    name->setFixedWidth(w);
}

void QPresetWidget::setToolTip(QString tip)
{
    value->setToolTip(tip);
}

void QPresetWidget::SetName(QString value)
{
    name->setText(value);
}

QString QPresetWidget::GetName() const
{
    return name->text();
}

void QPresetWidget::ShowGrid()
{
    while (layout->count() > 0)
    {
        layout->takeAt(0);
    }

    layout->addWidget(background, 0, 0, 1, 1);
    layout->addWidget(value, 0, 0, 1, 1);

    background->stackUnder(value);
    update();
    activateWindow();

    show();
    background->show();
    value->show();
    name->hide();
}

void QPresetWidget::ShowList()
{
    while (layout->count() > 0)
    {
        layout->takeAt(0);
    }

    layout->addWidget(background, 0, 0, 1, 1);
    layout->addWidget(value, 0, 0, 1, 1);
    layout->addWidget(name, 0, 1, 1, 1);

    background->stackUnder(value);

    update();
    activateWindow();

    show();
    background->show();
    value->show();
    name->show();
}