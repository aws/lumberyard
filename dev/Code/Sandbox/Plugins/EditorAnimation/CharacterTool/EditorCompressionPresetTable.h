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

#include "../Shared/CompressionPresetTable.h"
#include "Serialization/Decorators/TagList.h"
#include <QObject>

namespace CharacterTool
{
    class FilterAnimationList;

    struct EditorCompressionPreset
    {
        SCompressionPresetEntry entry;
        vector<string> matchingAnimations;

        void Serialize(Serialization::IArchive& ar);
        EditorCompressionPreset() {}
    };


    class EditorCompressionPresetTable
        : public QObject
    {
        Q_OBJECT
    public:

        EditorCompressionPresetTable()
            : m_filterAnimationList(0) {}
        void SetFilterAnimationList(const FilterAnimationList* filterAnimationList) { m_filterAnimationList = filterAnimationList; }

        void Reset();
        bool Load();
        bool Save();

        EditorCompressionPreset* FindPresetForAnimation(const SAnimationFilterItem& item);

        void Serialize(Serialization::IArchive& ar);
        void FindTags(std::vector<std::pair<string, string> >* tags) const;
    signals:
        void SignalChanged();
    private:
        void UpdateMatchingAnimations();

        const FilterAnimationList* m_filterAnimationList;
        std::vector<EditorCompressionPreset> m_entries;
    };
}
