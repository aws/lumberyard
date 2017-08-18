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
#include "native/utilities/IniConfiguration.h"
#include "native/utilities/assetUtils.h"
#include <QSettings>
#include <AzCore/Debug/Trace.h>
#include "native/assetprocessor.h"

namespace
{
    // singleton Pattern
    IniConfiguration* s_singleton = nullptr;
}

IniConfiguration::IniConfiguration(QObject* pParent)
    : QObject(pParent)
    , m_proxyInformation(QString())
    , m_listeningPort(0)
{
    Q_ASSERT(s_singleton == nullptr);
    s_singleton = this;
}

IniConfiguration::~IniConfiguration()
{
}

const IniConfiguration* IniConfiguration::Get()
{
    return s_singleton;
}

void IniConfiguration::parseCommandLine(QStringList args)
{
    for (QString arg : args)
    {
        if (arg.startsWith("--port="))
        {
            bool converted = false;
            quint16 port = arg.replace("--port=", "").toUInt(&converted);
            m_listeningPort = converted ? port : m_listeningPort;
        }
    }
}

void IniConfiguration::SetProxyInformation(QString info)
{
    if (info.isEmpty())
    {
        info = QString("127.0.0.1:%1").arg(m_listeningPort);
    }

    if (m_proxyInformation.compare(info) != 0)
    {
        QSettings saver(m_userConfigFilePath, QSettings::IniFormat);
        saver.setValue("AssetProcessorProxyInformation", info);
        m_proxyInformation = info;
        Q_EMIT ProxyInfoChanged(m_proxyInformation);
    }
}

void IniConfiguration::readINIConfigFile(QDir dir)
{
    m_userConfigFilePath = dir.filePath("AssetProcessorConfiguration.ini");
    QSettings loader(m_userConfigFilePath, QSettings::IniFormat);
    // Deleting the key that we used to use for reading the listening port,
    // we now read it from the bootstrap file
    loader.remove("AssetProcessorListeningPort");
    m_listeningPort = AssetUtilities::ReadListeningPortFromBootstrap();
    QString proxyInfo = loader.value("AssetProcessorProxyInformation", QString()).toString();
    if (proxyInfo.isEmpty())
    {
        proxyInfo = QString("localhost:%1").arg(m_listeningPort);
    }
    SetProxyInformation(proxyInfo);
}

quint16 IniConfiguration::listeningPort() const
{
    return m_listeningPort;
}

void IniConfiguration::SetListeningPort(quint16 port)
{
    m_listeningPort = port;
}

QString IniConfiguration::proxyInformation() const
{
    return m_proxyInformation;
}

#include <native/utilities/IniConfiguration.moc>


