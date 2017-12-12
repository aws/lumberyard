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
#include <AzCore/EBus/EBus.h>

#include <QWidget>

namespace AzQtComponents
{

    /**
     * The ShortcutDispatcBus is intended to allow systems to hook in and override providing a valid keyboard shortcut
     * scopeRoot. GraphCanvas uses sub-systems of Qt which don't play well with the regular
     * Qt widget hierarchy and as a result, some widgets which should have parents don't.
     *
     */
    class ShortcutDispatchTraits
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<ShortcutDispatchTraits>;

        ///////////////////////////////////////////////////////////////////////
        using BusIdType = QWidget*;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        ///////////////////////////////////////////////////////////////////////

        /**
        * Sent when the Editor's shortcut dispatcher can't find a parent for the given focus widget.
        * Specifically used for GraphCanvas widgets, as they don't have parents and aren't part
        * of the regular Qt hierarchy.
        */
        virtual QWidget* GetShortcutDispatchScopeRoot(QWidget* /*focus*/) { return nullptr; }
    };
    using ShortcutDispatchBus = AZ::EBus<ShortcutDispatchTraits>;

} // namespace AzToolsFramework