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

#ifndef CRYINCLUDE_EDITOR_UTIL_3DCONNEXIONDRIVER_H
#define CRYINCLUDE_EDITOR_UTIL_3DCONNEXIONDRIVER_H
#pragma once
#include "Include/IPlugin.h"

struct S3DConnexionMessage
{
    bool bGotTranslation;
    bool bGotRotation;

    int raw_translation[3];
    int raw_rotation[3];

    Vec3 vTranslate;
    Vec3 vRotate;

    unsigned char buttons[3];

    S3DConnexionMessage()
        : bGotRotation(false)
        , bGotTranslation(false)
    {
        raw_translation[0] = raw_translation[1] = raw_translation[2] = 0;
        raw_rotation[0] = raw_rotation[1] = raw_rotation[2] = 0;
        vTranslate.Set(0, 0, 0);
        vRotate.Set(0, 0, 0);
        buttons[0] = buttons[1] = buttons[2] = 0;
    };
};

class C3DConnexionDriver
    : public IPlugin
{
public:
    C3DConnexionDriver();
    ~C3DConnexionDriver();

    bool InitDevice();
    bool GetInputMessageData(LPARAM lParam, S3DConnexionMessage& msg);

    void Release() { delete this; };
    void ShowAbout() {};
    const char* GetPluginGUID() { return "{AD109901-9128-4ffd-8E67-137CB2B1C41B}"; };
    DWORD GetPluginVersion() { return 1; };
    const char* GetPluginName() { return "3DConnexionDriver"; };
    bool CanExitNow() { return true; };
    void OnEditorNotify(EEditorNotifyEvent aEventId){}

private:
    class C3DConnexionDriverImpl* m_pImpl;
#ifdef KDAB_MAC_PORT
    PRAWINPUTDEVICELIST m_pRawInputDeviceList;
    PRAWINPUTDEVICE m_pRawInputDevices;
#endif // KDAB_MAC_PORT
    int m_nUsagePage1Usage8Devices;
    float m_fMultiplier;
};
#endif // CRYINCLUDE_EDITOR_UTIL_3DCONNEXIONDRIVER_H
