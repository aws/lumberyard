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
#include "../EditorCommon/QPropertyTree/ContextList.h"
#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "CharacterDocument.h"
#include "Expected.h"
#include "PlaybackLayers.h"
#include "SceneContent.h"
#include "SceneParametersPanel.h"
#include "Serialization.h"
#include "Serialization/Decorators/INavigationProvider.h"
#include "CharacterGizmoManager.h"
#include "CharacterToolSystem.h"
#include "GizmoSink.h"
#include <ICryAnimation.h>
#include <QBoxLayout>

namespace CharacterTool
{
    // ---------------------------------------------------------------------------

    SceneParametersPanel::SceneParametersPanel(QWidget* parent, System* system)
        : QWidget(parent)
        , m_system(system)
        , m_ignoreSubselectionChange(false)
    {
        EXPECTED(connect(m_system->scene.get(), SIGNAL(SignalChanged(bool)), SLOT(OnSceneChanged(bool))));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalExplorerSelectionChanged()), SLOT(OnExplorerSelectionChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalCharacterLoaded()), SLOT(OnCharacterLoaded())));
        EXPECTED(connect(m_system->explorer.get(), SIGNAL(SignalEntryModified(ExplorerEntryModifyEvent &)), SLOT(OnExplorerEntryModified(ExplorerEntryModifyEvent &))));
        EXPECTED(connect(m_system->characterGizmoManager.get(), SIGNAL(SignalSubselectionChanged(int)), SLOT(OnSubselectionChanged(int))));

        QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->setMargin(0);
        layout->setSpacing(0);

        m_propertyTree = new QPropertyTree(this);
        m_propertyTree->setExpandLevels(0);
        m_propertyTree->setAutoRevert(false);
        m_propertyTree->setSliderUpdateDelay(0);
        m_propertyTree->setShowContainerIndices(false);
        m_propertyTree->setArchiveContext(m_system->contextList->Tail());
        m_propertyTree->attach(Serialization::SStruct(*m_system->scene));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalChanged()), this, SLOT(OnPropertyTreeChanged())));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalSelected()), this, SLOT(OnPropertyTreeSelected())));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalContinuousChange()), this, SLOT(OnPropertyTreeContinuousChange())));

        layout->addWidget(m_propertyTree, 1);
    }

    void SceneParametersPanel::OnExplorerEntryModified(ExplorerEntryModifyEvent& ev)
    {
        if (!ev.continuousChange)
        {
            m_propertyTree->update();
        }
    }

    void SceneParametersPanel::OnPropertyTreeSelected()
    {
        std::vector<PropertyRow*> rows;
        rows.resize(m_propertyTree->selectedRowCount());
        for (int i = 0; i < rows.size(); ++i)
        {
            rows[i] = m_propertyTree->selectedRowByIndex(i);
        }
        if (rows.empty())
        {
            return;
        }

        vector<const void*> handles;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            PropertyRow* row = rows[i];
            while (row->parent())
            {
                if (row->serializer())
                {
                    handles.push_back(row->searchHandle());
                }
                row = row->parent();
            }
        }

        m_ignoreSubselectionChange = true;
        m_system->characterGizmoManager->SetSubselection(GIZMO_LAYER_SCENE, handles);
        m_ignoreSubselectionChange = false;
    }

    void SceneParametersPanel::OnPropertyTreeChanged()
    {
        m_system->scene->SignalChanged(false);
        m_system->scene->CheckIfPlaybackLayersChanged(false);
    }


    void SceneParametersPanel::OnPropertyTreeContinuousChange()
    {
        m_system->scene->CheckIfPlaybackLayersChanged(true);
        m_system->scene->SignalChanged(true);
    }


    void SceneParametersPanel::OnSceneChanged(bool continuous)
    {
        if (!continuous)
        {
            m_propertyTree->revert();
        }
    }

    void SceneParametersPanel::OnCharacterLoaded()
    {
        m_propertyTree->revert();
    }

    void SceneParametersPanel::OnExplorerSelectionChanged()
    {
        // We need to update property tree as NavigatableReference-s may change they
        // look depending on selection.
        m_propertyTree->update();
    }


    void SceneParametersPanel::OnSubselectionChanged(int layer)
    {
        if (layer != GIZMO_LAYER_SCENE)
        {
            return;
        }
        if (m_ignoreSubselectionChange)
        {
            return;
        }

        const vector<const void*>& handles = m_system->characterGizmoManager->Subselection(GIZMO_LAYER_SCENE);
        m_propertyTree->selectByAddresses(handles.data(), handles.size(), true);
    }
}

#include <CharacterTool/SceneParametersPanel.moc>
