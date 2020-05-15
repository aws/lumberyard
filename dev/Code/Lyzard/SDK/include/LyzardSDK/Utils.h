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

namespace Lyzard
{
    namespace Utils
    {
        /// Utility function that returns the path to the running executable.
        /// Useful to use this without relying on ComponentApplication being up and running.
        const char* GetExecutablePath();

        /// Utility function that looks for the --app-root parameter in command-line args.
        /// Returns nullptr if not found.
        const char* GetOptionalExternalAppRoot(int argc, char** argv);

    } // namespace Utils

} // namespace Lyzard
