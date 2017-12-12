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

#include <AzCore/Outcome/Outcome.h>

#include <QMap>
#include <QString>
#include <QMutex>

namespace AzToolsFramework
{
    //! Static utility class for engine related activities
    /*!
    The helper functions defined here are designed to provide engine related support for any tool application
    */
    class EngineUtilities
    {
    public:
        //! Function to determine values from the current root folder's engine.json file.
        //!
        //! \param[in] currentAssetRoot	The current root asset folder (parent of the game asset folder) to look for the engine.json configuration file.
        //! \param[in] key The key to lookup in the engine configuration file
        //! \return AZ::Outcome The result of the lookup for a particular key value
        static AZ::Outcome<AZStd::string, AZStd::string> ReadEngineConfigurationValue(const AZStd::string& currentAssetRoot, const AZStd::string& key);

    private:
        static QMap<QString, QMap<QString, QString> >	m_engineConfigurationCache;
        static QMutex									m_engineConfigurationCacheMutex;
    };
} // namespace AzToolsFramework

