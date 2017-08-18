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

#ifndef __PROCEDURAL_CLIPS_POSITIONING__H__
#define __PROCEDURAL_CLIPS_POSITIONING__H__

#include <ICryMannequin.h>

struct SProceduralClipPosAdjustTargetLocatorParams
    : public IProceduralParams
{
    SProceduralClipPosAdjustTargetLocatorParams()
    {
    }

    virtual void Serialize(Serialization::IArchive& ar);

    virtual void GetExtraDebugInfo(StringWrapper& extraInfoOut) const override
    {
        extraInfoOut = targetScopeName.c_str();
    }

    SProcDataCRC targetScopeName;
    TProcClipString targetJointName;
    SProcDataCRC targetStateName;
};

#endif