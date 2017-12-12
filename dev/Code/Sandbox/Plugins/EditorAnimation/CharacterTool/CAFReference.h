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

struct SCAFReference
{
    SCAFReference()
        : pathCRC(0) { }
    SCAFReference(unsigned int pathCRC, const char* animationName)
        : pathCRC(0) { reset(pathCRC, animationName); }
    SCAFReference(const SCAFReference& rhs)
        : pathCRC(0) { reset(rhs.pathCRC, rhs.animationName.c_str()); }
    ~SCAFReference() { reset(0); }

    SCAFReference& operator=(const SCAFReference& rhs) { reset(rhs.pathCRC, rhs.animationName.c_str()); return *this; }

    void reset(unsigned int crc = 0, const char* animationName = "");
    unsigned int PathCRC() const{ return pathCRC; }
    const char* AnimationName() const { return animationName.c_str(); }
private:
    unsigned int pathCRC;
    string animationName;
};
