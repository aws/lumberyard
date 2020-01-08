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
#include "../EditorCommon/QPropertyTree/ContextList.h"
#include "AnimationList.h"
#include "AnimationTagList.h"
#include "CharacterDocument.h"
#include "CharacterList.h"
#include "CharacterPhysics.h"
#include "CharacterRig.h"
#include "SkeletonList.h"
#include "EditorCompressionPresetTable.h"
#include "EditorDBATable.h"
#include "ExplorerFileList.h"
#include "EntryListImpl.h"
#include "ExplorerNavigationProvider.h"
#include "FilterAnimationList.h"
#include "GizmoSink.h"
#include "CharacterToolSystem.h"
#include <CryExtension/CryCreateClassInstance.h>
#include <Cry_Camera.h>
#include <Cry_Geo.h>
#include <Cry_Math.h>
#include <ISystem.h>
#include <smartptr.h>
#include "SkeletonContent.h"
#include "SceneContent.h"
#include "SourceAssetContent.h"
#include "AnimationCompressionManager.h"
#include "CharacterGizmoManager.h"
#include "DependencyManager.h"
#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "../EditorCommon/Serialization/Decorators/INavigationProvider.h"

namespace CharacterTool
{
    System::System()
        : loaded(false)
        , dbaTable(0)
        , compressionPresetTable(0)
        , compressionSkeletonList(0)
    {
    }

    System::~System()
    {
    }

    void System::Initialize()
    {
        scene.reset(new SceneContent());
        document.reset(new CharacterDocument(this));

        filterAnimationList.reset(new FilterAnimationList());
        contextList.reset(new Serialization::CContextList());
        explorer.reset(new Explorer());
        int columnFrames = explorer->AddColumn("Frames", ExplorerColumn::FRAMES, false);
        int columnSize = explorer->AddColumn("Size", ExplorerColumn::FILESIZE, false);

        const ExplorerColumnValue audioValues[] = {
            { "No audio events", "" },
            { "Audio events present", "Editor/Icons/animation/audio_event_16.png" },
        };
        int audioValueCount = sizeof(audioValues) / sizeof(audioValues[0]);
        explorerColumnAudio = explorer->AddColumn("Audio", ExplorerColumn::ICON, false, audioValues, audioValueCount);

        // Matches the order of enum PakState
        const ExplorerColumnValue pakValues[] = {
            { "None", "" },
            { "Loose file(s)", "Editor/Icons/animation/in_folder_16.png" },
            { "Pak file(s)", "Editor/Icons/animation/in_pak_16.png" },
            { "Pak and loose file(s)", "Editor/Icons/animation/in_folder_and_pak_16.png" },
        };
        int pakValueCount = sizeof(pakValues) / sizeof(pakValues[0]);
        explorerColumnPak = explorer->AddColumn("Pak", ExplorerColumn::ICON, false, pakValues, pakValueCount);

        loaderContext.reset(new LoaderContext());
        loaderContext->system = this;

        compressionGlobalList.reset(new ExplorerFileList());
        compressionGlobalList->SetLoaderContext(loaderContext.get());
        compressionPresetTable = &compressionGlobalList->AddSingleFile<EditorCompressionPresetTable>("Animations/CompressionPresets.json", "Compression Presets", SUBTREE_COMPRESSION, ENTRY_COMPRESSION_PRESETS, new SelfLoader<EditorCompressionPresetTable>())->content;
        dbaTable = &compressionGlobalList->AddSingleFile<EditorDBATable>("Animations/DBATable.json", "DBA Table", SUBTREE_COMPRESSION, ENTRY_DBA_TABLE, new SelfLoader<EditorDBATable>())->content;
        compressionSkeletonList = &compressionGlobalList->AddSingleFile<SkeletonList>("Animations/SkeletonList.xml", "Skeleton List", SUBTREE_COMPRESSION, ENTRY_SKELETON_LIST, new SelfLoader<SkeletonList>())->content;
        explorer->AddProvider(SUBTREE_COMPRESSION, compressionGlobalList.get());

        animationTagList.reset(new AnimationTagList(compressionPresetTable, dbaTable));
        animationList.reset(new AnimationList(this, columnFrames, columnSize, explorerColumnAudio, explorerColumnPak));
        explorer->AddProvider(SUBTREE_ANIMATIONS, animationList.get());

#if 0
        // .rig and .phys formats are temporarily disabled
        physicsList.reset(new CExplorerFileList());
        physicsList->SetLoaderContext(loaderContext.get());
        physicsList->AddEntryType<SCharacterPhysicsContent>(SUBTREE_PHYSICS, ENTRY_PHYSICS)
            .AddFormat("phys", new SJSONLoader())
        ;
        explorer->AddProvider(SUBTREE_PHYSICS, physicsList.get());

        rigList.reset(new CExplorerFileList());
        rigList->SetLoaderContext(loaderContext.get());
        rigList->AddEntryType<SCharacterRigContent>(SUBTREE_RIGS, ENTRY_RIG)
            .AddFormat("rig", new SJSONLoader())
        ;
        explorer->AddProvider(SUBTREE_RIGS, rigList.get());
#endif

        gizmoSink.reset(new GizmoSink());
        characterSpaceProvider.reset(new CharacterSpaceProvider(document.get()));

        skeletonList.reset(new ExplorerFileList());
        skeletonList->SetLoaderContext(loaderContext.get());
        skeletonList->AddEntryType<SkeletonContent>(SUBTREE_SKELETONS, ENTRY_SKELETON, new CHRParamsDependencies())
            .AddFormat("chrparams", new CHRParamsLoader(), FORMAT_MAIN | FORMAT_LIST | FORMAT_SAVE | FORMAT_LOAD)
            .AddFormat("chr", 0, FORMAT_LIST)
        ;
        explorer->AddProvider(SUBTREE_SKELETONS, skeletonList.get());

        characterList.reset(new ExplorerFileList());
        characterList->SetLoaderContext(loaderContext.get());
        characterList->AddEntryType<CharacterContent>(SUBTREE_CHARACTERS, ENTRY_CHARACTER, new CDFDependencies())
            .AddFormat("cdf", new CDFLoader(), FORMAT_LIST | FORMAT_LOAD | FORMAT_SAVE)
        ;
        explorer->AddProvider(SUBTREE_CHARACTERS, characterList.get());

#if 0
        // fbx prototype disabled in mainline
        sourceAssetList.reset(new ExplorerFileList());
        sourceAssetList->SetLoaderContext(loaderContext.get());
        sourceAssetList->AddEntryType<SourceAssetContent>(SUBTREE_SOURCE_ASSETS, ENTRY_SOURCE_ASSET)
            .AddFormat("fbx", new RCAssetLoader(), FORMAT_LIST | FORMAT_LOAD)
            .AddFormat("import", new JSONLoader(), FORMAT_LOAD | FORMAT_SAVE)
        ;
        explorer->AddProvider(SUBTREE_SOURCE_ASSETS, sourceAssetList.get());
#endif

        explorerNavigationProvider.reset(CreateExplorerNavigationProvider(this));

        document->ConnectExternalSignals();

        contextList->Update(this);
        contextList->Update(document.get());
        contextList->Update(static_cast<ITagSource*>(animationTagList.get()));
        contextList->Update(compressionSkeletonList);
        contextList->Update(compressionPresetTable);
        contextList->Update(dbaTable);
        contextList->Update(explorerNavigationProvider.get());
        contextList->Update<const FilterAnimationList>(filterAnimationList.get());
        characterGizmoManager.reset(new CharacterGizmoManager(this));
    }

    void System::LoadGlobalData()
    {
        if (loaded)
        {
            return;
        }
        characterList->Populate();
        skeletonList->Populate();
        if (physicsList)
        {
            physicsList->Populate();
        }
        if (rigList)
        {
            rigList->Populate();
        }
        explorer->Populate();

        filterAnimationList->Populate();
        compressionPresetTable->SetFilterAnimationList(filterAnimationList.get());
        compressionPresetTable->Load();
        dbaTable->SetFilterAnimationList(filterAnimationList.get());
        dbaTable->Load();
        compressionSkeletonList->Load();
        compressionGlobalList->Populate();
        if (sourceAssetList)
        {
            sourceAssetList->Populate();
        }
        loaded = true;
    }

    void System::Serialize(Serialization::IArchive& ar)
    {
        ar(*document, "document");
    }
}

#include <CharacterTool/CharacterToolSystem.moc>
