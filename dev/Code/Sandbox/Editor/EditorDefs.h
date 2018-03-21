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
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios
// -------------------------------------------------------------------------
//  Created:     13/2/2003 by Timur.
//  Description: Main header included by every file in Editor.
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_EDITORDEFS_H
#define CRYINCLUDE_EDITOR_EDITORDEFS_H
#include <Include/SandboxAPI.h>
#include <Include/EditorCoreAPI.h>

//////////////////////////////////////////////////////////////////////////
// optimize away function call, in favour of inlining asm code
//////////////////////////////////////////////////////////////////////////
#pragma intrinsic( memset,memcpy,memcmp )
#pragma intrinsic( strcat,strcmp,strcpy,strlen,_strset )
//#pragma intrinsic( abs,fabs,fmod,sin,cos,tan,log,exp,atan,atan2,log10,sqrt,acos,asin )

// Warnings in STL
#pragma warning (disable : 4786) // identifier was truncated to 'number' characters in the debug information.
#pragma warning (disable : 4244) // conversion from 'long' to 'float', possible loss of data
#pragma warning (disable : 4018) // signed/unsigned mismatch
#pragma warning (disable : 4800) // BOOL bool conversion

// Disable warning when a function returns a value inside an __asm block
#pragma warning (disable : 4035)

//////////////////////////////////////////////////////////////////////////
// 64-bits related warnings.
#pragma warning (disable : 4267) // conversion from 'size_t' to 'int', possible loss of data

//////////////////////////////////////////////////////////////////////////
// Simple type definitions.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

// Which cfg file to use.
#define EDITOR_CFG_FILE "editor.cfg"

// If #defined, then shapes of newly created AITerritoryShapes will be limited to a single point, which is less confusing for level-designers nowadays
// as the actual shape has no practical meaning anymore (C1 and maybe C2 may have used the shape, but it was definitely no longer used for C3).
#define USE_SIMPLIFIED_AI_TERRITORY_SHAPE


//////////////////////////////////////////////////////////////////////////
// C runtime lib includes
//////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <climits>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

/////////////////////////////////////////////////////////////////////////////
#define _XTP_INCLUDE_DEPRECATED
// A hack to work around IEntity structure defined deep in windows OLE code
#define IEntity IEntity_AfxOleOverride
#undef rand // remove cryengine protection against wrong rand usage

// A hack to work around IEntity structure defined deep in windows OLE code
#undef IEntity

/////////////////////////////////////////////////////////////////////////////
// VARIOUS MACROS AND DEFINES
/////////////////////////////////////////////////////////////////////////////
#ifdef new
#undef new
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)          { if (p) { delete (p);       (p) = NULL; } \
}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)    { if (p) { delete[] (p);     (p) = NULL; } \
}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)         { if (p) { (p)->Release();   (p) = NULL; } \
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <platform.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <Range.h>
#include <StlUtils.h>

#include <smartptr.h>
#define TSmartPtr _smart_ptr

#define TOOLBAR_TRANSPARENT_COLOR QColor(0xC0, 0xC0, 0xC0)

/////////////////////////////////////////////////////////////////////////////
// Interfaces ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <IRenderer.h>
#include <CryFile.h>
#include <ISystem.h>
#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include <I3DEngine.h>
#include <IIndexedMesh.h>
#include <ITimer.h>
#include <IPhysics.h>
#include <IAISystem.h>
#include <IXml.h>
#include <IMovieSystem.h>
#include <functor.h>

//////////////////////////////////////////////////////////////////////////
// Commonly used Editor includes.
//////////////////////////////////////////////////////////////////////////

// Utility classes.
#include "Util/EditorUtils.h"
#include "Util/FileEnum.h"
#include "Util/Math.h"
#include "Util/AffineParts.h"

// Xml support.
#include "Util/XmlArchive.h"
#include "Util/XmlTemplate.h"

// Utility classes.
#include "Util/bitarray.h"
#include "Util/FunctorMulticaster.h"
#include "Util/RefCountBase.h"
#include "Util/TRefCountBase.h"
#include "Util/MemoryBlock.h"
#include "Util/PathUtil.h"
#include "Util/FileUtil.h"

// Variable.
#include "Util/Variable.h"

//////////////////////////////////////////////////////////////////////////
// Editor includes.
//////////////////////////////////////////////////////////////////////////

// Utility routines
#include "Util/fastlib.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/GuidUtil.h"

// Main Editor interface definition.
#include "IEditor.h"
#include "Include/IEditorClassFactory.h"

// Undo support.
#include "Undo/IUndoObject.h"

// Log file access
#include "LogFile.h"

//Editor Settings.
#include "Settings.h"

// Some standard controls to be always accessible.
#include "Controls/NumberCtrl.h"
#include "WaitProgress.h"

#include "UsedResources.h"
#include "Resource.h"

// Command Manager.
#include "Commands/CommandManager.h"
#include "Objects/ObjectManager.h"
#include "MainWindow.h"

#endif // CRYINCLUDE_EDITOR_EDITORDEFS_H
