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

#include <ATLControlsModel.h>
#include <AudioControl.h>
#include <IAudioSystemEditor.h>
#include <QConnectionsWidget.h>

#include <QWidget>
#include <QMenu>
#include <QListWidget>

#include <Source/Editor/ui_InspectorPanel.h>

namespace AudioControls
{
    class CATLControl;

    //-------------------------------------------------------------------------------------------//
    class CInspectorPanel
        : public QWidget
        , public Ui::InspectorPanel
        , public IATLControlModelListener
    {
        Q_OBJECT
    public:
        explicit CInspectorPanel(CATLControlsModel* pATLModel);
        ~CInspectorPanel();
        void Reload();

    public slots:
        void SetSelectedControls(const ControlList& selectedControls);
        void UpdateInspector();

    private slots:
        void SetControlName(QString name);
        void SetControlScope(QString scope);
        void SetAutoLoadForCurrentControl(bool bAutoLoad);
        void SetControlPlatforms();
        void FinishedEditingName();

    private:
        void UpdateNameControl();
        void UpdateScopeControl();
        void UpdateAutoLoadControl();
        void UpdateConnectionListControl();
        void UpdateConnectionData();
        void UpdateScopeData();

        void HideScope(bool bHide);
        void HideAutoLoad(bool bHide);
        void HideGroupConnections(bool bHide);

        //////////////////////////////////////////////////////////
        // IAudioSystemEditor implementation
        /////////////////////////////////////////////////////////
        virtual void OnControlModified(AudioControls::CATLControl* pControl) override;
        //////////////////////////////////////////////////////////

        CATLControlsModel* m_pATLModel;

        EACEControlType m_selectedType;
        AZStd::vector<CATLControl*> m_selectedControls;
        bool m_bAllControlsSameType;

        QColor m_notFoundColor;

        AZStd::vector<QConnectionsWidget*> m_connectionLists;
        AZStd::vector<QComboBox*> m_platforms;
    };
} // namespace AudioControls
