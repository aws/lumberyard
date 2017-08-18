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

#include <ILog.h>
#include <QString>

class CloudCanvasLogger
{
public:

    CloudCanvasLogger(ILog* logger)
        : m_logger(logger)
    {
    }

    template<class ... Args>
    void LogError(const char* format, Args&& ... args)
    {
        AZStd::string prefixed_format(LOG_PREFIX);
        prefixed_format.append(format);
        m_logger->LogError(prefixed_format.c_str(), std::forward<Args>(args) ...);
    }

    template<class ... Args>
    void LogWarning(const char* format, Args&& ... args)
    {
        AZStd::string prefixed_format(LOG_PREFIX);
        prefixed_format.append(format);
        m_logger->LogWarning(prefixed_format.c_str(), std::forward<Args>(args) ...);
    }

    template<class ... Args>
    void Log(const char* format, Args&& ... args)
    {
        AZStd::string prefixed_format(LOG_PREFIX);
        prefixed_format.append(format);
        m_logger->Log(prefixed_format.c_str(), std::forward<Args>(args) ...);
    }

    void Log(const QString& msg)
    {
        AZStd::string prefixed_format(LOG_PREFIX);
        prefixed_format.append("%s");
        m_logger->Log(prefixed_format.c_str(), msg.toStdString().c_str());
    }

    void LogWarning(const QString& msg)
    {
        AZStd::string prefixed_format(LOG_PREFIX);
        prefixed_format.append("%s");
        m_logger->LogWarning(prefixed_format.c_str(), msg.toStdString().c_str());
    }

    void LogError(const QString& msg)
    {
        AZStd::string prefixed_format(LOG_PREFIX);
        prefixed_format.append("%s");
        m_logger->LogError(prefixed_format.c_str(), msg.toStdString().c_str());
    }

private:

    static const char* LOG_PREFIX;

    ILog* m_logger;
};

