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

#include "EditorCommon.h"

ModeToolbar::ModeToolbar(EditorWindow* parent)
    : QToolBar("Mode Toolbar", parent)
    , m_group(nullptr)
    , m_previousAction(nullptr)
{
    setObjectName("ModeToolbar");    // needed to save state
    setFloatable(false);

    AddModes(parent);

    parent->addToolBar(this);
}

void ModeToolbar::SetCheckedItem(int index)
{
    if (m_group)
    {
        for (auto action : m_group->actions())
        {
            if (action->data().toInt() == index)
            {
                m_previousAction->setChecked(false);
                m_previousAction = action;
                m_previousAction->setChecked(true);
                return;
            }
        }
    }
}

void ModeToolbar::AddModes(EditorWindow* parent)
{
    m_group = new QActionGroup(this);

    int i = 0;
    for (const auto& m : ViewportInteraction::InteractionMode())
    {
        int key = (Qt::Key_1 + i++);

        QString iconUrl = QString(":/Icons/%1.png").arg(ViewportHelpers::InteractionModeToString((int)m));

        QAction* action = new QAction(QIcon(iconUrl),
                (QString("%1 (%2)").arg(ViewportHelpers::InteractionModeToString((int)m), QString((char)key))),
                this);

        action->setData((int)m);
        action->setShortcut(QKeySequence(key));
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        action->setCheckable(true);   // Give it the behavior of a toggle button.
        QObject::connect(action,
            &QAction::triggered,
            [ this, parent, action ](bool checked)
            {
                if (m_previousAction == action)
                {
                    // Nothing to do.
                    return;
                }

                CommandViewportInteractionMode::Push(parent->GetActiveStack(),
                    parent->GetViewport()->GetViewportInteraction(),
                    m_previousAction,
                    action);
                m_previousAction = action;
            });
        m_group->addAction(action);
    }

    // Give it the behavior of radio buttons.
    m_group->setExclusive(true);

    // Set the first action as the default.
    m_previousAction = *m_group->actions().begin();
    m_previousAction->setChecked(true);

    addActions(m_group->actions());
}

#include <ModeToolbar.moc>
