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

// EXPECTED macro is used in the situations where you want to have an assertion
// combine with a runtime check/action, that is done in all configurations.
//
// Examples of use:
//
//   EXPECTED(connect(button, SIGNAL(triggered()), this, SLOT(OnButtonTriggered())));
//
//   if (!EXPECTED(argument != nullptr))
//     return;
//
// This will break under the debugger, but still will performs check and calls in production build.

#define EXPECTED(x) ((x) || (ExpectedIsDebuggerPresent() && (__debugbreak(), true), false))

bool ExpectedIsDebuggerPresent();

