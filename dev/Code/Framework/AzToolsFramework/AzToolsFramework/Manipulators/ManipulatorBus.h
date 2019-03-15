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
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework
{
    class ManipulatorManager;
    class BaseManipulator;

    /**
     * Provide unique type alias for AZ::u32 for manipulator and manager.
     */
    template<typename T>
    class ManipulatorIdType
    {
    public:
        explicit ManipulatorIdType(AZ::u32 id = 0)
            : m_id(id) {}
        operator AZ::u32() const { return m_id; }

        bool operator==(ManipulatorIdType other) const { return m_id == other.m_id; }
        bool operator!=(ManipulatorIdType other) const { return m_id != other.m_id; }
        ManipulatorIdType& operator++() // pre-increment
        {
            ++m_id;
            return *this;
        }
        ManipulatorIdType operator++(int) // post-increment
        {
            ManipulatorIdType temp = *this;
            ++*this;
            return temp;
        }
    private:
        AZ::u32 m_id;
    };

    using ManipulatorId = ManipulatorIdType<struct ManipulatorType>;
    static const ManipulatorId InvalidManipulatorId = ManipulatorId(0);

    using ManipulatorManagerId = ManipulatorIdType<struct ManipulatorManagerType>;
    static const ManipulatorManagerId InvalidManipulatorManagerId = ManipulatorManagerId(0);

    enum class ManipulatorSpace
    {
        Local,
        World
    };

    /**
     * Requests made to adjust the current manipulators.
     */
    class ManipulatorRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~ManipulatorRequests() = default;

        /**
         * Clear/remove the active manipulator.
         */
        virtual void ClearSelected() = 0;

        /**
         * Set the position of the currently selected manipulator (must be transformed to local space of entity)
         */
        virtual void SetSelectedPosition(const AZ::Vector3&) = 0;
    };

    using ManipulatorRequestBus = AZ::EBus<ManipulatorRequests>;

    /**
     * Provide interface for destructive manipulator actions - for example if a point/vertex
     * can be removed from a container.
     */
    class DestructiveManipulatorRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~DestructiveManipulatorRequests() = default;

        /**
         * Clear/remove the active manipulator and destroy the thing associated with it (E.g. Remove vertex).
         */
        virtual void DestroySelected() = 0;
    };

    using DestructiveManipulatorRequestBus = AZ::EBus<DestructiveManipulatorRequests>;

    /**
     * Provide interface for Transform manipulator requests.
     */
    class TransformComponentManipulatorRequests
        : public AZ::ComponentBus
    {
    public:
        using BusIdType = ManipulatorManagerId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~TransformComponentManipulatorRequests() = default;
        
        enum class Mode
        {
            Translation,
            Scale,
            Rotation
        };

        /**
         * Set what kind of transform the type that implements this bus should use.
         */
        virtual void SetTransformMode(ManipulatorManagerId managerId, Mode mode) = 0;
    };

    using TransformComponentManipulatorRequestBus = AZ::EBus<TransformComponentManipulatorRequests>;

    /**
     * EBus interface used to send requests to ManipulatorManager.
     */
    class ManipulatorManagerRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; /**< We can have multiple manipulator managers.
                                                                                             In the case where there are multiple viewports, each displaying
                                                                                             a different set of entities, a different manipulator manager is required
                                                                                             to provide a different collision space for each viewport so that mouse
                                                                                             hit detection can be handled properly. */
        using BusIdType = ManipulatorManagerId;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~ManipulatorManagerRequests() {}

        /**
         * Register a manipulator with the Manipulator Manager.
         * @param manipulator The manipulator parameter is passed as a shared_ptr so
         * that the system responsible for managing manipulators can maintain ownership
         * of the manipulator even if is destroyed while in use.
         */
        virtual void RegisterManipulator(AZStd::shared_ptr<BaseManipulator> manipulator) = 0;

        /**
         * Unregister a manipulator from the Manipulator Manager.
         * After unregistering the manipulator, it will be excluded from mouse hit detection
         * and will not receive any mouse action events. The Manipulator Manager will also
         * relinquish ownership of the manipulator.
         */
        virtual void UnregisterManipulator(BaseManipulator* manipulator) = 0;

        /**
         * Delete a manipulator bound.
         */
        virtual void DeleteManipulatorBound(Picking::RegisteredBoundId boundId) = 0;

        /**
         * Mark the bound of a manipulator dirty so it's excluded from mouse hit detection.
         * This should be called whenever a manipulator is moved.
         */
        virtual void SetBoundDirty(Picking::RegisteredBoundId boundId) = 0;

        /**
         * Mark bounds of all manipulators dirty.
         */
        virtual void SetAllBoundsDirty() = 0;

        /**
         * Update the bound for a manipulator.
         * If \ref boundId hasn't been registered before or it's invalid, a new bound is created and set using \ref boundShapeData
         * @param manipulatorId The id of the manipulator whose bound needs to update.
         * @param boundId The id of the bound that needs to update.
         * @param boundShapeData The pointer to the new bound shape data.
         * @return If \ref boundId has been registered return the same id, otherwise create a new bound and return its id.
         */
        virtual Picking::RegisteredBoundId UpdateBound(
            ManipulatorId manipulatorId, Picking::RegisteredBoundId boundId,
            const Picking::BoundRequestShapeBase& boundShapeData) = 0;

        /**
         * Set the space in which Manipulators' directions will be defined.
         */
        virtual void SetManipulatorSpace(ManipulatorSpace referenceSpace) = 0;

        /**
         * Get the manipulator transform space.
         */
        virtual ManipulatorSpace GetManipulatorSpace() = 0;
    };

    using ManipulatorManagerRequestBus = AZ::EBus<ManipulatorManagerRequests>;

    /**
     * Utility for accessing manipulator space.
     */
    inline ManipulatorSpace GetManipulatorSpace(ManipulatorManagerId managerId)
    {
        ManipulatorSpace manipulatorSpace;
        ManipulatorManagerRequestBus::EventResult(
            manipulatorSpace, managerId,
            &ManipulatorManagerRequestBus::Events::GetManipulatorSpace);
        return manipulatorSpace;
    }

    /**
     * Notification of events occuring in the manipulator manager.
     */
    class ManipulatorManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~ManipulatorManagerNotifications() = default;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ManipulatorManagerId;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    };

    using ManipulatorManagerNotificationBus = AZ::EBus<ManipulatorManagerNotifications>;
}