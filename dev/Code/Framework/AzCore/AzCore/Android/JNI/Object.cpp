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
#include "Object.h"

#include <AzCore/std/smart_ptr/make_shared.h>


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
            Object::Object(const char* classPath, const char* className /* = nullptr */)
                : m_className()

                , m_classRef(nullptr)
                , m_objectRef(nullptr)

                , m_methods()
                , m_staticMethods()

                , m_fields()
                , m_staticFields()

                , m_ownsGlobalRefs(true)
                , m_instanceConstructed(false)
            {
                m_classRef = LoadClass(classPath);
                if (!className)
                {
                    AZStd::string classPathStr = classPath;
                    size_t last = classPathStr.find_last_of('/');
                    m_className = classPathStr.substr(last + 1, (classPathStr.length() - last) - 1);
                }
                else
                {
                    m_className = className;
                }
            }

            ////////////////////////////////////////////////////////////////
            Object::Object(jclass classRef, jobject objectRef, bool takeOwnership /* = false */)
                : m_className()

                , m_classRef(classRef)
                , m_objectRef(objectRef)

                , m_methods()
                , m_staticMethods()

                , m_fields()
                , m_staticFields()

                , m_ownsGlobalRefs(takeOwnership)
                , m_instanceConstructed(true)
            {
                m_className = GetSimpleClassName(classRef);
            }

            ////////////////////////////////////////////////////////////////
            Object::~Object()
            {
                if (m_ownsGlobalRefs)
                {
                    JNIEnv* jniEnv = GetEnv();
                    if (jniEnv)
                    {
                        jniEnv->DeleteGlobalRef(m_classRef);
                        jniEnv->DeleteGlobalRef(m_objectRef);
                    }
                    else
                    {
                        AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread object deletion on class %s", m_className.c_str());
                    }
                }

                m_methods.clear();
                m_staticMethods.clear();

                m_fields.clear();
                m_staticFields.clear();
            }

            ////////////////////////////////////////////////////////////////
            void Object::DestroyInstance()
            {
                if (!m_ownsGlobalRefs)
                {
                    AZ_Warning("JNI::Object", false, "This object does not have ownership of the global references.  Instance not destroyed");
                    return;
                }

                if (m_instanceConstructed)
                {
                    DeleteRef(m_objectRef);
                    m_objectRef = nullptr;

                    m_instanceConstructed = false;
                }
            }


            ////////////////////////////////////////////////////////////////
            bool Object::RegisterMethod(const char* methodName, const char* methodSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register method %s on class %s", methodName, m_className.c_str());
                    return false;
                }

                jmethodID methodId = jniEnv->GetMethodID(m_classRef, methodName, methodSignature);
                if (!methodId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to find method %s with signature %s in class %s", methodName, methodSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JMethodCachePtr newMethod = AZStd::make_shared<JMethodCache>();
                newMethod->m_methodId = methodId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetMethodSignature(newMethod, methodName, methodSignature);
            #endif

                m_methods.insert(AZStd::make_pair(methodName, newMethod));
                return true;
            }

            ////////////////////////////////////////////////////////////////
            bool Object::RegisterStaticMethod(const char* methodName, const char* methodSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register static method %s on class %s", methodName, m_className.c_str());
                    return false;
                }

                jmethodID methodId = jniEnv->GetStaticMethodID(m_classRef, methodName, methodSignature);
                if (!methodId || jniEnv->ExceptionCheck())
                {
                    AZ_Warning("JNI::Object", false, "Failed to find method %s with signature %s in class %s", methodName, methodSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JMethodCachePtr newMethod = AZStd::make_shared<JMethodCache>();
                newMethod->m_methodId = methodId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetMethodSignature(newMethod, methodName, methodSignature);
            #endif

                m_staticMethods.insert(AZStd::make_pair(methodName, newMethod));
                return true;
            }

            ////////////////////////////////////////////////////////////////
            bool Object::RegisterNativeMethods(AZStd::vector<JNINativeMethod> nativeMethods)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register native methods on class %s", m_className.c_str());
                    return false;
                }

                if (jniEnv->RegisterNatives(m_classRef, nativeMethods.data(), nativeMethods.size()) < 0)
                {
                    AZ_Error("JNI::Object", false, "Failed to register native methods with class %s", m_className.c_str());
                    return false;
                }
                return true;
            }


            ////////////////////////////////////////////////////////////////
            bool Object::RegisterField(const char* fieldName, const char* fieldSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register field %s on class %s", fieldName, m_className.c_str());
                    return false;
                }

                jfieldID fieldId = jniEnv->GetFieldID(m_classRef, fieldName, fieldSignature);
                if (!fieldId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to find field %s with signature %s in class %s", fieldName, fieldSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JFieldCachePtr newField = AZStd::make_shared<JFieldCache>();
                newField->m_fieldId = fieldId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetFieldSignature(newField, fieldName, fieldSignature);
            #endif

                m_fields.insert(AZStd::make_pair(fieldName, newField));
                return true;
            }

            ////////////////////////////////////////////////////////////////
            bool Object::RegisterStaticField(const char* fieldName, const char* fieldSignature)
            {
                JNIEnv* jniEnv = GetEnv();
                if (!jniEnv)
                {
                    AZ_Error("JNI::Object", false, "Failed to get JNIEnv* on thread to register static field %s on class %s", fieldName, m_className.c_str());
                    return false;
                }

                jfieldID fieldId = jniEnv->GetStaticFieldID(m_classRef, fieldName, fieldSignature);
                if (!fieldId || jniEnv->ExceptionCheck())
                {
                    AZ_Error("JNI::Object", false, "Failed to find static field %s with signature %s in class %s", fieldName, fieldSignature, m_className.c_str());
                    HANDLE_JNI_EXCEPTION(jniEnv);
                    return false;
                }

                JFieldCachePtr newField = AZStd::make_shared<JFieldCache>();
                newField->m_fieldId = fieldId;

            #if defined(JNI_SIGNATURE_VALIDATION)
                SetFieldSignature(newField, fieldName, fieldSignature);
            #endif

                m_staticFields.insert(AZStd::make_pair(fieldName, newField));
                return true;
            }


            ////////////////////////////////////////////////////////////////
            void Object::SetBooleanField(const char* fieldName, jboolean value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetBooleanField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetByteField(const char* fieldName, jbyte value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetByteField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetCharField(const char* fieldName, jchar value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetCharField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetShortField(const char* fieldName, jshort value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetShortField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetIntField(const char* fieldName, jint value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetIntField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetLongField(const char* fieldName, jlong value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetLongField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetFloatField(const char* fieldName, jfloat value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetFloatField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetDoubleField(const char* fieldName, jdouble value)
            {
                SetPrimitiveTypeFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        jniEnv->SetDoubleField(objectRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStringField(const char* fieldName, const AZStd::string& value)
            {
                jstring argString = ConvertStringToJstring(value);
                if (!argString)
                {
                    AZ_Error("JNI::Object", false, "Failed to set string field %s in class %s due to string conversion failure.", fieldName, m_className.c_str());
                    return;
                }

                SetObjectField<jstring>(fieldName, argString);

                DeleteRef(argString);
            }


            ////////////////////////////////////////////////////////////////
            jboolean Object::GetBooleanField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jboolean>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetBooleanField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jbyte Object::GetByteField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jbyte>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetByteField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jchar Object::GetCharField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jchar>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetCharField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jshort Object::GetShortField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jshort>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetShortField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jint Object::GetIntField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jint>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetIntField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jlong Object::GetLongField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jlong>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetLongField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jfloat Object::GetFloatField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jfloat>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetFloatField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jdouble Object::GetDoubleField(const char* fieldName)
            {
                return GetPrimitiveTypeFieldInternal<jdouble>(
                    fieldName,
                    [](JNIEnv* jniEnv, jobject objectRef, jfieldID fieldId)
                    {
                        return jniEnv->GetDoubleField(objectRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string Object::GetStringField(const char* fieldName)
            {
                jstring rawString = GetObjectField<jstring>(fieldName);
                AZStd::string returnValue = ConvertJstringToString(rawString);
                DeleteRef(rawString);
                return returnValue;
            }


            ////////////////////////////////////////////////////////////////
            void Object::SetStaticBooleanField(const char* fieldName, jboolean value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticBooleanField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticByteField(const char* fieldName, jbyte value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticByteField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticCharField(const char* fieldName, jchar value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticCharField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticShortField(const char* fieldName, jshort value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticShortField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticIntField(const char* fieldName, jint value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticIntField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticLongField(const char* fieldName, jlong value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetLongField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticFloatField(const char* fieldName, jfloat value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetFloatField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticDoubleField(const char* fieldName, jdouble value)
            {
                SetPrimitiveTypeStaticFieldInternal(
                    fieldName,
                    [value](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        jniEnv->SetStaticDoubleField(classRef, fieldId, value);
                    },
                    value);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetStaticStringField(const char* fieldName, const AZStd::string& value)
            {
                jstring argString = ConvertStringToJstring(value);
                if (!argString)
                {
                    AZ_Error("JNI::Object", false, "Failed to set string field %s in class %s due to string conversion failure.", fieldName, m_className.c_str());
                    return;
                }

                SetStaticObjectField<jstring>(fieldName, argString);

                DeleteRef(argString);
            }


            ////////////////////////////////////////////////////////////////
            jboolean Object::GetStaticBooleanField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jboolean>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticBooleanField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jbyte Object::GetStaticByteField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jbyte>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticByteField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jchar Object::GetStaticCharField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jchar>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticCharField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jshort Object::GetStaticShortField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jshort>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticShortField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jint Object::GetStaticIntField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jint>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticIntField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jlong Object::GetStaticLongField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jlong>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticLongField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jfloat Object::GetStaticFloatField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jfloat>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticFloatField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            jdouble Object::GetStaticDoubleField(const char* fieldName)
            {
                return GetPrimitiveTypeStaticFieldInternal<jdouble>(
                    fieldName,
                    [](JNIEnv* jniEnv, jclass classRef, jfieldID fieldId)
                    {
                        return jniEnv->GetStaticDoubleField(classRef, fieldId);
                    });
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string Object::GetStaticStringField(const char* fieldName)
            {
                jstring rawString = GetStaticObjectField<jstring>(fieldName);
                AZStd::string returnValue = ConvertJstringToString(rawString);
                DeleteRef(rawString);
                return returnValue;
            }


            // ----
            // Object (private)
            // ----

            ////////////////////////////////////////////////////////////////
            Object::JMethodCachePtr Object::GetMethod(const AZStd::string& methodName) const
            {
                JMethodMap::const_iterator methodIter = m_methods.find(methodName);
                if (methodIter == m_methods.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered method %s in class %s", methodName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return methodIter->second;
            }

            ////////////////////////////////////////////////////////////////
            Object::JMethodCachePtr Object::GetStaticMethod(const AZStd::string& methodName) const
            {
                JMethodMap::const_iterator methodIter = m_staticMethods.find(methodName);
                if (methodIter == m_methods.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered static method %s in class %s", methodName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return methodIter->second;
            }


            ////////////////////////////////////////////////////////////////
            Object::JFieldCachePtr Object::GetField(const AZStd::string& fieldName) const
            {
                JFieldMap::const_iterator fieldIter = m_fields.find(fieldName);
                if (fieldIter == m_fields.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered field %s in class %s", fieldName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return fieldIter->second;
            }

            ////////////////////////////////////////////////////////////////
            Object::JFieldCachePtr Object::GetStaticField(const AZStd::string& fieldName) const
            {
                JFieldMap::const_iterator fieldIter = m_staticFields.find(fieldName);
                if (fieldIter == m_staticFields.end())
                {
                    AZ_Error("JNI::Object", false, "Failed to find registered static method %s in class %s", fieldName.c_str(), m_className.c_str());
                    return nullptr;
                }

                return fieldIter->second;
            }


        #if defined(JNI_SIGNATURE_VALIDATION)
            ////////////////////////////////////////////////////////////////
            void Object::SetMethodSignature(JMethodCachePtr methodCache, const char* methodName, const char* methodSignature)
            {
                methodCache->m_methodName = methodName;

                AZStd::string signature = methodSignature;
                size_t first = signature.find('(');
                size_t last = signature.find_last_of(')');

                if ((last - first) > 1)
                {
                    methodCache->m_argumentSignature = signature.substr(first + 1, last - first - 1);
                }

                methodCache->m_returnSignature = signature.substr(last + 1, signature.size() - last - 1);
            }

            ////////////////////////////////////////////////////////////////
            void Object::SetFieldSignature(JFieldCachePtr fieldCache, const char* fieldName, const char* signature)
            {
                fieldCache->m_fieldName = fieldName;
                fieldCache->m_signature = signature;
            }

            ////////////////////////////////////////////////////////////////
            bool Object::ValidateSignature(JMethodCachePtr methodCache, const AZStd::string& argumentSignature, const AZStd::string& returnSignature)
            {
                bool hasValidSignature = (methodCache->m_argumentSignature.compare(argumentSignature) == 0);
                AZ_Error("JNI::Object",
                         hasValidSignature,
                         "Invalid argument signature supplied for method call %s on class %s.  Expecting %s, was given %s",
                         methodCache->m_methodName.c_str(),
                         m_className.c_str(),
                         methodCache->m_argumentSignature.c_str(),
                         argumentSignature.c_str());

                // This is a bit harder to validate, specifically when it comes from InvokeObjectMethod.
                // The best we can do is validate the prefix of the signature is correct.
                if (methodCache->m_returnSignature[0] == 'L')
                {
                    hasValidSignature = (returnSignature[0] == 'L');
                }
                else if (methodCache->m_returnSignature.compare(0, 2, "[L", 2) == 0)
                {
                    hasValidSignature = (returnSignature.compare(0, 2, "[L", 2) == 0);
                }
                else
                {
                    hasValidSignature = (methodCache->m_returnSignature.compare(returnSignature) == 0);
                }

                AZ_Error("JNI::Object",
                         hasValidSignature,
                         "Invalid return signature supplied for method call %s on class %s.  Expecting %s, was given %s",
                         methodCache->m_methodName.c_str(),
                         m_className.c_str(),
                         methodCache->m_returnSignature.c_str(),
                         returnSignature.c_str());

                return hasValidSignature;
            }

            ////////////////////////////////////////////////////////////////
            bool Object::ValidateSignature(JFieldCachePtr fieldCache, const AZStd::string& signature)
            {
                bool hasValidSignature = (fieldCache->m_signature.compare(signature) == 0);
                AZ_Error("JNI::Object",
                         hasValidSignature,
                         "Invalid argument signature supplied for field call %s on class %s.  Expecting %s, was given %s",
                         fieldCache->m_fieldName.c_str(),
                         m_className.c_str(),
                         fieldCache->m_signature.c_str(),
                         signature.c_str());

                return hasValidSignature;
            }
        #endif // defined(JNI_SIGNATURE_VALIDATION)
        }
    }
}