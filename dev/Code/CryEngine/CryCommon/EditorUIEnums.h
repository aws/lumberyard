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
#pragma once
#include <IGameRef.h>
#include <IEditorGame.h>
#include <IActionMapManager.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <AzCore/Debug/Trace.h>


class EditorUIEnums
{
public:

    EditorUIEnums(IGameRef game, IGameToEditorInterface* m_gameToEditorIface)
        : m_Game(game)
        , m_gameToEditor(m_gameToEditorIface)
    {
        AZ_Assert(m_gameToEditor, "Invalid editor interface");
        AZ_Assert(m_Game, "Invalid Game interface");

        InitActionFilterEnums();
        InitActionInputEnums();
        InitActionMapsEnums();
    }
private:

    IGameRef m_Game;
    IGameToEditorInterface* m_gameToEditor;

    void InitActionFilterEnums()
    {
        // init ActionFilter enums
        IActionMapManager* actionMapManager = m_Game->GetIGameFramework()->GetIActionMapManager();
        if (actionMapManager)
        {
            std::vector<const char*> filterNames;
            filterNames.push_back(""); // empty
            IActionFilterIteratorPtr actionFilterIter = actionMapManager->CreateActionFilterIterator();
            while (IActionFilter* actionFilter = actionFilterIter->Next())
            {
                filterNames.push_back(actionFilter->GetName());
            }

            m_gameToEditor->SetUIEnums("action_filter", filterNames.data(), filterNames.size());
        }
    }

    void InitActionInputEnums()
    {
        // init ActionInput Enums
        IActionMapManager* actionMapManager = m_Game->GetIGameFramework()->GetIActionMapManager();
        if (actionMapManager)
        {
            struct SActionList
                : public IActionMapPopulateCallBack
            {
                explicit SActionList(std::size_t actionCount)
                    : m_maxInputActions(actionCount)
                {
                    m_inputActionNames.reserve(m_maxInputActions);
                }

                //IActionMapPopulateCallBack
                void AddActionName(const char* const pName) override
                {
                    AZ_Assert(m_inputActionNames.size() < m_maxInputActions, "Can't add any more actions");
                    if (m_inputActionNames.size() < m_maxInputActions)
                    {
                        m_inputActionNames.push_back(pName);
                    }
                }
                //~IActionMapPopulateCallBack

                std::vector<const char*> m_inputActionNames;
                const std::size_t m_maxInputActions;
            };

            const std::size_t actionCount = actionMapManager->GetActionsCount();
            if (actionCount > 0)
            {
                SActionList actionList(actionCount);
                actionMapManager->EnumerateActions(&actionList);
                m_gameToEditor->SetUIEnums("input_actions", actionList.m_inputActionNames.data(), actionList.m_inputActionNames.size());
            }
        }
    }

    void InitActionMapsEnums()
    {
        // init ActionMap enums
        IActionMapManager* actionMapManager = m_Game->GetIGameFramework()->GetIActionMapManager();
        if (actionMapManager)
        {
            const std::size_t numActionMaps = actionMapManager->GetActionMapsCount();
            if (numActionMaps == 0)
            {
                return;
            }

            std::vector<const char*> actionMapNames;
            actionMapNames.reserve(numActionMaps);

            IActionMapIteratorPtr actionMapIter = actionMapManager->CreateActionMapIterator();
            while (IActionMap* actionMap = actionMapIter->Next())
            {
                actionMapNames.push_back(actionMap->GetName());
            }

            AZ_Assert(actionMapNames.size() == numActionMaps, "[InitActionMapsEnums] Encountered mismatch in number of ActionMaps found vs. reported.");

            m_gameToEditor->SetUIEnums("action_maps", actionMapNames.data(), actionMapNames.size());
        }
    }
};
