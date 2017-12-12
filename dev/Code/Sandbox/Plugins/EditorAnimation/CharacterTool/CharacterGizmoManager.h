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
#include "GizmoSink.h"
#include <QObject>

class QPropertyTree;
namespace Serialization
{
    class IArchive;
    class CContextList;
}

namespace CharacterTool
{
    struct System;

    class CharacterGizmoManager
        : public QObject
    {
        Q_OBJECT
    public:
        typedef vector<const void*> SelectionHandles;
        CharacterGizmoManager(System* system);
        QPropertyTree* Tree(GizmoLayer layer);

        void SetSubselection(GizmoLayer layer, const SelectionHandles& handles);
        const SelectionHandles& Subselection(GizmoLayer layer) const;
        void ReadGizmos();

    signals:
        void SignalSubselectionChanged(int layer);
        void SignalGizmoChanged();

    private slots:
        void OnActiveCharacterChanged();
        void OnActiveAnimationSwitched();

        void OnTreeAboutToSerialize(Serialization::IArchive& ar);
        void OnTreeSerialized(Serialization::IArchive& ar);
        void OnExplorerEntryModified(ExplorerEntryModifyEvent& ev);
        void OnExplorerBeginRemoveEntry(ExplorerEntry* entry);
        void OnSceneChanged();

    private:
        vector<unique_ptr<QPropertyTree> > m_trees;
        unique_ptr<Serialization::CContextList> m_contextList;
        vector<SelectionHandles> m_subselections;
        ExplorerEntry* m_attachedAnimationEntry;

        System* m_system;
    };
}
