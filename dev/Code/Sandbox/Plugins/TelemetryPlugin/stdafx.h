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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#include "targetver.h"

// HAX to resolve IEntity redefinition
#define __IEntity_INTERFACE_DEFINED__

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define NOT_USE_CRY_MEMORY_MANAGER
#include <platform.h>

// Re-disable virtual function override warnings, as MFC headers do not pass them.
#pragma warning(disable: 4264)
#pragma warning(disable: 4266)

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>          // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>         // MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)

// Include XT Toolkit
#define _XTP_INCLUDE_DEPRECATED
// Suppress certain warnings from 3rd Party XTToolkitPro library
#pragma warning (push)
#pragma warning (disable : 4264) // 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden
#pragma warning (disable : 4266) // 'function' : no override available for virtual member function from base 'type'; function is hidden
#include <XTToolkitPro.h>
#pragma warning (pop)

#include "Resource.h"



/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <algorithm>
#include <memory>

#include "CryExtension/CrySharedPtr.h"

#define SANDBOX_API
#define CRYEDIT_API
#include "IEditor.h"
#include "Include/SandboxAPI.h"

#include "Controls/NumberCtrl.h"

#include <IItem.h>


#ifdef WIN64
#if (_MSC_VER >= 1910)
#pragma comment(lib, "../../../../Bin64vc141/Editor.lib")
#elif (_MSC_VER >= 1900)
#pragma comment(lib, "../../../../Bin64vc140/Editor.lib")
#else // _MSC_VER
#pragma comment(lib, "../../../../Bin64vc120/Editor.lib")
#endif // _MSC_VER
#else
#pragma comment(lib, "../../../../Bin32/Editor.lib")
#endif

