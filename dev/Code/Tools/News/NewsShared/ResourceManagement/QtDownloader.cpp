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

#include "QtDownloader.h"

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslKey>

using namespace News;

static const char* s_serverHash = "+77ArWoHPEIIqyPWDyw00dbEbpeZgGqvEKC01WXtpzQ=";

QtDownloader::QtDownloader()
    : QObject()
{
    connect(&m_networkManager, &QNetworkAccessManager::encrypted,
        this, &QtDownloader::encryptedSlot);
    connect(&m_networkManager, &QNetworkAccessManager::finished,
        this, &QtDownloader::downloadFinishedSlot);
}

QtDownloader::~QtDownloader() {}

void QtDownloader::Download(const QString& url,
    std::function<void(QByteArray)> downloadSuccessCallback,
    std::function<void()> downloadFailCallback)
{
    m_downloadSuccessCallback = downloadSuccessCallback;
    m_downloadFailCallback = downloadFailCallback;
    m_reply = m_networkManager.get(QNetworkRequest(QUrl(url)));
}

void QtDownloader::Abort()
{
    if (m_reply)
    {
        m_reply->abort();
    }
}

void QtDownloader::SetUseCertPinning(bool value)
{
    m_useCertPinning = value;
}

void QtDownloader::encryptedSlot(QNetworkReply* pReply)
{
    if (m_useCertPinning)
    {
        m_certValid = false;
        QSslCertificate cert = pReply->sslConfiguration().peerCertificate();
        QString serverHash = QCryptographicHash::hash(cert.publicKey().toDer(),
                QCryptographicHash::Sha256).toBase64();

        if (QString(s_serverHash).compare(serverHash) == 0)
        {
            m_certValid = true;
        }
    }
}

void QtDownloader::downloadFinishedSlot(QNetworkReply* pReply)
{
    if (pReply->error() == QNetworkReply::NoError && (m_certValid || !m_useCertPinning))
    {
        m_downloadSuccessCallback(pReply->readAll());
    }
    else
    {
        m_downloadFailCallback();
    }
    pReply->deleteLater();
    m_reply = nullptr;
    emit downloadFinishedSignal(this);
}

#include "NewsShared/ResourceManagement/QtDownloader.moc"
