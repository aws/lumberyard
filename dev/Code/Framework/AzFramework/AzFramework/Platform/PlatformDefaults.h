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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

// On IOS builds IOS will be defined and interfere with the below enums
#pragma push_macro("IOS")
#undef IOS

namespace AzFramework
{
    constexpr char PlatformPC[] = "pc";
    constexpr char PlatformES3[] = "es3";
    constexpr char PlatformIOS[] = "ios";
    constexpr char PlatformOSX[] = "osx_gl";
    constexpr char PlatformXenia[] = "xenia";
    constexpr char PlatformProvo[] = "provo";
    constexpr char PlatformSalem[] = "salem";
    constexpr char PlatformServer[] = "server";
    constexpr char PlatformAll[] = "all";
    constexpr char PlatformAllCient[] = "all_client";

    //! This platform enum have platform values in sequence and can also be used to get the platform count.
    enum PlatformId : int
    {
        Invalid = -1,
        PC,
        ES3,
        IOS,
        OSX,
        XENIA,
        PROVO,
        SALEM,
        SERVER, // Corresponds to the customer's flavor of "server" which could be windows, ubuntu, etc
        ALL,
        ALL_CLIENT,

        // Add new platforms above this
        NumPlatformIds,
    };
    constexpr int NumClientPlatforms = 7;
    constexpr int NumPlatforms = NumClientPlatforms + 1; // 1 "Server" platform currently
    enum class PlatformFlags : AZ::u32
    {
        Platform_NONE = 0x00,
        Platform_PC = 1 << PlatformId::PC,
        Platform_ES3 = 1 << PlatformId::ES3,
        Platform_IOS = 1 << PlatformId::IOS,
        Platform_OSX = 1 << PlatformId::OSX,
        Platform_XENIA = 1 << PlatformId::XENIA,
        Platform_PROVO = 1 << PlatformId::PROVO,
        Platform_SALEM = 1 << PlatformId::SALEM,
        Platform_SERVER = 1 << PlatformId::SERVER,

        // A special platform that will always correspond to all platforms, even if new ones are added
        Platform_ALL = 1ULL << 30,

        // A special platform that will always correspond to all non-server platforms, even if new ones are added
        Platform_ALL_CLIENT = 1ULL << 31,

        AllNamedPlatforms = Platform_PC | Platform_ES3 | Platform_IOS | Platform_OSX | Platform_XENIA | Platform_PROVO | Platform_SALEM | Platform_SERVER,
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PlatformFlags);

    extern const char* PlatformNames[PlatformId::NumPlatformIds];

    //! Platform Helper is an utility class that can be used to retrieve platform related information
    class PlatformHelper
    {
    public:

        //! Given a platformIndex returns the platform name
        static const char* GetPlatformName(const PlatformId& platform);

        //! Given a platform name returns a platform index. 
        //! If the platform is not found, the method returns -1. 
        static int GetPlatformIndexFromName(const char* platformName);

        //! Given a platformIndex returns the platformFlags
        static PlatformFlags GetPlatformFlagFromPlatformIndex(const PlatformId& platform);

        //! Given a platformFlags returns all the platform identifiers that are set.
        static AZStd::vector<AZStd::string> GetPlatforms(const PlatformFlags& platformFlags);
        //! Given a platformFlags returns all the platform identifiers that are set, with special flags interpreted.  Do not use the result for saving
        static AZStd::vector<AZStd::string> GetPlatformsInterpreted(const PlatformFlags& platformFlags);

        //! Given a platformFlags return a list of PlatformId indices
        static AZStd::vector<PlatformId> GetPlatformIndices(const PlatformFlags& platformFlags);
        //! Given a platformFlags return a list of PlatformId indices, with special flags interpreted.  Do not use the result for saving
        static AZStd::vector<PlatformId> GetPlatformIndicesInterpreted(const PlatformFlags& platformFlags);

        //! Given a platform identifier returns its corresponding platform flag.
        static PlatformFlags GetPlatformFlag(const AZStd::string& platform);

        //! Given any platformFlags returns a string listing the input platforms
        static AZStd::string GetCommaSeparatedPlatformList(const PlatformFlags& platformFlags);

        //! If platformFlags contains any special flags, they are removed and replaced with the normal flags they represent
        static PlatformFlags GetPlatformFlagsInterpreted(const PlatformFlags& platformFlags);

        //! Returns true if platformFlags contains any special flags
        static bool IsSpecialPlatform(const PlatformFlags& platformFlags);

        //! Returns true if platformFlags has checkPlatform flag set.
        static bool HasPlatformFlag(PlatformFlags platformFlags, PlatformId checkPlatform);
    };
}

#pragma pop_macro("IOS")
