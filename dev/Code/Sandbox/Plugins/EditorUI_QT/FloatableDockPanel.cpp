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

#include <FloatableDockPanel.h>
#include "qcoreevent.h"
#include <QShortcutEvent>

FloatableDockPanel::FloatableDockPanel(const QString& title, QWidget* parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    connect(this, &QDockWidget::topLevelChanged, this, &FloatableDockPanel::onFloatingStatusChanged);
}

void FloatableDockPanel::onFloatingStatusChanged(bool floating)
{
    if (floating)
    {
        Qt::WindowFlags flags = Qt::Window | Qt::WindowMaximizeButtonHint;
#if defined(AZ_PLATFORM_APPLE)
        // On macOS, tool windows correspond to the Floating class of windows. This means that the
        // window lives on a level above normal windows making it impossible to put a normal window
        // on top of it. Therefore we need to add Qt::Tool to floating panels to ensure they are not
        // hidden under their original window.
        if (window()->windowFlags() & Qt::Tool)
        {
            flags |= Qt::Tool;
        }
#endif
        setWindowFlags(flags);
        showMaximized();
    }
    else
    {
        setWindowFlags(Qt::Widget);
    }
}

void FloatableDockPanel::SetHotkeyHandler(QObject* obj, AZStd::function<void(QKeyEvent* e)> hotkeyHandler)
{
    if (obj != nullptr)
    {
        obj->installEventFilter(this);
        m_hotkeyHandlers.insert(obj, hotkeyHandler);
    }
}

void FloatableDockPanel::SetShortcutHandler(QObject* obj, AZStd::function<bool(QShortcutEvent* e)> shortcutHandler)
{
    if (obj != nullptr)
    {
        obj->installEventFilter(this);
        m_shortcutHandlers.insert(obj, shortcutHandler);
    }
}

bool FloatableDockPanel::eventFilter(QObject* obj, QEvent* e)
{
    if (m_hotkeyHandlers.contains(obj))
    {
        if (e->type() == QEvent::ShortcutOverride)
        {
            QKeyEvent* kev = (QKeyEvent*)e;
            if (m_hotkeyHandlers[obj])
            {
                m_hotkeyHandlers[obj](kev);
                if (e->isAccepted())
                {
                    return true;
                }
            }
        }

        if (e->type() == QEvent::Shortcut)
        {
            QShortcutEvent* kev = (QShortcutEvent*)e;
            if (m_shortcutHandlers[obj])
            {
                return m_shortcutHandlers[obj](kev);
            }
        }
    }

    return QDockWidget::eventFilter(obj, e);
}

#include <FloatableDockPanel.moc>