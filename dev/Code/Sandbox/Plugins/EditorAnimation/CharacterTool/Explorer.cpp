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
#include "Strings.h"
#include "Explorer.h"
#include "ExplorerDataProvider.h"
#include "Serialization.h"
#include "IEditor.h"
#include "Util/PathUtil.h"
#include "EditorDBATable.h"
#include "EditorCompressionPresetTable.h"
#include "SkeletonList.h"
#include "UndoStack.h"
#include "functor.h"
#include "Expected.h"
#include "DependencyManager.h"
#include <AzCore/Casting/numeric_cast.h>

#include <QDir>
#include <QMimeData>
#include <QMessageBox>
#include <QProcess>

#if 0
#define TRACE_EXPLORER(fmt, ...) { char buf[1024] = ""; sprintf_s(buf, fmt "\n", __VA_ARGS__); OutputDebugStringA(buf); }
#else
#define TRACE_EXPLORER(...)
#endif

enum ECompressionEntryID
{
    COMPRESSION_ENTRY_DBA_TABLE = 1,
    COMPRESSION_ENTRY_PRESETS,
    COMPRESSION_ENTRY_SKELETON_LIST
};

namespace CharacterTool {
    string GetRelativePath(const string& fullPath, bool bRelativeToGameFolder)
    {
        if (fullPath.empty())
        {
            return "";
        }

        QString rootpath = QDir::currentPath();
        if (bRelativeToGameFolder == true)
        {
            string gameFolder = Path::GetEditingGameDataFolder().c_str();
            if (gameFolder.find(':') == string::npos)
            {
                rootpath += QString(QDir::separator()) + gameFolder.c_str();
            }
            else
            {
                rootpath = gameFolder.c_str();
            }
        }

        // Create relative path
        QString path = QDir(rootpath).relativeFilePath(fullPath.c_str());
        if (path.isEmpty())
        {
            return fullPath;
        }

        string relPath = path.toUtf8().data();
        size_t i = 0;
        while (relPath.c_str()[i] == '\\' ||
               relPath.c_str()[i] == '/' ||
               relPath.c_str()[i] == '.')
        {
            ++i;
        }
        relPath.erase(0, i);
        return relPath;
    }

    string GetPath(const char* path)
    {
        const char* pathEnd = strrchr(path, '/');
        if (pathEnd == 0)
        {
            return "/";
        }
        return string(path, pathEnd + 1);
    }

    const char* GetFilename(const char* path)
    {
        const char* sep = strrchr(path, '/');
        if (sep == 0)
        {
            return path;
        }
        return sep + 1;
    }

    // ---------------------------------------------------------------------------

    void ExplorerEntryId::Serialize(Serialization::IArchive& ar)
    {
        ar(subtree, "subtree");
        if (Explorer* explorer = ar.FindContext<Explorer>())
        {
            string path;
            ExplorerEntry* entry = size_t(subtree) < NUM_SUBTREES ? explorer->FindEntryById((EExplorerSubtree)subtree, id) : 0;
            if (entry)
            {
                path = entry->path;
            }
            ar(path, "path");
            if (ar.IsInput())
            {
                if (size_t(subtree) < NUM_SUBTREES)
                {
                    entry = explorer->FindEntryByPath((EExplorerSubtree)subtree, path.c_str());
                }
                else
                {
                    entry = 0;
                }
                if (entry)
                {
                    id = entry->id;
                }
                else
                {
                    id = 0;
                }
            }
        }
        else
        {
            ar(id, "id");
        }
    }

    // ---------------------------------------------------------------------------

    ExplorerEntry::ExplorerEntry(EExplorerSubtree subtree, EExplorerEntryType type, unsigned int id)
        : parent(0)
        , type(type)
        , subtree(subtree)
        , modified(false)
        , id(id)
        , icon("")
        , isDragEnabled(false)
    {
    }

    void ExplorerEntry::Serialize(Serialization::IArchive& ar)
    {
    }

    void ExplorerEntry::SetColumnValue(int column, unsigned int value)
    {
        if (column < 0)
        {
            return;
        }
        if (column >= columnValues.size())
        {
            columnValues.resize(column + 1, 0);
        }
        columnValues[column] = value;
    }

    unsigned int ExplorerEntry::GetColumnValue(int column) const
    {
        if (column < 0)
        {
            return 0;
        }
        if (column >= columnValues.size())
        {
            return 0;
        }
        return columnValues[column];
    }

    // ---------------------------------------------------------------------------

    bool LessCompareExplorerEntryByPath(const ExplorerEntry* a, const ExplorerEntry* b)
    {
        if (a->type == b->type)
        {
            return a->path < b->path;
        }
        else
        {
            return int(a->type) < int(b->type);
        }
    }

    // ---------------------------------------------------------------------------

    Explorer::Explorer()
        : m_dependencyManager(new DependencyManager())
        , m_undoCounter(1)
    {
        m_providerBySubtree.resize(NUM_SUBTREES, 0);

        m_root.reset(new ExplorerEntry(NUM_SUBTREES, ENTRY_GROUP, 0));

        AddColumn("Name", ExplorerColumn::TEXT, true);
    }

    Explorer::~Explorer()
    {
    }

    void Explorer::AddProvider(int subtree, IExplorerEntryProvider* provider)
    {
        EXPECTED(provider != 0);
        EXPECTED(m_providerBySubtree[subtree] == 0);
        m_providerBySubtree[subtree] = provider;

        if (std::find(m_providers.begin(), m_providers.end(), provider) == m_providers.end())
        {
            m_providers.push_back(provider);
            EXPECTED(connect(provider, SIGNAL(SignalSubtreeReset(int)), this, SLOT(OnProviderSubtreeReset(int))));
            EXPECTED(connect(provider, SIGNAL(SignalSubtreeLoadingFinished(int)), this, SLOT(OnProviderSubtreeLoadingFinished(int))));
            EXPECTED(connect(provider, SIGNAL(SignalEntryModified(EntryModifiedEvent &)), this, SLOT(OnProviderEntryModified(EntryModifiedEvent &))));
            EXPECTED(connect(provider, SIGNAL(SignalEntryAdded(int, unsigned int)), this, SLOT(OnProviderEntryAdded(int, unsigned int))));
            EXPECTED(connect(provider, SIGNAL(SignalEntryRemoved(int, unsigned int)), this, SLOT(OnProviderEntryRemoved(int, unsigned int))));
            EXPECTED(connect(provider, &IExplorerEntryProvider::SignalBeginBatchChange, this, &Explorer::OnProviderBeginBatchChange));
            EXPECTED(connect(provider, &IExplorerEntryProvider::SignalEndBatchChange, this, &Explorer::OnProviderEndBatchChange));
            EXPECTED(connect(provider, &IExplorerEntryProvider::SignalEntrySavedAs, this, [&](const char* oldName, const char* newName) { SignalEntrySavedAs(oldName, newName); }));
        }
        provider->SetExplorer(this);
    }

    int Explorer::AddColumn(const char* label, ExplorerColumn::Format format, bool visibleByDefault, const ExplorerColumnValue* values, size_t numValues)
    {
        ExplorerColumn column;
        column.label = label;
        column.visibleByDefault = visibleByDefault;
        column.format = format;
        column.values.assign(values, values + numValues);
        m_columns.push_back(column);
        return (int)m_columns.size() - 1;
    }

    int Explorer::FindColumn(const char* label) const
    {
        for (size_t i = 0; i < m_columns.size(); ++i)
        {
            if (azstricmp(m_columns[i].label, label) == 0)
            {
                return int(i);
            }
        }
        return -1;
    }


    int Explorer::GetColumnCount() const
    {
        return m_columns.size();
    }

    const char* Explorer::GetColumnLabel(int columnIndex) const
    {
        const ExplorerColumn* column = GetColumn(columnIndex);
        if (!column)
        {
            return "";
        }
        return column->label.c_str();
    }

    const ExplorerColumn* Explorer::GetColumn(int columnIndex) const
    {
        if (size_t(columnIndex) >= m_columns.size())
        {
            return 0;
        }
        return &m_columns[columnIndex];
    }

    static ExplorerEntry* FindEntryByID(FolderSubtree* subtree, unsigned int id)
    {
        EntriesById::iterator it = subtree->entriesById.find(id);
        if (it == subtree->entriesById.end())
        {
            return 0;
        }

        return it->second;
    }

    void Explorer::OnProviderEntryModified(EntryModifiedEvent& ev)
    {
        FolderSubtree& subtree = m_subtrees[ev.subtree];
        ExplorerEntry* entry = FindEntryByID(&subtree, ev.id);
        if (entry)
        {
            IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
            if (ev.contentChanged && !ev.continuousChange)
            {
                if (!entry->history)
                {
                    entry->history.reset(new CUndoStack());
                }
                entry->history->PushUndo(&ev.previousContent, ev.reason, m_undoCounter);
                ++m_undoCounter;
            }
            if (provider && !ev.continuousChange)
            {
                provider->UpdateEntry(entry);
            }

            EntryModified(entry, ev.continuousChange);
            return;
        }
    }

    void Explorer::EntryModified(ExplorerEntry* entry, bool continuousChange)
    {
        ExplorerEntryModifyEvent explorerEv;
        explorerEv.entry = entry;
        explorerEv.entryParts = ENTRY_PART_CONTENT;
        explorerEv.continuousChange = continuousChange;

        // TODO: ENTRY_PART_DEPENDENCY
        {
            vector<string> paths;
            IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
            if (provider)
            {
                provider->GetDependencies(&paths, entry->id);
            }
            m_dependencyManager->SetDependencies(entry->path.c_str(), paths);

            vector<string> users;
            m_dependencyManager->FindUsers(&users, entry->path.c_str());
            if (!users.empty())
            {
                for (size_t i = 0; i < users.size(); ++i)
                {
                    FindEntriesByPath(&explorerEv.dependentEntries, users[i].c_str());
                }
            }
        }

        SignalEntryModified(explorerEv);
    }

    void Explorer::OnProviderEntryAdded(int subtreeIndex, unsigned int id)
    {
        FolderSubtree& subtree = m_subtrees[subtreeIndex];
        IExplorerEntryProvider* provider = m_providerBySubtree[subtreeIndex];
        if (!provider)
        {
            return;
        }
        _smart_ptr<ExplorerEntry> entry(new ExplorerEntry(EExplorerSubtree(subtreeIndex), EExplorerEntryType(provider->GetEntryType(subtreeIndex)), id));
        subtree.entries.push_back(entry);
        subtree.entriesById[id] = entry;

        provider->UpdateEntry(entry);
        subtree.entriesByPath[entry->path] = entry;

        bool singleLevelGroup = false;
        if (singleLevelGroup)
        {
            UpdateSingleLevelGroups(&subtree, EExplorerSubtree(subtreeIndex));
        }
        else
        {
            if (strnicmp(entry->path.c_str(), subtree.commonPrefix.c_str(), subtree.commonPrefix.size()) == 0)
            {
                ExplorerEntry* parent = CreateGroupsForPath(&subtree, entry->path.c_str(), subtree.commonPrefix.c_str());
                ;
                if (parent)
                {
                    LinkEntryToParent(parent, entry);
                }
            }
            else
            {
                CreateGroups(m_root.get(), &subtree, EExplorerSubtree(subtreeIndex), ENTRY_GROUP);
            }
        }
    }

    void Explorer::OnProviderEntryRemoved(int subtree, unsigned int id)
    {
        ExplorerEntry* entry = FindEntryByID(&m_subtrees[subtree], id);
        if (!entry)
        {
            return;
        }

        UnlinkEntryFromParent(entry);
        RemoveEntry(entry);
        RemoveEmptyGroups(&m_subtrees[subtree]);
    }

    void Explorer::OnProviderSubtreeLoadingFinished(int subtreeIndex)
    {
        FolderSubtree* subtree = &m_subtrees[subtreeIndex];

        if (subtree->loadingEntry)
        {
            UnlinkEntryFromParent(subtree->loadingEntry);
            subtree->loadingEntry.reset();
        }

        SignalEntryLoaded(subtree->root);
    }

    void Explorer::OnProviderBeginBatchChange(int subtree)
    {
        SignalBeginBatchChange(subtree);
    }

    void Explorer::OnProviderEndBatchChange(int subtree)
    {
        SignalEndBatchChange(subtree);
    }

    void Explorer::ResetSubtree(FolderSubtree* subtree, EExplorerSubtree subtreeIndex, const char* text, bool addLoadingEntry)
    {
        if (!subtree->root)
        {
            subtree->root.reset(new ExplorerEntry(subtreeIndex, ENTRY_SUBTREE_ROOT, 0));
            LinkEntryToParent(m_root.get(), subtree->root);
        }
        else
        {
            RemoveChildren(subtree->root.get());
        }
        subtree->Clear();
        subtree->root->name = text;


        if (addLoadingEntry)
        {
            subtree->loadingEntry.reset(new ExplorerEntry(subtreeIndex, ENTRY_LOADING, 0));
            subtree->loadingEntry->name = "Loading...";
            LinkEntryToParent(subtree->root, subtree->loadingEntry);
        }
    }

    const char* GetSubtreeTitle(int subtree)
    {
        switch (subtree)
        {
        case SUBTREE_ANIMATIONS:
            return "Animations";
        case SUBTREE_COMPRESSION:
            return "Compression";
        case SUBTREE_SKELETONS:
            return "Skeletons";
        case SUBTREE_CHARACTERS:
            return "Characters";
        case SUBTREE_PHYSICS:
            return "Physics";
        case SUBTREE_RIGS:
            return "Rigs";
        case SUBTREE_SOURCE_ASSETS:
            return "Source Assets";
        default:
            return "";
        }
    }

    void Explorer::OnProviderSubtreeReset(int subtreeIndex)
    {
        FolderSubtree& subtree = m_subtrees[subtreeIndex];
        IExplorerEntryProvider* provider = m_providerBySubtree[subtreeIndex];
        ResetSubtree(&subtree, EExplorerSubtree(subtreeIndex), GetSubtreeTitle(subtreeIndex), provider->HasBackgroundLoading());

        if (!provider)
        {
            return;
        }

        int numEntries = provider->GetEntryCount(subtreeIndex);
        for (int i = 0; i < numEntries; ++i)
        {
            unsigned int id = provider->GetEntryIdByIndex(subtreeIndex, i);

            ExplorerEntry* entry = new ExplorerEntry(EExplorerSubtree(subtreeIndex), EExplorerEntryType(provider->GetEntryType(subtreeIndex)), id);
            provider->UpdateEntry(entry);

            subtree.entries.push_back(entry);
            subtree.entriesById[entry->id] = entry;
            subtree.entriesByPath[entry->path] = entry;
        }

        CreateGroups(m_root.get(), &subtree, EExplorerSubtree(subtreeIndex), ENTRY_GROUP);

        SignalRefreshFilter();
    }


    void Explorer::Populate()
    {
        for (size_t i = 0; i < m_providerBySubtree.size(); ++i)
        {
            int subtree = i;
            if (!m_providerBySubtree[i])
            {
                continue;
            }
            OnProviderSubtreeReset(i);
        }
    }


    void Explorer::UpdateSingleLevelGroups(FolderSubtree* subtree, EExplorerSubtree subtreeIndex)
    {
        typedef std::map<string, _smart_ptr<ExplorerEntry>, less_stricmp<string> > Groups;
        Groups groups;

        for (size_t i = 0; i < subtree->groups.size(); ++i)
        {
            string path = subtree->groups[i]->path.c_str();
            groups[path] = subtree->groups[i];
        }

        for (size_t i = 0; i < subtree->entries.size(); ++i)
        {
            _smart_ptr<ExplorerEntry> entry = subtree->entries[i];

            string path = GetPath(entry->path.c_str());
            _smart_ptr<ExplorerEntry>& group = groups[path];

            if (entry->parent != 0)
            {
                continue;
            }

            if (!group)
            {
                group.reset(new ExplorerEntry(subtreeIndex, ENTRY_GROUP, 0));
                subtree->groups.push_back(group);
                group->path = path;
                group->name = path;
                LinkEntryToParent(subtree->root, group);
            }

            LinkEntryToParent(group, entry);
        }
    }

    void Explorer::RemoveEmptyGroups(FolderSubtree* subtree)
    {
        TRACE_EXPLORER("RemoveEmptyGroupBefore before:");
        bool gotNewEmpties = true;
        while (gotNewEmpties)
        {
            gotNewEmpties = false;
            for (int i = (int)subtree->groups.size() - 1; i >= 0; --i)
            {
                ExplorerEntry* group = subtree->groups[i];
                if (group->children.empty())
                {
                    if (UnlinkEntryFromParent(group))
                    {
                        gotNewEmpties = true;
                    }

                    subtree->groupsByPath.erase(group->path);
                    subtree->groups.erase(subtree->groups.begin() + i);
                }
            }
        }
    }

    static string FindCommonPathPrefix(const FolderSubtree* subtree)
    {
        bool firstPath = true;
        string commonPathPrefix;
        for (size_t i = 0; i < subtree->entries.size(); ++i)
        {
            const ExplorerEntry* entry = subtree->entries[i];
            if (entry->path.find('/') == string::npos)
            {
                commonPathPrefix.clear();
                break;
            }

            if (firstPath)
            {
                commonPathPrefix = entry->path;
                string::size_type pos = commonPathPrefix.rfind('/');
                if (pos != string::npos)
                {
                    commonPathPrefix.erase(pos + 1, commonPathPrefix.size());
                }
                else
                {
                    commonPathPrefix.clear();
                }
                firstPath = false;
                continue;
            }
            else
            {
                if (strnicmp(entry->path.c_str(), commonPathPrefix.c_str(), commonPathPrefix.size()) == 0)
                {
                    continue;
                }
            }

            commonPathPrefix.erase(commonPathPrefix.size() - 1);
            while (!commonPathPrefix.empty())
            {
                string::size_type pos = commonPathPrefix.rfind('/');
                if (pos == string::npos)
                {
                    commonPathPrefix.clear();
                    break;
                }
                else
                {
                    commonPathPrefix.erase(pos + 1, commonPathPrefix.size());
                    if (strnicmp(entry->path.c_str(), commonPathPrefix.c_str(), commonPathPrefix.size()) == 0)
                    {
                        break;
                    }
                    else
                    {
                        commonPathPrefix.erase(commonPathPrefix.size() - 1);
                    }
                }
            }
        }
        return commonPathPrefix;
    }

    void Explorer::CreateGroups(ExplorerEntry* globalRoot, FolderSubtree* subtree, EExplorerSubtree subtreeIndex, EExplorerEntryType rootType)
    {
        string commonPathPrefix = FindCommonPathPrefix(subtree);

        if (subtree->commonPrefix != commonPathPrefix)
        {
            // if we change our common path we need to restructure the whole tree
            for (size_t i = 0; i < subtree->entries.size(); ++i)
            {
                ExplorerEntry* entry = subtree->entries[i];
                if (entry->parent)
                {
                    UnlinkEntryFromParent(entry);
                }
            }
            RemoveEmptyGroups(subtree);
            EXPECTED(subtree->groups.empty());

            subtree->groups.clear();
            subtree->groupsByPath.clear();
            for (size_t i = 0; i < subtree->entries.size(); ++i)
            {
                if (subtree->entries[i])
                {
                    subtree->entries[i]->parent = 0;
                }
            }
        }

        if (subtree->entries.empty())
        {
            return;
        }

        for (size_t i = 0; i < subtree->entries.size(); ++i)
        {
            ExplorerEntry* entry = subtree->entries[i];
            ExplorerEntry* group = CreateGroupsForPath(subtree, entry->path.c_str(), commonPathPrefix.c_str());
            if (group)
            {
                LinkEntryToParent(group, entry);
            }
            else
            {
                entry->parent = 0;
            }
        }
        subtree->commonPrefix = commonPathPrefix;
        subtree->root->name += " (";
        subtree->root->name += commonPathPrefix;
        subtree->root->name += ")";
    }

    ExplorerEntry* Explorer::CreateGroupsForPath(FolderSubtree* subtree, const char* animationPath, const char* commonPathPrefix)
    {
        ExplorerEntry* root = subtree->root.get();
        if (!root)
        {
            return 0;
        }
        const char* p = animationPath;
        if (strnicmp(p, commonPathPrefix, strlen(commonPathPrefix)) != 0)
        {
            return root;
        }
        p += strlen(commonPathPrefix);
        if (*p == '\0')
        {
            return 0;
        }

        ExplorerEntry* currentGroup = root;
        while (true)
        {
            string name;
            string groupPath;

            const char* folderName = p;
            const char* folderNameEnd = strchr(p, '/');
            if (folderNameEnd != 0)
            {
                p = folderNameEnd + 1;

                name.assign(folderName, folderNameEnd);
                groupPath.assign(animationPath, folderNameEnd);
            }
            else
            {
                break;
            }

            EntriesByPath::iterator it = subtree->groupsByPath.find(groupPath);
            if (it == subtree->groupsByPath.end())
            {
                ExplorerEntry* newGroup = new ExplorerEntry(currentGroup->subtree, ENTRY_GROUP, 0);
                newGroup->name = name;
                newGroup->path = groupPath;

                subtree->groups.push_back(newGroup);
                subtree->groupsByPath[groupPath] = newGroup;

                LinkEntryToParent(currentGroup, newGroup);

                currentGroup = newGroup;
            }
            else
            {
                currentGroup = it->second;
            }
        }
        return currentGroup;
    }

    void Explorer::LoadEntries(const ExplorerEntries& currentEntries)
    {
        for (size_t i = 0; i < currentEntries.size(); ++i)
        {
            ExplorerEntry* entry = currentEntries[i].get();
            if (!entry)
            {
                continue;
            }
            IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
            if (provider)
            {
                provider->LoadOrGetChangedEntry(entry->id);
                vector<string> paths;
                provider->GetDependencies(&paths, entry->id);
                m_dependencyManager->SetDependencies(entry->path.c_str(), paths);
            }
        }
    }

    void Explorer::CheckIfModified(ExplorerEntry* entry, const char* reason, bool continuousChange)
    {
        if (!entry)
        {
            return;
        }

        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
        if (provider)
        {
            provider->CheckIfModified(entry->id, reason, continuousChange); // can emit SignalProviderEntryModified
        }
    }

    void Explorer::SaveEntry(ExplorerEntry* entry)
    {
        SaveEntry(entry, AZStd::make_shared<AZ::ActionOutput>(), static_cast<AZ::SaveCompleteCallback>(0));
    }

    void Explorer::SaveEntry(ExplorerEntry* entry, const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo)
    {
        SaveEntry(entry, errorInfo, static_cast<AZ::SaveCompleteCallback>(0));
    }
    void Explorer::SaveEntry(ExplorerEntry* entry, AZ::SaveCompleteCallback onSaveComplete)
    {
        SaveEntry(entry, AZStd::make_shared<AZ::ActionOutput>(), onSaveComplete);
    }
    void Explorer::SaveEntry(ExplorerEntry* entry, const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo, AZ::SaveCompleteCallback onSaveComplete)
    {
        if (!entry)
        {
            if (errorInfo)
            {
                errorInfo->AddError("Failed to save ExplorerEntry, passed in value was null", nullptr);
            }

            if (onSaveComplete)
            {
                onSaveComplete(false);
            }
            return;
        }

        if (!entry->modified)
        {
            if (onSaveComplete)
            {
                onSaveComplete(true);
            }
            return;
        }

        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
        if (!provider)
        {
            errorInfo->AddError("Failed to discern entry provider for passed in entry/entries", entry->name.c_str());
            if (onSaveComplete)
            {
                onSaveComplete(false);
            }
            return;
        }

        provider->SaveEntry(errorInfo, entry->id,
            [onSaveComplete, entry](bool success)
            {
                if (success)
                {
                    CryLog("Geppetto - SaveEntry was successful for entry: '%s'", entry->name.c_str());
                }
                else
                {
                    CryLog("Geppetto - SaveEntry FAILED for entry: '%s'", entry->name.c_str());
                }

                if (onSaveComplete)
                {
                    onSaveComplete(success);
                }
            }
            );
    }

    void Explorer::SaveAll()
    {
        SaveAll(AZStd::make_shared<AZ::ActionOutput>(), static_cast<AZ::SaveCompleteCallback>(0));
    }
    void Explorer::SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo)
    {
        SaveAll(errorInfo, static_cast<AZ::SaveCompleteCallback>(0));
    }
    void Explorer::SaveAll(AZ::SaveCompleteCallback onSaveComplete)
    {
        SaveAll(AZStd::make_shared<AZ::ActionOutput>(), onSaveComplete);
    }

    void Explorer::SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo, AZ::SaveCompleteCallback onSaveComplete)
    {
        struct CaptureTrackerData
        {
            int m_numProviders;

            // If there is even one failure, we return false for failure through the callback.
            bool m_allAreSuccessful;
        };

        AZStd::shared_ptr<CaptureTrackerData> captureTrackerData = AZStd::make_shared<CaptureTrackerData>();
        captureTrackerData->m_numProviders = aznumeric_cast<int>(m_providers.size());
        captureTrackerData->m_allAreSuccessful = true;

        if (captureTrackerData->m_numProviders == 0)
        {
            if (onSaveComplete)
            {
                onSaveComplete(true);
            }
            return;
        }


        for (auto provider : m_providers)
        {
            provider->SaveAll(errorInfo,
                [captureTrackerData, onSaveComplete](bool success)
                {
                    captureTrackerData->m_numProviders -= 1;
                    if (!success)
                    {
                        captureTrackerData->m_allAreSuccessful = false;
                    }

                    AZ_Assert(captureTrackerData->m_numProviders >= 0, "Geppetto - More than expected save callbacks from the entry providers");

                    if (captureTrackerData->m_numProviders == 0 && onSaveComplete)
                    {
                        onSaveComplete(captureTrackerData->m_allAreSuccessful);
                    }
                }
                );
        }
    }

    void Explorer::Revert(ExplorerEntry* entry)
    {
        if (!entry)
        {
            return;
        }

        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
        if (!provider)
        {
            return;
        }

        provider->RevertEntry(entry->id);
    }

    bool Explorer::GetSerializerForEntry(Serialization::SStruct* out, ExplorerEntry* entry)
    {
        if (!entry)
        {
            return false;
        }

        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
        if (!provider)
        {
            return false;
        }

        if (entry->type == ENTRY_GROUP)
        {
            return false;
        }

        return provider->GetEntrySerializer(out, entry->id);
    }

    QMimeData* Explorer::GetMimeDataForEntries(const std::vector<ExplorerEntry*> entries)
    {
        std::map<EExplorerSubtree, std::vector<unsigned int>> entryIdsBySubtree;
        for (auto entry : entries)
        {
            entryIdsBySubtree[entry->subtree].push_back(entry->id);
        }

        QMimeData* mimeData = new QMimeData;

        for (auto& entryGroup : entryIdsBySubtree)
        {
            IExplorerEntryProvider* provider = m_providerBySubtree[entryGroup.first];
            if (provider)
            {
                provider->SetMimeDataForEntries(mimeData, entryGroup.second);
            }
        }

        return mimeData;
    }

    void Explorer::GetActionsForEntry(vector<ExplorerAction>* actions, ExplorerEntry* entry)
    {
        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];
        if (provider)
        {
            provider->GetEntryActions(actions, entry->id, this);
            return;
        }
        if (entry->type == ENTRY_GROUP)
        {
            actions->push_back(ExplorerAction("Show in Explorer", ACTION_NOT_STACKABLE, [=](ActionContext& x) { ActionShowInExplorer(x); }, "Editor/Icons/animation/show_in_explorer.png", "Locates file in Windows Explorer."));
        }
        else if (entry->type == ENTRY_DBA_TABLE ||
                 entry->type == ENTRY_COMPRESSION_PRESETS ||
                 entry->type == ENTRY_SKELETON_LIST)
        {
            actions->push_back(ExplorerAction("Show in Explorer", ACTION_NOT_STACKABLE, [=](ActionContext& x) { ActionShowInExplorer(x); }, "Editor/Icons/animation/show_in_explorer.png", "Locates file in Windows Explorer."));
            actions->push_back(ExplorerAction("Revert", 0, [=](ActionContext& x) { ActionRevert(x); }, "Editor/Icons/animation/revert.png", "Reload file from disk discarding current changes (can be undone)"));
            actions->push_back(ExplorerAction("Save", 0, [=](ActionContext& x) { ActionSave(x); }, "Editor/Icons/animation/save.png"));
        }
    }

    static bool IsSameAction(const ExplorerAction& a, const ExplorerAction& b)
    {
        return strcmp(a.text, b.text) == 0 && strcmp(a.icon, b.icon) == 0;
    }

    static const ExplorerAction* FindActionIn(const ExplorerAction& action, const ExplorerActions& actions)
    {
        for (size_t i = 0; i < actions.size(); ++i)
        {
            if (IsSameAction(actions[i], action))
            {
                return &actions[i];
            }
        }
        return 0;
    }

    static void FilterCommonActions(ExplorerActions* out, const ExplorerActions& in)
    {
        for (int i = 0; i < (int)out->size(); ++i)
        {
            ExplorerAction& action = (*out)[i];
            if (!action.func)
            {
                continue; // keep separators
            }
            const ExplorerAction* inAction = FindActionIn(action, in);
            if (!inAction || (action.flags & ACTION_NOT_STACKABLE) != 0 || (inAction->flags & ACTION_NOT_STACKABLE) != 0)
            {
                out->erase(out->begin() + i);
                --i;
                continue;
            }
            else
            {
                if ((inAction->flags & ACTION_DISABLED) == 0)
                {
                    action.flags &= ~ACTION_DISABLED;
                }
            }
        }

        bool lastSeparator = true;
        for (int i = 0; i < (int)out->size(); ++i)
        {
            ExplorerAction& action = (*out)[i];
            if (!action.func)
            {
                if (lastSeparator)
                {
                    out->erase(out->begin() + i);
                    --i;
                    lastSeparator = i < 0;
                    continue;
                }
                else
                {
                    lastSeparator = true;
                }
            }
            else
            {
                lastSeparator = false;
            }
        }

        if (!out->empty() && !out->back().func)
        {
            out->pop_back();
        }
    }

    void Explorer::GetCommonActions(ExplorerActions* actions, const ExplorerEntries& entries)
    {
        actions->clear();
        if (entries.empty())
        {
            return;
        }
        GetActionsForEntry(actions, entries[0]);

        ExplorerActions temp;
        for (size_t i = 1; i < entries.size(); ++i)
        {
            temp.clear();
            ExplorerEntry* e = entries[i];
            GetActionsForEntry(&temp, e);

            FilterCommonActions(actions, temp);
        }
    }

    void Explorer::SetEntryColumn(const ExplorerEntryId& id, int column, unsigned int value, bool notify)
    {
        if (id.subtree < 0 || id.subtree >= NUM_SUBTREES)
        {
            return;
        }
        ExplorerEntry* entry = FindEntryById(EExplorerSubtree(id.subtree), id.id);
        if (entry)
        {
            entry->SetColumnValue(column, value);
            if (notify)
            {
                ExplorerEntryModifyEvent ev;
                ev.entry = entry;
                ev.entryParts = ENTRY_PART_STATUS_COLUMNS;
                ev.continuousChange = false;
                SignalEntryModified(ev);
            }
        }
    }

    void Explorer::ActionSave(ActionContext& x)
    {
        x.isAsync = true;
        QWidget* targetWindow = x.window;
        AZStd::shared_ptr<int> numEntries = AZStd::make_shared<int>(x.entries.size());
        AZStd::shared_ptr<AZ::ActionOutput> output = x.output;
        for (size_t i = 0; i < x.entries.size(); ++i)
        {
            SaveEntry(x.entries[i], output,
                [numEntries, output, targetWindow](bool success)
                {
                    *numEntries -= 1;

                    AZ_Assert(*numEntries >= 0, "Geppetto - Too many lambda returns.");

                    if (*numEntries == 0 && output && output->HasAnyErrors())
                    {
                        QMessageBox::critical(targetWindow, "Geppetto - Failed to save entry", output->BuildErrorMessage().c_str());
                    }
                }
                );
        }
    }

    void Explorer::ActionRevert(ActionContext& x)
    {
        for (size_t i = 0; i < x.entries.size(); ++i)
        {
            Revert(x.entries[i]);
        }
    }

    void Explorer::ActionShowInExplorer(ActionContext& x)
    {
        ExplorerEntry* entry = x.entries.front();
        string fullPath = GetFilePathForEntry(entry);
        QString commandLineArgs = QStringLiteral("/n,/select,\"%1\"").arg(fullPath.c_str());
        QString commandLine = "explorer " + commandLineArgs;
        QProcess::startDetached(commandLine);
    }

    void Explorer::RemoveEntry(ExplorerEntry* entry)
    {
        FolderSubtree& subtree = m_subtrees[entry->subtree];

        for (size_t i = 0; i < subtree.entries.size(); ++i)
        {
            if (subtree.entries[i] == entry)
            {
                subtree.entriesById.erase(entry->id);
                subtree.entriesByPath.erase(entry->path);
                subtree.entries.erase(subtree.entries.begin() + i);
            }
        }
    }

    void Explorer::RemoveChildren(ExplorerEntry* entry)
    {
        int count = entry->children.size();
        for (int i = count - 1; i >= 0; --i)
        {
            RemoveChildren(entry->children[i]);

            SignalBeginRemoveEntry(entry->children[i]);
            entry->children[i]->parent = 0;
            entry->children.erase(entry->children.begin() + i);
            SignalEndRemoveEntry();
        }
    }

    void Explorer::LinkEntryToParent(ExplorerEntry* parent, ExplorerEntry* child)
    {
        TRACE_EXPLORER("0x%p Adding '%s->%s'", (void*)child, parent->name, child->name);
        CRY_ASSERT(child->parent == 0);
        child->parent = parent;
        SignalBeginAddEntry(child);
        EXPECTED(stl::push_back_unique(parent->children, child));
        SignalEndAddEntry();
    }

    bool Explorer::UnlinkEntryFromParent(ExplorerEntry* entry)
    {
        TRACE_EXPLORER("0x%p Removing '%s' from '%s'", (void*)entry, entry->name.c_str(), entry->parent->name.c_str());
        ExplorerEntry* parent = entry->parent;
        if (!parent)
        {
            return false;
        }

        bool gotAtLeastOneEmpty = false;
        for (size_t j = 0; j < parent->children.size(); ++j)
        {
            if (parent->children[j] == entry)
            {
                SignalBeginRemoveEntry(entry);
                parent->children.erase(parent->children.begin() + j);
                if (parent->children.empty())
                {
                    gotAtLeastOneEmpty = true;
                }
                entry->parent = 0;
                --j;
                SignalEndRemoveEntry();
            }
        }
        return gotAtLeastOneEmpty;
    }

    string Explorer::GetFilePathForEntry(const ExplorerEntry* entry)
    {
        string path = Path::GamePathToFullPath(entry->path.c_str()).toUtf8().data();
        if (!gEnv->pCryPak->IsFileExist(path.c_str(), ICryPak::eFileLocation_OnDisk))
        {
            if (azstricmp(PathUtil::GetExt(path.c_str()), "caf") == 0)
            {
                path = PathUtil::ReplaceExtension(path, "i_caf");
            }
        }

        if (GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            return string();
        }

        return path;
    }

    string Explorer::GetCanonicalPath(EExplorerSubtree subtree, const char* path) const
    {
        const char* canonicalExtension = m_providerBySubtree[subtree]->CanonicalExtension();
        if (canonicalExtension[0] != '\0')
        {
            return PathUtil::ReplaceExtension(path, canonicalExtension);
        }
        else
        {
            return path;
        }
    }

    bool Explorer::HasProvider(EExplorerSubtree subtree) const
    {
        if (size_t(subtree) >= m_providers.size())
        {
            return false;
        }
        return m_providers[subtree] != 0;
    }

    void Explorer::GetDependingEntries(vector<ExplorerEntry*>* entries, ExplorerEntry* entry)
    {
        vector<string> users;
        m_dependencyManager->FindDepending(&users, entry->path.c_str());

        for (size_t i = 0; i < users.size(); ++i)
        {
            FindEntriesByPath(&*entries, users[i].c_str());
        }
    }

    ExplorerEntry* Explorer::FindEntryById(EExplorerSubtree subtree, unsigned int id)
    {
        size_t subtreeIndex = size_t(subtree);
        if (subtreeIndex >= NUM_SUBTREES)
        {
            return 0;
        }
        FolderSubtree& f = m_subtrees[subtree];
        EntriesById::iterator it = f.entriesById.find(id);
        if (it == f.entriesById.end())
        {
            return 0;
        }
        return it->second;
    }

    ExplorerEntry* Explorer::FindEntryById(const ExplorerEntryId& id)
    {
        return FindEntryById(EExplorerSubtree(id.subtree), id.id);
    }

    ExplorerEntry* Explorer::FindEntryByPath(EExplorerSubtree subtree, const char* path)
    {
        FolderSubtree& f = m_subtrees[subtree];
        EntriesByPath::iterator it = f.entriesByPath.find(path);
        if (it == f.entriesByPath.end())
        {
            return 0;
        }
        return it->second;
    }

    void Explorer::FindEntriesByPath(vector<ExplorerEntry*>* entries, const char* path)
    {
        for (int subtree = 0; subtree < NUM_SUBTREES; ++subtree)
        {
            FolderSubtree& f = m_subtrees[subtree];
            EntriesByPath::iterator it = f.entriesByPath.find(path);
            if (it != f.entriesByPath.end())
            {
                entries->push_back(it->second);
            }
        }
    }

    const char* Explorer::IconForEntry(EExplorerEntryType type, const ExplorerEntry* entry)
    {
        if (entry && entry->icon && entry->icon[0])
        {
            return entry->icon;
        }

        switch (type)
        {
        case ENTRY_GROUP:
            return "Editor/Icons/Animation/group.png";
        case ENTRY_CHARACTER:
            return "Editor/Icons/Animation/character.png";
        case ENTRY_CHARACTER_SKELETON:
            return "Editor/Icons/Animation/skeleton.png";
        case ENTRY_PHYSICS:
            return "Editor/Icons/Animation/physics.png";
        case ENTRY_RIG:
            return "Editor/Icons/Animation/rig.png";
        case ENTRY_ANIMATION:
            return "Editor/Icons/Animation/animation.png";
        case ENTRY_SKELETON:
            return "Editor/Icons/Animation/skeleton.png";
        case ENTRY_SOURCE_ASSET:
            return "Editor/Icons/animation/source_asset_16.png";
        default:
            return "";
        }
    }

    void Explorer::Undo(const ExplorerEntries& entries, int count)
    {
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (!entry->history)
            {
                continue;
            }
            vector<char> currentState;
            GetEntryState(&currentState, entry);
            vector<char> newState;
            if (entry->history->Undo(&newState, currentState, count, ++m_undoCounter))
            {
                SetEntryState(entry, newState);
            }

            EntryModified(entry, false);
        }
    }

    void Explorer::UndoInOrder(const ExplorerEntries& entries)
    {
        int index = -1;
        unsigned long long newestUndo = 0;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (!entry->history)
            {
                continue;
            }
            unsigned long long lastUndo = entry->history->NewestUndoIndex();
            if (lastUndo > newestUndo)
            {
                index = i;
                newestUndo = lastUndo;
            }
        }

        if (index >= 0)
        {
            ExplorerEntries singleEntry(1, entries[index]);
            Undo(singleEntry, 1);
        }
    }

    void Explorer::RedoInOrder(const ExplorerEntries& entries)
    {
        int index = -1;
        unsigned long long newestRedo = 0;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (!entry->history)
            {
                continue;
            }
            unsigned long long lastRedo = entry->history->NewestRedoIndex();
            if (lastRedo > newestRedo)
            {
                index = i;
                newestRedo = lastRedo;
            }
        }

        if (index >= 0)
        {
            ExplorerEntries singleEntry(1, entries[index]);
            Redo(singleEntry);
        }
    }

    bool Explorer::CanUndo(const ExplorerEntries& entries) const
    {
        if (entries.empty())
        {
            return false;
        }
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (entry->history && entry->history->HasUndo())
            {
                return true;
            }
        }
        return false;
    }

    void Explorer::Redo(const ExplorerEntries& entries)
    {
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (!entry->history)
            {
                continue;
            }

            vector<char> currentState;
            GetEntryState(&currentState, entry);
            vector<char> newState;
            if (entry->history->Redo(&newState, currentState, ++m_undoCounter))
            {
                SetEntryState(entry, newState);
            }

            EntryModified(entry, false);
        }
    }

    bool Explorer::CanRedo(const ExplorerEntries& entries) const
    {
        if (entries.empty())
        {
            return false;
        }
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (entry->history && entry->history->HasRedo())
            {
                return true;
            }
        }
        return false;
    }

    void Explorer::GetUndoActions(vector<string>* actionNames, int maxActionCount, const ExplorerEntries& entries) const
    {
        actionNames->clear();
        if (entries.size() > 1)
        {
            if (CanUndo(entries))
            {
                actionNames->push_back("[Change in multiple entries]");
            }
        }
        else if (entries.size() == 1)
        {
            if (entries[0]->history)
            {
                entries[0]->history->GetUndoActions(actionNames, maxActionCount);
            }
        }
    }

    void Explorer::SetEntryState(ExplorerEntry* entry, const vector<char>& state)
    {
        if (state.empty())
        {
            return;
        }

        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];

        Serialization::SStruct ser;
        if (!provider->GetEntrySerializer(&ser, entry->id))
        {
            return;
        }

        MemoryIArchive ia;
        if (!ia.open(state.data(), state.size()))
        {
            return;
        }

        ser(ia);

        provider->CheckIfModified(entry->id, 0, false);
    }

    void Explorer::GetEntryState(vector<char>* state, ExplorerEntry* entry)
    {
        state->clear();
        IExplorerEntryProvider* provider = m_providerBySubtree[entry->subtree];

        Serialization::SStruct ser;
        if (!provider->GetEntrySerializer(&ser, entry->id))
        {
            return;
        }

        MemoryOArchive oa;
        ser(oa);

        state->assign(oa.buffer(), oa.buffer() + oa.length());
    }

    void Explorer::GetUnsavedEntries(ExplorerEntries* unsavedEntries)
    {
        for (int subtreeIndex = 0; subtreeIndex < NUM_SUBTREES; ++subtreeIndex)
        {
            FolderSubtree& subtree = m_subtrees[subtreeIndex];
            for (size_t entryIndex = 0; entryIndex < subtree.entries.size(); ++entryIndex)
            {
                ExplorerEntry* entry = subtree.entries[entryIndex];
                if (!entry)
                {
                    continue;
                }
                if (entry->modified)
                {
                    unsavedEntries->push_back(entry);
                }
            }
        }
    }

    void Explorer::GetSaveFilenames(std::vector<string>* filenames, const ExplorerEntries& entries) const
    {
        filenames->resize(entries.size());

        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            if (!entry)
            {
                continue;
            }

            int subtreeIndex = int(entry->subtree);
            if (size_t(subtreeIndex) > m_providerBySubtree.size())
            {
                continue;
            }
            IExplorerEntryProvider* provider = m_providerBySubtree[subtreeIndex];
            if (!provider)
            {
                continue;
            }

            (*filenames)[i] = provider->GetSaveFilename(entry->id);
        }
    }
}

#include <CharacterTool/ExplorerDataProvider.moc>
#include <CharacterTool/Explorer.moc>
