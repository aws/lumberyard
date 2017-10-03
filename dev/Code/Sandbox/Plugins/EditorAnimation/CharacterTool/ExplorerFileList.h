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
#include "Strings.h"
#include "Pointers.h"
#include "ExplorerDataProvider.h"
#include "EntryList.h"
#include <IFileChangeMonitor.h>
#include <memory>
#include <vector>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace CharacterTool
{
    class ExplorerFileList;

    using std::vector;
    using std::unique_ptr;

    struct System;
    struct ActionContext;
    struct LoaderContext
    {
        System* system;
        ExplorerFileList* rigList;

        LoaderContext()
            : rigList()
            , system()
        {}
    };

    struct IEntryLoader
        : _i_reference_target_t
    {
        virtual ~IEntryLoader() {}
        virtual bool Load(EntryBase* entry, const char* filename, LoaderContext* context) = 0;
        virtual bool Save(EntryBase* entry, const char* filename, LoaderContext* context, string& errorString) = 0;
    };

    struct IEntryDependencyExtractor
        : _i_reference_target_t
    {
        virtual ~IEntryDependencyExtractor() {}
        virtual void Extract(vector<string>* paths, const EntryBase* entry) = 0;
    };

    struct JSONLoader
        : IEntryLoader
    {
        bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
        bool Save(EntryBase* entry, const char* filename, LoaderContext* context, string& errorString) override;
    };

    template<class TSelf>
    struct SelfLoader
        : IEntryLoader
    {
        bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override
        {
            return ((SEntry<TSelf>*)entry)->content.Load();
        }
        bool Save(EntryBase* entry, const char* filename, LoaderContext* context, string& errorString) override
        {
            return ((SEntry<TSelf>*)entry)->content.Save();
        }
    };

    struct ScanLoadedFile;

    enum FormatUsage
    {
        FORMAT_LOAD = 1 << 0,
        FORMAT_SAVE = 1 << 1,
        FORMAT_LIST = 1 << 2,
        FORMAT_MAIN = 1 << 3
    };

    struct EntryFormat
    {
        string extension;
        string path;
        int usage;
        _smart_ptr<IEntryLoader> loader;

        string MakeFilename(const char* entryPath) const;

        EntryFormat()
            : usage()
        {
        }
    };
    typedef vector<EntryFormat> EntryFormats;

    struct EntryType
    {
        EntryListBase* entryList;
        int subtree;
        int explorerEntryType;
        bool fileListenerRegistered;
        _smart_ptr<IEntryDependencyExtractor> dependencyExtractor;
        EntryFormats formats;
        EntryType()
            : subtree(0)
            , explorerEntryType(0)
            , fileListenerRegistered(false) {}

        EntryType& AddFormat(const char* extension, IEntryLoader* loader, int usage = FORMAT_LIST | FORMAT_LOAD | FORMAT_SAVE)
        {
            EntryFormat format;
            format.extension = extension;
            format.usage = usage;
            format.loader.reset(loader);
            formats.push_back(format);
            return *this;
        }
        EntryType& AddFile(const char* path)
        {
            EntryFormat format;
            format.path = path;
            format.usage = FORMAT_LIST | FORMAT_LOAD | FORMAT_SAVE;
            formats.push_back(format);
            return *this;
        }
    };

    class ExplorerFileList
        : public IExplorerEntryProvider
        , public IFileChangeListener
        , public AzFramework::LegacyAssetEventBus::MultiHandler
    {
        Q_OBJECT
    public:
        ExplorerFileList()
            : m_loaderContext(0)
            , m_explorer(0)
            , m_explorerColumnPak(-1) { }
        ~ExplorerFileList();

        template<class T>
        EntryType& AddEntryType(int subtree, int explorerEntryType, IEntryDependencyExtractor* extractor = 0)
        {
            return AddEntryType(new CEntryList<T>(), subtree, explorerEntryType, extractor);
        }
        template<class T>
        SEntry<T>* AddSingleFile(const char* path, const char* label, int subtree, int explorerEntryType, IEntryLoader* loader)
        {
            EntryType* f = AddSingleFileEntryType(new CEntryList<T>, path, label, subtree, explorerEntryType, loader);
            return ((CEntryList<T>*)f->entryList)->AddEntry(0, path, label, false);
        }

        void SetLoaderContext(LoaderContext* loaderContext) { m_loaderContext = loaderContext; }
        void SetExplorer(Explorer* explorer);

        void SetAssetWatchType(AZ::Crc32 type);

        ExplorerEntryId AddEntry(const char* filename, bool resetIfExists);
        void AddAndSaveEntry(const char* filename, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput, AZ::SaveCompleteCallback onSaveComplete);
        EntryBase* GetEntryBaseByPath(const char* path);
        EntryBase* GetEntryBaseById(unsigned int id);
        template<class T>
        SEntry<T>* GetEntryByPath(const char* path) { return static_cast<SEntry<T>*>(GetEntryBaseByPath(path)); }
        template<class T>
        SEntry<T>* GetEntryById(unsigned int id) { return static_cast<SEntry<T>*>(GetEntryBaseById(id)); }
        void Populate();

        // IExplorerDataProvider:
        int GetEntryCount(int subtree) const override;
        unsigned int GetEntryIdByIndex(int subtree, int index) const override;
        bool GetEntrySerializer(Serialization::SStruct* out, unsigned int id) const override;
        void UpdateEntry(ExplorerEntry* entry) override;
        void RevertEntry(unsigned int id) override;
        string GetSaveFilename(unsigned int id) override;

        void SaveEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, unsigned int id, AZ::SaveCompleteCallback onSaveComplete) override;
        void DeleteEntry(const AZStd::shared_ptr<AZ::ActionOutput>& output, const char * entryFullPath, const char * entryFileName) override;
        void SaveAll(const AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete) override;
        int GetEntryType(int subtreeIndex) const override;
        void CheckIfModified(unsigned int id, const char* reason, bool continuousChange) override;
        void GetEntryActions(vector<ExplorerAction>* actions, unsigned int id, Explorer* explorer) override;
        bool HasBackgroundLoading() const override;
        bool LoadOrGetChangedEntry(unsigned int id) override;
        const char* CanonicalExtension() const override;
        void GetDependencies(vector<string>* paths, unsigned int id) override;
        bool IsRunningAsyncSaveOperation() override;
        // ^^^

        void OnFileChange(const char* filename, EChangeType eType) override;
        void OnFileChanged(AZStd::string assetPath) override;
        void DeleteEntries(ActionContext& x);

    public slots:
        void OnBackgroundFileLoaded(const ScanLoadedFile&);
        void OnBackgroundLoadingFinished(int subtree);
    signals:
        void SignalEntryDeleted(const char* path);

    private:
        string GetSaveFileExtension(const char* entryPath) const;
        void ActionSaveAs(ActionContext& x);
        void ActionDelete(ActionContext& x);
        string GetCanonicalPath(const char* path) const;
        bool CanBeSaved(const char* path) const;

        EntryBase* GetEntry(EntryId id, const EntryType** format = 0) const;
        EntryType* GetEntryTypeByExtension(const char* ext);
        const EntryType* GetEntryTypeByExtension(const char* ext) const;
        EntryType* GetEntryTypeByPath(const char* singleFile);
        const EntryType* GetEntryTypeByPath(const char* singleFile) const;
        EntryType* GetEntryTypeByExplorerEntryType(int explorerEntryType);

        EntryType& AddEntryType(EntryListBase* list, int subtree, int explorerEntryType, IEntryDependencyExtractor* extractor);
        EntryType* AddSingleFileEntryType(EntryListBase* list, const char* path, const char* label, int subtree, int explorerEntryType, IEntryLoader* loader);

        static int GetEntryFilesPakState(const EntryType& format, const char* filename);
        void UpdateEntryPakState(const ExplorerEntryId& id, int pakState);

        vector<EntryType> m_entryTypes;
        LoaderContext* m_loaderContext;
        Explorer* m_explorer;
        int m_explorerColumnPak;

        bool m_isRunningSaveAll = false;
        AZStd::shared_ptr<AZ::AsyncSaveRunner> m_saveRunner;
    };
}
