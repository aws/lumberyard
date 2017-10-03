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
//  or project specific include files that are used frequently, but
//      are changed infrequently
//


#if !defined(AFX_STDAFX_H_448E473C_48A4_4BA1_8498_3F9DAA9FE6A4_INCLUDED_)
#define AFX_STDAFX_H_448E473C_48A4_4BA1_8498_3F9DAA9FE6A4_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// TODO investigate why this warning is thrown if ref new Platfrom::Array<...> is used
#pragma warning ( disable:4717 ) // stupid warning in vccorlib.h

#include <vector>
#include <list>

#define CRYINPUT_EXPORTS

#include <platform.h>

#if defined(_DEBUG) && !defined(LINUX) && !defined(APPLE) && !defined(ORBIS)
#include <crtdbg.h>
#endif

#include <ITimer.h>
#include <IInput.h>
#include <CryName.h>
#include <StlUtils.h>
#include "CryInput.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H_448E473C_48A4_4BA1_8498_3F9DAA9FE6A4_INCLUDED_)
