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

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Class for drawing test displays for testing the LyShine functionality
//
//! This is currently implemented as console variables and commands
class LyShineDebug
{
public: // static member functions

    //! Initialize debug vars
    static void Initialize();

    //! This is called when the game terminates
    static void Reset();

    //! Do the debug render
    static void RenderDebug();

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dFont, 0);

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dImage, 0);

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dLine, 0);

    DeclareStaticConstIntCVar(CV_r_DebugUIDraw2dDefer, 0);
};