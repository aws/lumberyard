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
#include "JNI.h"

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Debug/Trace.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            ////////////////////////////////////////////////////////////////
            JNIEnv* GetEnv()
            {
                return AndroidEnv::Get()->GetJniEnv();
            }

            ////////////////////////////////////////////////////////////////
            jclass LoadClass(const char* classPath)
            {
                return AndroidEnv::Get()->LoadClass(classPath);
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string GetClassName(jclass classRef)
            {
                return AndroidEnv::Get()->GetClassName(classRef);
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string GetSimpleClassName(jclass classRef)
            {
                return AndroidEnv::Get()->GetSimpleClassName(classRef);
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string ConvertJstringToString(jstring stringValue)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("AZ::Android::JNI", false, "Failed to get JNIEnv* on thread for jstring conversion");
                    return AZStd::string();
                }

                const char* convertedStringValue = jniEnv->GetStringUTFChars(stringValue, nullptr);
                if (!convertedStringValue || jniEnv->ExceptionCheck())
                {
                    AZ_Error("AZ::Android::JNI", false, "Failed to convert a jstring to cstring");
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return AZStd::string();
                }

                AZStd::string localCopy(convertedStringValue);
                jniEnv->ReleaseStringUTFChars(stringValue, convertedStringValue);

                return localCopy;
            }

            ////////////////////////////////////////////////////////////////
            jstring ConvertStringToJstring(const AZStd::string& stringValue)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("AZ::Android::JNI", false, "Failed to get JNIEnv* on thread for jstring conversion");
                    return nullptr;
                }

                jstring localRef = jniEnv->NewStringUTF(stringValue.c_str());
                if (!localRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("AZ::Android::JNI", false, "Failed to convert the cstring to jstring");
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return nullptr;
                }

                jstring globalRef = static_cast<jstring>(jniEnv->NewGlobalRef(localRef));
                if (!globalRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("AZ::Android::JNI", false, "Failed to create a global reference to the return jstring");
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localRef);
                    return nullptr;
                }

                jniEnv->DeleteLocalRef(localRef);

                return globalRef;
            }

            ////////////////////////////////////////////////////////////////
            int GetRefType(jobject javaRef)
            {
                int refType = JNIInvalidRefType;

                if (javaRef)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Unable to get JNIEnv pointer to determine JNI reference type.");

                    if (jniEnv)
                    {
                        refType = jniEnv->GetObjectRefType(javaRef);
                    }
                }

                return refType;
            }

            ////////////////////////////////////////////////////////////////
            void DeleteRef(jobject javaRef)
            {
                if (javaRef)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Unable to get JNIEnv pointer to free JNI reference.");

                    if (jniEnv)
                    {
                        jobjectRefType refType = jniEnv->GetObjectRefType(javaRef);
                        switch (refType)
                        {
                            case JNIGlobalRefType:
                                jniEnv->DeleteGlobalRef(javaRef);
                                break;
                            case JNILocalRefType:
                                jniEnv->DeleteLocalRef(javaRef);
                                break;
                            case JNIWeakGlobalRefType:
                                jniEnv->DeleteWeakGlobalRef(javaRef);
                                break;
                            default:
                                AZ_Error("AZ::Android::JNI", false, "Unknown or invalid reference type detected.");
                                break;
                        }
                    }
                }
            }

            ////////////////////////////////////////////////////////////////
            void DeleteGlobalRef(jobject globalRef)
            {
                if (globalRef)
                {
                    JNIEnv* jniEnv = GetEnv();
                    AZ_Assert(jniEnv, "Unable to get JNIEnv pointer to free JNI reference.");

                    if (jniEnv)
                    {
                        jobjectRefType refType = jniEnv->GetObjectRefType(globalRef);
                        AZ_Assert(refType == JNIGlobalRefType,
                                "Call to DeleteGlobalRef with a non-global ref.  JNI reference will not be released properly and may leak.");

                        if (refType == JNIGlobalRefType)
                        {
                            jniEnv->DeleteGlobalRef(globalRef);
                        }
                    }
                }
            }
        }
    }
}
