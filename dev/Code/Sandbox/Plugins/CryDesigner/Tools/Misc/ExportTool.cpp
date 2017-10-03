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
#include "ExportTool.h"
#include "Tools/DesignerTool.h"
#include "CryEdit.h"
#include "GameEngine.h"
#include "Serialization/Decorators/EditorActionButton.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <QFileDialog>
#include <QString>

using Serialization::ActionButton;

void ExportTool::ExportToCgf()
{
    std::vector<CD::SSelectedInfo> selections;
    CD::GetSelectedObjectList(selections);

    if (selections.size() == 1)
    {
        QString filename;
        QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
        if (CFileUtil::SelectSaveFile("CGF Files (*.cgf)", "cgf", levelPath, filename))
        {
            selections[0].m_pCompiler->SaveToCgf(filename.toStdString().c_str());
        }
    }
    else
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Warning"), QObject::tr("Only one object must be selected to save it to cgf file."));
    }
}

void ExportTool::ExportToGrp()
{
    QString filePath = QFileDialog::getSaveFileName(
            NULL,
            QFileDialog::tr("Open group"),
            GetIEditor()->GetGameEngine()->GetLevelPath(),
            QFileDialog::tr("Object Group Files (*.grp)"));

    if (filePath.isEmpty())
    {
        return;
    }

    CSelectionGroup* sel = GetIEditor()->GetSelection();
    XmlNodeRef root = XmlHelpers::CreateXmlNode("Objects");
    CObjectArchive ar(GetIEditor()->GetObjectManager(), root, false);
    for (int i = 0; i < sel->GetCount(); i++)
    {
        ar.SaveObject(sel->GetObject(i));
    }

    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, filePath.toStdString().c_str());
}

void ExportTool::ExportToObj()
{
    CCryEditApp::instance()->OnExportSelectedObjects();
}

void ExportTool::Serialize(Serialization::IArchive& ar)
{
    ar(ActionButton([this] {ExportToObj();
            }), "OBJ", ".OBJ");
    ar(ActionButton([this] {ExportToCgf();
            }), "CGF", ".CGF");
    ar(ActionButton([this] {ExportToGrp();
            }), "GRP", ".GRP");
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Export, CD::eToolGroup_Misc, "Export", ExportTool, PropertyTreePanel<ExportTool>)