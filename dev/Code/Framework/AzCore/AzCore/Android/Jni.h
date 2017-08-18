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

#include <AzCore/Android/JNI/JNI.h>


namespace AZ
{
    namespace Android
    {
        namespace Jni
        {
            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Request a thread specific JNIEnv pointer from the Android environment.
            //! \return A pointer to the JNIEnv on the current thread.
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline JNIEnv* GetEnv() { return JNI::GetEnv(); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Loads a Java class as opposed to attempting to find a loaded class from the call stack.
            //! \param classPath The fully qualified forward slash separated Java class path.
            //! \return A global reference to the desired jclass.  Caller is responsible for making a
            //!         call do DeleteGlobalJniRef when the jclass is no longer needed.
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline jclass LoadClass(const char* classPath) { return JNI::LoadClass(classPath); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Get the fully qualified forward slash separated Java class path of Java class ref.
            //! e.g. android.app.NativeActivity ==> android/app/NativeActivity
            //! \param classRef A valid reference to a java class
            //! \return A copy of the class name
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline AZStd::string GetClassName(jclass classRef) { return JNI::GetClassName(classRef); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Get just the name of the Java class from a Java class ref.
            //! e.g. android.app.NativeActivity ==> NativeActivity
            //! \param classRef A valid reference to a java class
            //! \return A copy of the class name
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline AZStd::string GetSimpleClassName(jclass classRef) { return JNI::GetSimpleClassName(classRef); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Converts a jstring to a AZStd::string
            //! \param stringValue A local or global reference to a jstring object
            //! \return A copy of the converted string
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline AZStd::string ConvertJstringToString(jstring stringValue) { return JNI::ConvertJstringToString(stringValue); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Converts a AZStd::string to a jstring
            //! \param stringValue A local or global reference to a jstring object
            //! \return A global reference to the converted jstring.  The caller is responsible for
            //!         deleting it when no longer needed
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline jstring ConvertStringToJstring(const AZStd::string& stringValue) { return JNI::ConvertStringToJstring(stringValue); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Gets the reference type of the Java object.  Can be Local, Global or Weak Global.
            //! \param javaRef Raw Java object reference, can be null.
            //! \return The result of GetObjectRefType as long as the object is valid,
            //!         otherwise JNIInvalidRefType.
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline int GetRefType(jobject javaRef) { return JNI::GetRefType(javaRef); }

            //! \deprecated This function has been moved to the AZ::Android::JNI namespace
            //! Deletes a JNI object/class reference.  Will handle local, global and weak global references.
            //! \param javaRef Raw java object reference.
            [[deprecated("This function has been moved to the AZ::Android::JNI namespace")]]
            inline void DeleteRef(jobject javaRef) { JNI::DeleteRef(javaRef); }

            //! \deprecated Please use AZ::Android::JNI::DeleteRef for JNI reference deletions
            //! \brief Deletes a global JNI object/class reference.  If the reference is not global an
            //!         assert will be triggered and the reference will leak.
            //! \param globalRef The global java object/class reference.
            [[deprecated("Please use AZ::Android::JNI::DeleteRef for JNI reference deletions")]]
            void DeleteGlobalRef(jobject globalRef);
        }
    }
}
