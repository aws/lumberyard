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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <vector>
#include "Serialization/Strings.h"
using Serialization::string;
#include "Serialization.h"

struct BlockPaletteItem
{
    string name;
    ColorB color;
    union
    {
        long long userId;
        void* userPointer;
    };
    std::vector<char> userPayload;

    BlockPaletteItem()
        : color(255, 255, 255, 255)
        , userId(0)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(name, "name", "Name");
        ar(color, "color", "Color");
        ar(userId, "userId", "UserID");
    }
};
typedef std::vector<BlockPaletteItem> BlockPaletteItems;

struct BlockPaletteContent
{
    BlockPaletteItems items;

    int lineHeight;
    int padding;

    BlockPaletteContent()
        : lineHeight(24)
        , padding(2)
    {
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(lineHeight, "lineHeight", "Line Height");
        ar(padding, "padding", "Padding");
        ar(items, "items", "Items");
    }
};

