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

#include "WhiteBox_precompiled.h"

#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "WhiteBoxVertexSelectionModifier.h"

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/SelectionManipulator.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(VertexSelectionModifier, AZ::SystemAllocator, 0)

    VertexSelectionModifier::VertexSelectionModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair, Api::VertexHandle vertexHandle,
        [[maybe_unused]] const AZ::Vector3& intersectionPoint)
        : m_entityComponentIdPair{entityComponentIdPair}
        , m_vertexHandle{vertexHandle}
    {
        CreateManipulator();
    }

    VertexSelectionModifier::~VertexSelectionModifier()
    {
        DestroyManipulator();
    }

    void VertexSelectionModifier::CreateManipulator()
    {
        using AzToolsFramework::ViewportInteraction::MouseInteraction;

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        m_selectionManipulator = AzToolsFramework::SelectionManipulator::MakeShared(
            AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId()));

        m_selectionManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_selectionManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_selectionManipulator->SetPosition(Api::VertexPosition(*whiteBox, m_vertexHandle));

        CreateView();

        // setup callback for selection manipulator
        m_selectionManipulator->InstallLeftMouseDownCallback(
            [this](const MouseInteraction& mouseInteraction)
            {
                EditorWhiteBoxDefaultModeRequestBus::Event(
                    m_entityComponentIdPair,
                    &EditorWhiteBoxDefaultModeRequestBus::Events::AssignSelectedVertexSelectionModifier);
            });
    }

    void VertexSelectionModifier::CreateView()
    {
        using AzToolsFramework::ViewportInteraction::MouseInteraction;

        if (!m_vertexView)
        {
            m_vertexView = AzToolsFramework::CreateManipulatorViewSphere(
                m_color, cl_whiteBoxVertexManipulatorSize,
                []([[maybe_unused]] const MouseInteraction& mouseInteraction, const bool mouseOver,
                   const AZ::Color& defaultColor)
                {
                    return defaultColor;
                });
        }

        m_vertexView->m_color = m_color;

        m_selectionManipulator->SetViews(AzToolsFramework::ManipulatorViews{m_vertexView});
    }

    void VertexSelectionModifier::DestroyManipulator()
    {
        m_selectionManipulator->Unregister();
        m_selectionManipulator.reset();
    }

    bool VertexSelectionModifier::MouseOver() const
    {
        return m_selectionManipulator->MouseOver();
    }

    void VertexSelectionModifier::ForwardMouseOverEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction)
    {
        m_selectionManipulator->ForwardMouseOverEvent(interaction);
    }

    void VertexSelectionModifier::Refresh()
    {
        DestroyManipulator();
        CreateManipulator();
    }

    void VertexSelectionModifier::UpdateIntersectionPoint([[maybe_unused]] const AZ::Vector3& intersectionPoint)
    {
        CreateView();
    }

    bool VertexSelectionModifier::PerformingAction() const
    {
        return m_selectionManipulator->PerformingAction();
    }
} // namespace WhiteBox
