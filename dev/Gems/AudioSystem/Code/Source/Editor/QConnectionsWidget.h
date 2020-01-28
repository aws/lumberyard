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

#include <AudioControl.h>

#include <QWidget>
#include <QListWidget>
#include <QColor>

#include <Source/Editor/ui_ConnectionsWidget.h>

namespace AudioControls
{
    class CATLControl;
    class IAudioConnectionInspectorPanel;

    //-------------------------------------------------------------------------------------------//
    class QConnectionsWidget
        : public QWidget
        , public Ui::ConnectionsWidget
    {
        Q_OBJECT
    public:
        QConnectionsWidget(QWidget* pParent = nullptr, const AZStd::string& sGroup = "");

    public slots:
        void SetControl(CATLControl* pControl);
        void Init(const AZStd::string& sGroup);

    private slots:
        void ShowConnectionContextMenu(const QPoint& pos);
        void SelectedConnectionChanged();
        void CurrentConnectionModified();
        void RemoveSelectedConnection();

    private:
        bool eventFilter(QObject* pObject, QEvent* pEvent) override;
        void MakeConnectionTo(IAudioSystemControl* pAudioMiddlewareControl);
        void UpdateConnections();
        void CreateItemFromConnection(IAudioSystemControl* pAudioSystemControl);

        AZStd::string m_sGroup;
        CATLControl* m_pControl;
        QColor m_notFoundColor;
        QColor m_localisedColor;
    };
} // namespace AudioControls
