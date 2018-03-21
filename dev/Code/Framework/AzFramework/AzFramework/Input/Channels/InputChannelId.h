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

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/hash.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that identifies a specific input channel
    class InputChannelId
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelId, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_TYPE_INFO(InputChannelId, "{7004B466-F6B4-41EF-AFFD-96A456121271}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] name Name of the input channel
        explicit InputChannelId(const char* name = "");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying
        AZ_DEFAULT_COPY(InputChannelId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelId() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's name
        //! \return Name of the input channel
        const char* GetName() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the crc32 of the input channel's name
        //! \return crc32 of the input channel name
        const AZ::Crc32& GetNameCrc32() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Equality comparison operator
        //! \param[in] other Another instance of the class to compare for equality
        bool operator==(const InputChannelId& other) const;
        bool operator!=(const InputChannelId& other) const;
        ///@}

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        const char* m_name;  //!< Name of the input channel
        AZ::Crc32   m_crc32; //!< Crc32 of the input channel
    };
} // namespace AzFramework

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZStd
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Hash structure specialization for InputChannelId
    template<> struct hash<AzFramework::InputChannelId>
    {
        inline size_t operator()(const AzFramework::InputChannelId& inputChannelId) const
        {
            return inputChannelId.GetNameCrc32();
        }
    };
} // namespace AZStd
