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
#include "QCopyableWidget.h"
#include "ISystem.h"
#include "qevent.h"
#include "../ContextMenu.h"

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QCopyableWidget.moc>
#endif
#include "qapplication.h"
#include "IEditorParticleUtils.h"

#include <Undo/Undo.h>

QCopyableWidget::QCopyableWidget(QWidget* parent /*= 0*/)
    : QWidget(parent)
    , m_flags(COPY_MENU_FLAGS(COPY_MENU_DISABLED))
{
}

void QCopyableWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MouseButton::RightButton)
    {
        return LaunchMenu(event->globalPos());
    }
    return QWidget::mousePressEvent(event);
}

void QCopyableWidget::BuildMenu(QMenu *menu)
{
    if (m_flags & COPY_MENU_DISABLED)
    {
        return;
    }

    // just rebuild menu by clearing
    menu->clear();

    QAction* action = nullptr;

    menu->addSeparator();
    if (m_flags & ENABLE_COPY)
    {
        action = menu->addAction("Copy");
        connect(action, &QAction::triggered, this, [this]()
            {
                if ((bool)onCopyCallback)
                {
                    this->onCopyCallback();
                }
            });
    }
    if (m_flags & ENABLE_PASTE)
    {
        action = menu->addAction("Paste");
        if (onCheckPasteCallback)
        {
            action->setDisabled(!onCheckPasteCallback());
        }
        else
        {
            action->setDisabled(true); //not sure what is expected here...
        }

        connect(action, &QAction::triggered, this, [this]()
            {
                if ((bool)onPasteCallback)
                {
                    this->onPasteCallback();
                }
            });
    }
    if (m_flags & ENABLE_DUPLICATE)
    {
        action = menu->addAction("Duplicate");
        connect(action, &QAction::triggered, this, [this](bool state)
            {
                if ((bool)onDuplicateCallback)
                {
                    this->onDuplicateCallback();
                }
            });
    }
    if (m_flags & ENABLE_RESET)
    {
        action = menu->addAction("Reset to default");
        connect(action, &QAction::triggered, this, [this]()
            {
                if ((bool)onResetCallback)
                {
                    this->onResetCallback();
                }
            });
        if (onCheckResetCallback)
        {
            action->setEnabled(onCheckResetCallback());
        }
    }
    menu->addSeparator();

    if (onCustomMenuCreationCallback)
    {
        onCustomMenuCreationCallback(menu);
    }
}

void QCopyableWidget::SetCopyMenuFlags(COPY_MENU_FLAGS flags)
{
    m_flags = flags;
}

void QCopyableWidget::SetCustomMenuCreationCallback(std::function<void(QMenu*)> custom)
{
    onCustomMenuCreationCallback = custom;
}

std::function<void(QMenu*)> QCopyableWidget::GetCustomMenuCreationCallback()
{
    return onCustomMenuCreationCallback;
}

void QCopyableWidget::SetResetCallback(std::function<void()> callback)
{
    onResetCallback = callback;
}

void QCopyableWidget::SetDuplicateCallback(std::function<void()> callback)
{
    onDuplicateCallback = callback;
}

void QCopyableWidget::SetPasteCallback(std::function<void()> callback)
{
    onPasteCallback = callback;
}

void QCopyableWidget::SetCheckPasteCallback(std::function<bool()> callback)
{
    onCheckPasteCallback = callback;
}

void QCopyableWidget::SetCopyCallback(std::function<void()> callback)
{
    onCopyCallback = callback;
}

void QCopyableWidget::SetCheckResetCallback(std::function<bool()> callback)
{
    onCheckResetCallback = callback;
}

void QCopyableWidget::LaunchMenu(QPoint pos)
{
    ContextMenu menu(this);
    BuildMenu(&menu); //Ensure that the show/hide checkboxes and the paste disabled state are in sync
    menu.setFocus();
    menu.exec(pos);
}

void QCopyableWidget::keyPressEvent(QKeyEvent* event)
{
    return QWidget::keyPressEvent(event);
}
