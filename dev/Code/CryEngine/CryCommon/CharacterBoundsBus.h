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

//////////////////////////////////////////////////////////////////////////
//
// Ebus support for triggering necessary update of Entity/Object when
// associated characterInstances .chrparams file is hot loaded
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <AzCore/EBus/EBus.h>

struct ICharacterInstance;

namespace AZ {
    /**
    * listener for bounding changes coming from an character reload.
    */
    class CharacterBoundsNotification
        : public AZ::EBusTraits
    {
    public:
        virtual ~CharacterBoundsNotification() = default;

        /// Called when the Bounding Data should be re-evaluated
        virtual void OnCharacterBoundsReset() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef ICharacterInstance* BusIdType;
        //////////////////////////////////////////////////////////////////////////
    };

    using CharacterBoundsNotificationBus = EBus<CharacterBoundsNotification>;
} //AZ namespace


