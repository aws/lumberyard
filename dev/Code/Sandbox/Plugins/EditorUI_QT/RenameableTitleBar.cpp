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
#include "RenameableTitleBar.h"

//qt
#include <QLineEdit>
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QStyle>

#include "VariableWidgets/BaseVariableWidget.h"


RenameableTitleBar::RenameableTitleBar(QWidget* parent)
    : QWidget(parent)
    , m_ignoreEndRename(false)
{
    m_label = new QLabel("", this);
    m_editor = new QLineEdit(this);
    m_icon = new QWidget(this);
    m_collapseIcon = new QWidget(this);
    m_layout = new QHBoxLayout(this);

    connect(m_editor, &QLineEdit::editingFinished, this, &RenameableTitleBar::EndRename);

    m_icon->setObjectName("RenameableIcon");
    m_collapseIcon->setObjectName("RenameableCollapsIcon");
    m_label->setObjectName("RenameableTitle");
    setObjectName("RenameableTitleBar");

    m_layout->setAlignment(Qt::AlignLeft);
    m_layout->addWidget(m_collapseIcon);
    m_layout->addWidget(m_icon);
    m_layout->addWidget(m_label);
    m_layout->addWidget(m_editor);
    m_layout->addStretch(1);
    m_editor->hide();

    setLayout(m_layout);

    setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void RenameableTitleBar::SetEditorsPlaceholderText(const QString& text)
{
    m_editor->setPlaceholderText(text);
}

void RenameableTitleBar::SetTitle(const QString& text)
{
    //Prevent m_editor send another rename event
    QSignalBlocker blocker(m_editor);
    m_label->setText(text);
    m_editor->setText(text);
    m_editor->hide();
    m_label->show();
}

void RenameableTitleBar::BeginRename()
{
    m_label->hide();
    if (!m_label->text().isEmpty())
    {
        m_editor->setText(m_label->text());
    }
    m_editor->show();
    m_editor->setFocus();
    m_editor->selectAll();
}

void RenameableTitleBar::EndRename()
{
    //QLineEdit::editingFinished can be triggered again when popout a message box in EndRename so the line edit lost focus
    SelfCallFence(m_ignoreEndRename);

    bool validName = true;
    emit SignalTitleValidationCheck(m_editor->text(), validName);
    if (validName)
    {
        SetTitle(m_editor->text());
        emit SignalTitleChanged(m_editor->text());
    }
    else
    {
        BeginRename(); //resets name to label text
    }
}

void RenameableTitleBar::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        emit SignalLeftClicked(mouseEvent->globalPos());
    }
    else if (mouseEvent->button() == Qt::RightButton)
    {
        emit SignalRightClicked(mouseEvent->globalPos());
    }
    return QWidget::mouseReleaseEvent(mouseEvent);
}

void RenameableTitleBar::UpdateSelectedLibStyle(bool LibIsSelected)
{
    m_label->setProperty("Selected", LibIsSelected ? "true" : "false");
    style()->unpolish(m_label);
    style()->polish(m_label);
    m_label->update();
}

void RenameableTitleBar::UpdateExpandedLibStyle(bool LibIsExpanded)
{
    m_collapseIcon->setProperty("Expanded", LibIsExpanded ? "true" : "false");
    style()->unpolish(m_collapseIcon);
    style()->polish(m_collapseIcon);
    m_collapseIcon->update();
}
#include <RenameableTitleBar.moc>