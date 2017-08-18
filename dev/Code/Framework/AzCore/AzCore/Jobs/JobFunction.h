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
#ifndef AZCORE_JOBS_JOBFUNCTION_H
#define AZCORE_JOBS_JOBFUNCTION_H 1

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_cv.h>

namespace AZ
{
    /**
     * A job which uses a templated function (can be AZStd::function, AZStd::delegate, result of AZStd::bind, lambda, reference to a functor, regular function).
     */
    template<class Function>
    class JobFunction
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(JobFunction, ThreadPoolAllocator, 0)

        typedef const typename AZStd::remove_cv<typename AZStd::remove_reference<Function>::type>::type& FunctionCRef;

        JobFunction(FunctionCRef processFunction, bool isAutoDelete, JobContext* context)
            : Job(isAutoDelete, context)
            , m_function(processFunction)
        {
        }

        virtual void Process()
        {
            m_function();
        }

    protected:
        JobFunction(const JobFunction&);
        JobFunction& operator=(const JobFunction&);
        Function m_function;
    };

    /// Convenience function to create (aznew JobFunction with any function signature). You should delte the function with delete (or isAutoDelete set to true)
    template<class Function>
    inline JobFunction<Function>* CreateJobFunction(const Function& processFunction, bool isAutoDelete, JobContext* context = nullptr)
    {
        return aznew JobFunction<Function>(processFunction, isAutoDelete, context);
    }

    /// For delete symmetry if you care.
    inline void DeleteJobFunction(Job* jobFunction)
    {
        delete jobFunction;
    }
}

#endif
#pragma once