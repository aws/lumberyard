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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "SDL_stdinc.h"

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SDL_error.h>

static int s_active = 0;
struct LocalReferenceHolder
{
    JNIEnv *m_env;
    const char *m_func;
};

static jclass mActivityClass;
static jobject mAssetManager;
static AAssetManager *manager;

static struct LocalReferenceHolder LocalReferenceHolder_Setup(const char *func)
{
    struct LocalReferenceHolder refholder;
    refholder.m_env = NULL;
    refholder.m_func = func;
#ifdef DEBUG_JNI
    SDL_Log("Entering function %s", func);
#endif
    return refholder;
}

static SDL_bool LocalReferenceHolder_Init(struct LocalReferenceHolder *refholder, JNIEnv *env)
{
    const int capacity = 16;
    if ((*env)->PushLocalFrame(env, capacity) < 0) {
        SDL_SetError("Failed to allocate enough JVM local references");
        return SDL_FALSE;
    }
    ++s_active;
    refholder->m_env = env;
    return SDL_TRUE;
}

static void LocalReferenceHolder_Cleanup(struct LocalReferenceHolder *refholder)
{
#ifdef DEBUG_JNI
    SDL_Log("Leaving function %s", refholder->m_func);
#endif
    if (refholder->m_env) {
        JNIEnv* env = refholder->m_env;
        (*env)->PopLocalFrame(env, NULL);
        --s_active;
    }
}

static SDL_bool LocalReferenceHolder_IsActive()
{
    return s_active > 0;    
}

void SDLExt_Android_Init(JNIEnv* env, jclass cls, jobject mgr)
{
    mActivityClass = (jclass)((*env)->NewGlobalRef(env, cls));
    mAssetManager = (jobject)((*env)->NewGlobalRef(env, mgr));

    manager = AAssetManager_fromJava(env, mAssetManager);
    if (manager == NULL) {
	__android_log_print(ANDROID_LOG_ERROR, "SDLExt", "Error loading asset manager");
    }
    else {
	__android_log_print(ANDROID_LOG_VERBOSE, "SDLExt", "Asset manager loaded.");
    }
}

const char * SDLExt_AndroidGetExternalStorageDirectory()
{
    static char *s_AndroidExternalStorageDirectory = NULL;

    if (!s_AndroidExternalStorageDirectory) {
	struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
	jmethodID mid;
	jclass cls;
	jobject fileObject;
	jstring pathString;
	const char *path;

	JNIEnv *env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	if (!LocalReferenceHolder_Init(&refs, env)) {
	    LocalReferenceHolder_Cleanup(&refs);
	    return NULL;
	}

	cls = (*env)->FindClass(env, "android/os/Environment");
	mid = (*env)->GetStaticMethodID(env, cls,
					"getExternalStorageDirectory", "()Ljava/io/File;");
	fileObject = (*env)->CallStaticObjectMethod(env, cls, mid);
	if (!fileObject) {
	    SDL_SetError("Couldn't get external storage directory");
	    LocalReferenceHolder_Cleanup(&refs);
	    return NULL;
	}

	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject),
				  "getAbsolutePath", "()Ljava/lang/String;");
	pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, mid);

	path = (*env)->GetStringUTFChars(env, pathString, NULL);
	s_AndroidExternalStorageDirectory = SDL_strdup(path);
	(*env)->ReleaseStringUTFChars(env, pathString, path);

	LocalReferenceHolder_Cleanup(&refs);
    }
    return s_AndroidExternalStorageDirectory;
}

const char * SDLExt_AndroidGetPackageName()
{
    static char *s_AndroidPackageName = NULL;

    if (!s_AndroidPackageName) {
	struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
	jmethodID mid;
	jobject context;
	jstring nameString;
	const char *name;

	JNIEnv *env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	if (!LocalReferenceHolder_Init(&refs, env)) {
	    LocalReferenceHolder_Cleanup(&refs);
	    return NULL;
	}

	/* context = SDLActivity.getContext(); */
	mid = (*env)->GetStaticMethodID(env, mActivityClass,
					"getContext", "()Landroid/content/Context;");
	context = (*env)->CallStaticObjectMethod(env, mActivityClass, mid);

	/* nameString = context.getPackageName(); */
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context),
				  "getPackageName", "()Ljava/lang/String;");
	nameString = (jstring)(*env)->CallObjectMethod(env, context, mid);

	name = (*env)->GetStringUTFChars(env, nameString, NULL);
	s_AndroidPackageName = SDL_strdup(name);
	(*env)->ReleaseStringUTFChars(env, nameString, name);

	LocalReferenceHolder_Cleanup(&refs);
    }
    return s_AndroidPackageName;
}

AAssetManager * SDLExt_GetAssetManager()
{
	return manager;
}
