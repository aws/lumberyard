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
#ifndef AZSTD_TYPE_TRAITS_REMOVE_CV_INCLUDED
#define AZSTD_TYPE_TRAITS_REMOVE_CV_INCLUDED

#include <AzCore/std/typetraits/internal/cv_traits_impl.h>

namespace AZStd
{
    using std::remove_cv;
    template<class T>
    using remove_cv_t = std::remove_cv_t<T>;
}

#endif // AZSTD_TYPE_TRAITS_REMOVE_CV_INCLUDED
#pragma once
