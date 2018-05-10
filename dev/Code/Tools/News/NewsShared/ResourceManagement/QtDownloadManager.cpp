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

#include "QtDownloadManager.h"
#include "QtDownloader.h"

using namespace News;

QtDownloadManager::QtDownloadManager()
    : QObject() {}

QtDownloadManager::~QtDownloadManager() {}

void QtDownloadManager::Download(const QString& url,
    std::function<void(QByteArray)> downloadSuccessCallback,
    std::function<void()> downloadFailCallback)
{
    QtDownloader* pDownloader;
    // if there is a free downloader, use it
    if (m_free.size() > 0)
    {
        pDownloader = m_free.pop();
    }
    // otherwise create a new one
    else
    {
        pDownloader = new QtDownloader;
        connect(
            pDownloader, &QtDownloader::downloadFinishedSignal,
            this, &QtDownloadManager::downloadFinishedSlot);
        m_busy.append(pDownloader);
    }

    pDownloader->Download(url,
        downloadSuccessCallback,
        downloadFailCallback);
}

void QtDownloadManager::Abort()
{
    for (auto downloader : m_busy)
    {
        downloader->Abort();
    }
}

void QtDownloadManager::downloadFinishedSlot(QtDownloader* pDownloader)
{
    // once finished move QtDownloader to a free stack
    m_busy.removeAll(pDownloader);
    m_free.push(pDownloader);
}

#include "NewsShared/ResourceManagement/QtDownloadManager.moc"
