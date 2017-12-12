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

#include <aws/core/client/AsyncCallerContext.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <utility>

namespace LmbrAWS
{
    template<typename EXTRA_TYPE>
    class WithExtra
    {
    public:
        WithExtra(EXTRA_TYPE extra)
            : m_extra(extra)
        {
        }
        virtual ~WithExtra()
        {
        }

        const EXTRA_TYPE& GetExtra() const { return m_extra; }

    private:

        EXTRA_TYPE m_extra;
    };

    template<typename EXTRA_TYPE>
    class AsyncContextWithExtra
        : public Aws::Client::AsyncCallerContext
        , public WithExtra<EXTRA_TYPE>
    {
    public:
        AsyncContextWithExtra(EXTRA_TYPE extra)
            : WithExtra<EXTRA_TYPE>(extra) { }
    };
} //namescape LmbrAWS