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

#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINAGEXISTANCEQUERY_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINAGEXISTANCEQUERY_H
#pragma once

#include "IAnimationGraph.h"

namespace MannequinAG
{
    // ============================================================================
    // ============================================================================

    class CMannequinAGExistanceQuery
        : public IAnimationGraphExistanceQuery
    {
    public:
        CMannequinAGExistanceQuery(IAnimationGraphState* pAnimationGraphState);
        virtual ~CMannequinAGExistanceQuery();

    private:
        // IAnimationGraphExistanceQuery
        virtual IAnimationGraphState* GetState();
        virtual void SetInput(InputID, float);
        virtual void SetInput(InputID, int);
        virtual void SetInput(InputID, const char*);

        virtual bool Complete();
        virtual CTimeValue GetAnimationLength() const;
        virtual void Reset();
        virtual void Release() { delete this; }
        // ~IAnimationGraphExistanceQuery

    private:
        IAnimationGraphState* m_pAnimationGraphState;
    };


    // ============================================================================
    // ============================================================================
} // namespace MannequinAG

#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINAGEXISTANCEQUERY_H
