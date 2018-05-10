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

#include <mutex>
#include <functional>

#include <QObject>
#include <QStack>

namespace News
{
    class QtDownloader;

    //! QtDownloadManager handles multiple asynchronous downloads
    class QtDownloadManager
        : public QObject
    {
        Q_OBJECT
    public:
        QtDownloadManager();
        ~QtDownloadManager();

        //! Download a file from url and return it as QByteArray
        /*!
        This function retrieves a free QtDownloader from object pool, if none are found, creates a new one
        and starts the download process.
        \param url - file url to download
        \param downloadSuccessCallback - if download is successful pass file's data as QByteArray
        \param downloadFailCallback - if download failed, pass error message
        */
        void Download(const QString& url,
            std::function<void(QByteArray)> downloadSuccessCallback,
            std::function<void()> downloadFailCallback);
        void Abort();

    private:
        QStack<QtDownloader*> m_free;
        QList<QtDownloader*> m_busy;

    private Q_SLOTS:
        void downloadFinishedSlot(QtDownloader* pDownloader);
    };
}
