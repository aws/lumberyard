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
#include <AzCore/Component/EntityId.h>

#include <GraphCanvas/Types/Endpoint.h>

class QGraphicsLayoutItem;
class QMimeData;

namespace GraphCanvas
{
    //! SceneVariableRequestBus
	//! Requests to be made about variables in a particular scene.
    class SceneVariableRequests : public AZ::EBusTraits
    {
    public:
        // The BusId here is the SceneId of the scene that contains the variable.
        //
        // Should mainly be used with enumeration for collecting information and not as a way of directly interacting
        // with variables inside of a particular scene.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::EntityId GetVariableId() const = 0;
    };

    using SceneVariableRequestBus = AZ::EBus<SceneVariableRequests>;

	
	//! SceneVariableNotificationBus
	//! Notifications sent about Variable activity within a scene.
    class SceneVariableNotifications : public AZ::EBusTraits
    {
    public:
        // The BusId here is the SceneId of the scene that the variables belong to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnVariableCreated(const AZ::EntityId& variableId) = 0;
        virtual void OnVariableDestroyed(const AZ::EntityId& variableId) = 0;
    };

    using SceneVariableNotificationBus = AZ::EBus<SceneVariableNotifications>;

    //! VariableRequestBus
	//! Requests to be made about a particular variable.
    class VariableRequests : public AZ::EBusTraits
    {
    public:
        // The BusId here is the VariableId of the variable that information is required about.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZStd::string GetVariableName() const = 0;
        virtual void SetVariableName(const AZStd::string& variableName) = 0;
        virtual AZ::Uuid GetVariableDataType() const = 0;
    };

    using VariableRequestBus = AZ::EBus<VariableRequests>;

    //! VariableNotifications
    //! Notifications that are produced about a variable
    class VariableNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnNameChanged() { };
        virtual void OnVariableActivated() { };
        virtual void OnVariableDestroyed() { };
    };

    using VariableNotificationBus = AZ::EBus<VariableNotifications>;
	
	//! VariableActionRequestBus
	//! Actions that GraphCanvas Variables will request of the owning scene relating to the underlying data model.
    class VariableActionRequests : public AZ::EBusTraits
    {
    public:
        // BusId is the scene that the varaible belongs to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void AssignVariableValue(const AZ::EntityId& variableId, const Endpoint& endpoint) = 0;
        virtual void UnassignVariableValue(const AZ::EntityId& variableId, const Endpoint& endpoint) = 0;
    };

    using VariableActionRequestBus = AZ::EBus<VariableActionRequests>;

    //! VariableReferenceSceneNotificationBus
    //! This is a bus that the scene will fire notifications off to to deal with remapping variables
    //! mainly after objects are deserialized(say for a copy and paste)
    class VariableReferenceSceneNotifications : public AZ::EBusTraits
    {
    public:
        // BusId is the scene that the variable belongs to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void ResolvePastedReferences() = 0;
    };

    using VariableReferenceSceneNotificationBus = AZ::EBus<VariableReferenceSceneNotifications>;    
}