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
#include "SelectMissionObjectiveDialog.h"
#include <IMovieSystem.h>
#include <ILocalizationManager.h>
#include "IEditor.h"
#include "GameEngine.h"
#include "UnicodeFunctions.h"
#include <ui_GenericSelectItemDialog.h>

// CSelectMissionObjectiveDialog

//////////////////////////////////////////////////////////////////////////
CSelectMissionObjectiveDialog::CSelectMissionObjectiveDialog(QWidget* pParent)
    :    CGenericSelectItemDialog(pParent)
{
    setWindowTitle(tr("Select MissionObjective"));
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
CSelectMissionObjectiveDialog::OnInitDialog()
{
    SetMode(eMODE_TREE);
    ShowDescription(true);
    CGenericSelectItemDialog::OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
CSelectMissionObjectiveDialog::GetItems(std::vector<SItem>& outItems)
{
    // load MOs
    QString path = (Path::GetEditingGameDataFolder() + "/Libs/UI/Objectives_new.xml").c_str();
    GetItemsInternal(outItems, path.toUtf8().data(), false);
    path = GetIEditor()->GetLevelDataFolder() + "Objectives.xml";
    GetItemsInternal(outItems, path.toUtf8().data(), true);
}

//////////////////////////////////////////////////////////////////////////
void
CSelectMissionObjectiveDialog::GetItemsInternal(std::vector<SItem>& outItems, const char* path, const bool isOptional)
{
    XmlNodeRef missionObjectives = GetISystem()->LoadXmlFromFile(path);
    if (missionObjectives == 0)
    {
        if (!isOptional)
        {
            Error(tr("Error while loading MissionObjective file '%1'").arg(path).toUtf8().data());
        }
        return;
    }

    for (int tag = 0; tag < missionObjectives->getChildCount(); ++tag)
    {
        XmlNodeRef mission = missionObjectives->getChild(tag);
        const char* attrib;
        const char* objective;
        const char* text;
        for (int obj = 0; obj < mission->getChildCount(); ++obj)
        {
            XmlNodeRef objectiveNode = mission->getChild(obj);
            QString id(mission->getTag());
            id += ".";
            id += objectiveNode->getTag();
            if (objectiveNode->getAttributeByIndex(0, &attrib, &objective) && objectiveNode->getAttributeByIndex(1, &attrib, &text))
            {
                SObjective obj;
                obj.shortText = objective;
                obj.longText = text;
                m_objMap [id] = obj;
                SItem item;
                item.name = id;
                outItems.push_back(item);
            }
            else if (!isOptional)
            {
                Error(tr("Error while loading MissionObjective file '%1'").arg(path).toUtf8().data());
                return;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void CSelectMissionObjectiveDialog::ItemSelected()
{
    TObjMap::const_iterator iter = m_objMap.find(m_selectedItem);
    if (iter != m_objMap.end())
    {
        const SObjective& obj = (*iter).second;
        QString objText = obj.shortText;
        if (objText.startsWith(QChar('@')))
        {
            SLocalizedInfoGame locInfo;
            const QByteArray objTextBuffer = objText.toUtf8(); // keeps the buffer alive
            const char* key = objTextBuffer.constData() + 1;
            bool bFound = gEnv->pSystem->GetLocalizationManager()->GetLocalizedInfoByKey(key, locInfo);

            objText = tr("Label: %1").arg(obj.shortText);

            if (bFound)
            {
                objText += tr("\r\nPlain Text: %1").arg(QString::fromWCharArray(Unicode::Convert<wstring>(locInfo.sUtf8TranslatedText).c_str()));
            }
        }

        ui->m_desc->setText(objText);
#if 0
        CryLogAlways("Objective: ID='%s' Text='%s' LongText='%s'",
            m_selectedItem.GetString(), obj.shortText.GetString(), obj.longText.GetString());
#endif
    }
    else
    {
        ui->m_desc->setText(tr("<No Objective Selected>"));
    }
}

#include <SelectMissionObjectiveDialog.moc>
