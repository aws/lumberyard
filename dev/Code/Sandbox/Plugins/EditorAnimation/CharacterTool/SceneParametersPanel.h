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

class QPropertyTree;

namespace CharacterTool
{
    using std::unique_ptr;
    struct System;

    class SceneParametersPanel
        : public QWidget
    {
        Q_OBJECT
    public:
        SceneParametersPanel(QWidget* parent, System* system);
        QSize sizeHint() const override { return QSize(240, 100); }
    protected slots:
        void OnPropertyTreeChanged();
        void OnPropertyTreeContinuousChange();
        void OnPropertyTreeSelected();

        void OnSubselectionChanged(int layer);
        void OnSceneChanged(bool continuous);
        void OnCharacterLoaded();
        void OnExplorerSelectionChanged();
        void OnExplorerEntryModified(ExplorerEntryModifyEvent& ev);
    private:

        QPropertyTree* m_propertyTree;
        System* m_system;
        bool m_ignoreSubselectionChange;
    };
}
