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

#pragma once

#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_WINDOWS)
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER              // Allow use of features specific to Windows XP or later.
#define WINVER 0x0600       // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0600 // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600    // Change this to the appropriate value to target other versions of IE.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4266) // no override available for virtual member function from base 'CObject'; function is hidden
#endif

#define eCryModule eCryM_Game

// I couldn't figure out the culprit include for making QList not link due to #define realloc in CryMemoryManager
// The editor StdAfx was able to put the undef around QList but it wasn't working here so I took a "scorched earth" approach
// undef before each include. This worked and I'll leave it until I have time to be more surgical about it
#include <platform.h>
#undef realloc

#include <CryName.h>
#undef realloc

#include <I3DEngine.h>
#undef realloc

#include <ISystem.h>
#undef realloc

#include <ISerialize.h>
#undef realloc

#include <IGem.h>
#undef realloc

#include <EditorDefs.h>
#undef realloc

#include <PythonWorker.h>
#undef realloc

#include <QtViewPane.h>
#undef realloc

#include <Util/PathUtil.h>
#undef realloc

#include <IAWSResourceManager.h>
#undef realloc

#include <AzCore/EBus/EBus.h>
#undef realloc

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#undef realloc

#include <QList>
#include <QVariantList>
#include <QMainWindow>
#include <QScopedPointer>
#include <QAbstractTableModel>
#include <QStandardItemModel>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QQueue>
#include <QMimeData>
#include <QMenu>
#include <QProgressDialog>
#include <QPushButton>


