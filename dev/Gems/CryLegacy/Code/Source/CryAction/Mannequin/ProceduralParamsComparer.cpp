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

#include "CryLegacy_precompiled.h"
#include <CryExtension/Impl/ClassWeaver.h>
#include <ICryMannequin.h>
#include <Mannequin/Serialization.h>
#include <Serialization/IArchiveHost.h>

class CProceduralParamsComparerDefault
    : public IProceduralParamsComparer
{
public:
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IProceduralParamsComparer)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CProceduralParamsComparerDefault, IProceduralParamsComparerDefaultName, 0xfc53bd9248534faa, 0xab0fb42b24e55b3e)

    virtual bool Equal(const IProceduralParams& lhs, const IProceduralParams& rhs) const
    {
        return Serialization::CompareBinary(lhs, rhs);
    }
};

CProceduralParamsComparerDefault::CProceduralParamsComparerDefault()
{
}

CProceduralParamsComparerDefault::~CProceduralParamsComparerDefault()
{
}

CRYREGISTER_CLASS(CProceduralParamsComparerDefault);

