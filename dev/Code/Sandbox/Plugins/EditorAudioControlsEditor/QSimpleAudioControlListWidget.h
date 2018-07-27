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

#include <QTreeWidget>
#include "AudioControl.h"
#include "ATLControlsModel.h"
#include "QtUtil.h"
#include "common/ACETypes.h"

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    class QSimpleAudioControlListWidget
        : public QTreeWidget
        , public IATLControlModelListener
    {
        Q_OBJECT

    public:
        explicit QSimpleAudioControlListWidget(QWidget* parent = nullptr);
        ~QSimpleAudioControlListWidget();
        void UpdateModel();
        void Refresh(bool reload = true);

        //
        QTreeWidgetItem* GetItem(CID id, bool bLocalised);
        TImplControlType GetControlType(QTreeWidgetItem* item);
        CID GetItemId(QTreeWidgetItem* item);
        bool IsLocalised(QTreeWidgetItem* item);
        std::vector<CID> GetSelectedIds();
        bool IsConnected(QTreeWidgetItem* item);

    private:
        void LoadControls();
        void LoadControl(IAudioSystemControl* pControl, QTreeWidgetItem* pRoot);
        QTreeWidgetItem* InsertControl(IAudioSystemControl* pControl, QTreeWidgetItem* pRoot);

        void InitItem(QTreeWidgetItem* pItem);
        void InitItemData(QTreeWidgetItem* pItem, IAudioSystemControl* pControl = nullptr);

        //////////////////////////////////////////////////////////
        // IATLControlModelListener implementation
        /////////////////////////////////////////////////////////
        void OnConnectionAdded(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl) override;
        void OnConnectionRemoved(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl) override;
        //////////////////////////////////////////////////////////

    private slots:
        void UpdateControl(IAudioSystemControl* pControl);

    private:
        // Icons and colours
        QColor m_connectedColor;
        QColor m_disconnectedColor;
        QColor m_localisedColor;
    };
} // namespace AudioControls
