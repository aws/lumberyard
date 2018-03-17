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
#include "ParticleLibrary.h"
#include "ParticleItem.h"
#include "ErrorReport.h"
#include "ILogFile.h"

//////////////////////////////////////////////////////////////////////////
// CParticleLibrary implementation.
//////////////////////////////////////////////////////////////////////////
bool CParticleLibrary::Save()
{
    // save particle library even if it is empty
    return SaveLibrary("ParticleLibrary", true);
}

//////////////////////////////////////////////////////////////////////////
bool CParticleLibrary::Load(const QString& filename)
{
    if (filename.isEmpty())
    {
        return false;
    }
    SetModified(false);
    m_bNewLibrary = false;
    // Always set the filename. Ignored the unique check.
    SetFilename(filename, /*checkForUnique = */false);
    XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(filename.toUtf8().data());
    if (!root)
    {
        return false;
    }

    Serialize(root, true);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CParticleLibrary::Serialize(XmlNodeRef& root, bool bLoading)
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
            if (_stricmp(itemNode->getTag(), "Particles") != 0 && _stricmp(itemNode->getTag(), "Folder") != 0)
            {
                continue;
            }
            QString effectName = name + "." + itemNode->getAttr("Name");
            IParticleEffect* pEffect = GetIEditor()->GetEnv()->pParticleManager->LoadEffect(effectName.toUtf8().data(), itemNode, false);
            if (pEffect)
            {
                CParticleItem* pItem = new CParticleItem(pEffect);
                AddItem(pItem);
                pItem->AddAllChildren();
                if (_stricmp(itemNode->getTag(), "Folder") == 0) // if the item is a folder
                {
                    pItem->IsParticleItem = false;
                }
            }
            else
            {
                CParticleItem* pItem = new CParticleItem();
                AddItem(pItem);
                GetIEditor()->GetErrorReport()->SetCurrentValidatorItem(pItem);
                CBaseLibraryItem::SerializeContext ctx(itemNode, bLoading);
                pItem->Serialize(ctx);
                GetIEditor()->GetErrorReport()->SetCurrentValidatorItem(NULL);
                if (_stricmp(itemNode->getTag(), "Folder") == 0) // if the item is a folder
                {
                    pItem->IsParticleItem = false;
                }
            }
        }
        SetModified(false);
    }
    else
    {
        // Saving.
        root->setAttr("Name", GetName().toUtf8().data());
        char version[50];
        GetIEditor()->GetFileVersion().ToString(version, AZ_ARRAY_SIZE(version));
        root->setAttr("SandboxVersion", version);

        // Serialize prototypes.
        for (int i = 0; i < GetItemCount(); i++)
        {
            CParticleItem* pItem = (CParticleItem*)GetItem(i);
            // Save materials with childs under thier parent table.
            CParticleItem* pParent = pItem->GetParent();
            if (pParent && pParent->IsParticleItem)
            {
                continue;
            }

            //NOTE: In the Particle Editor "Folders" are objects that are virtual so we need to skip those from being serialized out.
            // If we dont do this we will get extra unwanted items in our library when we next load from the file.
            //
            if (pItem->IsParticleItem)
            {
                XmlNodeRef itemNode = root->newChild("Particles");
                CBaseLibraryItem::SerializeContext ctx(itemNode, bLoading);
                pItem->Serialize(ctx);
            }
            else // For folders
            {
                XmlNodeRef itemNode = root->newChild("Folder");
                CBaseLibraryItem::SerializeContext ctx(itemNode, bLoading);
                pItem->Serialize(ctx);
            }
        }
    }
}


bool CParticleLibrary::SetFilename(const QString& filename, bool checkForUnique /*=true*/)
{
    CCryFile xmlFile;

    if (checkForUnique)
    {
        // If this is a new library and already exist a file with the same name
        if (xmlFile.Open(filename.toUtf8().data(), "rb"))
        {
            return false;
        }
    }
    m_filename = filename;
    m_filename.toLower();
    return true;
}
