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

#include "stdafx.h"
#include "MainTools.h"
#include "ObjectCreateTool.h"

/////////////////////////////////////////////////////////////////////////////
// CMainTools dialog

const QString& GetToolTip(const QString& category)
{
    static QString empty;
    static std::map<QString, QString>s_mainToolTips =
    {
        {"AI", CMainTools::tr("AI System Objects")},
        {"Actor Entity", CMainTools::tr("Actor Entity System Objects")},
        {"Archetype Entity", CMainTools::tr("Archetypes created in DB Editor")},
        {"Area", CMainTools::tr("Volume and Plane Objects")},
        {"Audio", CMainTools::tr("Audio System Objects")},
        {"Brush", CMainTools::tr("Non-interactive Scene Objects")},
        {"Custom", CMainTools::tr("Game-defined Objects")},
        {"Designer", CMainTools::tr("Create Custom Static Geometry")},
        {"Entity", CMainTools::tr("Entity System Objects")},
        {"Geom Entity", CMainTools::tr("Static Objects")},
        {"GreyBox", CMainTools::tr("Convex Brush Editor")},
        {"Misc", CMainTools::tr("Miscellaneous Objects")},
        {"Particle Entity", CMainTools::tr("Particle Systems")},
        {"Prefab", CMainTools::tr("Reusable Entity Collections")},
    };

    auto toolTip = s_mainToolTips.find(category);

    if (toolTip != s_mainToolTips.end())
    {
        return (*toolTip).second;
    }
    else
    {
        return empty;
    }
}

CMainTools::CMainTools(QWidget* parent)
    : CButtonsPanel(parent)
{
    std::vector< std::pair<QString, QString> > categoryToolClassNamePairs;
    GetIEditor()->GetObjectManager()->GetClassCategoryToolClassNamePairs(categoryToolClassNamePairs);
    for (int i = 0; i < categoryToolClassNamePairs.size(); i++)
    {
        QString category = categoryToolClassNamePairs[i].first;
        QString toolClassName = categoryToolClassNamePairs[i].second;
        SButtonInfo bi;
        bi.toolClassName = toolClassName;
        bi.name = category;
        bi.toolUserDataKey = "category";
        bi.toolUserData = category.toUtf8().data();
        bi.toolTip = GetToolTip(category);

        AddButton(bi);
    }

    OnInitDialog();
}
void CMainTools::OnButtonPressed(const SButtonInfo& button)
{
    // Create browse mode for this category.
    //CObjectCreateTool *tool = new CObjectCreateTool;
    //GetIEditor()->SetEditTool( tool );
    //tool->SelectCategory( button.name );
}
