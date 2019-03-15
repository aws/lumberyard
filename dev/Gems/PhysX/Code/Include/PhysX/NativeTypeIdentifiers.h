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

namespace PhysX
{
    namespace NativeTypeIdentifiers
    {
        static const AZ::Crc32 PhysXWorld = AZ_CRC("PhysXWorld", 0x87c3e7ad);
        static const AZ::Crc32 PhysXRigidBody = AZ_CRC("PhysXRigidBody", 0xb2dcf053);
        static const AZ::Crc32 PhysXGhost = AZ_CRC("PhysXGhost", 0xbee7664f);
        static const AZ::Crc32 PhysXCharacter = AZ_CRC("PhysXCharacter", 0x21ea2dd0);
    } // namespace NativeTypeIdentifiers
} // namespace PhysX
