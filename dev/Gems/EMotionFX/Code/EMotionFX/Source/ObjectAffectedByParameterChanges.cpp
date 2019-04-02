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

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>


namespace EMotionFX
{
    void ObjectAffectedByParameterChanges::SortAndRemoveDuplicates(AnimGraph* animGraph, AZStd::vector<AZStd::string>& parameterNames)
    {
        // get the list of value parameters. Then iterate over them and build another vector with the ones contained in ports
        // This way we sort them based in value parameter and remove port duplicates
        AZStd::vector<AZStd::string> sortedPorts;
        const ValueParameterVector& valueParameters = animGraph->RecursivelyGetValueParameters();
        for (const ValueParameter* valueParameter : valueParameters)
        {
            if (AZStd::find(parameterNames.begin(), parameterNames.end(), valueParameter->GetName()) != parameterNames.end())
            {
                sortedPorts.push_back(valueParameter->GetName());
            }
        }
        parameterNames.swap(sortedPorts);
    }
}   // namespace EMotionFX
