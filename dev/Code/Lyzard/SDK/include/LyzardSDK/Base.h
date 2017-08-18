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

#include <AzCore/Outcome/Outcome.h>

#include <AzCore/std/string/string.h>


namespace Lyzard
{
    //! Common return type for operations that can fail.
    using StringOutcome = AZ::Outcome<void, AZStd::string>;

    /**
     * Shorthand for checking a condition, and failing if false.
     * Works with any function that returns AZ::Outcome<..., AZStd::string>.
     * Unlike assert, it is not removed in release builds.
     * Ensure all strings are passed with c_str(), as they are passed to AZStd::string::format().
     */
    #define LMBR_ENSURE(cond, ...) if (!(cond)) { return AZ::Failure(AZStd::string::format(__VA_ARGS__)); }

    // Similar to above macro, but ensures on an AZ::Outcome. Not removed in release builds.
    #define LMBR_ENSURE_OUTCOME(outcome) if (!(outcome.IsSuccess())) { return AZ::Failure(outcome.GetError()); }
}
