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

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    //////////////////////
    // EditorContextMenu
    //////////////////////
    
    EditorContextMenu::EditorContextMenu(QWidget* parent)
        : QMenu(parent)
        , m_finalized(false)
    {
    }
    
    void EditorContextMenu::AddActionGroup(const ActionGroupId& actionGroup)
    {
        AZ_Error("GraphCanvas", !m_finalized, "Trying to configure a Menu that has already been finalized");
        if (!m_finalized)
        {
            auto insertResult = m_actionGroups.insert(actionGroup);

            if (insertResult.second)
            {
                m_actionGroupOrdering.push_back(actionGroup);
            }
        }
    }

    void EditorContextMenu::AddMenuAction(QAction* contextMenuAction)
    {
        AZ_Error("GraphCanvas", !m_finalized, "Trying to configure a Menu that has already been finalized.");
        if (!m_finalized)
        {
            m_unprocessedActions.emplace_back(contextMenuAction);
        }
        else
        {
            delete contextMenuAction;
        }
    }

    void EditorContextMenu::RefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        if (!m_finalized)
        {
            ConstructMenu();
        }

        if (m_finalized)
        {
            for (QAction* action : actions())
            {
                ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>(action);

                if (contextMenuAction)
                {
                    contextMenuAction->RefreshAction(graphId, targetMemberId);
                }
            }

            for (const auto& mapPair : m_subMenuMap)
            {
                bool enableMenu = false;
                for (QAction* action : mapPair.second->actions())
                {
                    ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>(action);

                    if (contextMenuAction)
                    {
                        contextMenuAction->RefreshAction(graphId, targetMemberId);
                        enableMenu = enableMenu || contextMenuAction->isEnabled();
                    }
                }

                mapPair.second->setEnabled(enableMenu);
            }
        }

        OnRefreshActions(graphId, targetMemberId);
    }

    void EditorContextMenu::showEvent(QShowEvent* showEvent)
    {
        ConstructMenu();
        QMenu::showEvent(showEvent);
    }
    
    void EditorContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);
    }
    
    void EditorContextMenu::ConstructMenu()
    {
        if (!m_finalized)
        {
            m_finalized = true;

            for (ActionGroupId currentGroup : m_actionGroupOrdering)
            {
                bool addedElement = false;
                auto actionIter = m_unprocessedActions.begin();

                while (actionIter != m_unprocessedActions.end())
                {
                    ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>((*actionIter));
                    if (contextMenuAction)
                    {
                        if (contextMenuAction->GetActionGroupId() == currentGroup)
                        {
                            addedElement = true;

                            if (contextMenuAction->IsInSubMenu())
                            {
                                AZStd::string menuString = contextMenuAction->GetSubMenuPath();

                                auto subMenuIter = m_subMenuMap.find(menuString);

                                if (subMenuIter == m_subMenuMap.end())
                                {
                                    QMenu* menu = addMenu(menuString.c_str());

                                    auto insertResult = m_subMenuMap.insert(AZStd::make_pair(menuString, menu));

                                    subMenuIter = insertResult.first;
                                }

                                subMenuIter->second->addAction(contextMenuAction);
                            }
                            else
                            {
                                addAction(contextMenuAction);
                            }

                            actionIter = m_unprocessedActions.erase(actionIter);
                            continue;
                        }
                    }

                    ++actionIter;
                }

                if (addedElement)
                {
                    addSeparator();
                }
            }

            for (QAction* uncategorizedAction : m_unprocessedActions)
            {
                addAction(uncategorizedAction);
            }

            m_unprocessedActions.clear();
        }
    }

#include <StaticLib/GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.moc>
}