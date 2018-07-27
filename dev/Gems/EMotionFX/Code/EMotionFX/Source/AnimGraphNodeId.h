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

#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    /**
     * Anim graph node ID type.
     * Anim graph node IDs are used to uniquely identify nodes.
     */
    class AnimGraphNodeId
    {
    public:
        AZ_TYPE_INFO(AnimGraphNodeId, "{B7DCAC0C-0F48-4350-B169-0387C2602328}")
        AZ_CLASS_ALLOCATOR(AnimGraphNodeId, EMotionFX::AnimGraphAllocator, 0)

        static const AnimGraphNodeId InvalidId;

        AnimGraphNodeId()
            : m_id(InvalidId)
        {}

        AnimGraphNodeId(const AnimGraphNodeId& id)
            : m_id(id)
        {}

        AnimGraphNodeId(AZ::u64 id)
            : m_id(id)
        {}

        /**
         * Casts the id to u64. Needed for reflection.
         * @result The id as an 64-bit unsigned int.
         */
        operator AZ::u64() const
        {
            return m_id;
        }

        /**
         * Determines whether this id is valid.
         * An id is invalid if you did not provide an argument to the constructor.
         * @result Returns true if the id is valid. Otherwise, false.
         */
        bool IsValid() const
        {
            return m_id != InvalidId;
        }

        /**
         * Set the id to the invalid value.
         */
        void SetInvalid()
        {
            m_id = InvalidId;
        }

        /**
         * Convert the id to a string and return it.
         */
        AZStd::string ToString() const;

        /**
         * Convert the given string to an id.
         * @param[in] text The string to parse and convert.
         * @result The resulting id. The id will be invalid in case the text could not be parsed correctly.
         */
        static AnimGraphNodeId FromString(const AZStd::string& text);

        /**
         * Compare two ids for equality.
         * @param rhs Id to compare against.
         * @return True if the ids are equal. Otherwise, false.
         */
        bool operator==(const AnimGraphNodeId& rhs) const
        {
            return m_id == rhs.m_id;
        }

        /**
         * Compare two ids for inequality.
         * @param rhs Id to compare against.
         * @return True if the ids are different. Otherwise, false.
         */
        bool operator!=(const AnimGraphNodeId& rhs) const
        {
            return m_id != rhs.m_id;
        }

        /**
         * Generate a random and valid id.
         * @result A randomized, valid id.
         */
        static AnimGraphNodeId Create();

    protected:
        AZ::u64 m_id;
    };
} // namespace EMotionFX