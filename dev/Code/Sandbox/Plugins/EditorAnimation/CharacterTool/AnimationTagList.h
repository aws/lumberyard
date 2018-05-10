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

#include "Serialization/Decorators/TagList.h"
#include <QObject>

namespace CharacterTool
{
    class EditorCompressionPresetTable;
    class EditorDBATable;
    class AnimationTagList
        : public QObject
        , public ITagSource
    {
        Q_OBJECT
    public:
        explicit AnimationTagList(EditorCompressionPresetTable* presets, EditorDBATable* dbaTable);

    protected:
        void AddRef() override {}
        void Release() override {}

        unsigned int TagCount(unsigned int group) const override;
        const char* TagValue(unsigned int group, unsigned int index) const override;
        const char* TagDescription(unsigned int group, unsigned int index) const override;
        const char* GroupName(unsigned int group) const override;
        unsigned int GroupCount() const override;
    protected slots:
        void OnCompressionPresetsChanged();
        void OnDBATableChanged();
    private:
        EditorCompressionPresetTable* m_presets;
        EditorDBATable* m_dbaTable;

        struct SEntry
        {
            string tag;
            string description;
        };

        struct SGroup
        {
            string name;
            std::vector<SEntry> entries;
        };

        std::vector<SGroup> m_groups;
    };
}
