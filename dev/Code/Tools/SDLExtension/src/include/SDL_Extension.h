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

#ifndef CRYINCLUDE_TOOLS_SDLEXTENSION_SRC_INCLUDE_SDL_EXTENSION_H
#define CRYINCLUDE_TOOLS_SDLEXTENSION_SRC_INCLUDE_SDL_EXTENSION_H
#pragma once


#include "begin_code.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__ANDROID__) && __ANDROID__
/* Return the primary external storage directory. This directory may not
   currently be accessible if it has been mounted by the user on their
   computer, has been removed from the device, or some other problem has
   happened.
   If external storage is current unavailable, this will return 0.
*/
extern DECLSPEC const char * SDLCALL SDLExt_AndroidGetExternalStorageDirectory();

/* Return the name of application package */
extern DECLSPEC const char * SDLCALL SDLExt_AndroidGetPackageName();

/* Return the asset manager of the application */
extern DECLSPEC AAssetManager * SDLCALL SDLExt_GetAssetManager();
	
#endif /* __ANDROID__ */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif // CRYINCLUDE_TOOLS_SDLEXTENSION_SRC_INCLUDE_SDL_EXTENSION_H
