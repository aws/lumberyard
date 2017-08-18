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
#include "StdAfx.h"
#include "UiNavigationHelpers.h"

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiNavigationBus.h>

namespace UiNavigationHelpers
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId GetNextElement(AZ::EntityId curEntityId, EKeyId keyId,
        const LyShine::EntityArray& navigableElements, AZ::EntityId defaultEntityId, ValidationFunction isValidResult)
    {
        AZ::EntityId nextEntityId;
        bool found = false;
        do
        {
            nextEntityId.SetInvalid();
            UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
            EBUS_EVENT_ID_RESULT(navigationMode, curEntityId, UiNavigationBus, GetNavigationMode);
            if (navigationMode == UiNavigationInterface::NavigationMode::Custom)
            {
                // Ask the current interactable what the next interactable should be
                nextEntityId = FollowCustomLink(curEntityId, keyId);

                if (nextEntityId.IsValid())
                {
                    // Skip over elements that are not valid
                    if (isValidResult(nextEntityId))
                    {
                        found = true;
                    }
                    else
                    {
                        curEntityId = nextEntityId;
                    }
                }
                else
                {
                    found = true;
                }
            }
            else if (navigationMode == UiNavigationInterface::NavigationMode::Automatic)
            {
                nextEntityId = SearchForNextElement(curEntityId, keyId, navigableElements);
                found = true;
            }
            else
            {
                // If navigationMode is None we should never get here via keyboard navigation
                // and we may not be able to get to other elements from here (e.g. this could be
                // a full screen button in the background). So go to the passed in default.
                nextEntityId = defaultEntityId;
                found = true;
            }
        } while (!found);

        return nextEntityId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId SearchForNextElement(AZ::EntityId curElement, EKeyId keyId, const LyShine::EntityArray& navigableElements)
    {
        UiTransformInterface::RectPoints srcPoints;
        EBUS_EVENT_ID(curElement, UiTransformBus, GetViewportSpacePoints, srcPoints);
        AZ::Vector2 srcCenter = srcPoints.GetCenter();

        // Go through the navigable elements and find the closest element to the current hover interactable
        float shortestDist = FLT_MAX;
        float shortestCenterToCenterDist = FLT_MAX;
        AZ::EntityId closestElement;
        for (auto navigableElement : navigableElements)
        {
            UiTransformInterface::RectPoints destPoints;
            EBUS_EVENT_ID(navigableElement->GetId(), UiTransformBus, GetViewportSpacePoints, destPoints);
            AZ::Vector2 destCenter = destPoints.GetCenter();

            bool correctDirection = false;
            if (keyId == eKI_Up)
            {
                correctDirection = destCenter.GetY() < srcPoints.GetAxisAlignedTopLeft().GetY();
            }
            else if (keyId == eKI_Down)
            {
                correctDirection = destCenter.GetY() > srcPoints.GetAxisAlignedBottomLeft().GetY();
            }
            else if (keyId == eKI_Left)
            {
                correctDirection = destCenter.GetX() < srcPoints.GetAxisAlignedTopLeft().GetX();
            }
            else if (keyId == eKI_Right)
            {
                correctDirection = destCenter.GetX() > srcPoints.GetAxisAlignedTopRight().GetX();
            }

            if (correctDirection)
            {
                // Calculate an overlap value from 0 to 1
                float overlapValue = 0.0f;
                if (keyId == eKI_Up || keyId == eKI_Down)
                {
                    float srcLeft = srcPoints.GetAxisAlignedTopLeft().GetX();
                    float srcRight = srcPoints.GetAxisAlignedTopRight().GetX();
                    float destLeft = destPoints.GetAxisAlignedTopLeft().GetX();
                    float destRight = destPoints.GetAxisAlignedTopRight().GetX();

                    if ((srcLeft <= destLeft && srcRight >= destRight)
                        || (srcLeft >= destLeft && srcRight <= destRight))
                    {
                        overlapValue = 1.0f;
                    }
                    else
                    {
                        float x1 = max(srcLeft, destLeft);
                        float x2 = min(srcRight, destRight);
                        if (x1 <= x2)
                        {
                            float overlap = x2 - x1;
                            overlapValue = max(overlap / (srcRight - srcLeft), overlap / (destRight - destLeft));
                        }
                    }
                }
                else // eKI_Left || eKI_Right
                {
                    float destTop = destPoints.GetAxisAlignedTopLeft().GetY();
                    float destBottom = destPoints.GetAxisAlignedBottomLeft().GetY();
                    float srcTop = srcPoints.GetAxisAlignedTopLeft().GetY();
                    float srcBottom = srcPoints.GetAxisAlignedBottomLeft().GetY();

                    if ((srcTop <= destTop && srcBottom >= destBottom)
                        || (srcTop >= destTop && srcBottom <= destBottom))
                    {
                        overlapValue = 1.0f;
                    }
                    else
                    {
                        float y1 = max(srcTop, destTop);
                        float y2 = min(srcBottom, destBottom);
                        if (y1 <= y2)
                        {
                            float overlap = y2 - y1;
                            overlapValue = max(overlap / (srcBottom - srcTop), overlap / (destBottom - destTop));
                        }
                    }
                }

                // Set src and dest points used for distance test
                AZ::Vector2 srcPoint;
                AZ::Vector2 destPoint;
                if ((keyId == eKI_Up) || keyId == eKI_Down)
                {
                    float srcY;
                    float destY;
                    if (keyId == eKI_Up)
                    {
                        srcY = srcPoints.GetAxisAlignedTopLeft().GetY();
                        destY = destPoints.GetAxisAlignedBottomLeft().GetY();
                        if (destY > srcY)
                        {
                            destY = srcY;
                        }
                    }
                    else // eKI_Down
                    {
                        srcY = srcPoints.GetAxisAlignedBottomLeft().GetY();
                        destY = destPoints.GetAxisAlignedTopLeft().GetY();
                        if (destY < srcY)
                        {
                            destY = srcY;
                        }
                    }

                    srcPoint = AZ::Vector2((overlapValue < 1.0f ? srcCenter.GetX() : destCenter.GetX()), srcY);
                    destPoint = AZ::Vector2(destCenter.GetX(), destY);
                }
                else // eKI_Left || eKI_Right
                {
                    float srcX;
                    float destX;
                    if (keyId == eKI_Left)
                    {
                        srcX = srcPoints.GetAxisAlignedTopLeft().GetX();
                        destX = destPoints.GetAxisAlignedTopRight().GetX();
                        if (destX > srcX)
                        {
                            destX = srcX;
                        }
                    }
                    else // eKI_Right
                    {
                        srcX = srcPoints.GetAxisAlignedTopRight().GetX();
                        destX = destPoints.GetAxisAlignedTopLeft().GetX();
                        if (destX < srcX)
                        {
                            destX = srcX;
                        }
                    }

                    srcPoint = AZ::Vector2(srcX, (overlapValue < 1.0f ? srcCenter.GetY() : destCenter.GetY()));
                    destPoint = AZ::Vector2(destX, destCenter.GetY());
                }

                // Calculate angle distance value from 0 to 1
                float angleDist;
                AZ::Vector2 dir = destPoint - srcPoint;
                float angle = RAD2DEG(atan2(-dir.GetY(), dir.GetX()));
                if (angle < 0.0f)
                {
                    angle += 360.0f;
                }

                if (keyId == eKI_Up)
                {
                    angleDist = fabs(90.0f - angle);
                }
                else if (keyId == eKI_Down)
                {
                    angleDist = fabs(270.0f - angle);
                }
                else if (keyId == eKI_Left)
                {
                    angleDist = fabs(180.0f - angle);
                }
                else // eKI_Right
                {
                    angleDist = fabs((angle <= 180.0f ? 0.0f : 360.0f) - angle);
                }
                float angleValue = angleDist / 90.0f;

                // Calculate final distance value biased by overlap and angle values
                float dist = (destPoint - srcPoint).GetLength();
                const float distMultConstant = 1.0f;
                dist += dist * distMultConstant * angleValue * (1.0f - overlapValue);

                if (dist < shortestDist)
                {
                    shortestDist = dist;
                    shortestCenterToCenterDist = (destCenter - srcCenter).GetLengthSq();
                    closestElement = navigableElement->GetId();
                }
                else if (dist == shortestDist)
                {
                    // Break a tie using center to center distance
                    float centerToCenterDist = (destCenter - srcCenter).GetLengthSq();
                    if (centerToCenterDist < shortestCenterToCenterDist)
                    {
                        shortestCenterToCenterDist = centerToCenterDist;
                        closestElement = navigableElement->GetId();
                    }
                }
            }
        }

        return closestElement;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId FollowCustomLink(AZ::EntityId curEntityId, EKeyId keyId)
    {
        AZ::EntityId nextEntityId;

        // Ask the current interactable what the next interactable should be
        if (keyId == eKI_Up)
        {
            EBUS_EVENT_ID_RESULT(nextEntityId, curEntityId, UiNavigationBus, GetOnUpEntity);
        }
        else if (keyId == eKI_Down)
        {
            EBUS_EVENT_ID_RESULT(nextEntityId, curEntityId, UiNavigationBus, GetOnDownEntity);
        }
        else if (keyId == eKI_Left)
        {
            EBUS_EVENT_ID_RESULT(nextEntityId, curEntityId, UiNavigationBus, GetOnLeftEntity);
        }
        else
        {
            EBUS_EVENT_ID_RESULT(nextEntityId, curEntityId, UiNavigationBus, GetOnRightEntity);
        }

        return nextEntityId;
    }
 

} // namespace UiNavigationHelpers