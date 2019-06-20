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
#include <QFile>
#include <AzCore/Debug/Trace.h>
#include "native/assetprocessor.h"

namespace
{
    // singleton Pattern
    IniConfiguration* s_singleton = nullptr;
}

IniConfiguration::IniConfiguration(QObject* pParent)
    : QObject(pParent)
    , m_listeningPort(0)
{
    AZ_Assert(s_singleton == nullptr, "Duplicate singleton installation detected.");
    s_singleton = this;
}

IniConfiguration::~IniConfiguration()
{
    AZ_Assert(s_singleton == this, "There should always be a single singleton!");
    s_singleton = nullptr;
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

void IniConfiguration::readINIConfigFile(QDir dir)
{
    m_userConfigFilePath = dir.filePath("AssetProcessorConfiguration.ini");
    // if AssetProcessorProxyInformation.ini file exists then delete it
    // we used to store proxy info in this file
    if (QFile::exists(m_userConfigFilePath))
    {
        QFile::remove(m_userConfigFilePath);
    }
    
    m_listeningPort = AssetUtilities::ReadListeningPortFromBootstrap();
}

quint16 IniConfiguration::listeningPort() const
{
    return m_listeningPort;
}

void IniConfiguration::SetListeningPort(quint16 port)
{
    m_listeningPort = port;
}

#include <native/utilities/IniConfiguration.moc>


