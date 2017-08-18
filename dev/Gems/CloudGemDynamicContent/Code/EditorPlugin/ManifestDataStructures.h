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

#pragma once

#include "ManifestTableKeys.h"

namespace DynamicContent
{
    struct ManifestFile
    {
        QString bucketPrefix;
        QString cacheRoot;
        QString hash;
        bool isManifest;
        QString keyName;
        QString localFolder;
        QString outputRoot;
        QString pakFile;
        QString platformType;


        ManifestFile(const QVariantMap& vFile)
        {
            bucketPrefix = vFile[KEY_BUCKET_PREFIX].toString();
            cacheRoot = vFile[KEY_CACHE_ROOT].toString();
            hash = vFile[KEY_HASH].toString();
            isManifest = vFile[KEY_IS_MANIFEST].toBool();
            keyName = vFile[KEY_KEY_NAME].toString();
            localFolder = vFile[KEY_LOCAL_FOLDER].toString();
            outputRoot = vFile[KEY_OUTPUT_ROOT].toString();
            pakFile = vFile[KEY_PAK_FILE].toString();
            platformType = vFile[KEY_PLATFORM_TYPE].toString();
        }

        QVariantMap GetVariantMap()
        {
            QVariantMap newFile;

            newFile[KEY_BUCKET_PREFIX] = bucketPrefix;
            newFile[KEY_CACHE_ROOT] = cacheRoot;
            newFile[KEY_HASH] = hash;
            newFile[KEY_IS_MANIFEST] = isManifest;
            newFile[KEY_KEY_NAME] = keyName;
            newFile[KEY_LOCAL_FOLDER] = localFolder;
            newFile[KEY_OUTPUT_ROOT] = outputRoot;
            newFile[KEY_PAK_FILE] = pakFile;
            newFile[KEY_PLATFORM_TYPE] = platformType;
            
            return newFile;
        }

    };

    using ManifestFiles = QList<ManifestFile>;
}