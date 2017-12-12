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

#include <QNetworkAccessManager>

namespace News
{
    //! QtDownloader is a wrapper around Qt's file download functions
    class QtDownloader
        : public QObject
    {
        Q_OBJECT
    public:
        QtDownloader();
        ~QtDownloader();

        //! Download a file from url and return it as QByteArray
        /*!
        \param url - file url to download
        \param downloadSuccessCallback - if download is successful pass file's data as QByteArray
        \param downloadFailCallback - if download failed, pass error message
        */
        void Download(const QString& url,
            std::function<void(QByteArray)> downloadSuccessCallback,
            std::function<void()> downloadFailCallback);

        void Abort();

        void SetUseCertPinning(bool value);

Q_SIGNALS:
        void downloadFinishedSignal(QtDownloader*);

    private:
        QNetworkAccessManager m_networkManager;
        QNetworkReply* m_reply;
        bool m_useCertPinning = true;
        bool m_certValid = false;

        std::function<void(QByteArray)> m_downloadSuccessCallback;
        std::function<void()> m_downloadFailCallback;

    private Q_SLOTS:
        //! Performs certificate verification of the request
        /*!
        This is always called before downloadFinishedSlot when SSL/TLS session 
        has successfully completed the initial handshake. The request's public key 
        hash is then checked against local copy to verify whether certificate is valid.
        */
        void encryptedSlot(QNetworkReply* pReply);
        //! Performs actual handling of the request
        void downloadFinishedSlot(QNetworkReply* pReply);
    };
}
