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
#include "LayerWeight.h"
#include "Layer.h"

LayerWeight::LayerWeight(const AZStd::vector<uint8>& layerIds, const AZStd::vector<uint8>& weights)
{
    auto sortedLayerIds = layerIds;
    auto sortedWeights = weights;
    auto count = layerIds.size();
    assert(count == weights.size());

    // Layers are always monotonically decreasing with respect to weights.
    for (decltype(count)i = 1; i < count; ++i)
    {
        auto j = i;

        // By using >=, we ensure that the highest priority layers (which are at the end of the unsorted list) are always placed first in the list
        while (j > 0 && sortedWeights[j] >= sortedWeights[j - 1])
        {
            std::swap(sortedWeights[j], sortedWeights[j - 1]);
            std::swap(sortedLayerIds[j], sortedLayerIds[j - 1]);
            --j;
        }
    }

    // Pick the top three (or less), padding out the remainder so normalize doesn't get confused
    count = std::min(count, decltype(count)(WeightCount));
    int weight = 0;
    for (decltype(count)i = 0; i < count; ++i)
    {
        Ids[i] = sortedLayerIds[i];
        Weights[i] = sortedWeights[i];
        weight += sortedWeights[i];
    }
    while (count < WeightCount)
    {
        Ids[count] = Undefined;
        Weights[count++] = 0;
    }

    // Normalize the data
    weight -= 255;
    Normalize(WeightCount, weight);
}

uint8 LayerWeight::GetLayerId(int index) const
{
    assert (index < WeightCount);
    return Ids[index] & CLayer::e_undefined;
}

void LayerWeight::SetWeight(uint8 layerId, uint8 weight)
{
    int foundIndex = 0;
    for (; foundIndex < (WeightCount - 1); ++foundIndex)
    {
        if (GetLayerId(foundIndex) == layerId)
        {
            break;
        }
    }

    int remainder = static_cast<int>(weight) - static_cast<int>(Weights[foundIndex]);
    Weights[foundIndex] = weight;
    Ids[foundIndex] = layerId | (Ids[foundIndex] & CLayer::e_hole);

    Normalize(foundIndex, remainder);
    Sort();
    Validate();
}

uint8 LayerWeight::GetWeight(uint8 layerId) const
{
    for (int i = 0; i < WeightCount; ++i)
    {
        if (GetLayerId(i) == layerId)
        {
            return Weights[i];
        }
    }
    return 0;
}

void LayerWeight::EraseLayer(uint8 layerId, uint8 replacementId)
{
    assert(replacementId != Undefined);
    assert(replacementId != layerId);

    // If we're deleting our last layer, fill it in with the default one instead.
    if (GetLayerId(0) == layerId && Weights[0] == 255)
    {
        Ids[0] = (Ids[0] & CLayer::e_hole) | replacementId;
        return;
    }

    // Otherwise, find the layer in the remaining channels and zero out the weight.
    int remainder = 0;
    int foundIndex = 0;
    for (; foundIndex < WeightCount; ++foundIndex)
    {
        if (GetLayerId(foundIndex) == layerId)
        {
            Ids[foundIndex] = (Ids[foundIndex] & CLayer::e_hole) | CLayer::e_undefined;
            remainder = -static_cast<int>(Weights[foundIndex]);
            Weights[foundIndex] = 0;
            break;
        }
    }

    // A remainder has to get added to another layer to maintain unity.
    if (remainder)
    {
        Normalize(foundIndex, remainder);
        Sort();
        Validate();
    }
}

void LayerWeight::Sort()
{
    // Only first element should ever have a hole.
    uint8 holeMask = Ids[0] & CLayer::e_hole;
    Ids[0] &= ~CLayer::e_hole;

    // Layers are always monotonically decreasing with respect to weights.
    for (int i = 1; i < LayerWeight::WeightCount; ++i)
    {
        int j = i;
        while (j > 0 && Weights[j] > Weights[j - 1])
        {
            std::swap(Weights[j], Weights[j - 1]);
            std::swap(Ids[j], Ids[j - 1]);
            --j;
        }
    }

    Ids[0] |= holeMask;
}

void LayerWeight::Normalize(int ignoreIndex, int remainder)
{
    // Too much total weight in system. Incrementally remove weight from other layers.
    if (remainder > 0)
    {
        for (int i = WeightCount - 1; i >= 0; --i)
        {
            if (i != ignoreIndex)
            {
                const uint8 AmountToChange = static_cast<uint8>(std::min(static_cast<int>(Weights[i]), remainder));
                Weights[i] -= AmountToChange;
                if (Weights[i] == 0)
                {
                    Ids[i] = (Ids[i] & CLayer::e_hole) | CLayer::e_undefined;
                }

                remainder -= AmountToChange;
                if (remainder == 0)
                {
                    break;
                }
            }
        }
    }

    // Too little weight, just add the remainder to the next least-significant layer.
    else
    {
        for (int i = WeightCount - 1; i >= 0; --i)
        {
            if (i != ignoreIndex && GetLayerId(i) != Undefined)
            {
                Weights[i] -= remainder;
                break;
            }
        }
    }
}

void LayerWeight::Validate()
{
#if defined(DEBUG)
    uint8 prevWeight = 255;
    uint32 totalWeight = 0;

    for (int i = 0; i < LayerWeight::WeightCount; ++i)
    {
        // Weights should be monotonically decreasing.
        assert(Weights[i] <= prevWeight);
        prevWeight = Weights[i];

        // Invalid ids should not have weights.
        if (GetLayerId(i) == LayerWeight::Undefined)
        {
            assert(Weights[i] == 0);
        }

        // Make sure we don't have duplicate IDs.
        if (GetLayerId(i) != LayerWeight::Undefined)
        {
            for (int j = 0; j < LayerWeight::WeightCount; ++j)
            {
                if (j != i)
                {
                    assert(GetLayerId(i) != GetLayerId(j));
                }
            }
        }

        // Non-zero index ids cannot have a hole bit.
        if (i != 0)
        {
            assert((Ids[i] & CLayer::e_hole) == 0);
        }

        totalWeight += Weights[i];
    }

    // Weights should sum to unity.
    assert(totalWeight == 255);
#endif
}
