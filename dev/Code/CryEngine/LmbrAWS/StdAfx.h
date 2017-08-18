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
#ifndef LMBRAWS_STDAFX_H
#define LMBRAWS_STDAFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector> // required to be included before platform.h

#include <CryModuleDefs.h>
#define eCryModule eCryM_AWS

#define LMBRAWS_EXPORTS

#include <platform.h>
#include <ISystem.h>

#endif  // LMBRAWS_STDAFX_H