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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <jni.h>
#include <pthread.h>


struct AAssetManager;


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
           class Object;
        }

        class AndroidEnv
        {
        public:
            AZ_TYPE_INFO(AndroidEnv, "{E51A8876-7A26-4CB1-BA88-394A128728C7}")
            AZ_CLASS_ALLOCATOR(AndroidEnv, AZ::SystemAllocator, 0);


            //! Creation POD for the AndroidEnv
            struct Descriptor
            {
                Descriptor()
                    : m_jvm(nullptr)
                    , m_activityRef(nullptr)
                    , m_assetManager(nullptr)
                    , m_internalStoragePath()
                    , m_externalStoragePath()
                    , m_obbStoragePath()
                {
                }

                JavaVM* m_jvm; //!< Global pointer to the Java virtual machine
                jobject m_activityRef; //!< Local or global reference to the activity instance
                AAssetManager* m_assetManager; //!< Global pointer to the Android asset manager, used for APK file i/o
                AZStd::string m_internalStoragePath; //!< Access restricted location. E.G. /data/data/<package_name>/files
                AZStd::string m_externalStoragePath; //!< Public storage specifically for the application. E.G. <public_storage>/Android/data/<package_name>/files
                AZStd::string m_obbStoragePath; //!< Public storage specifically for the application's obb files. E.G. <public_storage>/Android/obb/<package_name>/files
            };

            //! Public accessor to the global AndroidEnv instance
            static AndroidEnv* Get();

            //! The prefered entry point for the construction of the global AndoridEnv instance
            static bool Create(const Descriptor& descriptor);

            //! Public accessor to destory the AndroidEnv global instance
            static void Destroy();


            // ----

            //! Request a thread specific JNIEnv pointer from the JVM.
            //! \return A pointer to the JNIEnv on the current thread.
            JNIEnv* GetJniEnv() const;

            //! Request the global reference to the activity class
            jclass GetActivityClassRef() const { return m_activityClass; }

            //! Request the global reference to the activity instance
            jobject GetActivityRef() const { return m_activityRef; }

            //! Get the global pointer to the Android asset manager, which is used for APK file i/o.
            AAssetManager* GetAssetManager() const { return m_assetManager; }

            //! Get the hidden internal storage, typically this is where the application is installed
            //! on the device.
            //! e.g. /data/data/<package_name/files
            const AZStd::string& GetInternalStoragePath() const { return m_internalStoragePath; }

            //! Get the root directory for external (or public) storage.
            //! e.g. /storage/sdcard0/, /storage/self/primary/, etc.
            const AZStd::string& GetExternalStorageRoot() const { return m_externalStorageRoot; }

            //! Get the application specific directory for external (or public) storage.
            //! e.g. <public_storage>/Android/data/<package_name/files
            const AZStd::string& GetExternalStoragePath() const { return m_externalStoragePath; }

            //! Get the application specific directory for obb files.
            //! e.g. <public_storage>/Android/obb/<package_name/files
            const AZStd::string& GetObbStoragePath() const { return m_obbStoragePath; }

            //! Get the dot separated package name for the current application.
            //! e.g. com.lumberyard.samples for SamplesProject
            const AZStd::string& GetPackageName() const { return m_packageName; }

            //! Get the game project name from the Java string resources.
            const AZStd::string& GetGameProjectName() const { return m_gameProjectName; }

            //! Get the app version code (android:versionCode in the manifest).
            int GetAppVersionCode() const { return m_appVersionCode; }

            //! Get the filename of the obb. This doesn't include the path to the obb folder.
            AZStd::string GetObbFileName(bool mainFile) const;

            //! Check to see if the application should be running, e.g. not paused.
            bool IsRunning() const { return m_isRunning; }

            //! Set whether or not the application should be running
            void SetIsRunning(bool isRunning) { m_isRunning = isRunning; }

            //! Set wheather or not the application should be running
            bool IsReady() const { return m_isReady; }

            //! Loads a Java class as opposed to attempting to find a loaded class from the call stack.
            //! \param classPath The fully qualified forward slash separated Java class path.
            //! \return A global reference to the desired jclass.  Caller is responsible for making a
            //!         call to DeleteGlobalJniRef when the jclass is no longer needed.
            jclass LoadClass(const char* classPath);

            //! Get the fully qualified forward slash separated Java class path of Java class ref.
            //! e.g. android.app.NativeActivity ==> android/app/NativeActivity
            //! \param classRef A valid reference to a java class
            //! \return A copy of the class name
            AZStd::string GetClassName(jclass classRef) const;

            //! Get just the name of the Java class from a Java class ref.
            //! e.g. android.app.NativeActivity ==> NativeActivity
            //! \param classRef A valid reference to a java class
            //! \return A copy of the class name
            AZStd::string GetSimpleClassName(jclass classRef) const;

            //! Retrieve a boolean resource from the android resources
            //! \param resourceName The name of the boolean resource.
            //! \return The boolean value of the resource.
            bool GetBooleanResource(const char* resourceName) const;

        private:
            //! Callback for when a thread exists to detach the jni env from the thread
            //! \param threadData Expected to be the JNIEnv pointer
            static void DestroyJniEnv(void* threadData);


            // ----

            AndroidEnv();
            ~AndroidEnv();

            AndroidEnv(const AndroidEnv&) = delete;
            AndroidEnv(const AndroidEnv&&) = delete;
            AndroidEnv& operator=(const AndroidEnv&) = delete;
            AndroidEnv& operator=(const AndroidEnv&&) = delete;

            //! Public global accessor to the android application environment
            //! \param descriptor
            bool Initialize(const Descriptor& descriptor);

            //! Handle the deletion of the global jni references
            void Cleanup();

            //! Finds the java/lang/Class jclass to get the method IDs to getName and getSimpleName
            //! \return True if succesfully, False otherwise
            bool LoadClassNameMethods(JNIEnv* jniEnv);

            //! Calls some java methods on the activity instance and constructs the class loader
            //! \return True if succesfully, False otherwise
            bool CacheActivityData(JNIEnv* jniEnv);

            //! Helper to call one of the get*Name methods on a jclass reference
            AZStd::string GetClassNameInternal(jclass classRef, jmethodID methodId) const;


            // ----

            static pthread_key_t s_jniEnvKey; //!< Thread key for accessing the thread specific jni env pointers
            static AZ::EnvironmentVariable<AndroidEnv*> s_instance; //!< Reference to the global object, created in the main function (AndroidLauncher)


            JavaVM* m_jvm; //!< Mostly used for [de/a]ttaching JNIEnv pointers to threads

            jobject m_activityRef; //!< Reference to the global instance of the current activity object, used for instance method invocation, field access
            jclass m_activityClass; //!< Reference to the global instance of the current activity class, used for method / field extraction, static method invocation

            AZStd::unique_ptr<JNI::Object> m_classLoader; //!< Class loader instance, used for finding Java classes on any thread

            jmethodID m_getClassNameMethod; //!< Method ID for getName from java/lang/Class which returns a fully qualified dot separated Java class path
            jmethodID m_getSimpleClassNameMethod; //!< Method ID for getSimpleName from java/lang/Class which returns just the class name from a Java class path

            AAssetManager* m_assetManager; //!< Used for file i/o from the APK

            AZStd::string m_internalStoragePath;  //!< Access restricted location. E.G. /data/data/<package_name>/files

            AZStd::string m_externalStorageRoot; //!< Root of the public storage directory. E.G. /storage/emulated/0, /storage/self/primary, etc.
            AZStd::string m_externalStoragePath;  //!< Public storage specifically for the application. E.G. <public_storage>/Android/data/<package_name>/files
            AZStd::string m_obbStoragePath; //!< Public storage  specifically for the application's obb files. E.G. <public_storage>/Android/obb/<package_name>/files

            AZStd::string m_packageName; //!< The dot separated package id of the application
            AZStd::string m_gameProjectName; //!< The name of the game project to load, retrieved from the Java string resources
            int m_appVersionCode; //!< The version code of the app (android:versionCode in the AndroidManifest.xml)

            bool m_ownsActivityRef; //!< For when a local activity ref is passed into the construction and needs to be cleaned up
            bool m_isRunning; //!< Used to pause the main loop (lumberyard code)
            bool m_isReady; //!< Set only once the object has been successfully constructed
        };
    }
}