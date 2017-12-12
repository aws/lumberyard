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
#include "BaseObject.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    class EMFX_API AnimGraphParameterGroup
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphParameterGroup, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH);

    public:
        /**
         * Constructor.
         * @param[in] name The name of the group.
         */
        static AnimGraphParameterGroup* Create(const char* name);

        /**
         * Set the name of the group. Please keep in mind that group names must be unique inside the anim graph objects. So you should not have two or more groups with the same name.
         * @param name The name of the group.
         */
        void SetName(const char* name);

        /**
         * Get the name of the group as null terminated character buffer.
         * @result The name of the group.
         */
        const char* GetName() const;

        /**
         * Get the name of the group, in form of a AZStd::string object.
         * @result The name as a reference to a AZStd::string object.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Add a given parameter to this group.
         * @param parameterIndex The parameter index to add.
         */
        void AddParameter(uint32 parameterIndex);

        /**
         * Set the number of parameters that remain inside this group.
         * This will resize the array of parameter indices. Don't forget to initialize the parameter index values after increasing the number of parameters.
         * @param numParameters The number of parameters that are inside this group.
         */
        void SetNumParameters(uint32 numParameters);

        /**
         * Set the value of a given parameter.
         * @param index The index in range [0, GetNumParameters()] of the parameter to set.
         * @param parameterIndex The parameter index to set at the given position in range [0, AnimGraph::GetNumParameters()].
         */
        void SetParameter(uint32 index, uint32 parameterIndex);

        /**
         * Get the number of parameters that remain inside this group.
         * @result The number of parameters inside this group.
         */
        uint32 GetNumParameters() const;
        size_t GetParameterCount() const;

        /**
         * Get the given parameter index which is part of this group.
         * @param[in] index The index in range [0, GetNumParameters()] of the parameter index to return.
         * @return The parameter index which can be used to retrieve the parameter from the anim graph and is in range [0, AnimGraph::GetNumParameters()].
         */
        uint32 GetParameter(uint32 index) const;

        //! Find the group local index for a parameter based on the global parameter index within the anim graph.
        //! @param[in] parameterindex The global parameter index of the anim graph in range [0, AnimGraph::GetNumParameters()].
        //! @result The local index in the parameter group [0, AnimGraphParameterGroup::GetParameterCount()]. MCORE_INVALIDINDEX32 in case the parameter is not part of the group.
        AZ::u32 FindLocalParameterIndex(uint32 parameterIndex) const;

        /**
         * Check if the given parameter is inside the node group.
         * @param parameterIndex The parameter index to check.
         * @result True in case the parameter is inside the group, false if not.
         */
        bool Contains(uint32 parameterIndex) const;

        /**
         * Get access the parameter index array of this group.
         * @return A constant reference to the index array.
         */
        const AZStd::vector<uint32>& GetParameterArray() const;

        /**
         * Remove a given parameter.
         * @param parameterIndex The parameter index to remove in range [0, AnimGraph::GetNumParameters()].
         */
        void RemoveParameter(uint32 parameterIndex);

        /**
         * Remove all parameters from this group.
         */
        void RemoveAllParameters();

        /**
        * Set the expanded/collapsed flag used when visualizing the parameter groups in an interface.
        * @param[in] isCollapsed Indicates if the parameters inside the group are visible and if the group is expanded or collapsed.
        */
        void SetIsCollapsed(bool isCollapsed)               { m_isCollapsed = isCollapsed; }

        /**
        * Get the expanded/collapsed flag used when visualizing the parameter groups in an interface.
        * @return Indicates if the parameters inside the group are visible and if the group is expanded or collapsed.
        */
        bool GetIsCollapsed() const                         { return m_isCollapsed; }

        /**
         * Initialize the internal data from another parameter group, basically copying it.
         * @param other The parameter group to copy the data from.
         */
        void InitFrom(const AnimGraphParameterGroup& other);

    protected:
        AZStd::vector<uint32>   m_parameterIndices; //! The parameter index array.
        AZStd::string           m_name;             //! The name of the group.
        bool                    m_isCollapsed;      //! Used when visualizing the parameter groups in an interface. Indicates if the parameters inside the group are visible and if the group is expanded or collapsed.

        /**
         * Constructor.
         * @param[in] name The name of the group.
         */
        AnimGraphParameterGroup(const char* name);

        /**
         * Destructor.
         */
        ~AnimGraphParameterGroup();
    };
} // namespace EMotionFX