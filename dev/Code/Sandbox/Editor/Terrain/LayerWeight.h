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

#include <ITerrain.h>
#include <AzCore/std/containers/vector.h>

//
// LayerWeight stores compressed weights for detail layers across the terrain. The compression mechanism
// restricts the maximum layers that can influence a single element to a fixed amount. (LayerWeight::WeightCount) This
// means that attempting to add more layers will "kick out" the least significant layer. In order to simplify the engine
// code, this structure adheres to some strict data constraints that are ensured by the API.
//
// 1) A weight and id at index N are paired. Weight[N] / Ids[N] go together.
//
// 2) Weights MUST sum to 255.
//
// 3) Id / Weight pairs are sorted in monotonically decreasing order with respect to weight. Index 0 is
//    guaranteed to be your most significant layer; Index N-1 is your least significant one.
//
// 4) The hole sentinel value is stored in Id[0]. This means the element represents a hole in the terrain.
//
// 5) Unused layer channels must be set to ID: Undefined / Weight: 0.
//
// 6) Layers cannot be duplicated.
//
// The public API on this struct enforces these constraints and checks them heavily using asserts in debug mode.
// If you plan to make changes to this struct, please maintain these constraints.
//
struct LayerWeight
    : public ITerrain::SurfaceWeight
{
    LayerWeight()
        : SurfaceWeight() {}

    LayerWeight(uint8 id)
        : SurfaceWeight()
    {
        Ids[0] = id;
        Weights[0] = 255;
    }
    LayerWeight(const AZStd::vector<uint8>& layerIds, const AZStd::vector<uint8>& weights);

    uint8 GetLayerId(int index) const;

    void SetWeight(uint8 layerId, uint8 weight);

    uint8 GetWeight(uint8 layerId) const;

    void EraseLayer(uint8 layerId, uint8 replacementId);

private:
    void Sort();

    void Normalize(int ignoreIndex, int remainder);

    void Validate();
};

//
// NOTE: You may not change the size of this struct. It gets trivially casted back to the parent type on the engine side.
//
static_assert(sizeof(LayerWeight) == sizeof(ITerrain::SurfaceWeight), "LayerWeight size must match parent SurfaceWeight size.");

using Weightmap = TImage<LayerWeight>;
