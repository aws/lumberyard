/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "TestTypes.h"

#include <AzCore/Math/VectorFloat.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/Debug/Timer.h>

#if 0
// \note g_numVectors and g_numVectors should be the same.
#if defined(AZ_PLATFORM_WINDOWS) && !defined(_DEBUG)
const int g_numTransforms = 1000;
const int g_numVectors = 100000;
const int g_numVectorFloats = 100000;
const int g_numFloats = g_numVectorFloats;
#else
const int g_numTransforms = 100;
const int g_numVectors = 10000;
const int g_numVectorFloats = 10000;
const int g_numFloats = g_numVectorFloats;
#endif

Transform* g_inTransforms = 0;
Transform* g_outTransforms = 0;
Vector3* g_inVectors = 0;
Vector3* g_outVectors = 0;
VectorFloat* g_inVectorFloats = 0;
VectorFloat* g_outVectorFloats = 0;
float* g_inFloats = 0;
float* g_outFloats = 0;

#include <stdlib.h>     // used for rand
//=========================================================================
// VectorFloatPerformance
// [6/5/2008]
//=========================================================================
void VectorFloatPerformance()
{
    Debug::Timer timer;
    //////////////////////////////////////////////////////////////////////////
    // Sin
    {
        VectorFloat sin;
        timer.Stamp();
        for (int i = 0; i < g_numVectorFloats; ++i)
        {
            VectorFloat angle(DegToRad(static_cast<float>(rand() % 360)));
            sin = angle.GetSin();
        }
        AZ_Printf("UnitTest", "VectorFloat::Sin %d operations: %0.4f sec\n", g_numVectorFloats, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Sin Cos Tests
    {
        VectorFloat sin, cos;
        timer.Stamp();
        for (int i = 0; i < g_numVectorFloats; ++i)
        {
            VectorFloat angle(DegToRad(static_cast<float>(rand() % 360)));
            angle.GetSinCos(sin, cos);
        }
        AZ_Printf("UnitTest", "VectorFloat::SinCos %d operations: %0.4f sec\n", g_numVectorFloats, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Reciprocal
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectorFloats; ++i)
        {
            g_outVectorFloats[i] = g_inVectorFloats[i].GetReciprocal();
        }
        AZ_Printf("UnitTest", "VectorFloat::GetReciprocal %d operations: %0.4f sec\n", g_numVectorFloats, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectorFloats; ++i)
        {
            g_outVectorFloats[i] = g_inVectorFloats[i].GetSqrtReciprocal();
        }
        AZ_Printf("UnitTest", "VectorFloat::GetSqrtReciprocal %d operations: %0.4f sec\n", g_numVectorFloats, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Store/load
    {
        timer.Stamp();
        for (int i = 0; i < g_numFloats; ++i)
        {
            //g_outVectorFloats[i] = VectorFloat::CreateFromFloat(&g_inFloats[i]);
            g_outVectorFloats[i] = g_inFloats[i];
        }
        AZ_Printf("UnitTest", "VectorFloat::LoadFloat %d operations: %0.4f sec\n", g_numFloats, timer.GetDeltaTimeInSeconds());
    }
    {
        timer.Stamp();
        for (int i = 0; i < g_numFloats; ++i)
        {
            //g_outFloats[i] = g_inVectorFloats[i];
            g_inVectorFloats[i].StoreToFloat(&g_outFloats[i]);
        }
        AZ_Printf("UnitTest", "VectorFloat::StoreFloat %d operations: %0.4f sec\n", g_numFloats, timer.GetDeltaTimeInSeconds());
    }
    {
        timer.Stamp();
        for (int i = 0; i < g_numFloats - 1; ++i)
        {
            g_outVectorFloats[i] = g_inFloats[i] + g_inFloats[i + 1]; //add ensures result is in a register
        }
        AZ_Printf("UnitTest", "VectorFloat::LoadFloatRegister %d operations: %0.4f sec\n", g_numFloats, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////
}

//=========================================================================
// Vector3Performance
// [6/5/2008]
//=========================================================================
void Vector3Performance()
{
    Debug::Timer timer;

    // We will start with the most expensive and often used functions

    //////////////////////////////////////////////////////////////////////////
    // Dot
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectorFloats[i] = vec.Dot(vec);
        }
        AZ_Printf("UnitTest", "Vector3::Dot %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Cross
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec.Cross(vec);
        }
        AZ_Printf("UnitTest", "Vector3::Cross %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Normalize
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec.GetNormalized();
        }
        AZ_Printf("UnitTest", "Vector3::GetNormalized %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            Vector3 vec = g_inVectors[i];
            g_outVectorFloats[i] = vec.NormalizeWithLength();
        }
        AZ_Printf("UnitTest", "Vector3::NormalizeWithLength %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // GetLength
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectorFloats[i] = vec.GetLength();
        }
        AZ_Printf("UnitTest", "Vector3::GetLength %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectorFloats[i] = vec.GetLengthReciprocal();
        }
        AZ_Printf("UnitTest", "Vector3::GetLengthReciprocal %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // operators

    // operator -
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = -vec;
        }
        AZ_Printf("UnitTest", "Vector3::operator neg %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec + vec;
        }
        AZ_Printf("UnitTest", "Vector3::operator + %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec - vec;
        }
        AZ_Printf("UnitTest", "Vector3::operator - %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec * vec;
        }
        AZ_Printf("UnitTest", "Vector3::operator *(Vector3) %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec / vec;
        }
        AZ_Printf("UnitTest", "Vector3::operator /(Vector3) %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    VectorFloat factor(2.0f);
    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec * factor;
        }
        AZ_Printf("UnitTest", "Vector3::operator *(VectorFloat) %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            const Vector3& vec = g_inVectors[i];
            g_outVectors[i] = vec / factor;
        }
        AZ_Printf("UnitTest", "Vector3::operator /(VectorFloat) %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    {
        timer.Stamp();
        for (int i = 0; i < g_numVectors; ++i)
        {
            g_outVectors[i] = g_inVectors[i].GetAbs();
        }
        AZ_Printf("UnitTest", "Vector3::GetAbs %d operations: %0.4f sec\n", g_numVectors, timer.GetDeltaTimeInSeconds());
    }
}

//=========================================================================
// TransformPerformance
// [6/27/2008]
//=========================================================================
void TransformPerformance()
{
    Debug::Timer timer;
    //////////////////////////////////////////////////////////////////////////
    // Mult
    {
        timer.Stamp();
        for (int i = 0; i < g_numTransforms; ++i)
        {
            const Transform& tm = g_inTransforms[i];
            g_outTransforms[i] = tm * tm;
        }
        AZ_Printf("UnitTest", "Transform::operator*(Transform&) %d operations: %0.4f sec\n", g_numTransforms, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Inverse
    {
        timer.Stamp();
        for (int i = 0; i < g_numTransforms; ++i)
        {
            g_outTransforms[i] = g_inTransforms[i].GetInverseFast();
        }
        AZ_Printf("UnitTest", "Transform::GetInverse %d operations: %0.4f sec\n", g_numTransforms, timer.GetDeltaTimeInSeconds());
    }
    //////////////////////////////////////////////////////////////////////////
}


//=========================================================================
// PerformanceTests
// [6/5/2008]
//=========================================================================
void PerformanceTests()
{
    //////////////////////////////////////////////////////////////////////////
    // Init used vectors
    g_inTransforms = reinterpret_cast<Transform*>(AZMemAlloc(sizeof(Transform) * g_numTransforms, AZStd::alignment_of<Transform>::value));
    g_outTransforms = reinterpret_cast<Transform*>(AZMemAlloc(sizeof(Transform) * g_numTransforms, AZStd::alignment_of<Transform>::value));

    g_inVectors = reinterpret_cast<Vector3*>(AZMemAlloc(sizeof(Vector3) * g_numVectors, AZStd::alignment_of<Vector3>::value));
    g_outVectors = reinterpret_cast<Vector3*>(AZMemAlloc(sizeof(Vector3) * g_numVectors, AZStd::alignment_of<Vector3>::value));

    g_inVectorFloats = reinterpret_cast<VectorFloat*>(AZMemAlloc(sizeof(VectorFloat) * g_numVectorFloats, AZStd::alignment_of<VectorFloat>::value));
    g_outVectorFloats = reinterpret_cast<VectorFloat*>(AZMemAlloc(sizeof(VectorFloat) * g_numVectorFloats, AZStd::alignment_of<VectorFloat>::value));

    g_inFloats = reinterpret_cast<float*>(AZMemAlloc(sizeof(float) * g_numFloats, AZStd::alignment_of<float>::value));
    g_outFloats = reinterpret_cast<float*>(AZMemAlloc(sizeof(float) * g_numFloats, AZStd::alignment_of<float>::value));

    for (int i = 0; i < g_numTransforms; ++i)
    {
        g_inTransforms[i] = Transform::CreateRotationX(DegToRad(static_cast<float>(rand() % 360))) * Transform::CreateRotationY(DegToRad(static_cast<float>(rand() % 360)))
            * Transform::CreateRotationZ(DegToRad(static_cast<float>(rand() % 360)));
    }
    for (int i = 0; i < g_numVectors; ++i)
    {
        g_inVectors[i].Set(static_cast<float>(rand() % 100), static_cast<float>(rand() % 100), static_cast<float>(rand() % 100));
    }

    for (int i = 0; i < g_numVectorFloats; ++i)
    {
        g_inVectorFloats[i] = static_cast<float>(rand() % 100);
    }

    for (int i = 0; i < g_numFloats; ++i)
    {
        g_inFloats[i] = static_cast<float>(rand() % 100);
    }
    //////////////////////////////////////////////////////////////////////////

    VectorFloatPerformance();

    Vector3Performance();

    TransformPerformance();

    AZFree(g_inTransforms);
    AZFree(g_outTransforms);
    AZFree(g_inVectors);
    AZFree(g_outVectors);
    AZFree(g_inVectorFloats);
    AZFree(g_outVectorFloats);
}
#endif
