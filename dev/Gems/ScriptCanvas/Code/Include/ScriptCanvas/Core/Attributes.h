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

namespace ScriptCanvas
{
    namespace Attributes
    {
        namespace Slot
        {
            const static AZ::Crc32 Type = AZ_CRC("SlotType", 0x53811c3e);
        }

        namespace Node
        {
            //! ScriptCanvas needs to know whether some nodes should be executed as soon as the graph is 
            //! activated. This is the case of the OnGraphStart event, but it's valid for any nodes
            //! that need to process without an explicit execution in signal.
            const static AZ::Crc32 GraphEntryPoint = AZ_CRC("ScriptCanvasGraphEntryPoint", 0xa3702458);
        }

        const static AZ::Crc32 Input = AZ_CRC("Input", 0xd82832d7);
        const static AZ::Crc32 Output = AZ_CRC("Output", 0xccde149e);
        const static AZ::Crc32 Setter = AZ_CRC("Setter", 0x7c825e44);
        const static AZ::Crc32 Getter = AZ_CRC("Getter", 0xe4c51ec9);
        const static AZ::Crc32 AutoExpose = AZ_CRC("AutoExpose", 0xb29a8440);
        const static AZ::Crc32 Contract  = AZ_CRC("Contract", 0xe98f2859);

        const static AZ::Crc32 AllowSetterSlot  = AZ_CRC("AllowSetterSlot", 0xfe7e175b);
        const static AZ::Crc32 AllowGetterSlot  = AZ_CRC("AllowGetterSlot", 0xd03b36c9);
        const static AZ::Crc32 ShowSetterByDefault  = AZ_CRC("ShowSetterByDefault", 0x482bc9f4);
        const static AZ::Crc32 ShowGetterByDefault  = AZ_CRC("ShowGetterByDefault", 0x863533d8);
    }
}
