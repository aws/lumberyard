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

#include "Signature.h"

namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            template<>
            AZStd::string GetSignature(jobject value)
            {
                AZStd::string signature("");

                AZ_Error("JniSignature", value, "Call to GetSignature with null jobject");
                if (value)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Failed to get JNIEnv* on thread for get signature call");

                    if (jniEnv)
                    {
                        jclass objectClass = jniEnv->GetObjectClass(value);
                        signature = "L" + GetClassName(objectClass) + ";";
                        jniEnv->DeleteLocalRef(objectClass);

                        AZStd::replace(signature.begin(), signature.end(), '.', '/');
                    }
                }
                return signature;
            }

            template<>
            AZStd::string GetSignature(jobjectArray value)
            {
                AZStd::string signature("");

                AZ_Error("JniSignature", value, "Call to GetSignature with null jobjectArray");
                if (value)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Failed to get JNIEnv* on thread for get signature call");

                    if (jniEnv)
                    {
                        jobject element = jniEnv->GetObjectArrayElement(value, 0);
                        if (!element || jniEnv->ExceptionCheck())
                        {
                            AZ_Error("JniSignature", false, "Unable to determine jobject array type");
                            HANDLE_JNI_EXCEPTION(jniEnv);
                        }
                        else
                        {
                            signature = "[" + GetSignature(element);
                            jniEnv->DeleteLocalRef(element);
                        }
                    }
                }
                return signature;
            }
        } // namespace JNI
    } // namespace Android
} // namespace AZ