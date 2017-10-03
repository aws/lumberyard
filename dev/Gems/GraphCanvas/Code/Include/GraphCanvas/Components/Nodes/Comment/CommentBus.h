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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>

class QGraphicsLayoutItem;

namespace GraphCanvas
{
    class Scene;

    //! CommentRequests
    //! Requests that get or set the properties of a comment.
    class CommentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the name of the comment. This often acts as a kind of visual title for the comment.
        virtual void SetComment(const AZStd::string&) = 0;
        //! Get the name of the comment.
        virtual const AZStd::string& GetComment() const = 0;
    };

    using CommentRequestBus = AZ::EBus<CommentRequests>;

    //! CommentNotifications
    //! Notifications about changes to the state of comments
    class CommentNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! When the comment is changed, this is emitted.
        virtual void OnCommentChanged(const AZStd::string&) {}
        //! Emitted when the position of a comment changes
        virtual void OnPositionChanged(const AZ::EntityId&, const AZ::Uuid&, const AZ::Vector2&) {};
    };

    using CommentNotificationBus = AZ::EBus<CommentNotifications>;

    class CommentLayoutRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual QGraphicsLayoutItem* GetGraphicsLayoutItem() = 0;
    };

    using CommentLayoutRequestBus = AZ::EBus<CommentLayoutRequests>;
}