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
#include <AzCore/Debug/Trace.h>
#include <AzCore/Android/JNI/Internal/ClassName.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                template<typename StringType>
                StringType GetTypeSignature(jobject value)
                {
                    StringType signature("");

                    AZ_Error("JNI::Signature", value, "Call to ConstructSignature with null jobject");
                    if (value)
                    {
                        JNIEnv* jniEnv = GetEnv();
                        AZ_Assert(jniEnv, "Failed to get JNIEnv* on thread for get signature call");

                        if (jniEnv)
                        {
                            jclass objectClass = jniEnv->GetObjectClass(value);

                            StringType typeSig = ClassName<StringType>::GetName(objectClass);
                            signature.reserve(typeSig.size() + 3);

                            signature.append("L");
                            signature.append(typeSig);
                            signature.append(";");

                            jniEnv->DeleteLocalRef(objectClass);

                            AZStd::replace(signature.begin(), signature.end(), '.', '/');
                        }
                    }
                    return signature;
                }

                template<typename StringType>
                StringType GetTypeSignature(jobjectArray value)
                {
                    StringType signature("");

                    AZ_Error("JNI::Signature", value, "Call to ConstructSignature with null jobjectArray");
                    if (value)
                    {
                        JNIEnv* jniEnv = GetEnv();
                        AZ_Assert(jniEnv, "Failed to get JNIEnv* on thread for get signature call");

                        if (jniEnv)
                        {
                            jobject element = jniEnv->GetObjectArrayElement(value, 0);
                            if (!element || jniEnv->ExceptionCheck())
                            {
                                AZ_Error("JNI::Signature", false, "Unable to determine jobject array type");
                                HANDLE_JNI_EXCEPTION(jniEnv);
                            }
                            else
                            {
                                signature.append("[");
                                signature.append(GetTypeSignature(element));

                                jniEnv->DeleteLocalRef(element);
                            }
                        }
                    }
                    return signature;
                }
            } // namespace Internal
        } // namespace JNI
    } // namespace Android
} // namespace AZ
