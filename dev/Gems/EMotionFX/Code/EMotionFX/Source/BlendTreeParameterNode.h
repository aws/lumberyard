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

#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     *
     */
    class EMFX_API BlendTreeParameterNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeParameterNode, "{4510529A-323F-40F6-B773-9FA8FC4DE53D}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        BlendTreeParameterNode();
        ~BlendTreeParameterNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;
        
        uint32 GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const AZStd::vector<AZ::u32>& GetParameterIndices() const;
        uint32 GetParameterIndex(uint32 portNr) const;

        /**
         * Find the port index for the given value parameter index.
         * @param[in] valueParameterIndex The index of the value parameter in the range [0, AnimGraph::GetNumValueParameters()].
         * @result The port index for the given parameter. In case a parameter mask is specified, the port number equals the value parameter index.
         *         In case a parameter mask is specified and the parameter is part of the mask, the parameter's local mask index equals the port and is returned.
         *         In case a parameter mask is specified while the parameter is not in the mask, MCORE_INVALIDINDEX32 is returned.
         */
        size_t FindPortForParameterIndex(size_t valueParameterIndex) const;

        size_t CalcNewPortIndexForParameter(const AZStd::string& parameterName, const AZStd::vector<AZStd::string>& parametersToBeRemoved) const;

        /// Add a parameter to the parameter mask and also add a port for it.
        void AddParameter(const AZStd::string& parameterName);

        /// Set the parameter mask and create ports for each of them. (An empty parameter list means that all parameters are shown).
        void SetParameters(const AZStd::string& parameterNamesWithSemicolons);
        void SetParameters(const AZStd::vector<AZStd::string>& parameterNames);

        const AZStd::vector<AZStd::string>& GetParameters() const;

        /// Construct a string containing all parameter names separated by semicolons.
        AZStd::string ConstructParameterNamesString() const;
        static AZStd::string ConstructParameterNamesString(const AZStd::vector<AZStd::string>& parameterNames);
        static AZStd::string ConstructParameterNamesString(const AZStd::vector<AZStd::string>& parameterNames, const AZStd::vector<AZStd::string>& excludedParameterNames);

        /// Remove the given parameter by name. This removes the parameter from the parameter mask and also deletes the port.
        void RemoveParameterByName(const AZStd::string& parameterName);

        /// Renames the given currentName parameter to newName
        void RenameParameterName(const AZStd::string& currentName, const AZStd::string& newName);

        /// Sort the parameter names based on the order of the parameters in the anim graph.
        static void SortParameterNames(AnimGraph* animGraph, AZStd::vector<AZStd::string>& outParameterNames);

        /// Get the names of the parameters whose ports are connected.
        void CalcConnectedParameterNames(AZStd::vector<AZStd::string>& outParameterNames);

        static void Reflect(AZ::ReflectContext* context);

    private:
        bool GetTypeSupportsFloat(uint32 parameterType);

        AZStd::vector<AZStd::string>    m_parameterNames;
        AZStd::vector<AZ::u32>          m_parameterIndices;              /**< The indices of the visible and available parameters. */

        bool CheckIfParameterIndexUpdateNeeded() const;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
} // namespace EMotionFX