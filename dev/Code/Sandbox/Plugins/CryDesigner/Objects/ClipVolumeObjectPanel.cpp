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

#include "StdAfx.h"
#include "Util/Converter.h"
#include "Tools/ClipVolumeTool.h"
#include <Objects/ui_ClipVolumeObjectPanel.h>
#include "ClipVolumeObject.h"
#include "ClipVolumeObjectPanel.h"

#include <QTimer>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

ClipVolumeObjectPanel::ClipVolumeObjectPanel(ClipVolumeObject* pClipVolume, QWidget* pParent)
    : QWidget(pParent)
    , ui(new Ui::ClipVolumeObjectPanel)
    , m_pClipVolume(pClipVolume)
{
    ui->setupUi(this);

    connect(ui->m_loadCgfButton, &QPushButton::clicked, this, &ClipVolumeObjectPanel::OnBnClickedLoadCgf);

    ui->m_editClipVolumeButton->SetToolClass(&ClipVolumeTool::staticMetaObject, "object", m_pClipVolume);
}

void ClipVolumeObjectPanel::GotoEditModeAsync()
{
    QTimer::singleShot(0, ui->m_editClipVolumeButton, SLOT(click()));
}

void ClipVolumeObjectPanel::OnBnClickedLoadCgf()
{
    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    if (_smart_ptr<IStatObj> pStatObj = GetIEditor()->Get3DEngine()->LoadStatObjAutoRef(selection.GetResult()->GetFullPath().c_str(), NULL, NULL, false))
    {
        _smart_ptr<CD::Model> pNewModel = new CD::Model();
        if (CD::Converter::ConvertMeshToBrushDesigner(pStatObj->GetIndexedMesh(true), pNewModel))
        {
            GetIEditor()->BeginUndo();
            m_pClipVolume->GetModel()->RecordUndo("ClipVolume: Load CGF", m_pClipVolume);
            m_pClipVolume->SetModel(pNewModel);
            CD::UpdateAll(m_pClipVolume->MainContext());
            GetIEditor()->AcceptUndo("ClipVolume: Load CGF");
        }
    }
}

#include <Objects/ClipVolumeObjectPanel.moc>
