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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "pch.h"
#include "UndoStack.h"
#include "Expected.h"

namespace CharacterTool
{
    void CUndoStack::PushUndo(vector<char>* previousContentToMove, const char* description, unsigned long long sequentialIndex)
    {
        EXPECTED(description && description[0]);
        m_undos.push_back(SUndoState());
        SUndoState& back = m_undos.back();
        back.state.swap(*previousContentToMove);
        back.description = description;
        back.sequentialIndex = sequentialIndex;

        m_redos.clear();
    }

    bool CUndoStack::Undo(vector<char>* newState, const vector<char>& firstState, int count, unsigned long long redoSequentialIndex)
    {
        if (m_undos.empty())
        {
            return false;
        }
        if (count <= 0)
        {
            return false;
        }

        vector<char> lastState;

        for (int i = 0; i < count; ++i)
        {
            if (m_undos.empty())
            {
                break;
            }
            SUndoState& undo = m_undos.back();
            lastState.swap(undo.state);

            m_redos.push_back(SUndoState());
            SUndoState& redo = m_redos.back();
            redo.description = undo.description;
            redo.state = i == 0 ? firstState : lastState;
            redo.sequentialIndex = redoSequentialIndex;

            m_undos.pop_back();
        }
        newState->swap(lastState);
        return true;
    }

    bool CUndoStack::Redo(vector<char>* newState, const vector<char>& currentState, unsigned long long undoSequentialIndex)
    {
        if (m_redos.empty())
        {
            return false;
        }

        SUndoState& redo = m_redos.back();
        newState->swap(redo.state);

        m_undos.push_back(SUndoState());
        SUndoState& undo = m_undos.back();
        undo.description = redo.description;
        undo.state = currentState;
        undo.sequentialIndex = undoSequentialIndex;

        m_redos.pop_back();
        return true;
    }

    void CUndoStack::GetUndoActions(vector<string>* actionNames, int maxActionCount)
    {
        actionNames->clear();
        int count = m_undos.size();
        int lastAction = count - maxActionCount < 0 ? 0 : count - maxActionCount;
        for (int i = count - 1; i >= lastAction; --i)
        {
            actionNames->push_back(m_undos[i].description);
        }
    }
}
