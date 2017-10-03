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

/*
 * Most of the algorithm code is made by Ken Shoemake. This code has been modified and turned into this class.
 * The license allows to use his code, which was part of Graphics Gems.
 */

#include "MatrixDecomposer.h"


namespace MCore
{
    // the static variable identity matrix
    MatrixDecomposer::HMatrix MatrixDecomposer::IdentityMatrix = {
        {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}
    };


    // the default constructor
    MatrixDecomposer::MatrixDecomposer()
    {
        mTranslation.Zero();
        mScale.Set(1.0f, 1.0f, 1.0f);
        //mRotation.Identity();
        //mScaleRotation.Identity();
        mDeterminantSign = 1.0f;
    }


    // the extended constructor
    MatrixDecomposer::MatrixDecomposer(const MCore::Matrix& inMatrix)
    {
        InitFromMatrix(inMatrix);
    }


    // initialize from a matrix
    void MatrixDecomposer::InitFromMatrix(const MCore::Matrix& inMatrix)
    {
        // convert to the column major matrix
        HMatrix matrix;

#ifdef MCORE_MATRIX_ROWMAJOR
        MCore::MemCopy(matrix, inMatrix.Transposed().m16, sizeof(Matrix));
#else
        MCore::MemCopy(matrix, inMatrix.m16, sizeof(Matrix));
#endif

        // decompose the matrix
        DecompAffine(matrix);

        // do some cleanup:
        //if scale is close/equal to identity, scaleRotation may be ignored as well
        if ((mScale - Vector3(1, 1, 1)).SquareLength() <=  Math::epsilon)
        {
            // wipe them clean
            mScaleRotation.Identity();
            mScale.Set(1, 1, 1);
        }
        else
        {
            //if scale.x=scale.y=scale.z, scaleRotation may be ignored as well:
            //   in that case: SR*Scale*inv(SR) = Scale*SR*inv(SR) = Scale
            float delta = mScale.x - mScale.y;
            if ((delta * delta) <= Math::epsilon)
            {
                delta = mScale.x - mScale.z;
                if ((delta * delta) <= Math::epsilon)
                {
                    mScaleRotation.Identity();
                }
            }
        }
    }

    // construct a matrix from the decomposed parts again
    void MatrixDecomposer::ToMatrix(MCore::Matrix* outMatrix) const
    {
        MCORE_ASSERT(outMatrix);
        outMatrix->InitFromPosRotScaleScaleRot(mTranslation, mRotation, mScale, mScaleRotation);
    }


    // return a matrix from the current decomposed values
    MCore::Matrix MatrixDecomposer::ToMatrix() const
    {
        Matrix result; // TODO: create a Matrix constructor which takes these parts
        result.InitFromPosRotScaleScaleRot(mTranslation, mRotation, mScale, mScaleRotation);
        return result;
    }


    //------------------------------------------------------------------------------

    // copy nxn matrix A to C using "gets" for assignment
#define MCORE_MATDECOMP_MATCOPY(C, gets, A, n) { for (int i = 0; i < n; i++) {for (int j = 0; j < n; j++) { \
                                                                                  C[i][j] gets (A[i][j]); } \
                                                 }                                                          \
}

    // copy transpose of nxn matrix A to C using "gets" for assignment
#define MCORE_MATDECOMP_MATTPOSE(AT, gets, A, n) { for (int i = 0; i < n; i++) {for (int j = 0; j < n; j++) {  \
                                                                                    AT[i][j] gets (A[j][i]); } \
                                                   }                                                           \
}

    // assign nxn matrix C the element-wise combination of A and B using "op"
#define MCORE_MATDECOMP_MATBINOP(C, gets, A, op, B, n) { for (int i = 0; i < n; i++) {for (int j = 0; j < n; j++) {              \
                                                                                          C[i][j] gets (A[i][j]) op (B[i][j]); } \
                                                         }                                                                       \
}

#define MCORE_MATDECOMP_CASEMACRO(i, j, k, I, J, K)                    \
case I:                                                                \
    s = Math::Sqrt((mat[I][I] - (mat[J][J] + mat[K][K])) + mat[3][3]); \
    qu.SetElement(i, s * 0.5f);                                        \
    s = 0.5f / s;                                                      \
    qu.SetElement(j, (mat[I][J] + mat[J][I]) * s);                     \
    qu.SetElement(k, (mat[K][I] + mat[I][K]) * s);                     \
    qu.SetW((mat[K][J] - mat[J][K]) * s);                              \
    break


    //------------------------------------------------------------------------------

    // Construct a unit quaternion from rotation matrix.  Assumes matrix is
    // used to multiply column vector on the left: vnew = mat vold.  Works
    // correctly for right-handed coordinate system and right-handed rotations.
    // Translation and perspective components ignored.
    AZ::Vector4 MatrixDecomposer::QuatFromMatrix(HMatrix mat)
    {
        // This algorithm avoids near-zero divides by looking for a large component
        // - first w, then x, y, or z.  When the trace is greater than zero,
        // |w| is greater than 1/2, which is as small as a largest component can be.
        // Otherwise, the largest diagonal entry corresponds to the largest of |x|,
        // |y|, or |z|, one of which must be larger than |w|, and at least 1/2.
        AZ::Vector4 qu = AZ::Vector4::CreateZero();
        float tr, s;

        tr = mat[0][0] + mat[1][1] + mat[2][2];
        if (tr >= 0.0f)
        {
            s = MCore::Math::Sqrt(tr + mat[3][3]);
            qu.SetW(s * 0.5f);
            s = 0.5f / s;
            qu.SetX((mat[2][1] - mat[1][2]) * s);
            qu.SetY((mat[0][2] - mat[2][0]) * s);
            qu.SetZ((mat[1][0] - mat[0][1]) * s);
        }
        else
        {
            int h = 0;
            if (mat[1][1] > mat[0][0])
            {
                h = 1;
            }
            if (mat[2][2] > mat[h][h])
            {
                h = 2;
            }
            switch (h)
            {
                MCORE_MATDECOMP_CASEMACRO(0, 1, 2, 0, 1, 2);
                MCORE_MATDECOMP_CASEMACRO(1, 2, 0, 1, 2, 0);
                MCORE_MATDECOMP_CASEMACRO(2, 0, 1, 2, 0, 1);
            }
        }

        if (MCore::Math::IsFloatEqual(mat[3][3], 1.0f) == false)
        {
            qu = QuatScale(qu, 1.0f / MCore::Math::Sqrt(mat[3][3]));
        }

        return (qu);
    }


    // Compute either the 1 or infinity norm of M, depending on tpose
    float MatrixDecomposer::MatNorm(HMatrix M, int tpose)
    {
        int i;
        float sum;
        float max = 0.0f;
        for (i = 0; i < 3; i++)
        {
            if (tpose)
            {
                sum = MCore::Math::Abs(M[0][i]) + MCore::Math::Abs(M[1][i]) + MCore::Math::Abs(M[2][i]);
            }
            else
            {
                sum = MCore::Math::Abs(M[i][0]) + MCore::Math::Abs(M[i][1]) + MCore::Math::Abs(M[i][2]);
            }

            if (max < sum)
            {
                max = sum;
            }
        }

        return max;
    }


    // return index of column of M containing maximum abs entry, or -1 if M=0
    int MatrixDecomposer::FindMaxCol(HMatrix M)
    {
        float abs, max;
        int i, j, col;
        max = 0.0f;
        col = -1;

        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 3; j++)
            {
                abs = M[i][j];
                if (abs < 0.0f)
                {
                    abs = -abs;
                }

                if (abs > max)
                {
                    max = abs;
                    col = j;
                }
            }
        }

        return col;
    }


    // setup u for Household reflection to zero all v components but first
    void MatrixDecomposer::MakeReflector(float* v, float* u)
    {
        float s = MCore::Math::Sqrt(VDot(v, v));
        u[0] = v[0];
        u[1] = v[1];
        u[2] = v[2] + ((v[2] < 0.0f) ? -s : s);
        s = MCore::Math::Sqrt(2.0f / VDot(u, u));
        u[0] = u[0] * s;
        u[1] = u[1] * s;
        u[2] = u[2] * s;
    }


    // apply Householder reflection represented by u to column vectors of M
    void MatrixDecomposer::ReflectCols(HMatrix M, float* u)
    {
        int i, j;
        for (i = 0; i < 3; i++)
        {
            const float s = u[0] * M[0][i] + u[1] * M[1][i] + u[2] * M[2][i];
            for (j = 0; j < 3; j++)
            {
                M[j][i] -= u[j] * s;
            }
        }
    }


    // apply Householder reflection represented by u to row vectors of M
    void MatrixDecomposer::ReflectRows(HMatrix M, float* u)
    {
        int i, j;
        for (i = 0; i < 3; i++)
        {
            float s = VDot(u, M[i]);
            for (j = 0; j < 3; j++)
            {
                M[i][j] -= u[j] * s;
            }
        }
    }


    // find orthogonal factor Q of rank 1 (or less) M
    void MatrixDecomposer::DoRank1(HMatrix M, HMatrix Q)
    {
        float v1[3], v2[3], s;
        int col;
        MCORE_MATDECOMP_MATCOPY(Q, =, IdentityMatrix, 4);

        // if rank(M) is 1, we should find a non-zero column in M
        col = FindMaxCol(M);
        if (col < 0)
        {
            return;
        }

        v1[0] = M[0][col];
        v1[1] = M[1][col];
        v1[2] = M[2][col];
        MakeReflector(v1, v1);
        ReflectCols(M, v1);

        v2[0] = M[2][0];
        v2[1] = M[2][1];
        v2[2] = M[2][2];
        MakeReflector(v2, v2);
        ReflectRows(M, v2);

        s = M[2][2];
        if (s < 0.0f)
        {
            Q[2][2] = -1.0f;
        }

        ReflectCols(Q, v1);
        ReflectRows(Q, v2);
    }


    // find orthogonal factor Q of rank 2 (or less) M using adjoint transpose
    void MatrixDecomposer::DoRank2(HMatrix M, HMatrix MadjT, HMatrix Q)
    {
        float v1[3], v2[3];
        float w, x, y, z, c, s, d;
        int col;

        // if rank(M) is 2, we should find a non-zero column in MadjT
        col = FindMaxCol(MadjT);
        if (col < 0)
        {
            DoRank1(M, Q);
            return;
        }

        v1[0] = MadjT[0][col];
        v1[1] = MadjT[1][col];
        v1[2] = MadjT[2][col];
        MakeReflector(v1, v1);
        ReflectCols(M, v1);
        VCross(M[0], M[1], v2);
        MakeReflector(v2, v2);
        ReflectRows(M, v2);

        w = M[0][0];
        x = M[0][1];
        y = M[1][0];
        z = M[1][1];

        if (w * z > x * y)
        {
            c = z + w;
            s = y - x;
            d = MCore::Math::Sqrt(c * c + s * s);
            c = c / d;
            s = s / d;
            Q[0][0] = Q[1][1] = c;
            Q[0][1] = -(Q[1][0] = s);
        }
        else
        {
            c = z - w;
            s = y + x;
            d = MCore::Math::Sqrt(c * c + s * s);
            c = c / d;
            s = s / d;
            Q[0][0] = -(Q[1][1] = c);
            Q[0][1] = Q[1][0] = s;
        }

        Q[0][2] = Q[2][0] = Q[1][2] = Q[2][1] = 0.0;
        Q[2][2] = 1.0;
        ReflectCols(Q, v1);
        ReflectRows(Q, v2);
    }


    // Polar Decomposition of 3x3 matrix in 4x4,
    // M = QS.  See Nicholas Higham and Robert S. Schreiber,
    // Fast Polar Decomposition of An Arbitrary Matrix,
    // Technical Report 88-942, October 1988,
    // Department of Computer Science, Cornell University.
    float MatrixDecomposer::PolarDecomp(HMatrix M, HMatrix Q, HMatrix S)
    {
        HMatrix Mk, MadjTk, Ek;
        float det, M_one, M_inf, MadjT_one, MadjT_inf, E_one, gamma, g1, g2;

        MCORE_MATDECOMP_MATTPOSE(Mk, =, M, 3);
        M_one = NormOne(Mk);
        M_inf = NormInf(Mk);

        do
        {
            AdjointTranspose(Mk, MadjTk);
            det = VDot(Mk[0], MadjTk[0]);
            if (det > -Math::epsilon && det < Math::epsilon)
            {
                DoRank2(Mk, MadjTk, Mk);
                break;
            }
            MadjT_one = NormOne(MadjTk);
            MadjT_inf = NormInf(MadjTk);
            gamma = MCore::Math::Sqrt(MCore::Math::Sqrt((MadjT_one * MadjT_inf) / (M_one * M_inf)) / MCore::Math::Abs(det));
            g1 = gamma * 0.5f;
            g2 = 0.5f / (gamma * det);
            MCORE_MATDECOMP_MATCOPY(Ek, =, Mk, 3);
            MCORE_MATDECOMP_MATBINOP(Mk, =, g1 * Mk, +, g2 * MadjTk, 3);
            MCORE_MATDECOMP_MATCOPY(Ek, -=, Mk, 3);
            E_one = NormOne(Ek);
            M_one = NormOne(Mk);
            M_inf = NormInf(Mk);
        } while (E_one > (M_one * 1.0e-6));

        MCORE_MATDECOMP_MATTPOSE(Q, =, Mk, 3);
        MatPad(Q);
        MatMult(Mk, M, S);
        MatPad(S);

        for (int i = 0; i < 3; i++)
        {
            for (int j = i; j < 3; j++)
            {
                S[i][j] = S[j][i] = 0.5f * (S[i][j] + S[j][i]);
            }
        }

        return (det);
    }


    // Compute the spectral decomposition of symmetric positive semi-definite S.
    // Returns rotation in U and scale factors in result, so that if K is a diagonal
    // matrix of the scale factors, then S = U K (U transpose). Uses Jacobi method.
    // See Gene H. Golub and Charles F. Van Loan. Matrix Computations. Hopkins 1983.
    AZ::Vector4 MatrixDecomposer::SpectralDecomp(HMatrix S, HMatrix U)
    {
        AZ::Vector4 kv;
        float Diag[3], OffD[3];
        float g, h, fabsh, fabsOffDi, t, theta, c, s, tau, ta, OffDq, a, b;
        static char nxt[] = { 1, 2, 0 };

        MCORE_MATDECOMP_MATCOPY(U, =, IdentityMatrix, 4);
        Diag[0] = S[0][0];
        Diag[1] = S[1][1];
        Diag[2] = S[2][2];
        OffD[0] = S[1][2];
        OffD[1] = S[2][0];
        OffD[2] = S[0][1];

        for (int sweep = 20; sweep > 0; sweep--)
        {
            float sm = MCore::Math::Abs(OffD[0]) + MCore::Math::Abs(OffD[1]) + MCore::Math::Abs(OffD[2]);
            if (sm < MCore::Math::epsilon)
            {
                break;
            }

            for (int i = 2; i >= 0; i--)
            {
                int p = nxt[i];
                int q = nxt[p];
                fabsOffDi = MCore::Math::Abs(OffD[i]);
                g = 100.0f * fabsOffDi;

                if (fabsOffDi > 0.0f)
                {
                    h = Diag[q] - Diag[p];
                    fabsh = MCore::Math::Abs(h);
                    if (Math::IsFloatEqual(fabsh + g, fabsh))
                    {
                        if (fabsh <= MCore::Math::epsilon)
                        {
                            h = MCore::Math::epsilon;
                        }

                        t = OffD[i] / h;
                    }
                    else
                    {
                        theta = 0.5f * h / OffD[i];
                        float divider = (MCore::Math::Abs(theta) + MCore::Math::Sqrt(theta * theta + 1.0f));
                        if (divider < MCore::Math::epsilon)
                        {
                            divider = MCore::Math::epsilon;
                        }

                        t = 1.0f / divider;
                        if (theta < 0.0f)
                        {
                            t = -t;
                        }
                    }

                    c = 1.0f / MCore::Math::Sqrt(t * t + 1.0f);
                    s = t * c;
                    tau = s / (c + 1.0f);
                    ta = t * OffD[i];
                    OffD[i] = 0.0f;
                    Diag[p] -= ta;
                    Diag[q] += ta;
                    OffDq = OffD[q];
                    OffD[q] -= s * (OffD[p] + tau * OffD[q]);
                    OffD[p] += s * (OffDq   - tau * OffD[p]);
                    for (int j = 2; j >= 0; j--)
                    {
                        a = U[j][p];
                        b = U[j][q];
                        U[j][p] -= s * (b + tau * a);
                        U[j][q] += s * (a - tau * b);
                    }
                }
            }
        }

        kv.SetX(Diag[0]);
        kv.SetY(Diag[1]);
        kv.SetZ(Diag[2]);
        kv.SetW(1.0f);

        return kv;
    }


    //
    void MatrixDecomposer::Cycle(float* input, bool p)
    {
        if (p)
        {
            input[3] = input[0];
            input[0] = input[1];
            input[1] = input[2];
            input[2] = input[3];
        }
        else
        {
            input[3] = input[2];
            input[2] = input[1];
            input[1] = input[0];
            input[0] = input[3];
        }
    }


    // Given a unit quaternion, q, and a scale vector, k, find a unit quaternion, p,
    // which permutes the axes and turns freely in the plane of duplicate scale
    // factors, such that q p has the largest possible w component, i.e. the
    // smallest possible angle. Permutes k's components to go with q p instead of q.
    // See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
    // Proceedings of Graphics Interface 1992. Details on p. 262-263.
    AZ::Vector4 MatrixDecomposer::Snuggle(AZ::Vector4 q, AZ::Vector4* k)
    {
        AZ::Vector4 p;
        p.SetX(0.0f); // prevent compiler warning about unreferenced local variable p

        float ka[4];
        int turn = -1;
        ka[0] = k->GetX();
        ka[1] = k->GetY();
        ka[2] = k->GetZ();

        if (Math::IsFloatEqual(ka[0], ka[1]))
        {
            if (Math::IsFloatEqual(ka[0], ka[2]))
            {
                turn = 3;
            }
            else
            {
                turn = 2;
            }
        }
        else
        {
            if (Math::IsFloatEqual(ka[0], ka[2]))
            {
                turn = 1;
            }
            else
            if (Math::IsFloatEqual(ka[1], ka[2]))
            {
                turn = 0;
            }
        }

        if (turn >= 0)
        {
            AZ::Vector4 qtoz, qp;
            unsigned neg[3], win;
            float mag[3], t;
            static AZ::Vector4 qxtoz(0, Math::sqrtHalf, 0, Math::sqrtHalf);
            static AZ::Vector4 qytoz(Math::sqrtHalf, 0, 0, Math::sqrtHalf);
            static AZ::Vector4 qppmm(0.5f, 0.5f, -0.5f, -0.5f);
            static AZ::Vector4 qpppp(0.5f, 0.5f, 0.5f, 0.5f);
            static AZ::Vector4 qmpmm(-0.5f, 0.5f, -0.5f, -0.5f);
            static AZ::Vector4 qpppm(0.5f, 0.5f, 0.5f, -0.5f);
            static AZ::Vector4 q0001(0.0f, 0.0f, 0.0f, 1.0f);
            static AZ::Vector4 q1000(1.0f, 0.0f, 0.0f, 0.0f);
            switch (turn)
            {
            default:
                return (QuatConj(q));
            case 0:
                q = QuatMul(q, qtoz = qxtoz);
                Swap(ka, 0, 2);
                break;
            case 1:
                q = QuatMul(q, qtoz = qytoz);
                Swap(ka, 1, 2);
                break;
            case 2:
                qtoz = q0001;
                break;
            }

            q = QuatConj(q);
            mag[0] = (float)q.GetZ() * q.GetZ() + (float)q.GetW() * q.GetW() - 0.5f;
            mag[1] = (float)q.GetX() * q.GetZ() - (float)q.GetY() * q.GetW();
            mag[2] = (float)q.GetY() * q.GetZ() + (float)q.GetX() * q.GetW();
            for (int i = 0; i < 3; i++)
            {
                if ((bool)(neg[i] = (mag[i] < 0.0f)))
                {
                    mag[i] = -mag[i];
                }
            }

            if (mag[0] > mag[1])
            {
                if (mag[0] > mag[2])
                {
                    win = 0;
                }
                else
                {
                    win = 2;
                }
            }
            else
            {
                if (mag[1] > mag[2])
                {
                    win = 1;
                }
                else
                {
                    win = 2;
                }
            }

            switch (win)
            {
            case 0:
                if (neg[0])
                {
                    p = q1000;
                }
                else
                {
                    p = q0001;
                } break;
            case 1:
                if (neg[1])
                {
                    p = qppmm;
                }
                else
                {
                    p = qpppp;
                } Cycle(ka, 0);
                break;
            case 2:
                if (neg[2])
                {
                    p = qmpmm;
                }
                else
                {
                    p = qpppm;
                } Cycle(ka, 1);
                break;
            }

            qp = QuatMul(q, p);
            t = MCore::Math::Sqrt(mag[win] + 0.5f);
            p = QuatMul(p, AZ::Vector4(0.0f, 0.0f, -qp.GetZ() / t, qp.GetW() / t));
            p = QuatMul(qtoz, QuatConj(p));
        }
        else
        {
            float qa[4], pa[4];
            unsigned lo, hi, neg[4], par = 0;
            float all, big, two;
            qa[0] = q.GetX();
            qa[1] = q.GetY();
            qa[2] = q.GetZ();
            qa[3] = q.GetW();
            for (int i = 0; i < 4; i++)
            {
                pa[i] = 0.0;
                if ((bool)(neg[i] = (qa[i] < 0.0f)))
                {
                    qa[i] = -qa[i];
                }
                par ^= neg[i];
            }

            // find two largest components, indices in hi and lo
            if (qa[0] > qa[1])
            {
                lo = 0;
            }
            else
            {
                lo = 1;
            }
            if (qa[2] > qa[3])
            {
                hi = 2;
            }
            else
            {
                hi = 3;
            }
            if (qa[lo] > qa[hi])
            {
                if (qa[lo ^ 1] > qa[hi])
                {
                    hi = lo;
                    lo ^= 1;
                }
                else
                {
                    hi ^= lo;
                    lo ^= hi;
                    hi ^= lo;
                }
            }
            else
            {
                if (qa[hi ^ 1] > qa[lo])
                {
                    lo = hi ^ 1;
                }
            }

            all = (qa[0] + qa[1] + qa[2] + qa[3]) * 0.5f;
            two = (qa[hi] + qa[lo]) * Math::sqrtHalf;
            big = qa[hi];
            if (all > two)
            {
                if (all > big)
                {
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            pa[i] = Sign(static_cast<uint8>(neg[i]), 0.5f);
                        }
                    }
                    Cycle(ka, (par != 0));
                }
                else
                {
                    pa[hi] = Sign(static_cast<uint8>(neg[hi]), 1.0f);
                }
            }
            else
            {
                if (two > big)
                {
                    pa[hi] = Sign(static_cast<uint8>(neg[hi]), Math::sqrtHalf);
                    pa[lo] = Sign(static_cast<uint8>(neg[lo]), Math::sqrtHalf);
                    if (lo > hi)
                    {
                        hi ^= lo;
                        lo ^= hi;
                        hi ^= lo;
                    }

                    if (hi == 3)
                    {
                        hi = "\001\002\000"[lo];
                        lo = 3 - hi - lo;
                    }

                    Swap(ka, hi, lo);
                }
                else
                {
                    pa[hi] = Sign(static_cast<uint8>(neg[hi]), 1.0f);
                }
            }
            p.SetX(-pa[0]);
            p.SetY(-pa[1]);
            p.SetZ(-pa[2]);
            p.SetW(pa[3]);
        }

        k->SetX(ka[0]);
        k->SetY(ka[1]);
        k->SetZ(ka[2]);
        return p;
    }


    // Decompose 4x4 affine matrix A as TFRUK(U transpose), where t contains the
    // translation components, q contains the rotation R, u contains U, k contains
    // scale factors, and f contains the sign of the determinant.
    // Assumes inMatrix transforms column vectors in right-handed coordinates.
    // See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
    // Proceedings of Graphics Interface 1992.
    void MatrixDecomposer::DecompAffine(HMatrix inMatrix)
    {
        // extract the translation
        mTranslation.Set(inMatrix[0][3], inMatrix[1][3], inMatrix[2][3]);

        // extract the determinant sign and apply polar decomposition
        HMatrix Q, S, U;
        const float det = PolarDecomp(inMatrix, Q, S);
        if (det < 0.0f)
        {
            MCORE_MATDECOMP_MATCOPY(Q, =, -Q, 3);
            mDeterminantSign = -1;
        }
        else
        {
            mDeterminantSign = 1;
        }

        // extract the rotation
        AZ::Vector4 rot = QuatFromMatrix(Q);
        mRotation.Set(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());

        // extract the scale and scale rotation
        AZ::Vector4 scale = SpectralDecomp(S, U);
        AZ::Vector4 scaleRot = QuatFromMatrix(U);
        AZ::Vector4 p = Snuggle(scaleRot, &scale);
        scaleRot = QuatMul(scaleRot, p);

        mScaleRotation.Set(scaleRot.GetX(), scaleRot.GetY(), scaleRot.GetZ(), scaleRot.GetW());
        mScale.Set(scale.GetX(), scale.GetY(), scale.GetZ());
        mScale *= mDeterminantSign;
    }
}   // namespace MCore
