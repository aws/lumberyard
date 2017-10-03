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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>

#include "Styling/StyleHelper.h"

namespace GraphCanvas
{
    static const AZ::Crc32 ColorPaletteManagerServiceCrc = AZ_CRC("GraphCanvas_ColorPaletteManagerService", 0x6495addb);

    //! ColorPaletteBus
    class ColorPaletteManagerRequests
        : public AZ::EBusTraits
    {
    public:
        //! The key here is the scene Id for the scene that is being created.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void RefreshColorPalette() = 0;
        
        virtual const Styling::StyleHelper* FindColorPalette(const AZStd::string& paletteString) = 0;
        virtual const Styling::StyleHelper* FindDataColorPalette(const AZ::Uuid& uuid) = 0;
    };
    
    using ColorPaletteManagerRequestBus = AZ::EBus<ColorPaletteManagerRequests>;
}