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

#ifndef CRYINCLUDE_CRYAISYSTEM_AIPIDCONTROLLER_H
#define CRYINCLUDE_CRYAISYSTEM_AIPIDCONTROLLER_H
#pragma once

//====================================================================
// CAIPIDController
//====================================================================
struct CAIPIDController
{
    /// CP, CI and CD are the three coefficients. integralTimescale is the time over which
    /// to accululate the integral term (approximate).
    /// proportionalPower indicates the power that the error term should be raised to
    CAIPIDController(float CP = 0.0f, float CI = 0.0f, float CD = 0.0f, float integralTimescale = 1.0f, unsigned proportionalPower = 1);

    /// Update the intenal state and calculate an output
    float Update(float error, float dt);

    /// The proportional, integral and derivative coefficients
    float CP, CI, CD;

    /// The time over which to accululate the integral
    float integralTimescale;

    /// output is proportional to error raised to this power
    unsigned proportionalPower;

    void Serialize(TSerialize ser);

private:
    float runningIntegral;
    float lastError;
};

//====================================================================
// Update
//====================================================================
inline float CAIPIDController::Update(float error, float dt)
{
    float frac = min(dt / integralTimescale, 1.0f);
    runningIntegral = frac * error + (1.0f - frac) * runningIntegral; // not quite dt independant...
    float output = CP * powf(error, (float) proportionalPower);
    output += CI * runningIntegral;
    if (dt > 0.0f)
    {
        output += CD * (error - lastError) / dt;
    }
    lastError = error;
    return output;
}

//====================================================================
// CAIPIDController
//====================================================================
inline CAIPIDController::CAIPIDController(float CP, float CI, float CD,
    float integralTimescale, unsigned proportionalPower)
    : CP(CP)
    , CI(CI)
    , CD(CD)
    , integralTimescale(integralTimescale)
    , proportionalPower(proportionalPower)
    , runningIntegral(0.0f)
    , lastError(0.0f)
{
}


#endif // CRYINCLUDE_CRYAISYSTEM_AIPIDCONTROLLER_H
