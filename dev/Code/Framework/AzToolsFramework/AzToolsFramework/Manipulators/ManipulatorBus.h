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
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework
{
    class ManipulatorManager;
    class BaseManipulator;

    using ManipulatorId = AZ::u32;
    static const ManipulatorId InvalidManipulatorId = 0;

    using ManipulatorManagerId = AZ::u32;
    static const ManipulatorManagerId InvalidManipulatorManagerId = 0;

    enum class TransformManipulatorMode
    {
        None,
        Translation,
        Scale,
        Rotation
    };

    enum class ManipulatorReferenceSpace
    {
        Local,
        World
    };

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
         * Register a manipulator to the Manipulator Manager.
         */
        virtual void RegisterManipulator(BaseManipulator& manipulator) = 0;

        /**
         * Unregister a manipulator from the Manipulator Manager.
         * After unregistering, the manipulator will be excluded from mouse hit detection and will not
         * receive any mouse action event.
         */
        virtual void UnregisterManipulator(BaseManipulator& manipulator) = 0;

        virtual void SetActiveManipulator(BaseManipulator* ptrManip) = 0;

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
        virtual void SetAllManipulatorBoundsDirty() = 0;

        /**
         * Update the bound for a manipulator.
         * If \ref boundId hasn't been registered before or it's invalid, a new bound is created and set using \ref boundShapeData
         * @param manipulatorId The id of the manipulator whose bound needs to update.
         * @param boundId The id of the bound that needs to update. 
         * @param boundShapeData The pointer to the new bound shape data.
         * @return If \ref boundId has been registered return the same id, otherwise create a new bound and return its id.
         */
        virtual Picking::RegisteredBoundId UpdateManipulatorBound(ManipulatorId manipulatorId, Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData) = 0;

        /**
         * Set the mode of TransformManipulator to either Translation, Scale or Rotation. This will effect all TransformManipulators currently displayed in the viewport.
         * @param mode The mode of TransformManipulator to set to.
         */
        virtual void SetTransformManipulatorMode(TransformManipulatorMode mode) = 0;

        /**
         * Get the TransformManipulator mode.
         * @return The current mode of TransformManipulator.
         */
        virtual TransformManipulatorMode GetTransformManipulatorMode() = 0;

        /**
         * Set the reference space in which Manipulators' directions will be defined.
         */
        virtual void SetManipulatorReferenceSpace(ManipulatorReferenceSpace referenceSpace) = 0;

        /**
         * Get the TransformManipulator mode.
         */
        virtual ManipulatorReferenceSpace GetManipulatorReferenceSpace() = 0;
    };
    using ManipulatorManagerRequestBus = AZ::EBus<ManipulatorManagerRequests>;

    /**
     * ...
     */
    class ManipulatorManagerNotifications
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ManipulatorManagerId;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        /**
         * Set the mode of TransformManipulator to either Translation, Scale or Rotation. This will effect all TransformManipulators currently displayed in the viewport.
         * @param mode The mode of TransformManipulator to set to.
         */
        virtual void OnTransformManipulatorModeChanged(TransformManipulatorMode previousMode, TransformManipulatorMode currentMode) = 0;
    };
    using ManipulatorManagerNotificationBus = AZ::EBus<ManipulatorManagerNotifications>;
}
