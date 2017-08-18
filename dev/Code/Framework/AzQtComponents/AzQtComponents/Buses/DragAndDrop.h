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

#include <QDropEvent>

namespace AzQtComponents
{

    /**
     * The DragAndDropEvents bus is intended to act as a proxy for objects handling QT Drag and Drop Events on Widgets.
     * Multiple handlers can register with a single Widget name to handle drag and drop events from that name. 
     * It is the handler authors' responsibility to not conflict with others' drag and drop handlers.
     *
     * See http://doc.qt.io/qt-5.8/dnd.html for more complete documentation on QT Drag and Drop.
     */
    class DragAndDropEvents
        : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////
        using BusIdType = AZ::Crc32;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        ///////////////////////////////////////////////////////////////////////

        /**
        * Sent when a drag and drop action enters a widget.
        */
        virtual void DragEnter(QDragEnterEvent* /*event*/) {}
        /**
        * Sent when a drag and drop action is in progress.
        */
        virtual void DragMove(QDragMoveEvent* /*event*/) {}
        /**
        * Sent when a drag and drop action leaves a widget.
        */
        virtual void DragLeave(QDragLeaveEvent* /*event*/) {}
        /**
        * Sent when a drag and drop action completes.
        */
        virtual void Drop(QDropEvent* /*event*/) {}
    };
    using DragAndDropEventsBus = AZ::EBus<DragAndDropEvents>;

} // namespace AzToolsFramework