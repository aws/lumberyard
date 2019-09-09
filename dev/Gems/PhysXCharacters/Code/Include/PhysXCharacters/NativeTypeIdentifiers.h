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

namespace PhysXCharacters
{
    namespace NativeTypeIdentifiers
    {
        static const AZ::Crc32 CharacterController = AZ_CRC("PhysXCharacterController", 0x1b8787a6);
        static const AZ::Crc32 CharacterControllerReplica = AZ_CRC("PhysXCharacterControllerReplica", 0xa6f3cfcc);
        static const AZ::Crc32 RagdollNode = AZ_CRC("PhysXRagdollNode", 0xd1d24205);
        static const AZ::Crc32 Ragdoll = AZ_CRC("PhysXRagdoll", 0xe081b8b0);
    } // namespace NativeTypeIdentifiers
} // namespace PhysXCharacters