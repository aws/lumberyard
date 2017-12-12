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
#include "Strings.h"
#include "Pointers.h"
#include <IBackgroundTaskManager.h>
#include <vector>

namespace CharacterTool
{
    using std::vector;

    struct SLookupRule
    {
        vector<string> masks;

        SLookupRule()
        {
        }
    };

    struct ScanLoadedFile
    {
        string scannedFile;
        string loadedFile;
        vector<char> content;
        bool fromPak;
        bool fromDisk;

        ScanLoadedFile()
            : fromPak(false)
            , fromDisk(false) {}
    };

    struct SScanAndLoadFilesTask
        : QObject
        , IBackgroundTask
    {
        Q_OBJECT
    public:
        SLookupRule m_rule;
        int m_index;
        vector<ScanLoadedFile> m_loadedFiles;
        string m_description;

        SScanAndLoadFilesTask(const SLookupRule& rule, const char* description, int index);
        ETaskResult Work() override;
        void Finalize() override;
        const char* Description() const { return m_description; }
        void Delete() override { delete this; }

    signals:
        void SignalFileLoaded(const ScanLoadedFile& loadedFile);
        void SignalLoadingFinished(int index);
    };
}
