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

#include <QtGlobal>

#if defined(AZ_PLATFORM_APPLE_OSX)
#include <QUuid>
#include "AzCore/Math/Guid.h"
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN    // Exclude rarely-used stuff from Windows headers.
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

#ifndef WINVER                              // Allow use of features specific to Windows XP or later.
#define WINVER 0x0600                   // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT                    // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS              // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410   // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE                           // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600            // Change this to the appropriate value to target other versions of IE.
#endif

#pragma warning(disable: 4244)  // warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800)  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4266)  // no override available for virtual member function from base 'CObject'; function is hidden

#include "Resource.h"

#include <Cry_Math.h>

#include "CryWindows.h"

#include "platform.h"
#include "functor.h"
#include "Cry_Geo.h"
#include "ISystem.h"
#include "I3DEngine.h"
#include "IEntity.h"
#include "IRenderAuxGeom.h"

// STL headers.
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

#include "IEditor.h"



#include "WaitProgress.h"
#include "Settings.h"
#include "LogFile.h"

#include "Undo/IUndoObject.h"

#include "Util/RefCountBase.h"
#include "Util/fastlib.h"
#include "Util/EditorUtils.h"
#include "Util/PathUtil.h"
#include "Util/smartptr.h"
#include "Util/Variable.h"
#include "Util/XmlArchive.h"
#include "Util/Math.h"

#include "Controls/NumberCtrl.h"

#include "Objects/ObjectManager.h"
#include "Objects/SelectionGroup.h"
#include "Objects/BaseObject.h"

#include "Include/IDisplayViewport.h"
#include "EditTool.h"

#include "Core/BrushDeclaration.h"
#include "Core/BrushCommon.h"
#include "Util/DesignerSettings.h"
