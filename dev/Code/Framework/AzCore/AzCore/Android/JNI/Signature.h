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

#include <AzCore/Debug/Trace.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>

#include "JNI.h"


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            ///@{
            //! Generates a fully qualified Java signature from n-number of parameters.
            //! \return String containing the fully qualified JNI signature
            template<typename Type>
            inline AZStd::string GetSignature(Type value)
            {
                AZ_Assert(false, "Unknown type call to JNI::GetSignature");
                return "";
            }

            template<typename Type, typename... Args>
            inline AZStd::string GetSignature(Type first, Args&&... parameters)
            {
                return GetSignature(first) + GetSignature(AZStd::forward<Args>(parameters)...);
            }
            ///@}

            ///@{
            //! Known type specializations for GetSignature
            //! \return String containing the type specific JNI signature
            inline AZStd::string GetSignature() { return ""; }

            template<> inline AZStd::string GetSignature(jboolean) { return "Z"; }
            template<> inline AZStd::string GetSignature(bool) { return "Z"; }
            template<> inline AZStd::string GetSignature(jbooleanArray) { return "[Z"; }

            template<> inline AZStd::string GetSignature(jbyte) { return "B"; }
            template<> inline AZStd::string GetSignature(char) { return "B"; }
            template<> inline AZStd::string GetSignature(jbyteArray) { return "[B"; }

            template<> inline AZStd::string GetSignature(jchar) { return "C"; }
            template<> inline AZStd::string GetSignature(jcharArray) { return "[C"; }

            template<> inline AZStd::string GetSignature(jshort) { return "S"; }
            template<> inline AZStd::string GetSignature(jshortArray) { return "[S"; }

            template<> inline AZStd::string GetSignature(jint) { return "I"; }
            template<> inline AZStd::string GetSignature(jintArray) { return "[I"; }

            template<> inline AZStd::string GetSignature(jlong) { return "J"; }
            template<> inline AZStd::string GetSignature(jlongArray) { return "[J"; }

            template<> inline AZStd::string GetSignature(jfloat) { return "F"; }
            template<> inline AZStd::string GetSignature(jfloatArray) { return "[F"; }

            template<> inline AZStd::string GetSignature(jdouble) { return "D"; }
            template<> inline AZStd::string GetSignature(jdoubleArray) { return "[D"; }

            template<> inline AZStd::string GetSignature(jstring) { return "Ljava/lang/String;"; }

            template<> inline AZStd::string GetSignature(jclass) { return "Ljava/lang/Class;"; }

            template<> AZStd::string GetSignature(jobject value);
            template<> AZStd::string GetSignature(jobjectArray value);
            ///@}
        } // namespace JNI
    } // namespace Android
} // namespace AZ

