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
#include "LyShine_precompiled.h"
#include "UiLayoutHelpers.h"

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>
#include <LyShine/Bus/UiLayoutCellBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <LyShine/Bus/UiLayoutFitterBus.h>

namespace
{
    float GetLargestFloat(const AZStd::vector<float>& values)
    {
        float largestValue = 0.0f;

        for (auto value : values)
        {
            if (largestValue < value)
            {
                largestValue = value;
            }
        }

        return largestValue;
    }

    float GetElementDefaultMinWidth(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        EBUS_EVENT_ID_RESULT(results, elementId, UiLayoutCellDefaultBus, GetMinWidth);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultTargetWidth(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        EBUS_EVENT_ID_RESULT(results, elementId, UiLayoutCellDefaultBus, GetTargetWidth);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultExtraWidthRatio(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        EBUS_EVENT_ID_RESULT(results, elementId, UiLayoutCellDefaultBus, GetExtraWidthRatio);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultMinHeight(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        EBUS_EVENT_ID_RESULT(results, elementId, UiLayoutCellDefaultBus, GetMinHeight);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultTargetHeight(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        EBUS_EVENT_ID_RESULT(results, elementId, UiLayoutCellDefaultBus, GetTargetHeight);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultExtraHeightRatio(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        EBUS_EVENT_ID_RESULT(results, elementId, UiLayoutCellDefaultBus, GetExtraHeightRatio);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }
}

namespace UiLayoutHelpers
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LayoutCellSize::LayoutCellSize()
        : m_minSize(-1.0f)
        , m_targetSize(-1.0f)
        , m_extraSizeRatio(-1.0f)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void GetLayoutCellWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells, LayoutCellSizes& layoutCellsOut)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        EBUS_EVENT_ID_RESULT(childEntityIds, elementId, UiElementBus, GetChildEntityIds);

        layoutCellsOut.reserve(childEntityIds.size());

        for (auto childEntityId : childEntityIds)
        {
            LayoutCellSize layoutCell;

            // First check for overridden cell values
            if (UiLayoutCellBus::FindFirstHandler(childEntityId))
            {
                EBUS_EVENT_ID_RESULT(layoutCell.m_minSize, childEntityId, UiLayoutCellBus, GetMinWidth);
                EBUS_EVENT_ID_RESULT(layoutCell.m_targetSize, childEntityId, UiLayoutCellBus, GetTargetWidth);
                EBUS_EVENT_ID_RESULT(layoutCell.m_extraSizeRatio, childEntityId, UiLayoutCellBus, GetExtraWidthRatio);
            }

            // If not overridden, get the default cell values
            if (layoutCell.m_minSize < 0.0f)
            {
                layoutCell.m_minSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_minSize = GetElementDefaultMinWidth(childEntityId, layoutCell.m_minSize);
                }
            }
            if (layoutCell.m_targetSize < 0.0f)
            {
                layoutCell.m_targetSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_targetSize = GetElementDefaultTargetWidth(childEntityId, layoutCell.m_targetSize);
                }
            }
            if (layoutCell.m_extraSizeRatio < 0.0f)
            {
                layoutCell.m_extraSizeRatio = 1.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_extraSizeRatio = GetElementDefaultExtraWidthRatio(childEntityId, layoutCell.m_extraSizeRatio);
                }
            }

            layoutCell.m_targetSize = AZ::GetMax(layoutCell.m_targetSize, layoutCell.m_minSize);

            layoutCellsOut.push_back(layoutCell);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void GetLayoutCellHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells, LayoutCellSizes& layoutCellsOut)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        EBUS_EVENT_ID_RESULT(childEntityIds, elementId, UiElementBus, GetChildEntityIds);

        layoutCellsOut.reserve(childEntityIds.size());

        for (auto childEntityId : childEntityIds)
        {
            LayoutCellSize layoutCell;

            // First check for overridden cell values
            if (UiLayoutCellBus::FindFirstHandler(childEntityId))
            {
                EBUS_EVENT_ID_RESULT(layoutCell.m_minSize, childEntityId, UiLayoutCellBus, GetMinHeight);
                EBUS_EVENT_ID_RESULT(layoutCell.m_targetSize, childEntityId, UiLayoutCellBus, GetTargetHeight);
                EBUS_EVENT_ID_RESULT(layoutCell.m_extraSizeRatio, childEntityId, UiLayoutCellBus, GetExtraHeightRatio);
            }

            // If not overridden, get the default cell values
            if (layoutCell.m_minSize < 0.0f)
            {
                layoutCell.m_minSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_minSize = GetElementDefaultMinHeight(childEntityId, layoutCell.m_minSize);
                }
            }
            if (layoutCell.m_targetSize < 0.0f)
            {
                layoutCell.m_targetSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_targetSize = GetElementDefaultTargetHeight(childEntityId, layoutCell.m_targetSize);
                }
            }
            if (layoutCell.m_extraSizeRatio < 0.0f)
            {
                layoutCell.m_extraSizeRatio = 1.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_extraSizeRatio = GetElementDefaultExtraHeightRatio(childEntityId, layoutCell.m_extraSizeRatio);
                }
            }

            layoutCell.m_targetSize = AZ::GetMax(layoutCell.m_targetSize, layoutCell.m_minSize);

            layoutCellsOut.push_back(layoutCell);
        }
    }

    AZStd::vector<float> GetLayoutCellMinWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        EBUS_EVENT_ID_RESULT(childEntityIds, elementId, UiElementBus, GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = -1.0f;

            // First check for overridden cell values
            EBUS_EVENT_ID_RESULT(value, childEntityId, UiLayoutCellBus, GetMinWidth);

            // If not overridden, get the default cell values
            if (value < 0.0f)
            {
                value = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    value = GetElementDefaultMinWidth(childEntityId, value);
                }
            }

            values[i] = value;
            i++;
        }

        return values;
    }

    AZStd::vector<float> GetLayoutCellTargetWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        EBUS_EVENT_ID_RESULT(childEntityIds, elementId, UiElementBus, GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = -1.0f;

            // First check for overridden cell values
            EBUS_EVENT_ID_RESULT(value, childEntityId, UiLayoutCellBus, GetTargetWidth);

            // If not overridden, get the default cell values
            if (value < 0.0f)
            {
                value = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    value = GetElementDefaultTargetWidth(childEntityId, value);
                }
            }

            // Make sure that min width isn't greater than target width
            float minValue = -1.0f;
            EBUS_EVENT_ID_RESULT(minValue, childEntityId, UiLayoutCellBus, GetMinWidth);
            if (minValue < 0.0f)
            {
                minValue = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    minValue = GetElementDefaultMinWidth(childEntityId, minValue);
                }
            }
            value = AZ::GetMax(value, minValue);

            values[i] = value;
            i++;
        }

        return values;
    }

    AZStd::vector<float> GetLayoutCellMinHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        EBUS_EVENT_ID_RESULT(childEntityIds, elementId, UiElementBus, GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = -1.0f;

            // First check for overridden cell values
            EBUS_EVENT_ID_RESULT(value, childEntityId, UiLayoutCellBus, GetMinHeight);

            // If not overridden, get the default cell values
            if (value < 0.0f)
            {
                value = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    value = GetElementDefaultMinHeight(childEntityId, value);
                }
            }

            values[i] = value;
            i++;
        }

        return values;
    }

    AZStd::vector<float> GetLayoutCellTargetHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        EBUS_EVENT_ID_RESULT(childEntityIds, elementId, UiElementBus, GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = -1.0f;

            // First check for overridden cell values
            EBUS_EVENT_ID_RESULT(value, childEntityId, UiLayoutCellBus, GetTargetHeight);

            // If not overridden, get the default cell values
            if (value < 0.0f)
            {
                value = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    value = GetElementDefaultTargetHeight(childEntityId, value);
                }
            }

            // Make sure that min height isn't greater than target height
            float minValue = -1.0f;
            EBUS_EVENT_ID_RESULT(minValue, childEntityId, UiLayoutCellBus, GetMinHeight);
            if (minValue < 0.0f)
            {
                minValue = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    minValue = GetElementDefaultMinHeight(childEntityId, minValue);
                }
            }
            value = AZ::GetMax(value, minValue);

            values[i] = value;
            i++;
        }

        return values;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void CalculateElementSizes(const LayoutCellSizes& layoutCells, float availableSize, float spacing, AZStd::vector<float>& sizesOut)
    {
        int numElements = layoutCells.size();

        availableSize -= (numElements - 1) * spacing;

        // Check if there's enough space for all target sizes
        float totalTargetSize = 0.0f;
        for (int i = 0; i < numElements; i++)
        {
            totalTargetSize += layoutCells[i].m_targetSize;
        }

        if (totalTargetSize <= availableSize)
        {
            // Enough space for all target sizes, allocate target size
            // Target size is always greater than or equal to min size
            for (int i = 0; i < numElements; i++)
            {
                sizesOut[i] = layoutCells[i].m_targetSize;
                availableSize -= layoutCells[i].m_targetSize;
            }
        }
        else
        {
            // Not enough space for all target sizes

            // Allocate min size
            for (int i = 0; i < numElements; i++)
            {
                sizesOut[i] = layoutCells[i].m_minSize;
                availableSize -= layoutCells[i].m_minSize;
            }

            // If there is space left, try to allocate target size
            if (availableSize > 0.0f)
            {
                // Make a list of element indexes that have not met their target sizes
                AZStd::vector<int> needTargetSizeIndexes;
                AZStd::vector<float> neededAmounts;
                float totalNeededAmount = 0.0f;
                for (int i = 0; i < numElements; i++)
                {
                    if (sizesOut[i] < layoutCells[i].m_targetSize)
                    {
                        needTargetSizeIndexes.push_back(i);
                        float neededAmount = layoutCells[i].m_targetSize - sizesOut[i];
                        neededAmounts.push_back(neededAmount);
                        totalNeededAmount += neededAmount;
                    }
                }

                if (!needTargetSizeIndexes.empty())
                {
                    // Give each element part of the available space to get as close to target size as possible
                    int neededAmountIndex = 0;
                    for (auto index : needTargetSizeIndexes)
                    {
                        sizesOut[index] += (neededAmounts[neededAmountIndex] / totalNeededAmount) * availableSize;

                        neededAmountIndex++;
                    }
                }

                availableSize = 0.0f;
            }
        }

        // If there is still space left, allocate extra size based on ratios
        if (availableSize > 0.0f)
        {
            // Make a list of element indexes that need extra size
            AZStd::vector<int> needExtraSizeIndexes;
            float smallestRatio = -1.0f;
            for (int i = 0; i < numElements; i++)
            {
                if (layoutCells[i].m_extraSizeRatio > 0.0f)
                {
                    needExtraSizeIndexes.push_back(i);

                    // Track the smallest ratio
                    if (smallestRatio < 0.0f || smallestRatio > layoutCells[i].m_extraSizeRatio)
                    {
                        smallestRatio = layoutCells[i].m_extraSizeRatio;
                    }
                }
            }

            if (!needExtraSizeIndexes.empty())
            {
                // Normalize ratios so that the smallest ratio has a value of one
                AZStd::vector<float> normalizedExtraSizeRatios(needExtraSizeIndexes.size(), 0.0f);
                int normalizedRatioIndex = 0;
                float totalUnits = 0.0f;
                for (auto index : needExtraSizeIndexes)
                {
                    normalizedExtraSizeRatios[normalizedRatioIndex] = layoutCells[index].m_extraSizeRatio / smallestRatio;
                    totalUnits += normalizedExtraSizeRatios[normalizedRatioIndex];
                    normalizedRatioIndex++;
                }

                if (totalUnits > 0.0f)
                {
                    // Add extra size to each element
                    float sizePerUnit = availableSize / totalUnits;
                    normalizedRatioIndex = 0;
                    for (auto index : needExtraSizeIndexes)
                    {
                        float sizeToAdd = normalizedExtraSizeRatios[normalizedRatioIndex] * sizePerUnit;
                        sizesOut[index] += sizeToAdd;
                        normalizedRatioIndex++;
                    }
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float CalculateSingleElementSize(const LayoutCellSize& layoutCell, float availableSize)
    {
        float size = 0.0f;

        if (layoutCell.m_minSize > availableSize)
        {
            size = layoutCell.m_minSize;
        }
        else
        {
            if (layoutCell.m_extraSizeRatio > 0.0f)
            {
                size = availableSize;
            }
            else
            {
                size = AZ::GetMin(availableSize, layoutCell.m_targetSize);
            }
        }

        return size;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float GetHorizontalAlignmentOffset(IDraw2d::HAlign hAlignment, float availableSpace, float occupiedSpace)
    {
        float alignmentOffset = 0.0f;

        switch (hAlignment)
        {
        case IDraw2d::HAlign::Left:
            alignmentOffset = 0.0f;
            break;

        case IDraw2d::HAlign::Center:
            alignmentOffset = (availableSpace - occupiedSpace) * 0.5f;
            break;

        case IDraw2d::HAlign::Right:
            alignmentOffset = availableSpace - occupiedSpace;
            break;

        default:
            AZ_Assert(0, "Unrecognized HAlign type in UiLayout");
            alignmentOffset = 0.0f;
            break;
        }

        return alignmentOffset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float GetVerticalAlignmentOffset(IDraw2d::VAlign vAlignment, float availableSpace, float occupiedSpace)
    {
        float alignmentOffset = 0.0f;

        switch (vAlignment)
        {
        case IDraw2d::VAlign::Top:
            alignmentOffset = 0.0f;
            break;

        case IDraw2d::VAlign::Center:
            alignmentOffset = (availableSpace - occupiedSpace) * 0.5f;
            break;

        case IDraw2d::VAlign::Bottom:
            alignmentOffset = availableSpace - occupiedSpace;
            break;

        default:
            AZ_Assert(0, "Unrecognized VAlign type in UiLayout Component");
            alignmentOffset = 0.0f;
            break;
        }

        return alignmentOffset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControllingChild(AZ::EntityId parentId, AZ::EntityId childId)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, parentId, UiElementBus, FindChildByEntityId, childId);

        if (element == nullptr)
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void GetSizeInsidePadding(AZ::EntityId elementId, const UiLayoutInterface::Padding& padding, AZ::Vector2& size)
    {
        EBUS_EVENT_ID_RESULT(size, elementId, UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);

        float width = size.GetX() - (padding.m_left + padding.m_right);
        float height = size.GetY() - (padding.m_top + padding.m_bottom);

        // Add a small value to accommodate for rounding errors
        const float epsilon = 0.01f;
        width += epsilon;
        height += epsilon;

        size.Set(width, height);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void InvalidateLayout(AZ::EntityId elementId)
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, elementId, UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayout, elementId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void InvalidateParentLayout(AZ::EntityId elementId)
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, elementId, UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, elementId, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControlledByHorizontalFit(AZ::EntityId elementId)
    {
        bool isHorizontallyFit = false;
        EBUS_EVENT_ID_RESULT(isHorizontallyFit, elementId, UiLayoutFitterBus, GetHorizontalFit);
        return isHorizontallyFit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControlledByVerticalFit(AZ::EntityId elementId)
    {
        bool isVerticallyFit = false;
        EBUS_EVENT_ID_RESULT(isVerticallyFit, elementId, UiLayoutFitterBus, GetVerticalFit);
        return isVerticallyFit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void CheckFitterAndRefreshEditorTransformProperties(AZ::EntityId elementId)
    {
        if (IsControlledByHorizontalFit(elementId) || (IsControlledByVerticalFit(elementId)))
        {
            EBUS_EVENT(UiEditorChangeNotificationBus, OnEditorTransformPropertiesNeedRefresh);
        }
    }
} // namespace UiLayoutHelpers