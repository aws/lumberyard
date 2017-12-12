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
#include "ViewportMode.h"
#include <QObject>
#include <memory>

namespace Manip {
    class CScene;
}
namespace Serialization {
    struct SStruct;
}

class QAction;

namespace CharacterTool
{
    struct CharacterDefinition;

    using std::unique_ptr;

    class ModeCharacter
        : public QObject
        , public IViewportMode
    {
        Q_OBJECT
    public:
        ModeCharacter();

        void Serialize(Serialization::IArchive& ar) override;
        void EnterMode(const SModeContext& context) override;
        void LeaveMode() override;

        void GetPropertySelection(vector<const void*>* selection) const override;
        void SetPropertySelection(const vector<const void*>& items) override;

        void OnViewportRender(const SRenderContext& rc) override;
        void OnViewportKey(const SKeyEvent& ev) override;
        bool ProcessesViewportKey(const QKeySequence& key) override;
        void OnViewportMouse(const SMouseEvent& ev) override;
    protected slots:
        void OnTransformPanelChanged();
        void OnTransformPanelChangeFinished();
        void OnTransformPanelSpaceChanged();

        void OnSceneSelectionChanged();
        void OnSceneElementsChanged(unsigned int layerBits);
        void OnSceneElementContinousChange(unsigned int layerBits);
        void OnScenePropertiesChanged();
        void OnSceneManipulationModeChanged();
        void OnSceneUndo();
        void OnSceneRedo();

        void OnBindPoseModeChanged();
        void OnDisplayOptionsChanged();
        void OnSubselectionChanged(int layer);
        void OnGizmoChanged();

        void OnActionRotateTool();
        void OnActionMoveTool();
        void OnActionScaleTool();

    private:
        void WriteScene(const CharacterDefinition& def);
        void ReadScene(CharacterDefinition* def) const;
        void WriteTransformPanel();
        void UpdateToolbar();
        void HandleSceneChange(int layerMask, bool continuous);

        ICharacterInstance* m_character;
        unique_ptr<QAction> m_actionMoveTool;
        unique_ptr<QAction> m_actionRotateTool;
        unique_ptr<QAction> m_actionScaleTool;

        unique_ptr<Manip::CScene> m_scene;
        bool m_ignoreSceneSelectionChange;
        System* m_system;
        CharacterDocument* m_document;
        CharacterToolForm* m_window;
        TransformPanel* m_transformPanel;
        std::vector<QPropertyTree*> m_layerPropertyTrees;
    };
}
