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

//////////////////////////////////////////////////////////////////////////
// dependencies for ICryMannequinDefs
#include <CryName.h>
#include <ISystem.h>
#include <CryCrc32.h>
#include <IValidator.h>
//////////////////////////////////////////////////////////////////////////

#include <ICryMannequin.h>
#include <ICryMannequinDefs.h>
#include <AzCore/Component/ComponentBus.h>

class IAction;

namespace LmbrCentral
{
    using FragmentRequestId = AZ::u32;

    /*!
    * Services provided by the Mannequin Component
    */
    class MannequinRequests
        : public AZ::ComponentBus
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // Action controller interface

        //! Invalid request id
        static const FragmentRequestId s_invalidRequestId = -1;

        /**
        * \brief Queues the indicated Mannequin fragment
        * \param priority Higher number means higher priority
        * \param fragmentName Name of the fragment to be played
        * \param fragTags Fragment tags to be applied
        * \return Request Id that can be used to uniquely identify and make modifications to this request
        */
        virtual FragmentRequestId QueueFragment(int priority, const char* fragmentName, const char* fragTags, bool isPersistent) = 0;

        /**
        * \brief Queues the indicated mannequin fragment
        * \param priority Higher number means higher priority
        * \param fragmentId Id of the fragment to be played
        * \param fragTags Fragment tags to be applied
        * \return Request Id that can be used to uniquely identify and make modifications to this request
        */
        virtual FragmentRequestId QueueFragmentById(int /*priority*/, FragmentID /*fragmentId*/, const char* /*fragTags*/, bool /*isPersistent*/) { return s_invalidRequestId; }

        /**
         * Queue an already created action on the action controller
         * This is a raw queue mechanism. The component will not keep a persistent reference to the action.
         * \param Action the action to enqueue on the character's action controller.
         */
        virtual void QueueAction(_smart_ptr<IAction> /*action*/) {}

        /**
        * \brief Pauses all actions being managed by this Mannequin Component
        */
        virtual void PauseAll() = 0;

        /**
        * \brief Resumes all actions being managed by this Mannequin Component
        * \param A flag of type IActionController::EResumeFlags indicating how the animations are to be resumed
        */
        virtual void ResumeAll(IActionController::EResumeFlags resumeFlag) = 0;

        /**
        * \brief Sets indicated tag for the Action controller
        * \param tagname Name of the tag to be set
        */
        virtual void SetTag(const char* tagName) = 0;

        /**
        * \brief Sets indicated tag for the Action controller
        * \param tagId Id of the tag to be set
        */
        virtual void SetTagById(TagID /*tagId*/) {}

        /**
        * \brief Clears indicated tag for the Action controller
        * \param tagname Name of the tag to be cleared
        */
        virtual void ClearTag(const char* tagName) = 0;

        /**
        * \brief Clears indicated tag for the Action controller
        * \param tagId Id of the tag to be cleared
        */
        virtual void ClearTagById(TagID /*tagId*/) {}

        /**
        * \brief Sets a tag in the indicated group
        * \param groupName Id of the group
        * \param tagName Id of the tag to be cleared
        */
        virtual void SetGroupTag(const char* groupName, const char* tagName) = 0;

        /**
        * \brief Sets a tag in the indicated group
        * \param groupId Id of the group
        * \param tagId Id of the tag to be cleared
        */
        virtual void SetGroupTagById(TagGroupID /*groupId*/, TagID /*tagId*/) {}

        /**
        * \brief Clears tags for the indicated group
        * \param groupName Name of the group
        */
        virtual void ClearGroup(const char* groupName) = 0;

        /**
        * \brief Clears tags for the indicated group
        * \param groupId Id of the group
        */
        virtual void ClearGroupById(TagGroupID /*groupId*/) {}

        /**
        * \brief Sets the scope context for this animation controller
        * \param scopeContextName Name of the scope context that the adb file is to be attached to
        * \param entityId Reference to an entity whose character instance will be bound to this scope context
        * \param animationDatabase path to the animation database file
        */
        virtual void SetScopeContext(const char* scopeContextName, const AZ::EntityId& entityId, const char* animationDatabaseName) = 0;

        /**
        * \brief Sets the scope context for this animation controller
        * \param scopeContextId Id of the scope context that the adb file is to be attached to
        * \param entityId Reference to an entity whose character instance will be bound to this scope context
        * \param animationDatabase path to the animation database file
        */
        virtual void SetScopeContextById(TagID /*scopeContextId*/, const AZ::EntityId& /*entityId*/, const char* /*animationDatabaseName*/) {}

        /**
        * \brief Clears the indicated Scope context
        * \param scopeContextName Name of the scope context that is to be cleared
        */
        virtual void ClearScopeContext(const char* scopeContextName) = 0;

        /**
        * \brief Clears the indicated Scope context
        * \param scopeContextId Id of the scope context that is to be cleared
        */
        virtual void ClearScopeContextById(const TagID /*scopeContextId*/) {}

        /**
        * \brief Allows users to retrieve the Action controller attached to this instance of the mannequin component
        * \return The Action Controller being used by this Mannequin component
        */
        virtual IActionController* GetActionController() { return nullptr; }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Action interface

        /**
        * \brief Allows users to retrieve the Action associated with any given request id
        * \return Action associated with a Mannequin request Id
        */
        virtual IActionPtr GetActionForRequestId(FragmentRequestId /*requestId*/) { return nullptr; }

        /**
        * \brief Stops the Actions associated with an indicated request
        * \param The request id
        */
        virtual void StopRequest(FragmentRequestId requestId)  = 0;

        /**
        * \brief Indicates the status of any request
        * \param The request id
        * \return Status (type IAction::EStatus) of the request
        */
        virtual IAction::EStatus GetRequestStatus(FragmentRequestId requestId) = 0;

        /**
        * \brief Forces the Actions associated with an indicated request to finish
        * \param The request id
        */
        virtual void ForceFinishRequest(FragmentRequestId requestId) = 0;

        /**
        * \brief Sets Speed bias for the Actions associated with an indicated request
        * \param requestId The request id
        * \param speedBias The speed bias for this animaion
        */
        virtual void SetRequestSpeedBias(FragmentRequestId requestId, float speedBias) = 0;

        /**
        * \brief Gets the Speed bias for the Actions associated with an indicated request
        * \param The request id
        * \return Speed bias for the indicated request
        */
        virtual float GetRequestSpeedBias(FragmentRequestId requestId) = 0;

        /**
        * \brief Sets the Anim Weight for the Actions associated with an indicated request
        * \param requestId The request id
        * \param animWeight The weight for this animation
        */
        virtual void SetRequestAnimWeight(FragmentRequestId requestId, float animWeight) = 0;

        /**
        * \brief Gets the Anim Weight for the Actions associated with an indicated request
        * \param The request id
        * \return Anim Weight for the indicated request
        */
        virtual float GetRequestAnimWeight(FragmentRequestId requestId) = 0;

        //////////////////////////////////////////////////////////////////////////
    };

    // Bus to service the Mannequin component event group
    using MannequinRequestsBus = AZ::EBus<MannequinRequests>;

    /*
    * Broadcasts component events.
    */
    class MannequinNotifications
        : public AZ::ComponentBus
    {
    public:

        /**
         * Sent when an action has left the queue and begun playing.
         * \param requestId corresponding to the action
         */
        virtual void OnActionStart(FragmentRequestId /*requestId*/) {};

        /**
         * An action has begun transitioning out (playback is complete).
         * \param requestId corresponding to the action
         */
        virtual void OnActionComplete(FragmentRequestId /*requestId*/) {};

        /**
         * An action is being forcefully exited.
         * \param requestId corresponding to the action
         */
        virtual void OnActionExit(FragmentRequestId /*requestId*/) {};

        /**
        * An action is being deleted.
        * \param requestId corresponding to the action being destroyed.
        */
        virtual void OnActionDelete(FragmentRequestId /*requestId*/) {};
    };

    using MannequinNotificationBus = AZ::EBus<MannequinNotifications>;

    /*
    * Provides information about Mannequin configuration
    */
    class MannequinInformation
        : public AZ::ComponentBus
    {
    public:
        /**
        * \brief Fetches a vector of all available Scope Context names for any given Mannequin controller definition file
        * \param Out reference to a vector that will be filled
        */
        virtual void FetchAvailableScopeContextNames(AZStd::vector<AZStd::string>& scopeContextNames) const = 0;
    };

    // Bus to service the Mannequin information event group
    using MannequinInformationBus = AZ::EBus<MannequinInformation>;

    /*
    * Provides notification relevant to Mannequin file information
    */
    class MannequinAssetNotification
        : public AZ::ComponentBus
    {
    public:
        virtual void OnControllerDefinitionsChanged() = 0;
    };

    // Bus to service the Mannequin information Notification event group
    using MannequinAssetNotificationBus = AZ::EBus<MannequinAssetNotification>;

} // namespace LmbrCentral