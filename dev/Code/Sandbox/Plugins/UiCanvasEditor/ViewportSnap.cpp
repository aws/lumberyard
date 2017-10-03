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
#include "stdafx.h"

#include "EditorCommon.h"

namespace
{
    UiTransform2dInterface::Offsets ResizeAboutPivot(const UiTransform2dInterface::Offsets& offsets,
        const ViewportHelpers::GizmoParts& grabbedGizmoParts,
        const AZ::Vector2& pivot,
        const AZ::Vector2& translation)
    {
        UiTransform2dInterface::Offsets result(offsets);

        if (grabbedGizmoParts.m_right)
        {
            result.m_left -= translation.GetX() * pivot.GetX();
            result.m_right += translation.GetX() * (1.0f - pivot.GetX());
        }
        if (grabbedGizmoParts.m_top)
        {
            result.m_top += translation.GetY() * pivot.GetY();
            result.m_bottom -= translation.GetY() * (1.0f - pivot.GetY());
        }

        return result;
    }
} // anonymous namespace.

AZ::Vector2 ViewportSnap::Move(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    ViewportInteraction::CoordinateSystem coordinateSystem,
    const ViewportHelpers::GizmoParts& grabbedGizmoParts,
    AZ::Entity* element,
    const AZ::Vector2& translation)
{
    AZ::Vector2 deltaInCanvasSpace;

    AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(item->GetNonSnappedOffsets() + translation);

        // Update the currently used offset.
        {
            if (coordinateSystem == ViewportInteraction::CoordinateSystem::LOCAL)
            {
                UiTransform2dInterface::Offsets currentOffsets;
                EBUS_EVENT_ID_RESULT(currentOffsets, element->GetId(), UiTransform2dBus, GetOffsets);

                // Get the width and height in canvas space no scale rotate.
                AZ::Vector2 elementSize;
                EBUS_EVENT_ID_RESULT(elementSize, element->GetId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

                AZ::Vector2 pivot;
                EBUS_EVENT_ID_RESULT(pivot, element->GetId(), UiTransformBus, GetPivot);

                AZ::Vector2 pivotRelativeToTopLeftAnchor(currentOffsets.m_left + (elementSize.GetX() * pivot.GetX()),
                    currentOffsets.m_top + (elementSize.GetY() * pivot.GetY()));

                AZ::Vector2 snappedPivot;
                {
                    AZ::Vector2 nonSnappedPivot;
                    {
                        UiTransform2dInterface::Offsets nonSnappedOffsets(item->GetNonSnappedOffsets());

                        nonSnappedPivot = AZ::Vector2(nonSnappedOffsets.m_left + (elementSize.GetX() * pivot.GetX()),
                                nonSnappedOffsets.m_top + (elementSize.GetY() * pivot.GetY()));
                    }

                    float snapDistance = 1.0f;
                    EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiCanvasBus, GetSnapDistance);

                    snappedPivot = EntityHelpers::Snap(nonSnappedPivot, snapDistance);
                }

                if (grabbedGizmoParts.Single())
                {
                    if (!grabbedGizmoParts.m_right)
                    {
                        // Zero-out the horizontal delta.
                        snappedPivot.SetX(pivotRelativeToTopLeftAnchor.GetX());
                    }

                    if (!grabbedGizmoParts.m_top)
                    {
                        // Zero-out the vertical delta.
                        snappedPivot.SetY(pivotRelativeToTopLeftAnchor.GetY());
                    }
                }

                if (pivotRelativeToTopLeftAnchor == snappedPivot)
                {
                    deltaInCanvasSpace = AZ::Vector2(0.0f, 0.0f);
                }
                else
                {
                    AZ::Vector2 deltaInLocalSpace(snappedPivot - pivotRelativeToTopLeftAnchor);
                    EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, (currentOffsets + deltaInLocalSpace));
                    EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);

                    // Return value.
                    {
                        AZ::Matrix4x4 transformToCanvasSpace;
                        EBUS_EVENT_ID(parentElement->GetId(), UiTransformBus, GetTransformToCanvasSpace, transformToCanvasSpace);
                        AZ::Vector3 deltaInCanvasSpace3 = transformToCanvasSpace.Multiply3x3(EntityHelpers::MakeVec3(deltaInLocalSpace));
                        deltaInCanvasSpace = AZ::Vector2(deltaInCanvasSpace3.GetX(), deltaInCanvasSpace3.GetY());
                    }
                }
            }
            else if (coordinateSystem == ViewportInteraction::CoordinateSystem::VIEW)
            {
                UiTransform2dInterface::Offsets currentOffsetsInLocalSpace;
                EBUS_EVENT_ID_RESULT(currentOffsetsInLocalSpace, element->GetId(), UiTransform2dBus, GetOffsets);

                AZ::Vector2 currentPivotInCanvasSpace;
                EBUS_EVENT_ID_RESULT(currentPivotInCanvasSpace, element->GetId(), UiTransformBus, GetCanvasSpacePivot);

                AZ::Vector2 snappedPivotInCanvasSpace;
                {
                    UiTransform2dInterface::Offsets nonSnappedOffsetsInLocalSpace(item->GetNonSnappedOffsets());
                    AZ::Vector2 nonSnappedPivotInCNSR(EntityHelpers::ComputeCanvasSpacePivotNoScaleRotate(element->GetId(), nonSnappedOffsetsInLocalSpace));

                    AZ::Matrix4x4 transformToCanvasSpace;
                    EBUS_EVENT_ID(parentElement->GetId(), UiTransformBus, GetTransformToCanvasSpace, transformToCanvasSpace);
                    AZ::Vector3 nonSnappedPivotInCanvasSpace3 = transformToCanvasSpace * EntityHelpers::MakeVec3(nonSnappedPivotInCNSR);
                    AZ::Vector2 nonSnappedPivotInCanvasSpace(nonSnappedPivotInCanvasSpace3.GetX(), nonSnappedPivotInCanvasSpace3.GetY());

                    // Snap.
                    {
                        float snapDistance = 1.0f;
                        EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiCanvasBus, GetSnapDistance);

                        snappedPivotInCanvasSpace = EntityHelpers::Snap(nonSnappedPivotInCanvasSpace, snapDistance);
                    }
                }

                if (grabbedGizmoParts.Single())
                {
                    if (!grabbedGizmoParts.m_right)
                    {
                        // Zero-out the horizontal delta.
                        snappedPivotInCanvasSpace.SetX(currentPivotInCanvasSpace.GetX());
                    }

                    if (!grabbedGizmoParts.m_top)
                    {
                        // Zero-out the vertical delta.
                        snappedPivotInCanvasSpace.SetY(currentPivotInCanvasSpace.GetY());
                    }
                }

                if (currentPivotInCanvasSpace == snappedPivotInCanvasSpace)
                {
                    deltaInCanvasSpace = AZ::Vector2(0.0f, 0.0f);
                }
                else
                {
                    AZ::Vector2 deltaInLocalSpace;
                    {
                        // deltaInLocalSpace: The delta between the snapped and non-snapped point.
                        AZ::Matrix4x4 transform;
                        EBUS_EVENT_ID(parentElement->GetId(), UiTransformBus, GetTransformFromCanvasSpace, transform);

                        deltaInCanvasSpace = (snappedPivotInCanvasSpace - currentPivotInCanvasSpace);
                        AZ::Vector3 deltaInCanvasSpace3 = transform.Multiply3x3(EntityHelpers::MakeVec3(deltaInCanvasSpace));
                        deltaInLocalSpace = AZ::Vector2(deltaInCanvasSpace3.GetX(), deltaInCanvasSpace3.GetY());
                    }

                    EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, (currentOffsetsInLocalSpace + deltaInLocalSpace));
                    EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
                }
            }
            else
            {
                AZ_Assert(0, "Invalid CoordinateSystem.");
            }
        }
    }
    else // if (!isSnapping)
    {
        // Translate the offsets
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, (offsets + translation));

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);

        // Return value.
        {
            AZ::Matrix4x4 transformToCanvasSpace;
            EBUS_EVENT_ID(parentElement->GetId(), UiTransformBus, GetTransformToCanvasSpace, transformToCanvasSpace);

            AZ::Vector3 deltaInCanvasSpace3 = transformToCanvasSpace.Multiply3x3(EntityHelpers::MakeVec3(translation));
            deltaInCanvasSpace = AZ::Vector2(deltaInCanvasSpace3.GetX(), deltaInCanvasSpace3.GetY());
        }
    }

    return deltaInCanvasSpace;
}

void ViewportSnap::Rotate(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    AZ::Entity* element,
    float signedAngle)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped rotation.
        item->SetNonSnappedZRotation(item->GetNonSnappedZRotation() + signedAngle);

        // Update the currently used rotation.
        float realRotation;
        EBUS_EVENT_ID_RESULT(realRotation, element->GetId(), UiTransformBus, GetZRotation);

        float snappedRotation = item->GetNonSnappedZRotation();
        {
            float snapRotationInDegrees = 1.0f;
            EBUS_EVENT_ID_RESULT(snapRotationInDegrees, canvasId, UiCanvasBus, GetSnapRotationDegrees);

            snappedRotation = EntityHelpers::Snap(snappedRotation, snapRotationInDegrees);
        }

        if (snappedRotation != realRotation)
        {
            EBUS_EVENT_ID(element->GetId(), UiTransformBus, SetZRotation, snappedRotation);
            EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
        }
    }
    else // if (!isSnapping)
    {
        // Add the angle to the current rotation of this element
        float rotation;
        EBUS_EVENT_ID_RESULT(rotation, element->GetId(), UiTransformBus, GetZRotation);

        EBUS_EVENT_ID(element->GetId(), UiTransformBus, SetZRotation, (rotation + signedAngle));
        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeByGizmo(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::GizmoParts& grabbedGizmoParts,
    AZ::Entity* element,
    const AZ::Vector2& pivot,
    const AZ::Vector2& translation)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Resize the element about the pivot
        UiTransform2dInterface::Offsets offsets(ResizeAboutPivot(item->GetNonSnappedOffsets(), grabbedGizmoParts, pivot, translation));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(offsets);

        UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
        {
            float snapDistance = 1.0f;
            EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiCanvasBus, GetSnapDistance);

            UiTransform2dInterface::Anchors anchors;
            EBUS_EVENT_ID_RESULT(anchors, element->GetId(), UiTransform2dBus, GetAnchors);

            AZ::Entity* parentElement = EntityHelpers::GetParentElement(element);

            AZ::Vector2 parentSize;
            EBUS_EVENT_ID_RESULT(parentSize, parentElement->GetId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

            float newWidth  = parentSize.GetX() * (anchors.m_right  - anchors.m_left) + offsets.m_right  - offsets.m_left;
            float newHeight = parentSize.GetY() * (anchors.m_bottom - anchors.m_top)  + offsets.m_bottom - offsets.m_top;

            if (grabbedGizmoParts.m_right)
            {
                float snappedWidth = ViewportHelpers::IsHorizontallyFit(element) ? newWidth : EntityHelpers::Snap(newWidth, snapDistance);
                float deltaWidth = snappedWidth - newWidth;

                snappedOffsets.m_left = offsets.m_left - deltaWidth * pivot.GetX(); // move left when width increases, so decrease offset
                snappedOffsets.m_right = offsets.m_right + deltaWidth * (1.0f - pivot.GetX()); // move right when width increases so increase offset
            }

            if (grabbedGizmoParts.m_top)
            {
                float snappedHeight = ViewportHelpers::IsVerticallyFit(element) ? newHeight : EntityHelpers::Snap(newHeight, snapDistance);
                float deltaHeight = snappedHeight - newHeight;

                snappedOffsets.m_top = offsets.m_top - deltaHeight * pivot.GetY(); // move left when width increases, so decrease offset
                snappedOffsets.m_bottom = offsets.m_bottom + deltaHeight * (1.0f - pivot.GetY()); // move right when width increases so increase offset
            }
        }

        // Update the currently used offset.
        if (snappedOffsets != offsets)
        {
            EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, snappedOffsets);
            EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element about the pivot
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        UiTransform2dInterface::Offsets newOffsets(ResizeAboutPivot(offsets, grabbedGizmoParts, pivot, translation));

        // Resize the element
        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, newOffsets);

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeDirectlyWithScaleOrRotation(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const UiTransformInterface::RectPoints& translation)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(item->GetNonSnappedOffsets() + translation);

        // Update the currently used offset.
        {
            UiTransform2dInterface::Offsets currentOffsets;
            EBUS_EVENT_ID_RESULT(currentOffsets, element->GetId(), UiTransform2dBus, GetOffsets);

            UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
            {
                float snapDistance = 1.0f;
                EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiCanvasBus, GetSnapDistance);

                snappedOffsets = EntityHelpers::Snap(snappedOffsets, grabbedEdges, snapDistance);
            }

            if (snappedOffsets != currentOffsets)
            {
                EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, snappedOffsets);
                EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
            }
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, (offsets + translation));

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}

void ViewportSnap::ResizeDirectlyNoScaleNoRotation(HierarchyWidget* hierarchy,
    const AZ::EntityId& canvasId,
    const ViewportHelpers::ElementEdges& grabbedEdges,
    AZ::Entity* element,
    const AZ::Vector2& translation)
{
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, canvasId, UiCanvasBus, GetIsSnapEnabled);

    if (isSnapping)
    {
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(hierarchy, element, false));

        // Update the non-snapped offset.
        item->SetNonSnappedOffsets(ViewportHelpers::MoveGrabbedEdges(item->GetNonSnappedOffsets(), grabbedEdges, translation));

        // Update the currently used offset.
        {
            UiTransform2dInterface::Offsets currentOffsets;
            EBUS_EVENT_ID_RESULT(currentOffsets, element->GetId(), UiTransform2dBus, GetOffsets);

            UiTransform2dInterface::Offsets snappedOffsets(item->GetNonSnappedOffsets());
            {
                float snapDistance = 1.0f;
                EBUS_EVENT_ID_RESULT(snapDistance, canvasId, UiCanvasBus, GetSnapDistance);

                snappedOffsets = EntityHelpers::Snap(snappedOffsets, grabbedEdges, snapDistance);
            }

            if (snappedOffsets != currentOffsets)
            {
                EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, snappedOffsets);
                EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
            }
        }
    }
    else // if (!isSnapping)
    {
        // Resize the element
        UiTransform2dInterface::Offsets offsets;
        EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

        EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, ViewportHelpers::MoveGrabbedEdges(offsets, grabbedEdges, translation));

        EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
    }
}
