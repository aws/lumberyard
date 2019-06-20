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

namespace AZ
{
    namespace ScriptCanvasAttributes
    {
        ///< Marks a class for internal, usually unserialized, use
        static const AZ::Crc32 AllowInternalCreation = AZ_CRC("AllowInternalCreation", 0x4817d9fb);
        ///< This is used to provide a more readable name for ScriptCanvas nodes
        static const AZ::Crc32 PrettyName = AZ_CRC("PrettyName", 0x50ac3029);
        ///< This is used to forbid variable creation of types in ScriptCanvas nodes
        static const AZ::Crc32 VariableCreationForbidden = AZ_CRC("VariableCreationForbidden", 0x0034f5bb);
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeSlots = AZ_CRC("AutoUnpackOutputOutcomeSlots", 0x2664162d);
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeFailureSlotName= AZ_CRC("AutoUnpackOutputOutcomeFailureSlotName", 0xfc72dd3f);
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeSuccessSlotName = AZ_CRC("AutoUnpackOutputOutcomeSuccessSlotName", 0x22ac22d5);
        ///< Used to mark a function as a tuple retrieval function. The value for Index is what index is used to retrive the function
        static const AZ::Crc32 TupleGetFunctionIndex = AZ_CRC("TupleGetFunction", 0x50020c16);

        ///< Used to mark a function as a floating function separate from whatever class reflected it.
        static const AZ::Crc32 FloatingFunction = AZ_CRC("FloatingFunction", 0xdcf978f9);
    }
}
