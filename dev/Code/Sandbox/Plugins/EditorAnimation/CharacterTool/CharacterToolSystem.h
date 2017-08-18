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

#include <memory>
#include <QObject>

class CAnimationCompressionManager;

namespace Serialization
{
    class IArchive;
    class CContextList;
    struct SContextLink;
    struct INavigationProvider;
}

namespace CharacterTool {
    using std::unique_ptr;
    class AnimationList;
    class AnimationTagList;
    class CharacterGizmoManager;
    class CharacterSpaceProvider;
    class CharacterDocument;
    class EditorDBATable;
    class EditorCompressionPresetTable;
    class Explorer;
    class ExplorerFileList;
    class FilterAnimationList;
    class GizmoSink;
    class SkeletonList;
    struct CharacterDefinition;
    struct DisplayOptions;
    struct EntryModifiedEvent;
    struct ExplorerEntry;
    struct LoaderContext;
    struct SceneContent;

    struct System
        : public QObject
    {
        Q_OBJECT
    public:
        bool loaded;
        unique_ptr<SceneContent> scene;
        unique_ptr<CharacterDocument> document;
        int explorerColumnAudio;
        int explorerColumnPak;
        unique_ptr<Explorer> explorer;
        unique_ptr<LoaderContext> loaderContext;
        unique_ptr<CAnimationCompressionManager> animationCompressionManager;

        // explorer data providers:
        unique_ptr<AnimationList> animationList;
        unique_ptr<ExplorerFileList> characterList;
        unique_ptr<ExplorerFileList> physicsList;
        unique_ptr<ExplorerFileList> rigList;
        unique_ptr<ExplorerFileList> skeletonList;
        unique_ptr<ExplorerFileList> compressionGlobalList;
        unique_ptr<ExplorerFileList> sourceAssetList;
        EditorDBATable* dbaTable;
        EditorCompressionPresetTable* compressionPresetTable;
        SkeletonList* compressionSkeletonList;
        // ^^^

        // serialization contexts
        unique_ptr<Serialization::CContextList> contextList;
        unique_ptr<GizmoSink> gizmoSink;
        unique_ptr<Serialization::INavigationProvider> explorerNavigationProvider;
        unique_ptr<CharacterSpaceProvider> characterSpaceProvider;
        unique_ptr<FilterAnimationList> filterAnimationList;
        unique_ptr<AnimationTagList> animationTagList;
        // ^^^

        // Declaring this after the animationTagList prevents a crash on shutdown!
        unique_ptr<CharacterGizmoManager> characterGizmoManager;

        System();
        ~System();
        void Serialize(Serialization::IArchive& ar);
        void Initialize();
        void LoadGlobalData();
    };
}
