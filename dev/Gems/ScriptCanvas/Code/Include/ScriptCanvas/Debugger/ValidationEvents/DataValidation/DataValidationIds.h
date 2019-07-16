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

#include <AzCore/Math/Crc.h>

namespace ScriptCanvas
{
    namespace DataValidationIds
    {
        constexpr const char* ScopedDataConnectionId = "DV-0001";
        static const AZ::Crc32 ScopedDataConnectionCrc = AZ_CRC(ScopedDataConnectionId);

        constexpr const char* UnknownTargetEndpointId = "DV-0002";
        static const AZ::Crc32 UnknownTargetEndpointCrc = AZ_CRC(UnknownTargetEndpointId);

        constexpr const char* UnknownSourceEndpointId = "DV-0003";
        static const AZ::Crc32 UnknownSourceEndpointCrc = AZ_CRC(UnknownSourceEndpointId);

        constexpr const char* InvalidVariableTypeId = "DV-0004";
        static const AZ::Crc32 InvalidVariableTypeCrc = AZ_CRC(InvalidVariableTypeId);

        constexpr const char* ScriptEventVersionMismatchId = "DV-0005";
        static const AZ::Crc32 ScriptEventVersionMismatchCrc = AZ_CRC(ScriptEventVersionMismatchId);

    }
}
