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
#ifndef ASSETSCANNERWORKER_H
#define ASSETSCANNERWORKER_H
#include "native/assetprocessor.h"
#include "assetScanFolderInfo.h"
#include <QString>
#include <QSet>
#include <QObject>

namespace AssetProcessor
{
    class PlatformConfiguration;

    /** This Class is actually responsible for scanning the game folder
     * and finding file of interest files.
     * Its created on the main thread and then moved to the worker thread
     * so it should contain no QObject-based classes at construction time (it can make them later)
     */
    class AssetScannerWorker
        : public QObject
    {
        Q_OBJECT
    public:
        explicit AssetScannerWorker(PlatformConfiguration* config, QObject* parent = 0);

Q_SIGNALS:
        void ScanningStateChanged(AssetProcessor::AssetScanningStatus status);
        void FileOfInterestFound(QString filepath);

    public Q_SLOTS:
        void StartScan();
        void StopScan();

    protected:
        void ScanForSourceFiles(ScanFolderInfo scanFolderInfo);
        void EmitFiles();

    private:
        volatile bool m_doScan = true;
        QSet<QString> m_fileList; // note:  neither QSet nor QString are qobject-derived
        PlatformConfiguration* m_platformConfiguration;
    };
} // end namespace AssetProcessor

#endif // ASSETSCANNERWORKER_H
