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

//  include required headers
#include "StandardHeaders.h"
#include "Matrix4.h"
#include "Vector.h"
#include "Quaternion.h"


namespace MCore
{
    /**
     * The matrix decomposer.
     * The decomposer uses polar decomposition as described by Ken Schoemake. Most of the internal code is also based on the code he created.
     * This can decompose affine matrices into the following components:
     * Translation, Rotation, Scale and Scale/Stretch Rotation, as well as the sign of the determinant.
     */
    class MCORE_API MatrixDecomposer
    {
        typedef float HMatrix[4][4]; // column vectors, so the transpose of standard core matrices

    public:
        /**
         * The constructor.
         */
        MatrixDecomposer();

        /**
         * The extended constructor that automatically decomposes a specified matrix.
         * This automatically calls the InitFromMatrix method.
         * @param inMatrix The matrix to decompose.
         */
        MatrixDecomposer(const MCore::Matrix& inMatrix);

        /**
         * Perform the decomposition of a given matrix.
         * @param inMatrix The matrix to decompose. After calling this method, the functions like GetRotation and GetScale
         *                 etc will return the decomposed values for this matrix.
         */
        void InitFromMatrix(const MCore::Matrix& inMatrix);

        /**
         * Converts the decomposed parts (pos/rot/scale/scalerot) back into a matrix.
         * You can also use the Matrix::SetMatrix() method with these values to build one.
         * @param outMatrix The matrix that will be initialized from the decomposed values.
         */
        void ToMatrix(MCore::Matrix* outMatrix) const;

        /**
         * Converts the decomposed parts back into a matrix.
         * This does the same as the other ToMatrix method, except that it will return a new matrix.
         * @result The matrix that was composed from the stored decomposed parts.
         */
        MCore::Matrix ToMatrix() const;

        /**
         * Get the rotation stored in the matrix that we decomposed.
         * @result The quaternion representing the rotation.
         */
        MCORE_INLINE const MCore::Quaternion& GetRotation() const           { return mRotation; }

        /**
         * Get the scale rotation stored in the matrix that we decomposed.
         * This represents the space in which scaling is performed.
         * @result The quaternion representing the scale rotation.
         */
        MCORE_INLINE const MCore::Quaternion& GetScaleRotation() const      { return mScaleRotation; }

        /**
         * Get the scale for each axis that is stored in the matrix that we decomposed.
         * Please note that the scaling has to happen in the space as defined by the scale rotation.
         * @result The scale for each axis.
         */
        MCORE_INLINE const MCore::Vector3& GetScale() const                 { return mScale; }

        /**
         * Get the translation stored in the matrix that we decomposed.
         * @result The translation.
         */
        MCORE_INLINE const MCore::Vector3& GetTranslation() const           { return mTranslation; }

        /**
         * Get the sign of the matrix determinant, which is either -1 or +1.
         * @result Returns -1 in case of a negative determinant or +1 in case of a positive determinant.
         */
        MCORE_INLINE float GetDeterminantSign() const                       { return mDeterminantSign; }


    private:
        MCore::Quaternion   mRotation;          /**< The rotation stored in the matrix. */
        MCore::Quaternion   mScaleRotation;     /**< The scale rotation (space in which to scale). */
        MCore::Vector3      mScale;             /**< The scale factor for each axis. */
        MCore::Vector3      mTranslation;       /**< The translation. */
        float               mDeterminantSign;   /**< The sign of the determinant (-1 or +1). */
        static HMatrix      IdentityMatrix;     /**< An identity matrix, used internally. */

        MCORE_INLINE float Sign(unsigned char n, float v)               { return (n) ? -(v) : (v); }
        MCORE_INLINE float NormInf(HMatrix M)                           { return MatNorm(M, 0); }
        MCORE_INLINE float NormOne(HMatrix M)                           { return MatNorm(M, 1); }
        MCORE_INLINE void Swap(float* a, uint32 i, uint32 j)            { a[3] = a[i]; a[i] = a[j]; a[j] = a[3];}
        MCORE_INLINE void MatPad(HMatrix A)                             { A[3][0] = A[0][3] = A[3][1] = A[1][3] = A[3][2] = A[2][3] = 0.0f; A[3][3] = 1.0f; }

        AZ::Vector4 SpectralDecomp(HMatrix S, HMatrix U);
        AZ::Vector4 Snuggle(AZ::Vector4 q, AZ::Vector4* k);
        AZ::Vector4 QuatFromMatrix(HMatrix mat);
        void DecompAffine(HMatrix inMatrix);
        void Cycle(float* input, bool p);
        void DoRank1(HMatrix M, HMatrix Q);
        void DoRank2(HMatrix M, HMatrix MadjT, HMatrix Q);
        void MakeReflector(float* v, float* u);
        void ReflectCols(HMatrix M, float* u);
        void ReflectRows(HMatrix M, float* u);
        float MatNorm(HMatrix M, int tpose);
        float PolarDecomp(HMatrix M, HMatrix Q, HMatrix S);
        int FindMaxCol(HMatrix M);

        // return conjugate of quaternion
        MCORE_INLINE AZ::Vector4 QuatConj(const AZ::Vector4& q)
        {
            return AZ::Vector4(-q.GetX(), -q.GetY(), -q.GetZ(), q.GetW());
        }

        MCORE_INLINE AZ::Vector4 QuatScale(const AZ::Vector4& q, float w)
        {
            return AZ::Vector4(q.GetX() * w, q.GetY() * w, q.GetZ() * w, q.GetW() * w);
        }

        MCORE_INLINE AZ::Vector4 QuatMul(const AZ::Vector4& qL, const AZ::Vector4& qR)
        {
            AZ::Vector4 qq;
            qq.SetW(qL.GetW() * qR.GetW() - qL.GetX() * qR.GetX() - qL.GetY() * qR.GetY() - qL.GetZ() * qR.GetZ());
            qq.SetX(qL.GetW() * qR.GetX() + qL.GetX() * qR.GetW() + qL.GetY() * qR.GetZ() - qL.GetZ() * qR.GetY());
            qq.SetY(qL.GetW() * qR.GetY() + qL.GetY() * qR.GetW() + qL.GetZ() * qR.GetX() - qL.GetX() * qR.GetZ());
            qq.SetZ(qL.GetW() * qR.GetZ() + qL.GetZ() * qR.GetW() + qL.GetX() * qR.GetY() - qL.GetY() * qR.GetX());
            return qq;
        }

        MCORE_INLINE void MatMult(HMatrix A, HMatrix B, HMatrix AB)
        {
            uint32 i, j;
            for (i = 0; i < 3; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    AB[i][j] = A[i][0] * B[0][j] + A[i][1] * B[1][j] + A[i][2] * B[2][j];
                }
            }
        }

        MCORE_INLINE float VDot(float* va, float* vb)
        {
            return (va[0] * vb[0] + va[1] * vb[1] + va[2] * vb[2]);
        }

        MCORE_INLINE void VCross(float* va, float* vb, float* v)
        {
            v[0] = va[1] * vb[2] - va[2] * vb[1];
            v[1] = va[2] * vb[0] - va[0] * vb[2];
            v[2] = va[0] * vb[1] - va[1] * vb[0];
        }

        MCORE_INLINE void AdjointTranspose(HMatrix M, HMatrix MadjT)
        {
            VCross(M[1], M[2], MadjT[0]);
            VCross(M[2], M[0], MadjT[1]);
            VCross(M[0], M[1], MadjT[2]);
        }
    };
}   // namespace MCore
