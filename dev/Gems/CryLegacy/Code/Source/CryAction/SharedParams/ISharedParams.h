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

// Description : Shared parameters type information.


#ifndef CRYINCLUDE_CRYACTION_SHAREDPARAMS_ISHAREDPARAMS_H
#define CRYINCLUDE_CRYACTION_SHAREDPARAMS_ISHAREDPARAMS_H
#pragma once

#include "ISharedParamsManager.h"
#include "SharedParamsTypeInfo.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared parameters interface class.
////////////////////////////////////////////////////////////////////////////////////////////////////
class ISharedParams
{
public:

    virtual ~ISharedParams()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Clone.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ISharedParams* Clone() const = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Get type information.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const CSharedParamsTypeInfo& GetTypeInfo() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Begin shared parameters.
////////////////////////////////////////////////////////////////////////////////////////////////////
#define BEGIN_SHARED_PARAMS(name)                                \
    struct name;                                                 \
                                                                 \
    DECLARE_SMART_POINTERS(name);                                \
                                                                 \
    struct name                                                  \
        : public ISharedParams                                   \
    {                                                            \
        virtual ISharedParams* Clone() const                     \
        {                                                        \
            return new name(*this);                              \
        }                                                        \
                                                                 \
        virtual const CSharedParamsTypeInfo& GetTypeInfo() const \
        {                                                        \
            return s_typeInfo;                                   \
        }                                                        \
                                                                 \
        static const CSharedParamsTypeInfo s_typeInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////
// End shared parameters.
////////////////////////////////////////////////////////////////////////////////////////////////////
#define END_SHARED_PARAMS };

////////////////////////////////////////////////////////////////////////////////////////////////////
// Define shared parameters type information.
////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEFINE_SHARED_PARAMS_TYPE_INFO(name) const CSharedParamsTypeInfo name::s_typeInfo(sizeof(name), #name, __FILE__, __LINE__);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Cast shared parameters pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TO>
inline AZStd::shared_ptr<TO> CastSharedParamsPtr(ISharedParamsPtr pSharedParams)
{
    if (pSharedParams && (pSharedParams->GetTypeInfo() == TO::s_typeInfo))
    {
        return AZStd::static_pointer_cast<TO>(pSharedParams);
    }
    else
    {
        return AZStd::shared_ptr<TO>();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Cast shared parameters pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TO>
inline AZStd::shared_ptr<const TO> CastSharedParamsPtr(ISharedParamsConstPtr pSharedParams)
{
    if (pSharedParams && (pSharedParams->GetTypeInfo() == TO::s_typeInfo))
    {
        return AZStd::static_pointer_cast<const TO>(pSharedParams);
    }
    else
    {
        return AZStd::shared_ptr<const TO>();
    }
};

#endif // CRYINCLUDE_CRYACTION_SHAREDPARAMS_ISHAREDPARAMS_H
