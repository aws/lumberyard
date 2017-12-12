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
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include "ManipulatorBus.h"
#include "BaseManipulator.h"

namespace AzToolsFramework
{
    /**
     * TranslationManipulator is an aggregation of 3 linear manipulators and 3 planar manipulators who share the same origin.
     */
    class TranslationManipulator
    {
    public:

        /**
         * How many dimensions does this translation manipulator have
         */
        enum class Dimensions
        {
            Two,
            Three
        };

        AZ_RTTI(TranslationManipulator, "{D5E49EA2-30E0-42BC-A51D-6A7F87818260}");
        AZ_CLASS_ALLOCATOR(TranslationManipulator, AZ::SystemAllocator, 0);

        explicit TranslationManipulator(AZ::EntityId entityId, Dimensions dimensions);
        virtual ~TranslationManipulator();

        TranslationManipulator(const TranslationManipulator&) = delete;
        TranslationManipulator& operator=(const TranslationManipulator&) = delete;

        void Register(ManipulatorManagerId manipulatorManagerId);
        void Unregister();

        using LinearManipulatorMouseActionCallback = AZStd::function<void(const LinearManipulationData&)>;
        using PlanarManipulatorMouseActionCallback = AZStd::function<void(const PlanarManipulationData&)>;

        void InstallLinearManipulatorMouseDownCallback(LinearManipulatorMouseActionCallback onMouseDownCallback);
        void InstallLinearManipulatorMouseMoveCallback(LinearManipulatorMouseActionCallback onMouseMoveCallback);
        void InstallLinearManipulatorMouseUpCallback(LinearManipulatorMouseActionCallback onMouseUpCallback);

        void InstallPlanarManipulatorMouseDownCallback(PlanarManipulatorMouseActionCallback onMouseDownCallback);
        void InstallPlanarManipulatorMouseMoveCallback(PlanarManipulatorMouseActionCallback onMouseMoveCallback);
        void InstallPlanarManipulatorMouseUpCallback(PlanarManipulatorMouseActionCallback onMouseUpCallback);

        void SetBoundsDirty();

        void SetPosition(const AZ::Vector3& origin);
        const AZ::Vector3& GetPosition() const { return m_origin; }

        void SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3 = AZ::Vector3::CreateAxisZ());
        void SetAxesColor(const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));
        void SetPlanesColor(const AZ::Color& plane1Color, const AZ::Color& plane2Color = AZ::Color(0.0f, 1.0f, 0.0f, 0.5f), const AZ::Color& plane3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        /**
         * Set all three axes to the same length.
         */
        void SetAxisLength(float axisLength);

        /**
         * Set all three planes to the same size and they will displayed as squares.
         */
        void SetPlaneSize(float size);

    private:

        struct ManipulatorIndices
        {
            explicit ManipulatorIndices(Dimensions dimensions)
            {
                switch (dimensions)
                {
                case Dimensions::Two:
                    m_linearBeginIndex = 0;
                    m_linearEndIndex = 2;
                    m_planarBeginIndex = 2;
                    m_planarEndIndex = 3;
                    break;
                case Dimensions::Three:
                    m_linearBeginIndex = 0;
                    m_linearEndIndex = 3;
                    m_planarBeginIndex = 3;
                    m_planarEndIndex = 6;
                    break;
                default:
                    AZ_Assert(false, "Invalid dimensions provided");
                    break;
                }
            }

            int m_linearBeginIndex = 0;
            int m_linearEndIndex = 0;
            int m_planarBeginIndex = 0;
            int m_planarEndIndex = 0;
        };

        AZ::Vector3 m_origin; ///< Local space position of TranslationManipulator.
        const ManipulatorIndices m_manipulatorIndices; ///< Indices to control which manipulators to affect depending on if the TranslationManipulator is in 2 or 3 dimensions.
        const Dimensions m_dimensions; ///< How many dimensions of freedom does this manipulator have.

        /**
         * If dimensions is 2 - manipulators count is 3 - the first 2 are LinearManipulators and the last is a PlanarManipulator.
         * If dimensions is 3 - manipulators count is 6 - the first 3 are LinearManipulators and the last 3 are PlanarManipulators.
         */
        AZStd::vector<AZStd::unique_ptr<BaseManipulator>> m_manipulators; ///< Underlying manipulators to compose TranslationManipulator
    };
} // namespace AzToolsFramework