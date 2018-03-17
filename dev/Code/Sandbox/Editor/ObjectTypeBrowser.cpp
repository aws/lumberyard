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
#include "ObjectTypeBrowser.h"
#include "ObjectCreateTool.h"

/////////////////////////////////////////////////////////////////////////////
// ObjectTypeBrowser dialog

ObjectTypeBrowser::ObjectTypeBrowser(CObjectCreateTool* createTool, const QString& category, QWidget* parent)
    : CButtonsPanel(parent)
    , m_createTool(createTool)
    , m_category(category)
{
    SetCategory(m_createTool, m_category);

    OnInitDialog();
}

/////////////////////////////////////////////////////////////////////////////
// ObjectTypeBrowser message handlers

void ObjectTypeBrowser::SetCategory(CObjectCreateTool* createTool, const QString& category)
{
    assert(createTool != 0);
    m_createTool = createTool;
    m_category = category;
    QStringList types;
    GetIEditor()->GetObjectManager()->GetClassTypes(category, types);

    for (int i = 0; i < types.size(); i++)
    {
        SButtonInfo bi;
        bi.toolClassName = "EditTool.ObjectCreate";
        bi.name = types[i];
        bi.toolUserDataKey = "type";
        bi.toolUserData = types[i].toUtf8().data();
        AddButton(bi);
    }
}

#include <ObjectTypeBrowser.moc>
