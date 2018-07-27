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

// Description : Basic type definitions used by the CryMannequin AnimationGraph adapter
//               classes


#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINAGDEFS_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINAGDEFS_H
#pragma once


namespace MannequinAG
{
    // ============================================================================
    // ============================================================================


    typedef CryFixedStringT<64> TKeyValue;

    enum ESupportedInputID
    {
        eSIID_Action = 0, // has to start with zero (is an assumption all over the place).,
        eSIID_Signal,

        eSIID_COUNT,
    };

    enum EAIActionType
    {
        EAT_Looping,
        EAT_OneShot,
    };
    // ============================================================================
    // ============================================================================
} // namespace MannequinAG

#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINAGDEFS_H
