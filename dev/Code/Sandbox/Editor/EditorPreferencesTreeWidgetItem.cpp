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
#include "stdafx.h"
#include "EditorPreferencesTreeWidgetItem.h"
#include "IPreferencesPage.h"


EditorPreferencesTreeWidgetItem::EditorPreferencesTreeWidgetItem(IPreferencesPage* page, const QPixmap& selectedImage, QPixmap& unselectedImage)
    : QTreeWidgetItem(EditorPreferencesPage)
    , m_preferencesPage(page)
    , m_selectedImage(selectedImage)
    , m_unselectedImage(unselectedImage)
{
    setData(0, Qt::DisplayRole, m_preferencesPage->GetTitle());
    SetActivePage(false);
}


EditorPreferencesTreeWidgetItem::~EditorPreferencesTreeWidgetItem()
{
}


void EditorPreferencesTreeWidgetItem::SetActivePage(bool active)
{
    if (active)
    {
        setData(0, Qt::DecorationRole, m_selectedImage);
    }
    else
    {
        setData(0, Qt::DecorationRole, m_unselectedImage);
    }
}


IPreferencesPage* EditorPreferencesTreeWidgetItem::GetPreferencesPage() const
{
    return m_preferencesPage;
}


#include <EditorPreferencesTreeWidgetItem.moc>