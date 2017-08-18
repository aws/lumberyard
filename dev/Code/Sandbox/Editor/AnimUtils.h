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
//  Copyright (C), Crytek Studios, 2001-2012
// -------------------------------------------------------------------------
//  File name:   AnimUtils.h
//  Created:     9/11/2006 by Michael S.
//  Description: Animation utilities
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_ANIMUTILS_H
#define CRYINCLUDE_EDITOR_ANIMUTILS_H
struct ICharacterInstance;

namespace AnimUtils
{
    void StartAnimation(ICharacterInstance* pCharacter, const char* pAnimName);
    void SetAnimationTime(ICharacterInstance* pCharacter, float fNormalizedTime);
    void StopAnimations(ICharacterInstance* pCharacter);
}
#endif // CRYINCLUDE_EDITOR_ANIMUTILS_H
