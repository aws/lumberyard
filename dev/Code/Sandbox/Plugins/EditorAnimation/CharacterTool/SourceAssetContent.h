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

#include "Serialization.h"
#include "../Shared/SourceAssetScene.h"
#include "../Shared/SourceAssetSettings.h"
#include "ExplorerFileList.h" // IEntryLoader

namespace CharacterTool
{
    struct CreateAssetManifestTask;
    struct SourceAssetContent
    {
        enum State
        {
            STATE_EMPTY,
            STATE_LOADING,
            STATE_LOADED
        };

        State state;

        SourceAsset::Scene scene;
        SourceAsset::Settings settings;
        bool changingView;

        CreateAssetManifestTask* loadingTask;

        SourceAssetContent()
            : state(STATE_EMPTY)
            , changingView(false)
            , loadingTask()
        {
        }

        void Reset()
        {
            *this = SourceAssetContent();
        }

        void Serialize(IArchive& ar);
    };

    struct RCAssetLoader
        : IEntryLoader
    {
        bool Load(EntryBase* entry, const char* filename, LoaderContext* context) override;
        bool Save(EntryBase* entry, const char* filename, LoaderContext* context, string& errorString) override { return false; }
    };
}
