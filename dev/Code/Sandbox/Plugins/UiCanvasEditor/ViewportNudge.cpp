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

static const float slowNudgePixelDistance = 1.0f;
static const float fastNudgePixelDistance = 10.0f;
static const float slowNudgeRotationDegrees = 1.0f;
static const float fastNudgeRotationDegrees = 10.0f;

void ViewportNudge::KeyReleaseEvent(
    EditorWindow* editorWindow,
    ViewportInteraction::InteractionMode interactionMode,
    ViewportWidget* viewport,
    QKeyEvent* ev,
    QTreeWidgetItemRawPtrQList& selectedItems,
    ViewportInteraction::CoordinateSystem coordinateSystem,
    const AZ::Uuid& transformComponentType)
{
    if (interactionMode == ViewportInteraction::InteractionMode::MOVE)
    {
        if ((ev->key() != Qt::Key_Up) &&
            (ev->key() != Qt::Key_Right) &&
            (ev->key() != Qt::Key_Down) &&
            (ev->key() != Qt::Key_Left))
        {
            // Ignore this event.
            return;
        }

        float translation = (ev->modifiers().testFlag(Qt::ShiftModifier) ? fastNudgePixelDistance : slowNudgePixelDistance);

        // Translate.
        {
            auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElementsNotControlledByParent(editorWindow->GetHierarchy(), selectedItems);
            if (topLevelSelectedElements.empty())
            {
                // Nothing to do.
                return;
            }

            editorWindow->GetProperties()->BeforePropertyModified(nullptr);

            for (auto element : topLevelSelectedElements)
            {
                AZ::Vector2 deltaInCanvasSpace(0.0f, 0.0f);
                if (ev->key() == Qt::Key_Up)
                {
                    deltaInCanvasSpace.SetY(-translation);
                }
                else if (ev->key() == Qt::Key_Down)
                {
                    deltaInCanvasSpace.SetY(translation);
                }
                else if (ev->key() == Qt::Key_Left)
                {
                    deltaInCanvasSpace.SetX(-translation);
                }
                else if (ev->key() == Qt::Key_Right)
                {
                    deltaInCanvasSpace.SetX(translation);
                }
                else
                {
                    AZ_Assert(0, "Unexpected key.");
                }

                AZ::Vector2 deltaInLocalSpace;
                if (coordinateSystem == ViewportInteraction::CoordinateSystem::LOCAL)
                {
                    deltaInLocalSpace = deltaInCanvasSpace;
                }
                else // if (coordinateSystem == ViewportInteraction::CoordinateSystem::VIEW)
                {
                    AZ::Matrix4x4 transform;
                    EBUS_EVENT_ID(EntityHelpers::GetParentElement(element)->GetId(), UiTransformBus, GetTransformFromCanvasSpace, transform);
                    AZ::Vector3 deltaInLocalSpace3 = transform.Multiply3x3(EntityHelpers::MakeVec3(deltaInCanvasSpace));
                    deltaInLocalSpace = AZ::Vector2(deltaInLocalSpace3.GetX(), deltaInLocalSpace3.GetY());
                }

                // Get.
                UiTransform2dInterface::Offsets offsets;
                EBUS_EVENT_ID_RESULT(offsets, element->GetId(), UiTransform2dBus, GetOffsets);

                // Set.
                EBUS_EVENT_ID(element->GetId(), UiTransform2dBus, SetOffsets, (offsets + deltaInLocalSpace));

                // Update.
                EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
            }

            // Tell the Properties panel to update
            editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);

            editorWindow->GetProperties()->AfterPropertyModified(nullptr);
        }
    }
    else if (interactionMode == ViewportInteraction::InteractionMode::ROTATE)
    {
        float rotationDirection = 1.0f;
        float rotationDeltaInDegrees = (ev->modifiers().testFlag(Qt::ShiftModifier) ? fastNudgeRotationDegrees : slowNudgeRotationDegrees);
        {
            if ((ev->key() == Qt::Key_Up) ||
                (ev->key() == Qt::Key_Left))
            {
                rotationDirection = -1.0f;
            }
            else if ((ev->key() == Qt::Key_Down) ||
                     (ev->key() == Qt::Key_Right))
            {
                rotationDirection = 1.0f;
            }
            else
            {
                // Ignore this event.
                return;
            }
        }

        // Rotate.
        {
            auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(editorWindow->GetHierarchy(), selectedItems);
            if (topLevelSelectedElements.empty())
            {
                // Nothing to do.
                return;
            }

            editorWindow->GetProperties()->BeforePropertyModified(nullptr);

            for (auto element : topLevelSelectedElements)
            {
                // Get.
                float rotation;
                EBUS_EVENT_ID_RESULT(rotation, element->GetId(), UiTransformBus, GetZRotation);

                // Set.
                EBUS_EVENT_ID(element->GetId(), UiTransformBus, SetZRotation, (rotation + (rotationDeltaInDegrees * rotationDirection)));

                // Update.
                EBUS_EVENT_ID(element->GetId(), UiElementChangeNotificationBus, UiElementPropertyChanged);
            }

            // Tell the Properties panel to update
            editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);

            editorWindow->GetProperties()->AfterPropertyModified(nullptr);
        }
    }
}
