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
#include <AzCore/std/typetraits/is_same.h>


#if defined(JNI_SIGNATURE_VALIDATION)
    #include "Signature.h"
#endif


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            // ----
            // Object (public)
            // ----

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            bool Object::CreateInstance(const char* constructorSignature, Args&&... parameters)
            {
                if (m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The Instance has already been created");
                    return false;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for constructor call on class %s", m_className.c_str());
                    return false;
                }

                // get the constructor method
                jmethodID constructorMethodId = jniEnv->GetMethodID(m_classRef, "<init>", constructorSignature);
                if (!constructorMethodId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get the constructor for class %s with signature %s", m_className.c_str(), constructorSignature);
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = "(" + GetSignature(AZStd::forward<Args>(parameters)...) + ")V";
                if (signature.compare(constructorSignature) != 0)
                {
                    AZ_Error("JNI::Object", false, "Invalid constructor signature supplied for class %s.  Expecting %s, was given %s", m_className.c_str(), constructorSignature, signature.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }
            #endif

                jobject localObjectRef = jniEnv->NewObject(m_classRef, constructorMethodId, AZStd::forward<Args>(parameters)...);
                if (!localObjectRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to construct a local object of class %s", m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localObjectRef);
                    return false;
                }

                m_objectRef = jniEnv->NewGlobalRef(localObjectRef);
                if (!m_objectRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to construct a global object of class %s", m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localObjectRef);
                    return false;
                }

                jniEnv->DeleteLocalRef(localObjectRef);
                m_instanceConstructed = true;

                return true;
            }


            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            void Object::InvokeVoidMethod(const char* methodName, Args&&... parameters)
            {
                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance method call to %s failed for class %s", methodName, m_className.c_str());
                    return;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for method call %s on class %s", methodName, m_className.c_str());
                    return;
                }

                JMethodCachePtr methodCache = GetMethod(methodName);
                if (!methodCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string argumentSignature = GetSignature(AZStd::forward<Args>(parameters)...);
                if (!ValidateSignature(methodCache, argumentSignature, "V"))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate method signature for %s on class %s", methodName, m_className.c_str());
                    return;
                }
            #endif

                jniEnv->CallVoidMethod(m_objectRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jboolean Object::InvokeBooleanMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jboolean>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallBooleanMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jbyte Object::InvokeByteMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jbyte>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallByteMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jchar Object::InvokeCharMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jchar>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallCharMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jshort Object::InvokeShortMethod(const char *methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jshort>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallShortMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename... Args>
            jint Object::InvokeIntMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jint>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallIntMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jlong Object::InvokeLongMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jlong>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallLongMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jfloat Object::InvokeFloatMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jfloat>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallFloatMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);        }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jdouble Object::InvokeDoubleMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeMethodInternal<jdouble>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jobject objectRef, jmethodID methodId)
                    {
                        return jniEnv->CallDoubleMethod(objectRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            AZStd::string Object::InvokeStringMethod(const char* methodName, Args&&... parameters)
            {
                jstring rawStringValue = InvokeObjectMethod<jstring>(methodName, AZStd::forward<Args>(parameters)...);
                AZStd::string returnValue = ConvertJstringToString(rawStringValue);
                DeleteRef(rawStringValue);
                return returnValue;
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object::InvokeObjectMethod(const char* methodName, Args&&... parameters)
            {
                ReturnType defaultValue = nullptr;

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The Java object instance has not been created yet.  Instance method call to %s failed for class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string argumentSignature = GetSignature(AZStd::forward<Args>(parameters)...);

                AZStd::string returnSignature;
                if (AZStd::is_same<ReturnType, jobject>::value)
                {
                    returnSignature = "L";
                }
                else if (AZStd::is_same<ReturnType, jobjectArray>::value)
                {
                    returnSignature = "[L";
                }
                else
                {
                    returnSignature = GetSignature(defaultValue);
                }

                if (!ValidateSignature(methodCache, argumentSignature, returnSignature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate method signature for %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                jobject localRef = jniEnv->CallObjectMethod(m_objectRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (!localRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject globalRef = jniEnv->NewGlobalRef(localRef);
                if (!globalRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a return global ref to method call %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localRef);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localRef);
                return static_cast<ReturnType>(globalRef);
            }


            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            void Object::InvokeStaticVoidMethod(const char* methodName, Args&&... parameters)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static method call %s on class %s", methodName, m_className.c_str());
                    return;
                }

                JMethodCachePtr methodCache = GetStaticMethod(methodName);
                if (!methodCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string argumentSignature = GetSignature(AZStd::forward<Args>(parameters)...);
                if (!ValidateSignature(methodCache, argumentSignature, "V"))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate static method signature for %s on class %s", methodName, m_className.c_str());
                    return;
                }
            #endif

                jniEnv->CallStaticVoidMethod(m_classRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke static method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jboolean Object::InvokeStaticBooleanMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jboolean>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticBooleanMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jbyte Object::InvokeStaticByteMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jbyte>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticByteMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jchar Object::InvokeStaticCharMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jchar>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticCharMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jshort Object::InvokeStaticShortMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jshort>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticShortMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename... Args>
            jint Object::InvokeStaticIntMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jint>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticIntMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jlong Object::InvokeStaticLongMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jlong>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticLongMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jfloat Object::InvokeStaticFloatMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jfloat>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticFloatMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            jdouble Object::InvokeStaticDoubleMethod(const char* methodName, Args&&... parameters)
            {
                return InvokePrimitiveTypeStaticMethodInternal<jdouble>(
                    methodName,
                    [&parameters...](JNIEnv* jniEnv, jclass classRef, jmethodID methodId)
                    {
                        return jniEnv->CallStaticDoubleMethod(classRef, methodId, parameters...);
                    },
                    AZStd::forward<Args>(parameters)...);
            }

            ////////////////////////////////////////////////////////////////
            template<typename ...Args>
            AZStd::string Object::InvokeStaticStringMethod(const char* methodName, Args&&... parameters)
            {
                jstring rawStringValue = InvokeStaticObjectMethod<jstring>(methodName, AZStd::forward<Args>(parameters)...);
                AZStd::string returnValue = ConvertJstringToString(rawStringValue);
                DeleteRef(rawStringValue);
                return returnValue;
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object::InvokeStaticObjectMethod(const char* methodName, Args&&... parameters)
            {
                const ReturnType defaultValue = nullptr;

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetStaticMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string argumentSignature = GetSignature(AZStd::forward<Args>(parameters)...);

                AZStd::string returnSignature;
                if (AZStd::is_same<ReturnType, jobject>::value)
                {
                    returnSignature = "L";
                }
                else if (AZStd::is_same<ReturnType, jobjectArray>::value)
                {
                    returnSignature = "[L";
                }
                else
                {
                    returnSignature = GetSignature(defaultValue);
                }

                if (!ValidateSignature(methodCache, argumentSignature, returnSignature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate method signature for %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                jobject localRef = jniEnv->CallStaticObjectMethod(m_classRef, methodCache->m_methodId, AZStd::forward<Args>(parameters)...);
                if (!localRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke static method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject globalRef = jniEnv->NewGlobalRef(localRef);
                if (!globalRef || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a return global ref to method call %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localRef);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localRef);
                return static_cast<ReturnType>(globalRef);
            }


            ////////////////////////////////////////////////////////////////
            template<typename ValueType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ValueType, jobject>::value>::type
            Object::SetObjectField(const char* fieldName, ValueType value)
            {
                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The Java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = GetSignature(value);

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate field signature for %s on class %s", fieldName, m_className.c_str());
                    return;
                }
            #endif

                jniEnv->SetObjectField(m_objectRef, fieldCache->m_fieldId, value);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object::GetObjectField(const char* fieldName)
            {
                const ReturnType defaultValue(nullptr);

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature;
                if (AZStd::is_same<ReturnType, jobject>::value)
                {
                    signature = "L";
                }
                else if (AZStd::is_same<ReturnType, jobjectArray>::value)
                {
                    signature = "[L";
                }
                else
                {
                    signature = GetSignature(defaultValue);
                }

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate field signature for %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                jobject localValue = jniEnv->GetObjectField(m_objectRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject returnValue = jniEnv->NewGlobalRef(localValue);
                if (!returnValue || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a global ref from field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localValue);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localValue);

                return static_cast<ReturnType>(returnValue);
            }


            ////////////////////////////////////////////////////////////////
            template<typename ValueType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ValueType, jobject>::value>::type
            Object::SetStaticObjectField(const char* fieldName, ValueType value)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = GetSignature(value);

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate static field signature for %s on class %s", fieldName, m_className.c_str());
                    return;
                }
            #endif

                jniEnv->SetObjectField(m_objectRef, fieldCache->m_fieldId, value);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType, typename ...Args>
            typename AZStd::enable_if<AZStd::is_convertible<ReturnType, jobject>::value, ReturnType>::type
            Object::GetStaticObjectField(const char* fieldName)
            {
                const ReturnType defaultValue(nullptr);

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature;
                if (AZStd::is_same<ReturnType, jobject>::value)
                {
                    signature = "L";
                }
                else if (AZStd::is_same<ReturnType, jobjectArray>::value)
                {
                    signature = "[L";
                }
                else
                {
                    signature = GetSignature(defaultValue);
                }

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate static field signature for %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                jobject localValue = jniEnv->GetStaticObjectField(m_classRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                jobject returnValue = jniEnv->NewGlobalRef(localValue);
                if (!returnValue || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to create a global ref from static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    jniEnv->DeleteLocalRef(localValue);
                    return defaultValue;
                }

                jniEnv->DeleteLocalRef(localValue);

                return static_cast<ReturnType>(returnValue);
            }


            // ----
            // Object (private)
            // ----

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType, typename... Args>
            ReturnType Object::InvokePrimitiveTypeMethodInternal(
                const char* methodName,
                AZStd::function<ReturnType(JNIEnv*, jobject, jmethodID)> jniCallback,
                Args&&... parameters)
            {
                const ReturnType defaultValue(0);

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance method call to %s failed for class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string argumentSignature = GetSignature(AZStd::forward<Args>(parameters)...);
                AZStd::string returnSignature = GetSignature(defaultValue);

                if (!ValidateSignature(methodCache, argumentSignature, returnSignature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate method signature for %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_objectRef, methodCache->m_methodId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType, typename... Args>
            ReturnType Object::InvokePrimitiveTypeStaticMethodInternal(
                const char* methodName,
                AZStd::function<ReturnType(JNIEnv*, jclass, jmethodID)> jniCallback,
                Args&&... parameters)
            {
                const ReturnType defaultValue(0);

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static method call %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }

                JMethodCachePtr methodCache = GetStaticMethod(methodName);
                if (!methodCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string argumentSignature = GetSignature(AZStd::forward<Args>(parameters)...);
                AZStd::string returnSignature = GetSignature(defaultValue);

                if (!ValidateSignature(methodCache, argumentSignature, returnSignature.c_str()))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate static method signature for %s on class %s", methodName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_classRef, methodCache->m_methodId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to invoke static method %s on class %s", methodName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }


            ////////////////////////////////////////////////////////////////
            template<typename ValueType>
            void Object::SetPrimitiveTypeFieldInternal(
                const char* fieldName,
                AZStd::function<void(JNIEnv*, jobject, jfieldID)> jniCallback,
                ValueType value)
            {
                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = GetSignature(value);

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate field signature for %s on class %s", fieldName, m_className.c_str());
                    return;
                }
            #endif

                jniCallback(jniEnv, m_objectRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType>
            ReturnType Object::GetPrimitiveTypeFieldInternal(
                const char* fieldName,
                AZStd::function<ReturnType(JNIEnv*, jobject, jfieldID)> jniCallback)
            {
                const ReturnType defaultValue(0);

                if (!m_instanceConstructed)
                {
                    AZ_Warning("JNI::Object", false, "The java object instance has not been created yet.  Instance field call to %s failed for class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = GetSignature(defaultValue);

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate field signature for %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_objectRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }


            ////////////////////////////////////////////////////////////////
            template<typename ValueType>
            void Object::SetPrimitiveTypeStaticFieldInternal(
                const char* fieldName,
                AZStd::function<void(JNIEnv*, jclass, jfieldID)> jniCallback,
                ValueType value)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for static field call %s on class %s", fieldName, m_className.c_str());
                    return;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = GetSignature(value);

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate static field signature for %s on class %s", fieldName, m_className.c_str());
                    return;
                }
            #endif

                jniCallback(jniEnv, m_classRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to set static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                }
            }

            ////////////////////////////////////////////////////////////////
            template<typename ReturnType>
            ReturnType Object::GetPrimitiveTypeStaticFieldInternal(
                const char* fieldName,
                AZStd::function<ReturnType(JNIEnv*, jclass, jfieldID)> jniCallback)
            {
                const ReturnType defaultValue(0);

                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread for field call %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }

                JFieldCachePtr fieldCache = GetStaticField(fieldName);
                if (!fieldCache)
                {
                    return defaultValue;
                }

            #if defined(JNI_SIGNATURE_VALIDATION)
                AZStd::string signature = GetSignature(defaultValue);

                if (!ValidateSignature(fieldCache, signature))
                {
                    AZ_Error("JNI::Object", false, "Failed to validate field signature for %s on class %s", fieldName, m_className.c_str());
                    return defaultValue;
                }
            #endif

                ReturnType value = jniCallback(jniEnv, m_classRef, fieldCache->m_fieldId);
                if (jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to get static field %s on class %s", fieldName, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return defaultValue;
                }

                return value;
            }
        }
    }
}
