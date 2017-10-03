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

#include "FirstPersonCamera.h"


namespace MCommon
{
    // constructor
    FirstPersonCamera::FirstPersonCamera()
        : Camera()
    {
        Reset();
    }


    // destructor
    FirstPersonCamera::~FirstPersonCamera()
    {
    }


    // update the camera position, orientation and it's matrices
    void FirstPersonCamera::Update(float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        // lock pitching to [-90.0�, 90.0�]
        if (mPitch < -90.0f + 0.1f)
        {
            mPitch = -90.0f + 0.1f;
        }
        if (mPitch >  90.0f - 0.1f)
        {
            mPitch =  90.0f - 0.1f;
        }

        // calculate the camera direction vector based on the yaw and pitch
        MCore::Vector3 direction = (MCore::Vector3(0.0f, 0.0f, 1.0f) * (MCore::Matrix::RotationMatrixX(MCore::Math::DegreesToRadians(mPitch)) * MCore::Matrix::RotationMatrixY(MCore::Math::DegreesToRadians(mYaw)))).Normalized();

        // look from the camera position into the newly calculated direction
        mViewMatrix.LookAt(mPosition, mPosition + direction * 10.0f, MCore::Vector3(0.0f, 1.0f, 0.0f));

        // update our base camera
        Camera::Update();
    }


    // process input and modify the camera attributes
    void FirstPersonCamera::ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(rightButtonPressed);
        MCORE_UNUSED(middleButtonPressed);
        MCORE_UNUSED(leftButtonPressed);


        EKeyboardButtonState buttonState = (EKeyboardButtonState)keyboardKeyFlags;

        MCore::Matrix transposedViewMatrix(mViewMatrix);
        transposedViewMatrix.Transpose();

        // get the movement direction vector based on the keyboard input
        MCore::Vector3 deltaMovement(0.0f, 0.0f, 0.0f);
        if (buttonState & FORWARD)
        {
            deltaMovement += transposedViewMatrix.GetForward();
        }
        if (buttonState & BACKWARD)
        {
            deltaMovement -= transposedViewMatrix.GetForward();
        }
        if (buttonState & RIGHT)
        {
            deltaMovement += transposedViewMatrix.GetRight();
        }
        if (buttonState & LEFT)
        {
            deltaMovement -= transposedViewMatrix.GetRight();
        }
        if (buttonState & UP)
        {
            deltaMovement += transposedViewMatrix.GetUp();
        }
        if (buttonState & DOWN)
        {
            deltaMovement -= transposedViewMatrix.GetUp();
        }

        // only move the camera when the delta movement is not the zero vector
        if (deltaMovement.SafeLength() > MCore::Math::epsilon)
        {
            mPosition += deltaMovement.Normalized() * mTranslationSpeed;
        }

        // rotate the camera
        if (buttonState & ENABLE_MOUSELOOK)
        {
            mYaw    += mouseMovementX * mRotationSpeed;
            mPitch  += mouseMovementY * mRotationSpeed;
        }
    }


    // reset the camera attributes
    void FirstPersonCamera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        // reset the base class attributes
        Camera::Reset();

        mPitch  = 0.0f;
        mYaw    = 0.0f;
        mRoll   = 0.0f;
    }
} // namespace MCommon
