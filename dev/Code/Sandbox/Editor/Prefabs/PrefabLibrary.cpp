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
#include "PrefabLibrary.h"
#include "PrefabItem.h"

//////////////////////////////////////////////////////////////////////////
// CPrefabLibrary implementation.
//////////////////////////////////////////////////////////////////////////
bool CPrefabLibrary::Save()
{
    return SaveLibrary("PrefabsLibrary");
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabLibrary::Load(const QString& filename)
{
    if (filename.isEmpty())
    {
        return false;
    }

    SetFilename(filename);
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
    if (!root)
    {
        return false;
    }

    Serialize(root, true);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabLibrary::Serialize(XmlNodeRef& root, bool bLoading)
{
    if (bLoading)
    {
        // Loading.
        RemoveAllItems();
        QString name = GetName();
        root->getAttr("Name", name);
        SetName(name);
        for (int i = 0; i < root->getChildCount(); i++)
        {
            XmlNodeRef itemNode = root->getChild(i);
            // Only accept nodes with correct name.
            if (_stricmp(itemNode->getTag(), "Prefab") != 0)
            {
                continue;
            }
            CBaseLibraryItem* pItem = new CPrefabItem;
            AddItem(pItem);

            CBaseLibraryItem::SerializeContext ctx(itemNode, bLoading);
            pItem->Serialize(ctx);
        }
        SetModified(false);
        m_bNewLibrary = false;
    }
    else
    {
        // Saving.
        root->setAttr("Name", GetName().toUtf8().data());
        // Serialize prototypes.
        for (int i = 0; i < GetItemCount(); i++)
        {
            XmlNodeRef itemNode = root->newChild("Prefab");
            CBaseLibraryItem::SerializeContext ctx(itemNode, bLoading);
            GetItem(i)->Serialize(ctx);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabLibrary::UpdatePrefabObjects()
{
    for (int i = 0, iItemSize(m_items.size()); i < iItemSize; ++i)
    {
        CPrefabItem* pPrefabItem = (CPrefabItem*)&*m_items[i];
        pPrefabItem->UpdateObjects();
    }
}