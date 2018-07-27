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

#pragma once

#include <QtCore/QObject>

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "Strings.h"
#include "Pointers.h"
#include "functor.h"

#include <ActionOutput.h>
#include <SaveUtilities/AsyncSaveRunner.h>

class QMimeData;

namespace Serialization
{
    class IArchive;
    struct SStruct;
}


struct SDBATable;

namespace CharacterTool {
    using std::vector;
    using std::map;
    using std::unique_ptr;
    class AnimationList;
    class DependencyManager;
    class EditorDBATable;
    class EditorCompressionPresetTable;
    class SkeletonList;
    class CUndoStack;
    struct ActionContext;

    enum EExplorerSubtree
    {
        SUBTREE_CHARACTERS,
        SUBTREE_SKELETONS,
        SUBTREE_PHYSICS,
        SUBTREE_RIGS,
        SUBTREE_ANIMATIONS,
        SUBTREE_COMPRESSION,
        SUBTREE_SOURCE_ASSETS,
        NUM_SUBTREES
    };

    enum EExplorerEntryType
    {
        ENTRY_NONE,
        ENTRY_SUBTREE_ROOT,
        ENTRY_GROUP,
        ENTRY_ANIMATION,
        ENTRY_SKELETON,
        ENTRY_CHARACTER,
        ENTRY_CHARACTER_SKELETON,
        ENTRY_PHYSICS,
        ENTRY_RIG,
        ENTRY_DBA_TABLE,
        ENTRY_COMPRESSION_PRESETS,
        ENTRY_SKELETON_LIST,
        ENTRY_SOURCE_ASSET,
        ENTRY_LOADING
    };

    struct ExplorerEntry
        : _i_reference_target_t
    {
        EExplorerEntryType type;
        EExplorerSubtree subtree;
        string name;
        string path;
        bool modified;
        unsigned int id;
        const char* icon;
        bool isDragEnabled;

        ExplorerEntry* parent;
        vector<ExplorerEntry*> children;
        unique_ptr<CUndoStack> history;
        vector<unsigned int> columnValues;

        ExplorerEntry(EExplorerSubtree subtree, EExplorerEntryType type, unsigned int id);
        void Serialize(Serialization::IArchive& ar);

        void SetColumnValue(int column, unsigned int value);
        unsigned int GetColumnValue(int column) const;
    };
    typedef vector<_smart_ptr<ExplorerEntry> > ExplorerEntries;

    typedef map<string, ExplorerEntry*, less_stricmp<string> > EntriesByPath;
    typedef map<unsigned int, ExplorerEntry*> EntriesById;
    struct FolderSubtree
    {
        _smart_ptr<ExplorerEntry> root;
        ExplorerEntries entries;
        ExplorerEntries groups;
        ExplorerEntries helpers;
        _smart_ptr<ExplorerEntry> loadingEntry;

        EntriesByPath groupsByPath;
        EntriesByPath entriesByPath;
        EntriesById entriesById;
        string commonPrefix;

        void Clear()
        {
            entries.clear();
            groups.clear();
            groupsByPath.clear();
            entriesByPath.clear();
            entriesById.clear();
            commonPrefix.clear();
        }
    };

    struct ExplorerColumnValue
    {
        const char* tooltip;
        const char* icon;
    };

    struct ExplorerColumn
    {
        enum Format
        {
            TEXT,
            FRAMES,
            FILESIZE,
            ICON
        };

        string label;
        bool visibleByDefault;
        Format format;
        vector<ExplorerColumnValue> values;

        ExplorerColumn()
            : visibleByDefault(false)
            , format(TEXT) {}
    };


    struct EntryModifiedEvent;
    struct ExplorerAction;
    typedef vector<ExplorerAction> ExplorerActions;
    class IExplorerEntryProvider;
    struct ExplorerEntryId;

    enum
    {
        ENTRY_PART_STATUS_COLUMNS = 1 << 0,
        ENTRY_PART_CONTENT = 1 << 1
    };

    struct ExplorerEntryModifyEvent
    {
        ExplorerEntry* entry;
        int entryParts;
        bool continuousChange;
        vector<ExplorerEntry*> dependentEntries;

        ExplorerEntryModifyEvent()
            : entry()
            , entryParts()
            , continuousChange(false)
        {
        }
    };

    // Explorer aggregates available asset information in a uniform tree
    class Explorer
        : public QObject
    {
        Q_OBJECT
    public:
        Explorer();
        ~Explorer();

        void AddProvider(int subtree, IExplorerEntryProvider* provider);
        int AddColumn(const char* label, ExplorerColumn::Format format, bool visibleByDefault, const ExplorerColumnValue* values = 0, size_t numValues = 0);
        int FindColumn(const char* label) const;

        void Populate();
        ExplorerEntry* GetRoot() { return m_root.get(); }

        int GetColumnCount() const;
        const char* GetColumnLabel(int column) const;
        const ExplorerColumn* GetColumn(int column) const;

        bool GetSerializerForEntry(Serialization::SStruct* out, ExplorerEntry* entry);
        void GetActionsForEntry(ExplorerActions* actions, ExplorerEntry* entry);
        void GetCommonActions(ExplorerActions* actions, const ExplorerEntries& entries);
        void SetEntryColumn(const ExplorerEntryId& id, int column, unsigned int value, bool notify);

        QMimeData* GetMimeDataForEntries(const std::vector<ExplorerEntry*> entries);

        void LoadEntries(const ExplorerEntries& currentEntries);
        void CheckIfModified(ExplorerEntry* entry, const char* reason, bool continuousChange);
        string GetFilePathForEntry(const ExplorerEntry* entry);
        bool IsEntryAvailableLocally(const ExplorerEntry* entry);
        ExplorerEntry* FindEntryById(const ExplorerEntryId& id);
        ExplorerEntry* FindEntryById(EExplorerSubtree subtree, unsigned int id);
        ExplorerEntry* FindEntryByPath(EExplorerSubtree subtree, const char* path);
        void FindEntriesByPath(vector<ExplorerEntry*>* entries, const char* path);
        string GetCanonicalPath(EExplorerSubtree subtree, const char* path) const;
        bool HasProvider(EExplorerSubtree subtree) const;
        void GetDependingEntries(vector<ExplorerEntry*>* entries, ExplorerEntry* entry);

        static const char* IconForEntry(EExplorerEntryType type, const ExplorerEntry* entry);

        void Revert(ExplorerEntry* entry);

        // Saves an entry and marks it for edit in source control. Because the source control operations are asynchronous, the
        //  callback passed in will be triggered to after everything is complete and will tell you if the source control operation
        //  and the save operation were both successful.
        void SaveEntry(ExplorerEntry* entry);
        void SaveEntry(ExplorerEntry* entry, const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo);
        void SaveEntry(ExplorerEntry* entry, AZ::SaveCompleteCallback onSaveComplete);
        void SaveEntry(ExplorerEntry* entry, const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo, AZ::SaveCompleteCallback onSaveComplete);

        // Saves all entries and marks them for edit in source control. Because the source control operations are asynchronous, the
        //  callback passed in will be triggered to after everything is complete and will tell you if the source control operations
        //  and the save operations were successful for every entry that needed to be saved.
        void SaveAll();
        void SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo);
        void SaveAll(AZ::SaveCompleteCallback onSaveComplete);
        void SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& errorInfo, AZ::SaveCompleteCallback onSaveComplete);

        void GetUnsavedEntries(ExplorerEntries* unsavedEntries);
        void GetSaveFilenames(std::vector<string>* filenames, const ExplorerEntries& entries) const;
        bool HasAtLeastOneUndoAvailable(const ExplorerEntries& explorerEntries);

        void UndoInOrder(const ExplorerEntries& entries);
        void RedoInOrder(const ExplorerEntries& entries);
        void Undo(const ExplorerEntries& entries, int count);
        bool CanUndo(const ExplorerEntries& entries) const;
        void Redo(const ExplorerEntries& entries);
        bool CanRedo(const ExplorerEntries& entries) const;
        void GetUndoActions(vector<string>* actionNames, int maxActionCount, const ExplorerEntries& entries) const;

        void ActionRevert(ActionContext& x);
        void ActionSave(ActionContext& x);
        void ActionShowInExplorer(ActionContext& x);

        void BeginBatchChange(int subtree) { SignalBeginBatchChange(subtree); }
        void EndBatchChange(int subtree) { SignalEndBatchChange(subtree); }

    signals:
        void SignalEntryModified(ExplorerEntryModifyEvent& ev);
        void SignalEntryLoaded(ExplorerEntry* entry);
        void SignalBeginAddEntry(ExplorerEntry* entry);
        void SignalEndAddEntry();
        void SignalBeginRemoveEntry(ExplorerEntry* entry);
        void SignalEndRemoveEntry();
        void SignalEntryImported(ExplorerEntry* entry, ExplorerEntry* oldEntry);
        void SignalRefreshFilter();
        void SignalBeginBatchChange(int subtree);
        void SignalEndBatchChange(int subtree);
        void SignalEntrySavedAs(const char* oldPath, const char* newPath);
        void SignalSelectedEntryClicked(ExplorerEntry* entry);

    protected slots:
        void OnProviderSubtreeReset(int subtree);
        void OnProviderSubtreeLoadingFinished(int subtree);
        void OnProviderEntryModified(EntryModifiedEvent&);
        void OnProviderEntryAdded(int subtree, unsigned int id);
        void OnProviderEntryRemoved(int subtree, unsigned int id);
        void OnProviderBeginBatchChange(int subtree);
        void OnProviderEndBatchChange(int subtree);

    private:
        void EntryModified(ExplorerEntry* entry, bool continuousChange);
        void ResetSubtree(FolderSubtree* subtree, EExplorerSubtree subtreeIndex, const char* text, bool addLoadingEntry);
        void CreateGroups(ExplorerEntry* globalRoot, FolderSubtree* subtree, EExplorerSubtree subtreeIndex, EExplorerEntryType rootType);
        void UpdateSingleLevelGroups(FolderSubtree* subtree, EExplorerSubtree subtreeIndex);
        void UpdateGroups();
        ExplorerEntry* CreateGroupsForPath(FolderSubtree* subtree, const char* animationPath, const char* commonPathPrefix);
        void RemoveEmptyGroups(FolderSubtree* subtree);
        bool UnlinkEntryFromParent(ExplorerEntry* entry);
        void RemoveEntry(ExplorerEntry* entry);
        void RemoveChildren(ExplorerEntry* entry);
        void LinkEntryToParent(ExplorerEntry* parent, ExplorerEntry* entry);
        void SetEntryState(ExplorerEntry* entry, const vector<char>& state);
        void GetEntryState(vector<char>* state, ExplorerEntry* entry);

        _smart_ptr<ExplorerEntry> m_root;

        FolderSubtree m_subtrees[NUM_SUBTREES];
        unsigned long long m_undoCounter;

        vector<IExplorerEntryProvider*> m_providers;
        vector<IExplorerEntryProvider*> m_providerBySubtree;
        vector<ExplorerColumn> m_columns;

        unique_ptr<DependencyManager> m_dependencyManager;
    };

    enum
    {
        ACTION_DISABLED = 1 << 0,
        ACTION_IMPORTANT = 1 << 1,
        ACTION_NOT_STACKABLE = 1 << 2
    };

    struct ActionContext
    {
        bool isAsync;
        AZStd::shared_ptr<AZ::ActionOutput> output;
        std::vector<ExplorerEntry*> entries;

        QWidget* window;

        ActionContext()
            : output(AZStd::make_shared<AZ::ActionOutput>())
            , window()
            , isAsync(false)
        {
        }
    };

    struct ExplorerAction
    {
        const char* icon;
        const char* text;
        const char* description;
        int flags;

        typedef AZStd::function<void(ActionContext&)> ActionFunction;
        ActionFunction func;

        ExplorerAction()
            : icon("")
            , text("")
            , description("")
            , flags(0)
        {
        }

        ExplorerAction(const char* text, int actionFlags, const ActionFunction& function, const char* icon = "", const char* description = "")
            : text(text)
            , flags(actionFlags)
            , icon(icon)
            , description(description)
            , func(function)
        {
        }
    };

    class ExplorerActionHandler
        : public QObject
    {
        Q_OBJECT
    public:
        ExplorerActionHandler(const ExplorerAction& action)
            : m_action(action)
        {
        }
    public slots:
        void OnTriggered()
        {
            if (m_action.func)
            {
                SignalAction(m_action);
            }
        }
    signals:
        void SignalAction(const ExplorerAction& action);
    private:
        ExplorerAction m_action;
    };
}
