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

#include <IInput.h>
#include <CrySystemBus.h>
#include <vector>
#include <AzCore/Component/Component.h>
#include <Gestures/GesturesBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    class IRecognizer;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class Manager
        : public AZ::Component
        , public GesturesBus::Handler
        , public CrySystemEventBus::Handler
        , public IInputEventListener
    {
    public:
#ifdef SIMULATE_MULTI_TOUCH
        class MultiTouchSimulator
        {
        public:
            MultiTouchSimulator(Manager& mgr);
            virtual ~MultiTouchSimulator() {}

            //! Update state specific to touch simulation
            void Update(const SInputEvent& inputEvent);

            //! Simulate events
            bool SimulateEvents(const SInputEvent& inputEvent, uint32_t pointerIndex);

            //! TRUE, when index 1 pointer is actively being simulated
            bool IsSimulatingTouch() const { return m_simulating; }

            //! Retrieve the current location of the reflection point
            const Vec2& GetReflectPoint() const { return m_reflectPoint; }

        private:
            //! Calculate the location of a point reflected about the reflection point
            Vec2 ReflectPoint(const Vec2& pt) const { return 2.0f * m_reflectPoint - pt; }

            //! Draw persistent debug info to the screen for the actual and simulated pointer positions
            void DebugDraw();

            Manager& m_mgr;  //<! parent gesture manager
            bool m_simulating;  //<! when TRUE, index 1 will be simulated by reflecting index 0 about the reflection point
            bool m_shifting;  //<! when TRUE, the reflection point will move with mouse movement
            bool m_index0IsDown;  //<! cached state of pointer index 0
            Vec2 m_reflectPoint;  //<! center point about which pointer index 0 will be reflected
            Vec2 m_mousePosition;  //<! cached mouse pointer position
            Vec2 m_shiftOffset;  //<! during a shift, this is the offset for the position of the reflection point relative to pointer index 0
        };
#endif

        AZ_COMPONENT(Manager, "{B3DE4F09-B3E2-4176-AACA-0AD64B8B13F0}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        Manager();
        ~Manager() override = default;

        ////////////////////////////////////////////////////////////////////////
        // Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // GesturesBus::Handler
        void Register(IRecognizer& recognizer) override;
        void Deregister(IRecognizer& recognizer) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus::Handler
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // IInputEventListener
        bool OnInputEvent(const SInputEvent& inputEvent) override;
        ////////////////////////////////////////////////////////////////////////

        int GetRecognizerCount() const { return m_recognizers.size(); }

#ifdef SIMULATE_MULTI_TOUCH
        const MultiTouchSimulator& GetMultiTouchSimulator() const { return m_simulator; }
#endif

    private:
        bool OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex);
        bool OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex);
        bool OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex);

        std::vector<IRecognizer*> m_recognizers;

#ifdef SIMULATE_MULTI_TOUCH
        MultiTouchSimulator m_simulator;
#endif
    };
}
