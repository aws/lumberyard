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

#ifndef CRYINCLUDE_EDITOR_JOYSTICKUTILS_H
#define CRYINCLUDE_EDITOR_JOYSTICKUTILS_H
#pragma once


class IJoystick;
class IJoystickChannel;

namespace JoystickUtils
{
    float Evaluate(IJoystickChannel* pChannel, float time);
    void SetKey(IJoystickChannel* pChannel, float time, float value, bool createIfMissing = true);
    void Serialize(IJoystickChannel* pChannel, XmlNodeRef node, bool bLoading);
    bool HasKey(IJoystickChannel* pChannel, float time);
    void RemoveKey(IJoystickChannel* pChannel, float time);
    void RemoveKeysInRange(IJoystickChannel* pChannel, float startTime, float endTime);
    void PlaceKey(IJoystickChannel* pChannel, float time);
}

#endif // CRYINCLUDE_EDITOR_JOYSTICKUTILS_H
