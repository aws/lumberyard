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

#include "CryLegacy_precompiled.h"
#include "InputCVars.h"
#include <IConsole.h>
#include <ISystem.h>

CInputCVars* g_pInputCVars = 0;

CInputCVars::CInputCVars()
{
    REGISTER_CVAR(i_debug, 0, 0,
        "Toggles input event debugging.\n"
        "Usage: i_debug [0/1]\n"
        "Default is 0 (off). Set to 1 to spam console with key events (only press and release).");

    REGISTER_CVAR(i_debugDigitalButtons, 0, 0,
        "render controller's digital button pressed info\n"
        "Usage: 0 (off), 1(on)"
        "Default is 0. Value must be >=0");

#if defined(WIN32) || defined(WIN64)
    REGISTER_CVAR(i_kinectXboxConnect, 1, 0, // ACCEPTED_USE
        "Allow connection to Xbox for Kinect input.\n" // ACCEPTED_USE
        "Usage: 0 = disabled, 1 = allow connection, 2 = force connection (don't try local Kinect on system) \n"
        "Default is 1");
    REGISTER_CVAR(i_kinectXboxConnectPort, 62455, 0, // ACCEPTED_USE
        "Port used to connect to Xbox for Kinect input.\n" // ACCEPTED_USE
        "Default is 62455 (random value different than remote compiler)");
    i_kinectXboxConnectIP = REGISTER_STRING("i_kinectXboxConnectIP", "", VF_NULL, // ACCEPTED_USE
            "Usage: force set a specific Xbox Game IP to connect to for Kinect input (use default xbox from neighbourhood when empty) \n" // ACCEPTED_USE
            "Default is empty ");
#endif

    REGISTER_CVAR(i_useKinect, 1, 0,
        "Use Kinect");
    REGISTER_CVAR(i_kinSkeletonSmoothType, 1, 0,
        "Kinect skeleton tracking smooth type: 0 = not smoothed, 1 = using Double Exponential Smoothing");
    REGISTER_CVAR(i_kinectDebug, 1, 0,
        "1 = Draws skeleton on screen \n2 = Draws colour + depth buffers on screen \n3 = Draws both skeleton and buffers");
    REGISTER_CVAR(i_seatedTracking, 1, 0,
        "1 = Enable seated skeleton tracking");
    REGISTER_CVAR(i_kinSkeletonMovedDistance, 0.125f, 0,
        "Distance used to determine if kinect skeleton has moved in the play space.");


    //Exponential Smoothing Factors
    REGISTER_CVAR(i_kinGlobalExpSmoothFactor, 0.5f, 0, "Smoothing factor for double exponential smoothing.");
    REGISTER_CVAR(i_kinGlobalExpCorrectionFactor, 0.5f, 0, "Trend correction factor for double exponential smoothing.");
    REGISTER_CVAR(i_kinGlobalExpPredictionFactor, 0.5f, 0, "Prediction factor for double exponential smoothing.");
    REGISTER_CVAR(i_kinGlobalExpJitterRadius, 0.05f, 0, "Radius to determine jitter correction for double exponential smoothing.");
    REGISTER_CVAR(i_kinGlobalExpDeviationRadius, 0.04f, 0, "Maximum deviation radius from prediction for double exponential smoothing.");

    i_synergyServer = REGISTER_STRING("i_synergyServer", "", VF_NULL,
            "Usage: Set the host ip or hostname for the synergy client to connect to\n"
            "Default is empty ");
    i_synergyScreenName = REGISTER_STRING("i_synergyScreenName", "Console", VF_NULL,
            "Usage: Set a screen name for this target\n"
            "Default is \"Console\"");
}

CInputCVars::~CInputCVars()
{
    gEnv->pConsole->UnregisterVariable("i_debug");
    gEnv->pConsole->UnregisterVariable("i_debugDigitalButtons");

#if defined(WIN32) || defined(WIN64)
    gEnv->pConsole->UnregisterVariable("i_kinectXboxConnect"); // ACCEPTED_USE
    gEnv->pConsole->UnregisterVariable("i_kinectXboxConnectPort"); // ACCEPTED_USE
    gEnv->pConsole->UnregisterVariable("i_kinectXboxConnectIP"); // ACCEPTED_USE
#endif

    gEnv->pConsole->UnregisterVariable("i_kinSkeletonSmoothType");

    gEnv->pConsole->UnregisterVariable("i_seatedTracking");

    gEnv->pConsole->UnregisterVariable("i_kinGlobalExpSmoothFactor");
    gEnv->pConsole->UnregisterVariable("i_kinGlobalExpCorrectionFactor");
    gEnv->pConsole->UnregisterVariable("i_kinGlobalExpPredictionFactor");
    gEnv->pConsole->UnregisterVariable("i_kinGlobalExpJitterRadius");
    gEnv->pConsole->UnregisterVariable("i_kinGlobalExpDeviationRadius");

}
