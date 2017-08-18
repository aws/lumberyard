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

#include "AnimationFilter.h"

namespace Serialization {
    class IArchive;
}

struct SDBAEntry
{
    SAnimationFilter filter;
    string path;

    void Serialize(Serialization::IArchive& ar);
};

class XmlNodeRef;
struct SDBATable
{
    std::vector<SDBAEntry> entries;

    void Serialize(Serialization::IArchive& ar);

    // returns -1 when nothing is found
    int FindDBAForAnimation(const SAnimationFilterItem& animation) const;

    bool Load(const char* dbaTablePath);
    bool Save(const char* dbaTablePath);

    bool ImportOldXML(const XmlNodeRef& root);
};
