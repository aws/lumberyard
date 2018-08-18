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
#include "MannequinFileChangeWriter.h"

#include "IGameFramework.h"
#include "../MannequinDialog.h"
#include "Util/FileUtil.h"

#include <QDir>

CMannequinFileChangeWriter* CMannequinFileChangeWriter::sm_pActiveWriter = NULL;

//////////////////////////////////////////////////////////////////////////
CMannequinFileChangeWriter::CMannequinFileChangeWriter(bool changedFilesMode)
    : m_pControllerDef(NULL)
    , m_filterFilesByControllerDef(true)
    , m_changedFilesMode(changedFilesMode)
    , m_reexportAll(false)
    , m_fileManager(*this, changedFilesMode, CMannequinDialog::GetCurrentInstance())
{
}

//////////////////////////////////////////////////////////////////////////
EFileManagerResponse CMannequinFileChangeWriter::ShowFileManager(const bool reexportAll /* =false */)
{
    m_reexportAll = reexportAll;
    RefreshModifiedFiles();

    // Abort early if there are no modified files
    if (m_modifiedFiles.empty())
    {
        return eFMR_NoChanges;
    }

    sm_pActiveWriter = this;

    EFileManagerResponse result = eFMR_NoChanges;
    switch (m_fileManager.exec())
    {
    case QDialog::Accepted:
        result = eFMR_OK;
        break;
    case QDialog::Rejected:
        result = eFMR_Cancel;
        break;
    }

    sm_pActiveWriter = NULL;

    return result;
}

//////////////////////////////////////////////////////////////////////////
static void SaveTagDefinitionAndImports(IMannequinEditorManager* const pMannequinEditorManager, CMannequinFileChangeWriter* const pWriter, const CTagDefinition* const pTagdefinition)
{
    assert(pMannequinEditorManager);
    assert(pWriter);
    assert(pTagdefinition);

    pMannequinEditorManager->SaveTagDefinition(pWriter, pTagdefinition);

    DynArray<CTagDefinition*> includedTagDefs;
    pMannequinEditorManager->GetIncludedTagDefs(pTagdefinition, includedTagDefs);

    for (int i = 0; i < includedTagDefs.size(); ++i)
    {
        assert(includedTagDefs[i]);
        pMannequinEditorManager->SaveTagDefinition(pWriter, includedTagDefs[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::RefreshModifiedFiles()
{
    CMannequinDialog* const pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    if (!pMannequinDialog)
    {
        return;
    }

    m_modifiedFiles.clear();

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pMannequinEditorManager = mannequinSys.GetMannequinEditorManager();

    if (m_reexportAll)
    {
        // Save all
        pMannequinEditorManager->SaveAll(this);
    }
    else
    {
        // Only save files directly referenced by the current project
        const SMannequinContexts& mannequinContexts = *pMannequinDialog->Contexts();
        if (mannequinContexts.m_controllerDef)
        {
            // Controller def, fragment definition and global tags
            pMannequinEditorManager->SaveControllerDef(this, mannequinContexts.m_controllerDef);
            SaveTagDefinitionAndImports(pMannequinEditorManager, this, &mannequinContexts.m_controllerDef->m_fragmentIDs);
            SaveTagDefinitionAndImports(pMannequinEditorManager, this, &mannequinContexts.m_controllerDef->m_tags);

            // Fragment specific tags
            const TagID numFragmentIDs = mannequinContexts.m_controllerDef->m_fragmentIDs.GetNum();
            for (TagID itFragmentID = 0; itFragmentID < numFragmentIDs; ++itFragmentID)
            {
                const CTagDefinition* pFragmentTagDef = mannequinContexts.m_controllerDef->GetFragmentTagDef(itFragmentID);
                if (pFragmentTagDef)
                {
                    SaveTagDefinitionAndImports(pMannequinEditorManager, this, pFragmentTagDef);
                }
            }

            // Databases
            const uint32 numScopeContexts = mannequinContexts.m_contextData.size();
            for (uint32 itScopeContexts = 0; itScopeContexts != numScopeContexts; ++itScopeContexts)
            {
                const SScopeContextData& scopeContextData = mannequinContexts.m_contextData[itScopeContexts];
                if (scopeContextData.database)
                {
                    pMannequinEditorManager->SaveDatabase(this, scopeContextData.database);
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
size_t CMannequinFileChangeWriter::GetModifiedFilesCount() const
{
    return m_modifiedFiles.size();
}


//////////////////////////////////////////////////////////////////////////
const SFileEntry& CMannequinFileChangeWriter::GetModifiedFileEntry(const size_t index) const
{
    assert(index < m_modifiedFiles.size());

    return m_modifiedFiles[ index ];
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::SetModifiedFileType(SFileEntry& fileEntry) const
{
    switch (fileEntry.type)
    {
    case eFET_Database:
        fileEntry.typeDesc = "Animation Database";
        break;
    case eFET_ControllerDef:
        fileEntry.typeDesc = "Controller Definition";
        break;
    case eFET_TagDef:
        fileEntry.typeDesc = "Tag Definition";
        break;
    default:
        fileEntry.typeDesc = "Unknown Type";
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::WriteModifiedFiles()
{
    const size_t modifiedFilesCount = m_modifiedFiles.size();
    for (size_t i = 0; i < modifiedFilesCount; ++i)
    {
        SFileEntry& entry = m_modifiedFiles[ i ];
        entry.xmlNode->saveToFile((Path::GamePathToFullPath(entry.filename)).toUtf8().data());
    }
    m_modifiedFiles.clear();
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::UndoModifiedFile(const QString& filename)
{
    for (FileEntryVec::iterator it = m_modifiedFiles.begin(); it != m_modifiedFiles.end(); ++it)
    {
        if (it->filename == filename)
        {
            SFileEntry& entry = *it;

            IMannequinEditorManager* manager = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetMannequinEditorManager();

            switch (entry.type)
            {
            case eFET_Database:
                manager->RevertDatabase(entry.filename.toUtf8().data());
                break;
            case eFET_ControllerDef:
                manager->RevertControllerDef(entry.filename.toUtf8().data());
                break;
            case eFET_TagDef:
                manager->RevertTagDef(entry.filename.toUtf8().data());
                break;
            }

            m_modifiedFiles.erase(it);
            break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::AddEntry(const QString& filename, XmlNodeRef xmlNode, EFileEntryType fileEntryType)
{
    //if ( m_filterFilesByControllerDef )
    //{
    //  if ( m_pControllerDef )
    //  {
    //      IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    //      const bool fileUsedByControllerDef = mannequinSys.GetMannequinEditorManager()->IsFileUsedByControllerDef( *m_pControllerDef, filename );
    //      if ( ! fileUsedByControllerDef )
    //      {
    //          return;
    //      }
    //  }
    //}

    SFileEntry* pEntry = FindEntryByFilename(filename);
    if (pEntry)
    {
        pEntry->xmlNode = xmlNode;
        pEntry->type = fileEntryType;
        SetModifiedFileType(*pEntry);
        return;
    }

    SFileEntry entry;
    entry.filename = filename;
    entry.xmlNode = xmlNode;
    entry.type = fileEntryType;
    SetModifiedFileType(entry);

    m_modifiedFiles.push_back(entry);
}


//////////////////////////////////////////////////////////////////////////
SFileEntry* CMannequinFileChangeWriter::FindEntryByFilename(const QString& filename)
{
    const size_t modifiedFilesCount = m_modifiedFiles.size();
    for (size_t i = 0; i < modifiedFilesCount; ++i)
    {
        SFileEntry& entry = m_modifiedFiles[ i ];
        const bool filenamesMatch = (filename == entry.filename);
        if (filenamesMatch)
        {
            return &entry;
        }
    }

    return NULL;
}



//////////////////////////////////////////////////////////////////////////
bool CMannequinFileChangeWriter::SaveFile(const char* filename, XmlNodeRef xmlNode, QString& path)
{
    assert(filename);
    assert(xmlNode);

    path = filename;
    Path::ConvertBackSlashToSlash(path);

    const XmlNodeRef xmlNodeOnDisk = GetISystem()->LoadXmlFromFile(Path::GamePathToFullPath(path).toUtf8().data());
    if (!xmlNodeOnDisk)
    {
        return true;
    }

    IXmlUtils* pXmlUtils = GetISystem()->GetXmlUtils();
    assert(pXmlUtils);

    const QString hashXmlNode = pXmlUtils->HashXml(xmlNode);
    const QString hashXmlNodeOnDisk = pXmlUtils->HashXml(xmlNodeOnDisk);
    const bool fileContentsMatch = (hashXmlNode == hashXmlNodeOnDisk);

    return !fileContentsMatch;
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::SaveFile(const char* filename, XmlNodeRef xmlNode, EFileEntryType fileEntryType)
{
    QString path;
    if (SaveFile(filename, xmlNode, path))
    {
        AddEntry(path, xmlNode, fileEntryType);
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::SetFilterFilesByControllerDef(bool filterFilesByControllerDef)
{
    m_filterFilesByControllerDef = filterFilesByControllerDef;
}


//////////////////////////////////////////////////////////////////////////
bool CMannequinFileChangeWriter::GetFilterFilesByControllerDef() const
{
    return m_filterFilesByControllerDef;
}


//////////////////////////////////////////////////////////////////////////
void CMannequinFileChangeWriter::SetControllerDef(const SControllerDef* pControllerDef)
{
    m_pControllerDef = pControllerDef;
}