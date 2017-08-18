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

#include <AzCore/std/typetraits/has_member_function.h>

namespace AZ
{
    class ReflectContext;

    /// Function type to called for on demand reflection within methods, properties, etc.
    typedef void(*OnDemandReflectionFunction)(ReflectContext* context);

    AZ_HAS_MEMBER(OnDemandReflection, NoSpecializationFunction, void, ())

    /**
     *  This is the default implementation for OnDamandReflection (no reflection). You can specialize this template class
     * for the type you want to reflect on demand.
     * \note Currently only \ref BehaviorContext support on demand reflection.
     */ 
    template<class T>
    struct OnDemandReflection
    {
        void NoSpecializationFunction();
    };

    namespace Internal
    {
        template<class T, bool IsNoSpecialization = HasOnDemandReflection<OnDemandReflection<T>>::value>
        struct OnDemandReflectHook
        {
            static OnDemandReflectionFunction Get()
            {
                return nullptr;
            }
        };

        template<class T>
        struct OnDemandReflectHook<T, false>
        {
            static OnDemandReflectionFunction Get()
            {
                return &OnDemandReflection<T>::Reflect;
            }
        };
    }
} // namespace AZ

