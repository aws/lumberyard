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
#include <Cry_Math.h>

#include <memory>
#include <vector>
#include <map>
#include "../Shared/AnimSettings.h"
#include <IFileChangeMonitor.h>
#include "SkeletonParameters.h"
#include "CharacterDefinition.h"
#include "EntryList.h"
#include "ExplorerDataProvider.h"
#include "ExplorerFileList.h"

struct IAnimationSet;

namespace CharacterTool {
    using std::unique_ptr;
    using std::vector;
    using std::map;

    struct ActionOutput;
    struct ExplorerAction;

    struct CharacterContent
    {
        enum EngineLoadState
        {
            CHARACTER_NOT_LOADED,
            CHARACTER_LOADED,
            CHARACTER_INCOMPLETE,
            CHARACTER_LOAD_FAILED
        } engineLoadState;

        CharacterDefinition cdf;
        bool hasDefinitionFile;

        CharacterContent()
            : hasDefinitionFile(true)
            , engineLoadState(CHARACTER_NOT_LOADED) {}

        void Reset()
        {
            cdf = CharacterDefinition();
        }

        void GetDependencies(vector<string>* paths) const;
        void Serialize(Serialization::IArchive& ar);
    };

    struct CDFDependencies
        : IEntryDependencyExtractor
    {
        void Extract(vector<string>* paths, const EntryBase* entry) override;
    };

    struct CHRParamsLoader
        : IEntryLoader
    {
        bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
        bool Save(EntryBase* entry, const char* filename, LoaderContext* context, string& errorString) override;
    };

    struct CDFLoader
        : IEntryLoader
    {
        bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
        bool Save(EntryBase* entry, const char* filename, LoaderContext* context, string& errorString) override;
    };

    struct CHRParamsDependencies
        : IEntryDependencyExtractor
    {
        void Extract(vector<string>* paths, const EntryBase* entry) override;
    };
}
