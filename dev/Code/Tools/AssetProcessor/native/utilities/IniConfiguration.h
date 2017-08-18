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
#ifndef INICONFIGURATION_H
#define INICONFIGURATION_H

#include <QDir>
#include <QString>
#include <QCoreApplication>

/** Reads the user ini configuration file for listening port
 * and proxy Information
 */
class IniConfiguration
    : public QObject
{
    Q_OBJECT
public:
    explicit IniConfiguration(QObject* pParent = nullptr);
    virtual ~IniConfiguration();

    // Singleton pattern:
    static const IniConfiguration* Get();

    void parseCommandLine(QStringList cmdLine = QCoreApplication::arguments());
    void readINIConfigFile(QDir dir = qApp->applicationDirPath());
    quint16 listeningPort() const;
    QString proxyInformation() const;
    void SetListeningPort(quint16 port);
    void SetProxyInformation(QString info);
Q_SIGNALS:
    void ProxyInfoChanged(QString proxyInfo);

private:
    quint16 m_listeningPort;
    QString m_proxyInformation;
    QString m_userConfigFilePath;
};

#endif // INICONFIGURATION_H
