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

#include <AzCore/Component/ComponentApplication.h>

namespace AZ
{
    /// Calculates the Bin folder name where the application is running from (off of the engine root folder)
    void ComponentApplication::PlatformCalculateBinFolder()
    {
        azstrcpy(m_binFolder, AZ_ARRAY_SIZE(m_binFolder), "");

        if (strlen(m_exeDirectory) > 0)
        {
            bool engineMarkerFound = false;
            // Prepare a working mutable path initialized with the current exe path
            char workingExeDirectory[AZ_MAX_PATH_LEN];
            azstrcpy(workingExeDirectory, AZ_ARRAY_SIZE(workingExeDirectory), m_exeDirectory);

            const char* lastCheckFolderName = "";

            // Calculate the bin folder name by walking up the path folders until we find the 'engine.json' file marker
            size_t checkFolderIndex = strlen(workingExeDirectory);
            if (checkFolderIndex > 0)
            {
                checkFolderIndex--; // Skip the right-most trailing slash since exeDirectory represents a folder
                while (true)
                {
                    // Eat up all trailing slashes
                    while ((checkFolderIndex > 0) && workingExeDirectory[checkFolderIndex] == '/')
                    {
                        workingExeDirectory[checkFolderIndex] = '\0';
                        checkFolderIndex--;
                    }
                    // Check if the current index is the drive letter separator
                    if (workingExeDirectory[checkFolderIndex] == ':')
                    {
                        // If we reached a drive letter separator, then use the last check folder 
                        // name as the bin folder since we cant go any higher
                        break;
                    }
                    else
                    {

                        // Find the next path separator
                        while ((checkFolderIndex > 0) && workingExeDirectory[checkFolderIndex] != '/')
                        {
                            checkFolderIndex--;
                        }
                    }
                    // Split the path and folder and test for engine.json
                    if (checkFolderIndex > 0)
                    {
                        lastCheckFolderName = &workingExeDirectory[checkFolderIndex + 1];
                        workingExeDirectory[checkFolderIndex] = '\0';
                        if (CheckPathForEngineMarker(workingExeDirectory))
                        {
                            // engine.json was found at the folder, break now to register the lastCheckFolderName as the bin folder
                            engineMarkerFound = true;

                            // Set the bin folder name only if the engine marker was found
                            azstrcpy(m_binFolder, AZ_ARRAY_SIZE(m_binFolder), lastCheckFolderName);

                            break;
                        }
                        checkFolderIndex--;
                    }
                    else
                    {
                        // We've reached the beginning, break out of the loop
                        break;
                    }
                }
                AZ_Warning("ComponentApplication", engineMarkerFound, "Unable to determine Bin folder. Cannot locate engine marker file 'engine.json'");
            }
        }
    }
}
