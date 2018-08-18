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

#ifndef CRYINCLUDE_CRYANIMATION_PATHEXPANSION_H
#define CRYINCLUDE_CRYANIMATION_PATHEXPANSION_H
#pragma once

namespace PathExpansion
{
    // Expand patterns into paths. An example of a pattern before expansion:
    // animations/facial/idle_{0,1,2,3}.fsq
    // {} is used to specify options for parts of the string.
    // output must point to buffer that is at least as large as the pattern.
    void SelectRandomPathExpansion(const char* pattern, char* output);
    void EnumeratePathExpansions(const char* pattern, void (* enumCallback)(void* userData, const char* expansion), void* userData);
}

#endif // CRYINCLUDE_CRYANIMATION_PATHEXPANSION_H
