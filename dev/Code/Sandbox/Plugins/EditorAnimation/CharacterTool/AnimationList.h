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
#include "Strings.h"
#include "Pointers.h"

#include <memory>
#include <vector>
#include <map>
#include "../Shared/AnimSettings.h"
#include <IFileChangeMonitor.h>
#include <ICryAnimation.h>
#include "ExplorerDataProvider.h"
#include "EntryList.h"
#include "AnimationContent.h"
#include "SkeletonParameters.h"

struct IAnimationSet;
struct ICharacterInstance;

namespace Serialization
{
    struct SStruct;
};

namespace AZ
{
    class ActionOutput;
}

namespace CharacterTool
{
    struct System;
    using std::unique_ptr;
    using std::vector;
    using std::map;

    class Explorer;
    struct ActionContext;

    class AnimationList
        : public IExplorerEntryProvider
        , public IFileChangeListener
        , public IAnimationSetListener
    {
        Q_OBJECT
    public:
        AnimationList(System* system, int explorerColumnFrames, int explorerColumnSize, int explorerColumnAudio, int explorerColumnPak);
        ~AnimationList();
        void Populate(ICharacterInstance* character, const char* defaultSkeletonAlias, const AnimationSetFilter& filter, const char* animEventsFilename);
        void SetAnimationFilterAndScan(const AnimationSetFilter& filter);
        void SetAnimEventsFile(const string& fileName);

        void RemoveImportEntry(const char* animationPath);

        bool IsLoaded(unsigned int id) const;
        bool ImportAnimation(AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id);
        SEntry<AnimationContent>* GetEntry(unsigned int id) const;
        bool IsNewAnimation(unsigned int id) const;
        SEntry<AnimationContent>* FindEntryByPath(const char* animationPath);
        unsigned int FindIdByAlias(const char* animationName);
        bool ResaveAnimSettings(const char* filePath);

        void OnAnimationSetAddAnimation(const char* animationPath, const char* animationName) override;
        void OnAnimationSetReload() override;
        const char* CanonicalExtension() const override { return "caf"; }

        // IExplorerDataProvider:
        int GetEntryCount(int subtree) const override;
        unsigned int GetEntryIdByIndex(int subtree, int index) const override;
        bool GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const override;
        void SetMimeDataForEntries(QMimeData* mimeData, const std::vector<unsigned int>& entryIds) const override;
        void UpdateEntry(ExplorerEntry* entry) override;
        void RevertEntry(unsigned int id) override;
        void SaveEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, AZ::SaveCompleteCallback onSaveComplete) override;
        void DeleteEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, const char * entryFullPath, const char * entryFileName) override {}
        string GetSaveFilename(unsigned int id) override;
        void SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete) override;
        int GetEntryType(int subtreeIndex) const override;
        void CheckIfModified(unsigned int id, const char* reason, bool continuousChange) override;
        void GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer) override;
        bool LoadOrGetChangedEntry(unsigned int id) override;
        bool IsRunningAsyncSaveOperation() override;
        // ^^^
    private:
        void ActionImport(ActionContext& x);
        void ActionGenerateFootsteps(ActionContext& x);
        void ActionExportHTR(ActionContext& x);
        void ActionComputeVEG(ActionContext& x);
        void ActionCopyExamplePaths(ActionContext& x);

        void SaveAnimationEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, bool notifyOfChange, AZ::SaveCompleteCallback onSaveComplete);

        unsigned int MakeNextId();
        void ReloadAnimationList();
        void OnFileChange(const char* filename, EChangeType eType) override;
        bool UpdateImportEntry(SEntry<AnimationContent>* entry);
        void UpdateAnimationEntryByPath(const char* filename);
        void ScanForImportEntries(std::vector<std::pair<ExplorerEntryId, int> >* pakColumnValues, bool resetFollows);
        void SetEntryState(ExplorerEntry* entry, const vector<char>& state);
        void GetEntryState(vector<char>* state, ExplorerEntry* entry);
        AZStd::string CreateAnimEventsPathFromFilter();
        bool AddBSpaceSaveOperation(AZStd::shared_ptr<AZ::SaveOperationController> saveEntryController, SEntry<AnimationContent>* entry, const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete) const;

        System* m_system;
        IAnimationSet* m_animationSet;
        ICharacterInstance* m_character;
        string m_defaultSkeletonAlias;
        string m_animEventsFilename;

        CEntryList<AnimationContent> m_animations;
        bool m_importEntriesInitialized;
        std::vector<string> m_importEntries;
        typedef std::map<string, unsigned int, stl::less_stricmp<string> > AliasToId;
        AliasToId m_aliasToId;

        AnimationSetFilter m_filter;

        int m_explorerColumnFrames;
        int m_explorerColumnAudio;
        int m_explorerColumnSize;
        int m_explorerColumnPak;

        string m_commonPrefix;

        bool m_isRunningSaveAll;
        bool m_isSaveRunnerInMultiSaveMode;
        AZStd::shared_ptr<AZ::AsyncSaveRunner> m_saveRunner;
    };
}
