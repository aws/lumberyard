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

#include <QObject>
#include <vector>
#include "Pointers.h"
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SaveUtilities/AsyncSaveRunner.h>

namespace Serialization
{
    struct SStruct;
    class IArchive;
};

namespace AZ
{
    class ActionOutput;
}

namespace CharacterTool
{
    using std::vector;
    struct ExplorerEntry;
    struct ExplorerAction;
    class Explorer;

    enum EntryPakState
    {
        PAK_STATE_LOADING,
        PAK_STATE_LOOSE_FILES = 1 << 0,
        PAK_STATE_PAK = 1 << 1,
        PAK_STATE_PAK_AND_LOOSE_FILES = PAK_STATE_PAK | PAK_STATE_LOOSE_FILES
    };

    enum EntryAudioState
    {
        ENTRY_AUDIO_NONE,
        ENTRY_AUDIO_PRESENT
    };

    struct ExplorerEntryId
    {
        int subtree;
        unsigned int id;

        ExplorerEntryId(int subtree, unsigned int id)
            : subtree(subtree)
            , id(id)    {}
        ExplorerEntryId()
            : subtree(-1)
            , id(0)  { }

        bool operator==(const ExplorerEntryId& rhs) const { return subtree == rhs.subtree && id == rhs.id; }

        void Serialize(Serialization::IArchive& ar);
    };

    struct EntryModifiedEvent
    {
        int subtree;
        unsigned int id;
        const char* reason;
        bool contentChanged;
        bool continuousChange;
        vector<char> previousContent;

        EntryModifiedEvent()
            : subtree(-1)
            , id(0)
            , reason("")
            , contentChanged(false)
            , continuousChange(false)
        {
        }
    };

    class IExplorerEntryProvider
        : public QObject
    {
        Q_OBJECT
    public:
        virtual void UpdateEntry(ExplorerEntry* entry) = 0;
        virtual bool GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const = 0;
        virtual void SetMimeDataForEntries(QMimeData* mimeData, const std::vector<unsigned int>& entryIds) const {}
        virtual void GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer) = 0;

        virtual int GetEntryCount(int subtree) const = 0;
        virtual unsigned int GetEntryIdByIndex(int subtree, int index) const = 0;
        virtual int GetEntryType(int subtree) const = 0;
        virtual bool LoadOrGetChangedEntry(unsigned int id) { return true; }
        virtual void CheckIfModified(unsigned int id, const char* reason, bool continuous) = 0;
        virtual bool HasBackgroundLoading() const{ return false; }

        virtual void SaveEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, AZ::SaveCompleteCallback onSaveComplete) = 0;
        virtual void DeleteEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, const char * entryFullPath, const char * entryFileName) = 0;
        virtual string GetSaveFilename(unsigned int id) = 0;
        virtual void RevertEntry(unsigned int id) = 0;
        virtual void SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete) = 0;
        virtual void SetExplorer(Explorer* explorer) {}
        virtual const char* CanonicalExtension() const { return ""; }
        virtual void GetDependencies(vector<string>* paths, unsigned int id) {}

        virtual bool IsRunningAsyncSaveOperation() = 0;

    signals:
        void SignalSubtreeReset(int subtree);
        void SignalEntryModified(EntryModifiedEvent& ev);
        void SignalEntryAdded(int subtree, unsigned int id);
        void SignalEntryRemoved(int subtree, unsigned int id);
        void SignalEntrySavedAs(const char* oldName, const char* newName);
        void SignalEntry(int subtree, unsigned int id);
        void SignalSubtreeLoadingFinished(int subtree);
        void SignalBeginBatchChange(int subtree);
        void SignalEndBatchChange(int subtree);

    protected:
        int m_subtree;
    };
}
