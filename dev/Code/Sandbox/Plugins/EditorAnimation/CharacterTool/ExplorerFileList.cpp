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

#include "pch.h"
#include "ExplorerFileList.h"
#include "EntryListImpl.h"
#include <IBackgroundTaskManager.h>
#include <IEditorFileMonitor.h>
#include <IEditor.h>
#include <Serialization/IArchiveHost.h>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include "Util/PathUtil.h"
#include "ScanAndLoadFilesTask.h"
#include "Explorer.h"
#include "Expected.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"
#include "../EditorCommon/BatchFileDialog.h"

namespace CharacterTool
{
    string EntryFormat::MakeFilename(const char* entryPath) const
    {
        return path.empty() ? PathUtil::ReplaceExtension(entryPath, extension) : path;
    }

    static bool LoadFile(vector<char>* buf, const char* filename, LoaderContext* context)
    {
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, "rb");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return false;
        }

        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_END);
        size_t size = gEnv->pCryPak->FTell(fileHandle);
        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_SET);

        buf->resize(size);
        bool result = gEnv->pCryPak->FRead(&(*buf)[0], size, fileHandle) == size;
        gEnv->pCryPak->FClose(fileHandle);
        return result;
    }

    bool JSONLoader::Load(EntryBase* entry, const char* filename, LoaderContext* context)
    {
        string filePath = (Path::GetEditingGameDataFolder() + "\\" + filename).c_str();
        filePath.replace('/', '\\');

        vector<char> content;
        if (!LoadFile(&content, filePath.c_str(), context))
        {
            return false;
        }

        Serialization::JSONIArchive ia;
        if (content.empty())
        {
            return true;
        }

        if (!ia.open(content.data(), content.size()))
        {
            return false;
        }

        ia(*entry, "");
        return true;
    }

    bool JSONLoader::Save(EntryBase* entry, const char* filename, LoaderContext* context, string&)
    {
        Serialization::JSONOArchive oa;
        oa(*entry, "");

        string filePath = (Path::GetEditingGameDataFolder() + "\\" + filename).c_str();
        filePath.replace('/', '\\');
        return oa.save(filePath.c_str());
    }

    // ---------------------------------------------------------------------------

    ExplorerFileList::~ExplorerFileList()
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            delete m_entryTypes[i].entryList;
            m_entryTypes[i].entryList = 0;
        }

        GetIEditor()->GetFileMonitor()->UnregisterListener(this);
    }

    EntryType& ExplorerFileList::AddEntryType(EntryListBase* list, int subtree, int explorerEntryType, IEntryDependencyExtractor* extractor)
    {
        EntryType type;
        type.entryList = list;
        type.subtree = subtree;
        type.explorerEntryType = explorerEntryType;
        type.dependencyExtractor.reset(extractor);

        m_entryTypes.push_back(type);

        if (m_entryTypes.size() > 1)
        {
            m_entryTypes.back().entryList->SetIdProvider(m_entryTypes[0].entryList);
        }

        return m_entryTypes.back();
    }

    EntryType* ExplorerFileList::AddSingleFileEntryType(EntryListBase* list, const char* path, const char* label, int subtree, int explorerEntryType, IEntryLoader* loader)
    {
        EntryType entryType;
        entryType.entryList = list;
        entryType.subtree = subtree;
        entryType.explorerEntryType = explorerEntryType;

        EntryFormat format;
        format.path = path;
        format.loader = loader;
        format.usage = FORMAT_LOAD | FORMAT_SAVE | FORMAT_LIST;
        entryType.formats.push_back(format);

        m_entryTypes.push_back(entryType);

        if (m_entryTypes.size() > 1)
        {
            m_entryTypes.back().entryList->SetIdProvider(m_entryTypes[0].entryList);
        }

        return &m_entryTypes.back();
    }

    void ExplorerFileList::Populate()
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            EntryType& entryType = m_entryTypes[i];

            bool singleFileFormat = false;
            if (!entryType.fileListenerRegistered)
            {
                for (size_t j = 0; j < entryType.formats.size(); ++j)
                {
                    const EntryFormat& format = entryType.formats[j];
                    if (!format.path.empty())
                    {
                        GetIEditor()->GetFileMonitor()->RegisterListener(this, format.path.c_str());
                        singleFileFormat = true;
                    }
                    else
                    {
                        GetIEditor()->GetFileMonitor()->RegisterListener(this, "", format.extension);
                    }
                }
                entryType.fileListenerRegistered = true;
            }

            if (singleFileFormat)
            {
                continue;
            }

            entryType.entryList->Clear();
            SignalSubtreeReset(entryType.subtree);

            string firstExtension;
            SLookupRule r;
            for (size_t j = 0; j < entryType.formats.size(); ++j)
            {
                const EntryFormat& format = entryType.formats[j];
                if (!format.path.empty())
                {
                    continue;
                }
                if ((format.usage & FORMAT_LIST) == 0)
                {
                    continue;
                }
                r.masks.push_back(string("*.") + format.extension);
                if (firstExtension.empty())
                {
                    firstExtension = format.extension;
                }
            }
            string description = firstExtension.empty() ? string() : (firstExtension + " scan");
            SScanAndLoadFilesTask* task = new SScanAndLoadFilesTask(r, description.c_str(), entryType.subtree);
            EXPECTED(connect(task, SIGNAL(SignalFileLoaded(const ScanLoadedFile&)), this, SLOT(OnBackgroundFileLoaded(const ScanLoadedFile&))));
            EXPECTED(connect(task, SIGNAL(SignalLoadingFinished(int)), this, SLOT(OnBackgroundLoadingFinished(int))));
            GetIEditor()->GetBackgroundTaskManager()->AddTask(task, eTaskPriority_FileUpdate, eTaskThreadMask_IO);
        }
    }

    EntryType* ExplorerFileList::GetEntryTypeByExtension(const char* ext)
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            for (size_t j = 0; j < m_entryTypes[i].formats.size(); ++j)
            {
                if (stricmp(m_entryTypes[i].formats[j].extension, ext) == 0)
                {
                    return &m_entryTypes[i];
                }
            }
        }
        return 0;
    }

    const EntryType* ExplorerFileList::GetEntryTypeByExtension(const char* extension) const
    {
        return const_cast<ExplorerFileList*>(this)->GetEntryTypeByExtension(extension);
    }

    EntryType* ExplorerFileList::GetEntryTypeByPath(const char* singleFilePath)
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            for (size_t j = 0; j < m_entryTypes[i].formats.size(); ++j)
            {
                if (stricmp(m_entryTypes[i].formats[j].path.c_str(), singleFilePath) == 0)
                {
                    return &m_entryTypes[i];
                }
            }
        }
        return 0;
    }

    const EntryType* ExplorerFileList::GetEntryTypeByPath(const char* path) const
    {
        return const_cast<ExplorerFileList*>(this)->GetEntryTypeByPath(path);
    }

    EntryType* ExplorerFileList::GetEntryTypeByExplorerEntryType(int explorerEntryType)
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            if (m_entryTypes[i].explorerEntryType == explorerEntryType)
            {
                return &m_entryTypes[i];
            }
        }
        return 0;
    }

    void ExplorerFileList::AddAndSaveEntry(const char* filename, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput, AZ::SaveCompleteCallback onSaveComplete)
    {
        unsigned int id = AddEntry(filename, true).id;
        if (!id)
        {
            if (onSaveComplete)
            {
                onSaveComplete(false);
            }

            return;
        }

        SaveEntry(actionOutput, id, onSaveComplete);
    }

    void ExplorerFileList::SetExplorer(Explorer* explorer)
    {
        m_explorer = explorer;
        m_explorerColumnPak = explorer->FindColumn("Pak");
    }

    void ExplorerFileList::SetAssetWatchType(AZ::Crc32 type)
    {
        BusConnect(type);
    }

    ExplorerEntryId ExplorerFileList::AddEntry(const char* filename, bool resetIfExists)
    {
        const char* ext = PathUtil::GetExt(filename);
        const EntryType* format = GetEntryTypeByExtension(ext);
        if (!format)
        {
            return ExplorerEntryId();
        }

        bool newEntry;
        EntryBase* entry = format->entryList->AddEntry(&newEntry, filename, 0, resetIfExists);

        if (newEntry)
        {
            if (entry)
            {
                AZStd::string fullPath = Path::GamePathToFullPath(filename).toUtf8().data();
                if (!gEnv->pCryPak->IsFileExist(fullPath.c_str()))
                {
                    entry->pendingSave = true;
                }
            }

            SignalEntryAdded(format->subtree, entry->id);
        }
        else
        {
            EntryModifiedEvent ev;
            ev.subtree = format->subtree;
            ev.id = entry->id;
            SignalEntryModified(ev);
        }
        return ExplorerEntryId(format->subtree, entry->id);
    }

    EntryBase* ExplorerFileList::GetEntryBaseByPath(const char* path)
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            EntryBase* entry = m_entryTypes[i].entryList->GetBaseByPath(path);
            if (entry)
            {
                return entry;
            }
        }
        return 0;
    }

    EntryBase* ExplorerFileList::GetEntryBaseById(unsigned int id)
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            EntryBase* entry = m_entryTypes[i].entryList->GetBaseById(id);
            if (entry)
            {
                return entry;
            }
        }
        return 0;
    }

    void ExplorerFileList::OnBackgroundLoadingFinished(int subtree)
    {
        SignalSubtreeLoadingFinished(subtree);
    }

    string ExplorerFileList::GetCanonicalPath(const char* path) const
    {
        const char* canonicalExtension = CanonicalExtension();
        if (canonicalExtension[0] != '\0')
        {
            return PathUtil::ReplaceExtension(path, canonicalExtension);
        }
        else
        {
            return path;
        }
    }

    void ExplorerFileList::OnBackgroundFileLoaded(const ScanLoadedFile& file)
    {
        string path = GetCanonicalPath(file.scannedFile.c_str());
        ExplorerEntryId entryId = AddEntry(path.c_str(), false);

        int pakState = 0;
        if (file.fromPak)
        {
            pakState |= PAK_STATE_PAK;
        }
        if (file.fromDisk)
        {
            pakState |= PAK_STATE_LOOSE_FILES;
        }
        UpdateEntryPakState(entryId, pakState);
    }

    int ExplorerFileList::GetEntryCount(int subtree) const
    {
        int result = 0;
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            if (m_entryTypes[i].subtree == subtree)
            {
                result += m_entryTypes[i].entryList->Count();
            }
        }
        return result;
    }

    unsigned int ExplorerFileList::GetEntryIdByIndex(int subtree, int index) const
    {
        int startRange = 0;
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            if (m_entryTypes[i].subtree == subtree)
            {
                int size = m_entryTypes[i].entryList->Count();
                if (index >= startRange && index < startRange + size)
                {
                    if (EntryBase* entry = m_entryTypes[i].entryList->GetBaseByIndex(index - startRange))
                    {
                        return entry->id;
                    }
                    else
                    {
                        return 0;
                    }
                }
                startRange += size;
            }
        }

        return 0;
    }

    int ExplorerFileList::GetEntryType(int subtreeIndex) const
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            if (m_entryTypes[i].subtree == subtreeIndex)
            {
                return m_entryTypes[i].explorerEntryType;
            }
        }

        return 0;
    }

    void ExplorerFileList::SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete)
    {
        bool failed = false;
        m_isRunningSaveAll = true;
        for (size_t j = 0; j < m_entryTypes.size(); ++j)
        {
            EntryType& format = m_entryTypes[j];
            for (size_t i = 0; i < format.entryList->Count(); ++i)
            {
                EntryBase* entry = format.entryList->GetBaseByIndex(i);
                if (!entry || !entry->modified)
                {
                    continue;
                }

                SaveEntry(output, entry->id, static_cast<AZ::SaveCompleteCallback>(0));
            }
        }

        m_isRunningSaveAll = false;
        if (m_saveRunner)
        {
            m_saveRunner->Run(output,
                [this, onSaveComplete](bool success)
                {
                    if (onSaveComplete)
                    {
                        onSaveComplete(success);
                    }

                    m_saveRunner = AZStd::shared_ptr<AZ::AsyncSaveRunner>();
                }, AZ::AsyncSaveRunner::ControllerOrder::Random);
        }
        else if (onSaveComplete)
        {
            onSaveComplete(true);
        }
    }

    string ExplorerFileList::GetSaveFileExtension(const char* entryPath) const
    {
        string extension = PathUtil::GetExt(entryPath);
        const EntryType* entryType = GetEntryTypeByExtension(extension.c_str());
        if (!entryType)
        {
            return string();
        }

        for (size_t j = 0; j < entryType->formats.size(); ++j)
        {
            const EntryFormat& format = entryType->formats[j];
            if (format.usage & FORMAT_SAVE && !format.extension.empty())
            {
                return format.extension;
            }
        }

        return string();
    }

    bool ExplorerFileList::CanBeSaved(const char* entryPath) const
    {
        string extension = PathUtil::GetExt(entryPath);
        if (const EntryType* entryType = GetEntryTypeByExtension(extension.c_str()))
        {
            for (size_t j = 0; j < entryType->formats.size(); ++j)
            {
                const EntryFormat& format = entryType->formats[j];
                if (format.usage & FORMAT_SAVE)
                {
                    if (!format.extension.empty())
                    {
                        return true;
                    }
                }
            }
        }

        if (const EntryType* entryType = GetEntryTypeByPath(entryPath))
        {
            for (size_t j = 0; j < entryType->formats.size(); ++j)
            {
                const EntryFormat& format = entryType->formats[j];
                if (stricmp(format.path.c_str(), entryPath) == 0 && format.usage & FORMAT_SAVE)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void ExplorerFileList::SaveEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, AZ::SaveCompleteCallback onSaveComplete)
    {
        const EntryType* entryType;
        EntryBase* entry = GetEntry(id, &entryType);
        if (!entry)
        {
            if (output)
            {
                output->AddError("Saving nonexistant entry", "");
            }
            if (onSaveComplete)
            {
                onSaveComplete(false);
            }

            return;
        }

        if (!entry->loaded)
        {
            LoadOrGetChangedEntry(id);
        }

        if (IsRunningAsyncSaveOperation() && !m_isRunningSaveAll)
        {
            if (output)
            {
                output->AddError("Geppetto - SaveEntry failed, async save operation is already running", entry->name.c_str());
            }

            if (onSaveComplete)
            {
                onSaveComplete(false);
            }

            return;
        }

        if (!m_saveRunner)
        {
            m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();
        }

        AZStd::shared_ptr<AZ::SaveOperationController> saveEntryController = m_saveRunner->GenerateController();
        for (size_t i = 0; i < entryType->formats.size(); ++i)
        {
            const EntryFormat& format = entryType->formats[i];
            if (!format.loader.get())
            {
                continue;
            }
            if (!(format.usage & FORMAT_SAVE))
            {
                continue;
            }
            string filename = format.MakeFilename(entry->path.c_str());
            filename = Path::GamePathToFullPath(filename.c_str()).toUtf8().data();
            saveEntryController->AddSaveOperation(filename.c_str(),
                [entry, &format, this](const AZStd::string& fullPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
                {
                    string errorString;
                    if (!format.loader->Save(entry, fullPath.c_str(), m_loaderContext, errorString))
                    {
                        actionOutput->AddError("Save Errors", errorString.c_str());
                        return false;
                    }
                    return true;
                }
            );

            saveEntryController->SetOnCompleteCallback(
                [this, entryType, entry, id](bool success)
                {
                    if (!success)
                    {
                        return;
                    }

                    if (entryType->entryList->EntrySaved(entry))
                    {
                        EntryModifiedEvent ev;
                        ev.subtree = entryType->subtree;
                        ev.id = id;
                        SignalEntryModified(ev);
                    }

                    int pakState = GetEntryFilesPakState(*entryType, entry->path.c_str());
                    UpdateEntryPakState(ExplorerEntryId(entryType->subtree, id), pakState);
                }
            );
        }

        // If we aren't running save all, then this is the end, so run all the save operations
        if (!m_isRunningSaveAll)
        {
            m_saveRunner->Run(output,
                [this, onSaveComplete](bool success)
                {
                    if (onSaveComplete)
                    {
                        onSaveComplete(success);
                    }

                    m_saveRunner = AZStd::shared_ptr<AZ::AsyncSaveRunner>();
                }, AZ::AsyncSaveRunner::ControllerOrder::Random);
        }
    }
    
    void ExplorerFileList::DeleteEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, const char * entryFullPath, const char * entryPath)
    {
        if (IsRunningAsyncSaveOperation() && !m_isRunningSaveAll)
        {
            if (output)
            {
                output->AddError("Geppetto - Delete Entry failed, async save operation is already running %s", entryFullPath);
            }
            return;
        }

        if (!m_saveRunner)
        {
            m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();
        }

        AZStd::shared_ptr<AZ::SaveOperationController> saveEntryController = m_saveRunner->GenerateController();
       
        string fileName = entryFullPath;
        string fullFileName = Path::GamePathToFullPath(fileName.c_str()).toUtf8().data();
        saveEntryController->AddDeleteOperation(fullFileName.c_str(),
            [this](const AZStd::string& fullPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
            {
                if (gEnv->pCryPak->IsFileExist(fullPath.c_str(), ICryPak::eFileLocation_OnDisk))
                {
                    QFile::remove(QString::fromLocal8Bit(fullPath.c_str()));
                }
                return true;
            }
        );

        saveEntryController->SetOnCompleteCallback(
            [this, entryPath](bool success)
        {
            if (!success)
            {
                return;
            }

            SignalEntryDeleted(entryPath);
        }
        );
        

        // If we aren't running save all, then this is the end, so run all the save operations
        if (!m_isRunningSaveAll)
        {
           m_saveRunner = AZStd::shared_ptr<AZ::AsyncSaveRunner>();
        }
    }

    string ExplorerFileList::GetSaveFilename(unsigned int id)
    {
        EntryBase* entry = GetEntry(id);
        if (!entry)
        {
            return string();
        }

        return entry->path;
    }

    int ExplorerFileList::GetEntryFilesPakState(const EntryType& entryType, const char* filename)
    {
        int pakState = 0;
        for (size_t i = 0; i < entryType.formats.size(); ++i)
        {
            const char* extension = entryType.formats[i].extension.c_str();

            if (!extension[0])
            {
                continue;
            }

            string assetFile = PathUtil::ReplaceExtension(filename, extension);
            if (gEnv->pCryPak->IsFileExist(assetFile, ICryPak::eFileLocation_OnDisk))
            {
                pakState |= PAK_STATE_LOOSE_FILES;
            }
            if (gEnv->pCryPak->IsFileExist(assetFile, ICryPak::eFileLocation_InPak))
            {
                pakState |= PAK_STATE_PAK;
            }
        }
        return pakState;
    }

    void ExplorerFileList::RevertEntry(unsigned int id)
    {
        const EntryType* entryType;
        EntryBase* entry = GetEntry(id, &entryType);
        if (entry)
        {
            bool listedFileExists = false;
            for (size_t i = 0; i < entryType->formats.size(); ++i)
            {
                const EntryFormat& format = entryType->formats[i];
                if ((format.usage & FORMAT_LIST) == 0)
                {
                    continue;
                }

                string filename = format.MakeFilename(entry->path.c_str());
                if (gEnv->pCryPak->IsFileExist(filename))
                {
                    listedFileExists = true;
                }
            }

            if (!listedFileExists)
            {
                if (entryType->entryList->RemoveById(id))
                {
                    SignalEntryRemoved(entryType->subtree, id);
                }
                return;
            }

            entry->failedToLoad = false;
            for (size_t i = 0; i < entryType->formats.size(); ++i)
            {
                const EntryFormat& format = entryType->formats[i];

                string filename = format.MakeFilename(entry->path.c_str());
                bool fileExists = gEnv->pCryPak->IsFileExist(entry->path.c_str());
                if (!format.loader)
                {
                    continue;
                }
                if (!format.loader->Load(entry, filename.c_str(), m_loaderContext))
                {
                    entry->failedToLoad = true;
                }
            }
            entry->StoreSavedContent();

            EntryModifiedEvent ev;
            if (entryType->entryList->EntryReverted(&ev.previousContent, entry))
            {
                ev.subtree = entryType->subtree;
                ev.id = entry->id;
                ev.reason = "Revert";
                ev.contentChanged = ev.previousContent != entry->lastContent;
                SignalEntryModified(ev);
            }

            int pakState = GetEntryFilesPakState(*entryType, entry->path.c_str());
            UpdateEntryPakState(ExplorerEntryId(entryType->subtree, id), pakState);
        }
    }

    EntryBase* ExplorerFileList::GetEntry(EntryId id, const EntryType** outFormat) const
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            const EntryType& format = m_entryTypes[i];

            if (EntryBase* entry = format.entryList->GetBaseById(id))
            {
                if (outFormat)
                {
                    *outFormat = &format;
                }
                return entry;
            }
        }
        return 0;
    }

    void ExplorerFileList::CheckIfModified(unsigned int id, const char* reason, bool continuousChange)
    {
        const EntryType* format;
        EntryBase* entry = GetEntry(id, &format);
        if (!entry)
        {
            return;
        }
        EntryModifiedEvent ev;
        ev.continuousChange = continuousChange;
        if (continuousChange || format->entryList->EntryChanged(&ev.previousContent, entry))
        {
            ev.subtree = format->subtree;
            ev.id = entry->id;
            if (reason)
            {
                ev.reason = reason;
                ev.contentChanged = true;
            }
            SignalEntryModified(ev);
        }
    }

    void ExplorerFileList::GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer)
    {
        if (EntryBase* entry = GetEntry(id))
        {
            bool canBeSaved = CanBeSaved(entry->path.c_str());
            actions->push_back(ExplorerAction("Revert", 0, [=](ActionContext& x) { explorer->ActionRevert(x); }, "Editor/Icons/animation/revert.png"));
            if (canBeSaved)
            {
                actions->push_back(ExplorerAction("Save", 0, [=](ActionContext& x) { explorer->ActionSave(x); }, "Editor/Icons/animation/save.png"));
            }
            if (canBeSaved && !GetSaveFileExtension(entry->path).empty())
            {
                actions->push_back(ExplorerAction("Save As...", ACTION_NOT_STACKABLE, [&](ActionContext& x) { ActionSaveAs(x); }, "", ""));
            }
            actions->push_back(ExplorerAction());

            AZStd::string fullPathString = Path::MakeModPathFromGamePath(entry->path.c_str());
            if (gEnv->pCryPak->IsFileExist(fullPathString.c_str()))
            {
                if (canBeSaved)
                {
                    actions->push_back(ExplorerAction("Delete", 0, [=](ActionContext& x) { ActionDelete(x); }));
                }
            
                actions->push_back(ExplorerAction());
                actions->push_back(ExplorerAction("Show in Explorer", ACTION_NOT_STACKABLE, [=](ActionContext& x) { explorer->ActionShowInExplorer(x); }, "Editor/Icons/animation/show_in_explorer.png"));
            }
        }
    }

    void ExplorerFileList::UpdateEntry(ExplorerEntry* explorerEntry)
    {
        if (EntryBase* entry = GetEntry(explorerEntry->id))
        {
            explorerEntry->name = entry->name;
            explorerEntry->path = entry->path;
            explorerEntry->modified = entry->modified;
        }
    }

    bool ExplorerFileList::GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const
    {
        if (EntryBase* entry = GetEntry(id))
        {
            *out = Serialization::SStruct(*entry);
            return true;
        }
        return false;
    }

    static void LoadEntryOfType(EntryBase* entry, LoaderContext* loaderContext, const EntryType* entryType)
    {
        entry->loaded = true;
        entry->failedToLoad = false;
        for (size_t i = 0; i < entryType->formats.size(); ++i)
        {
            const EntryFormat& format = entryType->formats[i];
            if (!(format.usage & FORMAT_LOAD))
            {
                continue;
            }
            if (!format.loader)
            {
                continue;
            }
            string filename = format.MakeFilename(entry->path.c_str());
            if (!format.loader->Load(entry, filename.c_str(), loaderContext))
            {
                entry->failedToLoad = true;
            }
        }
    }

    static bool ListedFilesExist(const EntryBase* entry, const EntryType* entryType)
    {
        for (size_t i = 0; i < entryType->formats.size(); ++i)
        {
            const EntryFormat& format = entryType->formats[i];
            if (!(format.usage & FORMAT_LIST))
            {
                continue;
            }
            string filename = format.MakeFilename(entry->path.c_str());
            if (gEnv->pCryPak->IsFileExist(filename.c_str()))
            {
                return true;
            }
        }
        return false;
    }

    static bool FileExistsInUsageLocation(const char* filename, const string& extension, const EntryType* entryType)
    {
        for (size_t i = 0; i < entryType->formats.size(); ++i)
        {
            if (entryType->formats[i].extension == extension || entryType->formats[i].path == filename)
            {
                AZStd::string fullPath;
                if (entryType->formats[i].usage & FORMAT_SAVE)
                {
                    fullPath = Path::MakeModPathFromGamePath(filename);
                }
                else
                {
                    fullPath = Path::GamePathToFullPath(filename).toUtf8().data();
                }

                return gEnv->pCryPak->IsFileExist(fullPath.c_str());
            }
        }

        return false;
    }

    void ExplorerFileList::OnFileChange(const char* filename, EChangeType eType)
    {
        EntryType* entryType = GetEntryTypeByPath(filename);
        const string originalExt = PathUtil::GetExt(filename);
        if (!entryType)
        {
            entryType = GetEntryTypeByExtension(originalExt);
        }

        if (!entryType)
        {
            return;
        }

        bool fileExists = FileExistsInUsageLocation(filename, originalExt, entryType);

        string listedPath = GetCanonicalPath(filename);
        listedPath.replace('\\', '/');
        if (fileExists)
        {
            ExplorerEntryId entryId;

            EntryBase* entry = entryType->entryList->GetBaseByPath(listedPath.c_str());
            if (!entry)
            {
                entryId = AddEntry(listedPath.c_str(), false);

                entry = entryType->entryList->GetBaseByPath(listedPath.c_str());
            }

            if (entry && entry->loaded)
            {
                if (!entry->modified)
                {
                    LoadEntryOfType(entry, m_loaderContext, entryType);

                    EntryModifiedEvent ev;
                    if (entryType->entryList->EntryChanged(&ev.previousContent, entry))
                    {
                        ev.subtree = entryType->subtree;
                        ev.id = entry->id;
                        ev.reason = "Reload";
                        ev.contentChanged = true;
                        entry->modified = false;
                        SignalEntryModified(ev);
                    }
                }
            }
            entryId = ExplorerEntryId(entryType->subtree, entry->id);
            int pakState = GetEntryFilesPakState(*entryType, listedPath.c_str());
            UpdateEntryPakState(entryId, pakState);
        }
        else
        {
            EntryBase* entry = entryType->entryList->GetBaseByPath(listedPath.c_str());
            if (entry && !entry->modified && !entry->pendingSave && !ListedFilesExist(entry, entryType))
            {
                unsigned int id = entry->id;
                if (entryType->entryList->RemoveById(id))
                {
                    SignalEntryRemoved(entryType->subtree, id);
                }
            }
        }
    }

    //event generated from Asset Catalog system
    //fbx for example will not have the .chr in the development folder.
    void ExplorerFileList::OnFileChanged(AZStd::string assetPath)
    {
        const char* filename = assetPath.c_str();
        OnFileChange(filename, eChangeType_Unknown);
    }

    bool ExplorerFileList::HasBackgroundLoading() const
    {
        for (size_t i = 0; i < m_entryTypes.size(); ++i)
        {
            for (size_t j = 0; j < m_entryTypes[i].formats.size(); ++j)
            {
                if (m_entryTypes[i].formats[j].path.empty())
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ExplorerFileList::LoadOrGetChangedEntry(unsigned int id)
    {
        const EntryType* entryType = 0;
        if (EntryBase* entry = GetEntry(id, &entryType))
        {
            if (!entry->loaded)
            {
                LoadEntryOfType(entry, m_loaderContext, entryType);
                entry->StoreSavedContent();
                entry->lastContent = entry->savedContent;
            }
            return true;
        }
        return false;
    }

    const char* ExplorerFileList::CanonicalExtension() const
    {
        const EntryType& entryType = m_entryTypes[0];
        for (size_t i = 0; i < entryType.formats.size(); ++i)
        {
            const EntryFormat& format = entryType.formats[i];
            if (format.usage & FORMAT_MAIN)
            {
                return format.extension.c_str();
            }
        }
        return "";
    }


    void ExplorerFileList::UpdateEntryPakState(const ExplorerEntryId& id, int state)
    {
        if (m_explorer)
        {
            m_explorer->SetEntryColumn(id, m_explorerColumnPak, state, true);
        }
    }

    void ExplorerFileList::GetDependencies(vector<string>* paths, unsigned int id)
    {
        const EntryType* format = 0;
        EntryBase* entry = GetEntry(id, &format);
        if (!entry)
        {
            return;
        }
        if (format->dependencyExtractor)
        {
            format->dependencyExtractor->Extract(paths, entry);
        }
    }

    bool ExplorerFileList::IsRunningAsyncSaveOperation()
    {
        return m_saveRunner != nullptr;
    }

    void ExplorerFileList::ActionSaveAs(ActionContext& x)
    {
        if (x.entries.size() != 1)
        {
            return;
        }
        ExplorerEntry* entry = x.entries[0];
        string saveExtension = GetSaveFileExtension(entry->path.c_str());
        if (saveExtension.empty())
        {
            return;
        }

        QString filter = QString("(*.") + saveExtension.c_str() + ")";
        QString directory = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str()) + "/" + QString::fromLocal8Bit(entry->path.c_str());
        QString absoluteFilename = QFileDialog::getSaveFileName(x.window, "Save As...", directory, filter, &filter).toLocal8Bit().data();

        if (absoluteFilename.isEmpty())
        {
            return;
        }

        QDir dir(QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str()));

        string filename = dir.relativeFilePath(absoluteFilename).toLocal8Bit().data();
        if (filename.empty())
        {
            return;
        }

        MemoryOArchive oa;
        Serialization::SStruct existingSerializer;
        GetEntrySerializer(&existingSerializer, entry->id);
        oa(existingSerializer, "");
        // Note that AddEntry could reset content of entry pointed by
        // "entry" pointer.
        unsigned int newEntryId = AddEntry(filename.c_str(), true).id;

        Serialization::SStruct newSerializer;
        GetEntrySerializer(&newSerializer, newEntryId);

        MemoryIArchive ia;
        ia.open(oa.buffer(), oa.length());
        ia(newSerializer, "");

        EntryBase* newEntry = GetEntry(newEntryId, 0);
        newEntry->loaded = true;

        SaveEntry(x.output, newEntryId,
            [this, entry, filename](bool success)
            {
                if (success)
                {
                    SignalEntrySavedAs(entry->path.c_str(), filename.c_str());
                }
            }
        );
    }

    void ExplorerFileList::ActionDelete(ActionContext& x)
    {
        DeleteEntries(x);
        return;
    }

    void ExplorerFileList::DeleteEntries(ActionContext& x)
    {
        std::vector<string> gamePaths;
        for (size_t i = 0; i < x.entries.size(); ++i)
        {
            gamePaths.push_back(x.entries[i]->path);
        }

        std::vector<string> entryPaths;
        std::vector<string> realPaths;
        std::vector<int> gamePathByRealPath;

        // TODO: Needs revisiting after UX pass since this does not currently support Gems
        string gameFolder = Path::GetEditingGameDataFolder().c_str();
        gameFolder += "\\";
        for (size_t i = 0; i < gamePaths.size(); ++i)
        {
            const string& gamePath = gamePaths[i];
            string modDirectory = gameFolder;
            int modIndex = 0;
            do
            {
                string path = modDirectory + gamePath;
                path.replace('/', '\\');

                if (QFile::exists(QString::fromLocal8Bit(path.c_str())))
                {
                    entryPaths.push_back(gamePath);
                    realPaths.push_back(path);
                    gamePathByRealPath.push_back(i);
                }

                const char* modSubDirectory = gEnv->pCryPak->GetMod(modIndex);
                if (modSubDirectory && modSubDirectory[0] != '\0')
                {
                    modDirectory = gameFolder + gEnv->pCryPak->GetMod(modIndex);
                }
                else
                {
                    modDirectory = "";
                }
                ++modIndex;
            } while (modDirectory.length());
        }

        if (realPaths.empty())
        {
            QMessageBox::warning(x.window, "Warning", "Selected assets are stored in paks and can not be removed.");
            return;
        }

        SBatchFileSettings settings;
        settings.title = "Delete Assets";
        settings.descriptionText = "Are you sure you want to delete selected assets?";
        settings.explicitFileList.resize(realPaths.size());
        settings.readonlyList = false;
        for (size_t i = 0; i < realPaths.size(); ++i)
        {
            settings.explicitFileList[i] = realPaths[i];
        }
        settings.scanExtension = "";

        m_isRunningSaveAll = true;

        Serialization::StringList filenamesToRemove;
        if (ShowBatchFileDialog(&filenamesToRemove, settings, x.window))
        {
            for (size_t i = 0; i < filenamesToRemove.size(); ++i)
            {
                for (size_t entryIndex = 0; entryIndex < entryPaths.size(); ++entryIndex)
                {
                    if (realPaths[entryIndex] == filenamesToRemove[i])
                    {
                        DeleteEntry(x.output, filenamesToRemove[i].c_str(), entryPaths[entryIndex]);
                        break;
                    }
                }

            }
        }

        m_isRunningSaveAll = false;
        if (m_saveRunner)
        {
            m_saveRunner->Run(x.output,
                [this](bool success)
            {
                m_saveRunner = AZStd::shared_ptr<AZ::AsyncSaveRunner>();
            }, AZ::AsyncSaveRunner::ControllerOrder::Random);
        }

    }
}

#include <CharacterTool/ExplorerFileList.moc>
