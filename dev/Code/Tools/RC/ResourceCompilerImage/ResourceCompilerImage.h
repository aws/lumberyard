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


// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the RESOURCECOMPILERIMAGE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// RESOURCECOMPILERIMAGE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_RESOURCECOMPILERIMAGE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_RESOURCECOMPILERIMAGE_H
#pragma once

#ifdef RESOURCECOMPILERIMAGE_EXPORTS
#define RESOURCECOMPILERIMAGE_API __declspec(dllexport)
#else
#define RESOURCECOMPILERIMAGE_API __declspec(dllimport)
#endif



#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_RESOURCECOMPILERIMAGE_H
