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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Picking/BoundInterface.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBoundManager.h>

namespace AzFramework
{
    class EntityDebugDisplayRequests;
}

namespace AzToolsFramework
{

    namespace Picking
    {
        class DefaultContextBoundManager;
    }

    namespace ViewportInteraction
    {
        struct CameraState;
        struct MouseInteraction;
    }

    class BaseManipulator;
    class LinearManipulator;

    /**
     * This class serves to manage all relevant mouse events and coordinate all registered manipulators to function properly.
     * ManipulatorManager does not manage the life cycle of specific manipulators. The users of manipulators are responsible 
     * for creating and deleting them at right time, as well as registering and unregistering accordingly. 
     */
    class ManipulatorManager final
        : private ManipulatorManagerRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ManipulatorManager, AZ::SystemAllocator, 0);

        ManipulatorManager(ManipulatorManagerId managerId);
        ~ManipulatorManager();

        // NOTE: These are NOT EBus messages. They are called by the owner of the manipulator manager and they will return TRUE
        // if they have gobbled up the interaction. If this is the case you should not process it yourself.
        bool ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseMove(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction&);
        bool ConsumeViewportMouseWheel(const ViewportInteraction::MouseInteraction&);

        //////////////////////////////////////////////////////////////////////////
        /// ManipulatorManagerRequestBus::Handler
        void RegisterManipulator(BaseManipulator& manipulator) override;
        void UnregisterManipulator(BaseManipulator& manipulator) override;
        void SetActiveManipulator(BaseManipulator* ptrManip) override;
        void DeleteManipulatorBound(Picking::RegisteredBoundId boundId) override;
        void SetBoundDirty(Picking::RegisteredBoundId boundId) override;
        void SetAllManipulatorBoundsDirty() override;
        Picking::RegisteredBoundId UpdateManipulatorBound(ManipulatorId manipulatorId, Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData) override;
        void SetTransformManipulatorMode(TransformManipulatorMode mode) override;
        TransformManipulatorMode GetTransformManipulatorMode() override;
        void SetManipulatorReferenceSpace(ManipulatorReferenceSpace referenceSpace) override;
        ManipulatorReferenceSpace GetManipulatorReferenceSpace() override;
        //////////////////////////////////////////////////////////////////////////

        void DrawManipulators(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState);

    private:
        
        /**
         * @param rayOrigin    The origin of the ray to test intersection with.
         * @param rayDirection The direction of the ray to test intersection with.
         * @param[out] t       The result intersecting point equals "rayOrigin + t * rayDirection".
         * @return             A pointer to a manipulator that the ray intersects. Null pointer if no intersection is detected. 
         */
        BaseManipulator* PerformRaycast(const AZ::Vector3& rayOrigin, const AZ::Vector3 rayDirection, float& t);

    private:

        ManipulatorManagerId m_manipulatorManagerId;

        ManipulatorId m_nextManipulatorIDToGenerate;
        AZStd::unordered_map<ManipulatorId, BaseManipulator*> m_manipulatorIdToPtrMap;
        AZStd::unordered_map<Picking::RegisteredBoundId, ManipulatorId> m_boundIdToManipulatorIdMap;

        BaseManipulator* m_activeManipulator;

        Picking::ManipulatorBoundManager m_boundManager;

        TransformManipulatorMode m_transformManipulatorMode = TransformManipulatorMode::None;
        ManipulatorReferenceSpace m_manipulatorReferenceSpace = ManipulatorReferenceSpace::Local;
    };
} // namespace AzToolsFramework
