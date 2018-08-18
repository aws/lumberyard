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
#include "CryLegacy_precompiled.h"
#include <Tests/FlowNodeTestHarness.h>

inline void PrintTo(const SFlowAddress& value, std::ostream* os)
{
    *os << "SFlowAddress(port=" << (int)value.port << ", node=" << value.node << ", isOutput=" << value.isOutput << ")";
}
