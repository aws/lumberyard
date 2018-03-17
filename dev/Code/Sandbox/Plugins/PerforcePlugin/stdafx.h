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

#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_WINDOWS)
#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4266) // no override available for virtual member function from base 'CObject'; function is hidden
#endif

#include "resource.h"

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
#include <ISystem.h>
#include "Util/EditorUtils.h"
#include "IEditor.h"
