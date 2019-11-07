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

// It is vital that at least some Qt header is pulled in to this header, so that
// QT_VERSION is defined and the editor gets the correct set of overloads in the
// IXML interface. This also disambiguates the GUID/REFGUID situation in Guid.h
#include <QUuid>

#include <platform.h>

#include "ProjectDefines.h"

#ifdef NOMINMAX
#include "Cry_Math.h"
#endif //NOMINMAX

// Resource includes
#include "Resource.h"

//////////////////////////////////////////////////////////////////////////
// Main Editor include.
//////////////////////////////////////////////////////////////////////////
#include "EditorDefs.h"

#ifdef _DEBUG
#ifdef assert
#undef assert
#define assert CRY_ASSERT
#endif
#endif

#include "CryEdit.h"
#include "EditTool.h"
#include "PluginManager.h"

#include "Util/Variable.h"

#include <IGame.h>
#include <ISplines.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <CryListenerSet.h>

#define USE_PYTHON_SCRIPTING

#ifdef max
#undef max
#undef min
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef GetUserName
#undef GetUserName
#endif

#ifdef LoadCursor
#undef LoadCursor
#endif
