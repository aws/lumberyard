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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_HELPER_MANNEQUINFILECHANGEWRITER_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_HELPER_MANNEQUINFILECHANGEWRITER_H
#pragma once


#include <ICryMannequinEditor.h>
#include "../MannFileManager.h"

enum EFileManagerResponse
{
    eFMR_OK, eFMR_Cancel, eFMR_NoChanges
};

struct SFileEntry
{
    XmlNodeRef xmlNode;
    QString filename;
    QString typeDesc;
    EFileEntryType type;
};

class CMannequinFileChangeWriter
    : public IMannequinWriter
{
public:
    CMannequinFileChangeWriter(bool changedFilesMode = false);

    size_t GetModifiedFilesCount() const;
    const SFileEntry& GetModifiedFileEntry(const size_t index) const;

    void WriteModifiedFiles();
    void UndoModifiedFile(const QString& filename);

    bool SaveFile(const char* filename, XmlNodeRef xmlNode, QString& path);

    // IMannequinWriter
    virtual void SaveFile(const char* filename, XmlNodeRef xmlNode, EFileEntryType fileEntryType);
    // ~IMannequinWriter

    void SetFilterFilesByControllerDef(bool filterFilesByControllerDef);
    bool GetFilterFilesByControllerDef() const;

    void SetControllerDef(const SControllerDef* pControllerDef);

    EFileManagerResponse ShowFileManager(bool reexportAll = false);
    void RefreshModifiedFiles();

    static bool UpdateActiveWriter()
    {
        if (sm_pActiveWriter)
        {
            sm_pActiveWriter->RefreshModifiedFiles();
            sm_pActiveWriter->m_fileManager.OnRefresh();

            return true;
        }
        return false;
    }

private:
    void AddEntry(const QString& filename, XmlNodeRef xmlNode, EFileEntryType fileEntryType);
    SFileEntry* FindEntryByFilename(const QString& filename);
    void SetModifiedFileType(SFileEntry& fileEntry) const;

private:

    CMannFileManager m_fileManager;
    typedef std::vector< SFileEntry > FileEntryVec;
    FileEntryVec m_modifiedFiles;
    const SControllerDef* m_pControllerDef;
    bool m_filterFilesByControllerDef;
    bool m_changedFilesMode;
    bool m_reexportAll;

    static CMannequinFileChangeWriter* sm_pActiveWriter;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_HELPER_MANNEQUINFILECHANGEWRITER_H
