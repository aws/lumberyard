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
#include <QUuid>
#include <STLPortConfig.h>

// STL Port in debug for debug builds
#if defined(_DEBUG)
#  define _STLP_DEBUG 1
#endif

#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4266) // no override available for virtual member function from base 'CObject'; function is hidden
//#pragma warning(disable: 4264) // no override available for virtual member function from base 'CObject'; function is hidden

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <platform.h>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//#define SANDBOX_API
//#define CRYEDIT_API
#include <ISystem.h>
#include "EditorDefs.h"
#include "Util/EditorUtils.h"
#include "IEditor.h"
#include "Util/PathUtil.h"

#include <EditorUI_QTDLLBus.h>
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
