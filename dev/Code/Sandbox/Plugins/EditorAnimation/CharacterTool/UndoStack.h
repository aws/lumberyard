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

#pragma once

#include <vector>
#include "Strings.h"

namespace CharacterTool
{
    using std::vector;

    struct SUndoState
    {
        string description;
        vector<char> state;
        unsigned long long sequentialIndex;
    };

    class CUndoStack
    {
    public:
        void PushUndo(vector<char>* previousContentToMove, const char* description, unsigned long long sequentialIndex);
        bool Undo(vector<char>* newState, const vector<char>& currentState, int count, unsigned long long sequentialIndex);
        bool HasUndo() const { return !m_undos.empty(); }
        bool Redo(vector<char>* newState, const vector<char>& currentState, unsigned long long sequentialIndex);
        bool HasRedo() const{ return !m_redos.empty(); }
        unsigned long long NewestUndoIndex() const { return m_undos.empty() ? 0 : m_undos.back().sequentialIndex; }
        unsigned long long NewestRedoIndex() const { return m_redos.empty() ? 0 : m_redos.back().sequentialIndex; }

        void GetUndoActions(vector<string>* actionNames, int maxActionCount);
    private:
        vector<SUndoState> m_undos;
        vector<SUndoState> m_redos;
    };
}
