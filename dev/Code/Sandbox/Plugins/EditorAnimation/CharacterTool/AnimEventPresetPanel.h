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

#include <memory>
#include <QWidget>
#include <Cry_Math.h>
#include <Cry_Color.h>

class BlockPalette;
struct BlockPaletteItem;
class QPropertyTree;

namespace Serialization {
    class IArchive;
}

namespace CharacterTool {
    struct System;
    struct AnimEventPreset;
    struct AnimEventPresetCollection;


    typedef std::vector<std::pair<unsigned int, ColorB> > EventContentToColorMap;

    class AnimEventPresetPanel
        : public QWidget
    {
        Q_OBJECT
    public:
        AnimEventPresetPanel(QWidget* parent, System* system);
        ~AnimEventPresetPanel();

        const AnimEventPreset* GetPresetByHotkey(int number) const;
        const EventContentToColorMap& GetEventContentToColorMap() const{ return m_eventContentToColorMap; }
        void LoadPresets();
        void SavePresets();

        void Serialize(Serialization::IArchive& ar);
    signals:
        void SignalPutEvent(const AnimEventPreset& preset);
        void SignalPresetsChanged();
    protected slots:
        void OnPropertyTreeChanged();
        void OnPaletteSelectionChanged();
        void OnPaletteItemClicked(const BlockPaletteItem& item);
        void OnPaletteChanged();

    private:
        void WritePalette();
        void ReadPalette();
        void PresetsChanged();
        EventContentToColorMap m_eventContentToColorMap;
        BlockPalette* m_palette;
        QPropertyTree* m_tree;
        std::unique_ptr<AnimEventPresetCollection> m_presetCollection;
    };
}
