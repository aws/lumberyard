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

#include <qrect.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

class QGraphicsLayoutItem;

namespace GraphCanvas
{
    class Scene;

    enum class CommentMode
    {
        Unknown,
        Comment,
        BlockComment
    };

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

        //! Sets the type of comment that is being used(controls how the comment resizes, how excess text is handled).
        virtual void SetCommentMode(CommentMode commentMode) = 0;
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

        //! Signals when the comment begins being edited.
        virtual void OnEditBegin() {}

        //! Signals when the comment ends being edited
        virtual void OnEditEnd() {}

        //! When the comment is changed, this is emitted.
        virtual void OnCommentChanged(const AZStd::string&) {}

        //! Emitted when the size of a comment changes(in reaction to text updating)
        virtual void OnCommentSizeChanged(const QSizeF& /*oldSize*/, const QSizeF& /*newSize*/) {};

        virtual void OnCommentFontReloadBegin() {};
        virtual void OnCommentFontReloadEnd() {};
    };

    using CommentNotificationBus = AZ::EBus<CommentNotifications>;

    class CommentUIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void SetEditable(bool editable) = 0;
    };

    using CommentUIRequestBus = AZ::EBus<CommentUIRequests>;

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