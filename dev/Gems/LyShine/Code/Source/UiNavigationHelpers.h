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

#include <LyShine/Bus/UiTransformBus.h>
#include <IInput.h>

namespace UiNavigationHelpers
{
    using ValidationFunction = std::function<bool(AZ::EntityId)>;

    //! Find the next element given the current element and a direction
    AZ::EntityId GetNextElement(AZ::EntityId curEntityId, EKeyId keyId,
        const LyShine::EntityArray& navigableElements, AZ::EntityId defaultEntityId, ValidationFunction isValidResult);

    //! Find the next element in the given direction for automatic mode
    AZ::EntityId SearchForNextElement(AZ::EntityId curElement, EKeyId keyId, const LyShine::EntityArray& navigableElements);

    //! Follow one of the custom links (used when navigation mode is custom)
    AZ::EntityId FollowCustomLink(AZ::EntityId curEntityId, EKeyId keyId);


} // namespace UiNavigationHelpers