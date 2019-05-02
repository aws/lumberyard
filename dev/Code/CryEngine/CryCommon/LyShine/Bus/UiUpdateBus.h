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

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// This entire class is deprecated so that any existing customer components using the UiUpdateBus will get
// compile errors. This file is left here for the time being to guide users to these comments on what to use in place of
// UiUpdateBus.
// Rather than using UiUpdateBus and connecting with the entityId of the entity that wants updating you should now use
// UiCanvasUpdateNotificationBus and connect with the canvasEntityId. This makes the LyShine update more efficient.

AZ_DEPRECATED(class UiUpdateInterface, "UiUpdateInterface deprecated use UiCanvasUpdateNotification");
AZ_DEPRECATED(class UiUpdateBus, "UiUpdateBus deprecated, use UiCanvasUpdateNotificationBus instead");
