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
