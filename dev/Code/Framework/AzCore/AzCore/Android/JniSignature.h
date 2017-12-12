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

#include <AzCore/Android/JNI/Signature.h>

namespace AZ
{
    namespace Android
    {
        namespace Jni
        {
            ///@{
            //! \deprecated Please use the version of this function within the new AZ::Android::JNI namespace.
            //! \brief Generates a fully qualified Java signature from n-number of parameters.
            //! \return String containing the fully qualified JNI signature
            [[deprecated("Please use the version of this function within the new AZ::Android::JNI namespace")]]
            template<typename Type>
            inline AZStd::string GetSignature(Type param)
            {
                return JNI::GetSignature(param);
            }

            [[deprecated("Please use the version of this function within the new AZ::Android::JNI namespace")]]
            template<typename Type, typename... Args>
            inline AZStd::string GetSignature(Type first, Args&&... parameters)
            {
                return JNI::GetSignature(first, AZStd::forward<Args>(parameters)...);
            }
            ///@}
        }
    }
}