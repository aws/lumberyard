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

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QMap>
#include <QString>
#include <QMutex>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    //! Utility class for engine configuration related activities
    /*!
    The helper functions defined here are designed to provide engine related support for any tool application
    */
    struct EngineConfiguration
    {
        //! The name of the engine confiuguration file that must be present in any root folder.
        static const char* s_fileName;

        //! Function to determine values from the current root folder's engine.json file.
        //!
        //! \param[in] rootFolderPath The full path to the directory that contains the engine configuration file
        //! \param[in] key The key to lookup in the engine configuration file
        //! \return AZ::Outcome The result of the lookup for a particular key value
        static AZ::Outcome<AZStd::string, AZStd::string> ReadValue(const AZStd::string& rootFolderPath, const AZStd::string& key);
    };
} // namespace AzToolsFramework

