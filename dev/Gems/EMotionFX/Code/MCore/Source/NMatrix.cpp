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

#include "NMatrix.h"
#include "Algorithms.h"
#include "LogManager.h"


namespace MCore
{
    // default constructor
    NMatrix::NMatrix()
        : mData(nullptr)
        , mNumRows(0)
        , mNumColumns(0)
    {
    }


    // extended constructor
    NMatrix::NMatrix(uint32 numRows, uint32 numColumns, bool identity)
        : mData(nullptr)
        , mNumRows(0)
        , mNumColumns(0)
    {
        SetDimensions(numRows, numColumns);
        if (identity && numRows == numColumns)
        {
            Identity();
        }
    }


    // conversion from 4x4 Matrix
    NMatrix::NMatrix(const Matrix& other)
        : mData(nullptr)
        , mNumRows(0)
        , mNumColumns(0)
    {
        SetDimensions(4, 4);
        MCore::MemCopy(mData, other.m16, sizeof(float) * 16);
    }


    // copy constructor
    NMatrix::NMatrix(const NMatrix& other)
        : mData(nullptr)
        , mNumRows(0)
        , mNumColumns(0)
    {
        SetDimensions(other.mNumRows, other.mNumColumns);
        MCore::MemCopy(mData, other.mData, sizeof(float) * mNumRows * mNumColumns);
    }


    // destructor
    NMatrix::~NMatrix()
    {
        MCore::Free(mData);
    }


    // set the matrix to identity
    void NMatrix::Identity()
    {
        MCORE_ASSERT(mNumRows == mNumColumns); // only works on square matrices
        if (mNumRows != mNumColumns)
        {
            return;
        }

        MCore::MemSet(mData, 0, sizeof(float) * mNumRows * mNumColumns);
        for (uint32 i = 0; i < mNumRows; ++i)
        {
            mData[i + i * mNumRows] = 1.0f;
        }
    }


    // print to the log output to debug
    void NMatrix::Print(const char* description)
    {
        String finalMsg;
        finalMsg.Format("NMatrix - %s - rows=%d - columns=%d:\n", (description) ? description : "", mNumRows, mNumColumns);
        for (uint32 y = 0; y < mNumRows; ++y)
        {
            finalMsg += "[";

            for (uint32 x = 0; x < mNumColumns; ++x)
            {
                finalMsg.FormatAdd("%.8f%s", mData[x + y * mNumColumns], (x == mNumColumns - 1) ? "" : " ");
            }

            finalMsg += "]\n";
        }

        LogDetailedInfo(finalMsg);
    }


    // transpose the matrix
    void NMatrix::Transpose()
    {
        NMatrix temp(mNumColumns, mNumRows, false);

        for (uint32 y = 0; y < mNumRows; ++y)
        {
            for (uint32 x = 0; x < mNumColumns; ++x)
            {
                temp(x, y) = Get(y, x);
            }
        }

        *this = temp;
    }


    // init as a transposed version of another matrix
    void NMatrix::SetTransposed(const NMatrix& source)
    {
        SetDimensions(source.GetNumColumns(), source.GetNumRows());

        for (uint32 row = 0; row < mNumRows; ++row)
        {
            for (uint32 col = 0; col < mNumColumns; ++col)
            {
                Set(row, col, source.Get(col, row));
            }
        }
    }


    NMatrix operator + (const NMatrix& m1, const NMatrix& m2)
    {
        // make sure we have a square matrix
        MCORE_ASSERT(m1.GetNumRows() == m2.GetNumRows());
        MCORE_ASSERT(m1.GetNumColumns() == m2.GetNumColumns());

        const uint32 numRows = m1.GetNumRows();
        const uint32 numColumns = m1.GetNumColumns();
        NMatrix result;
        result.SetDimensions(numRows, numColumns);

        for (uint32 r = 0; r < numRows; ++r)
        {
            for (uint32 c = 0; c < numColumns; ++c)
            {
                result.Set(r, c, m1.Get(r, c) + m2.Get(r, c));
            }
        }

        return result;
    }


    // matrix multiply
    NMatrix operator * (const NMatrix& m1, const NMatrix& m2)
    {
        MCORE_ASSERT(m1.GetNumColumns() == m2.GetNumRows()); // if you get this assert, check your matrix multiplication order or matrix dimensions
                                                             // you might have to turn your columns into rows, or change the multiplication order from A*B into B*A.

        // resulting matrix dimentions: (n x m)(m x p) = (n x p)
        const uint32 numRows    = m1.GetNumRows();
        const uint32 numColumns = m2.GetNumColumns();
        NMatrix result(numRows, numColumns, false);

        // process all rows and columns
        for (uint32 y = 0; y < numRows; ++y)
        {
            for (uint32 x = 0; x < numColumns; ++x)
            {
                float sum = 0.0f;

                // perform the dot product between row and column
                const uint32 numS = m1.GetNumColumns();
                for (uint32 s = 0; s < numS; ++s)
                {
                    sum += m1.Get(y, s) * m2.Get(s, x);
                }

                // store the result in the resulting matrix
                result(y, x) = sum;
            }
        }

        return result;
    }



    // LU decompose the matrix
    void NMatrix::LUDecompose(Array<uint32>& outIndices, float& d)
    {
        MCORE_ASSERT(mNumRows == mNumColumns); // only works on square matrices

        NMatrix& mat = *this;

        uint32 i, j, k;
        uint32 imax = 0;
        float big, dum, sum, temp;

        const uint32 n = mat.GetNumRows();
        outIndices.Resize(n);
        Array<float> vv(n, MCORE_MEMCATEGORY_ARRAY);

        d = 1.0f;
        for (i = 0; i < n; i++)
        {
            big = 0.0f;

            for (j = 0; j < n; j++)
            {
                if ((temp = Math::Abs(mat[i][j])) > big)
                {
                    big = temp;
                }
            }

            // make sure the matrix isn't singular
            MCORE_ASSERT(Math::IsFloatZero(big) == false);

            vv[i] = 1.0f / big;
        }

        for (j = 0; j < n; j++)
        {
            for (i = 0; i < j; i++)
            {
                sum = mat[i][j];
                for (k = 0; k < i; k++)
                {
                    sum -= mat[i][k] * mat[k][j];
                }

                mat[i][j] = sum;
            }

            big = 0.0f;

            for (i = j; i < n; i++)
            {
                sum = mat[i][j];
                for (k = 0; k < j; k++)
                {
                    sum -= mat[i][k] * mat[k][j];
                }

                mat[i][j] = sum;

                if ((dum = vv[i] * Math::Abs(sum)) >= big)
                {
                    big = dum;
                    imax = i;
                }
            }

            if (j != imax)
            {
                for (k = 0; k < n; k++)
                {
                    dum = mat[imax][k];
                    mat[imax][k] = mat[j][k];
                    mat[j][k] = dum;
                }

                d = -d;
                vv[imax] = vv[j];
            }

            outIndices[j] = imax;
            if (Math::IsFloatZero(mat[j][j]))
            {
                mat[j][j] = 0.000001f;
            }

            if (j != n - 1)
            {
                dum = 1.0f / (mat[j][j]);
                for (i = j + 1; i < n; i++)
                {
                    mat[i][j] *= dum;
                }
            }
        }
    }


    // LU backsubstitution
    void NMatrix::LUBackSubstitution(const Array<uint32>& indices, Array<float>& b)
    {
        MCORE_ASSERT(mNumRows == mNumColumns); // only works on square matrices

        NMatrix& mat = *this;

        uint32 i, ii = 0, ip, j;
        float sum;

        const uint32 n = mat.GetNumRows();
        for (i = 0; i < n; i++)
        {
            ip = indices[i];
            sum = b[ip];
            b[ip] = b[i];

            if (ii != 0)
            {
                for (j = ii - 1; j < i; j++)
                {
                    sum -= mat[i][j] * b[j];
                }
            }
            else
            if (Math::IsFloatZero(sum) == false)
            {
                ii = i + 1;
            }

            b[i] = sum;
        }

        for (i = n - 1; i != MCORE_INVALIDINDEX32; i--)
        {
            sum = b[i];

            for (j = i + 1; j < n; j++)
            {
                sum -= mat[i][j] * b[j];
            }

            b[i] = sum / mat[i][i];
        }
    }



    // calculate the inverse of the matrix using LU decomposition and backsubstitution
    void NMatrix::LUInverse()
    {
        MCORE_ASSERT(mNumRows == mNumColumns); // only works on square matrices

        NMatrix mat(*this);

        // get the dimensions of the matrix (since numRows equals numColumns)
        const uint32 dimension = mNumRows;

        // decompose the matrix using LU decomposition
        Array<uint32> indices(dimension, MCORE_MEMCATEGORY_ARRAY);
        float d;
        mat.LUDecompose(indices, d);

        // find the inverse per column
        Array<float> column(dimension, MCORE_MEMCATEGORY_ARRAY);
        for (uint32 j = 0; j < dimension; ++j)
        {
            uint32 i;
            for (i = 0; i < dimension; ++i)
            {
                column[i] = 0.0f;
            }

            column[j] = 1.0f;

            mat.LUBackSubstitution(indices, column);

            for (i = 0; i < dimension; ++i)
            {
                Set(i, j, column[i]);
            }
        }
    }


    // calculate the determinant of the matrix
    float NMatrix::CalcDeterminant() const
    {
        MCORE_ASSERT(mNumRows == mNumColumns); // only works on square matrices

        const uint32 dimension = mNumRows;
        NMatrix mat(*this);
        Array<uint32> indices(dimension, MCORE_MEMCATEGORY_ARRAY);
        float d;

        mat.LUDecompose(indices, d);

        for (uint32 j = 0; j < dimension; ++j)
        {
            d *= mat[j][j];
        }

        return d;
    }


    // solve a linear system using LU decomposition and backsubstitution
    void NMatrix::LUSolve(const Array<float>& b, Array<float>& solution) const
    {
        MCORE_ASSERT(mNumRows == mNumColumns); // only works on square matrices

        // perform LU decomposition
        Array<uint32> indices(mNumRows, MCORE_MEMCATEGORY_ARRAY);
        float d;
        NMatrix mat(*this);
        mat.LUDecompose(indices, d);

        // perform backsubstitution
        solution = b; // input the b values, they will get modified and overwritten by the LUBackSubstitution method
        mat.LUBackSubstitution(indices, solution);
    }


    // solve a linear system using singular value decomposition
    void NMatrix::SVDSolve(const Array<float>& b, Array<float>& solution) const
    {
        // calculate the SVD result
        NMatrix mat(*this);
        NMatrix v;
        Array<float> w;
        Array<float> temp;
        mat.SVD(w, v, temp);

        // perform backsubsitution
        solution.Resize(mNumRows);
        mat.SVDBackSubstitute(w, v, b, solution);
    }


    // perform singular value decomposition on the matrix
    void NMatrix::SVD(Array<float>& w, NMatrix& v, Array<float>& rv1)
    {
        NMatrix& a = *this;

        bool flag;
        uint32 i, its, j, jj, k;
        uint32 l  = 0;
        uint32 nm = 0;
        float anorm, c, f, g, h, s;
        float scale, x, y, z;

        uint32 m = mNumRows;
        uint32 n = mNumColumns;
        rv1.Resize(n);

        v.SetDimensions(mNumRows, mNumColumns);
        //  v.SetDimensions(mNumColumns, mNumColumns);
        w.Resize(mNumColumns);

        g = scale = anorm = 0.0f;

        for (i = 0; i < n; ++i)
        {
            l = i + 2;
            rv1[i] = scale * g;
            g = s = scale = 0.0f;

            if (i < m)
            {
                for (k = i; k < m; k++)
                {
                    scale += Math::Abs(a[k][i]);
                }

                if (Math::IsFloatZero(scale) == false)
                {
                    for (k = i; k < m; k++)
                    {
                        a[k][i] /= scale;
                        s += a[k][i] * a[k][i];
                    }

                    f = a[i][i];
                    g = -Sign(Math::Sqrt(s), f);
                    h = f * g - s;
                    a[i][i] = f - g;

                    for (j = l - 1; j < n; j++)
                    {
                        for (s = 0.0f, k = i; k < m; k++)
                        {
                            s += a[k][i] * a[k][j];
                        }

                        f = s / h;
                        for (k = i; k < m; k++)
                        {
                            a[k][j] += f * a[k][i];
                        }
                    }

                    for (k = i; k < m; k++)
                    {
                        a[k][i] *= scale;
                    }
                }
            }

            w[i] = scale * g;
            g = s = scale = 0.0f;

            if (i + 1 <= m && i != n)
            {
                for (k = l - 1; k < n; k++)
                {
                    scale += Math::Abs(a[i][k]);
                }

                if (Math::IsFloatZero(scale) == false)
                {
                    for (k = l - 1; k < n; k++)
                    {
                        a[i][k] /= scale;
                        s += a[i][k] * a[i][k];
                    }

                    f = a[i][l - 1];
                    g = -Sign(Math::Sqrt(s), f);

                    h = f * g - s;
                    a[i][l - 1] = f - g;

                    for (k = l - 1; k < n; k++)
                    {
                        rv1[k] = a[i][k] / h;
                    }

                    for (j = l - 1; j < m; j++)
                    {
                        for (s = 0.0f, k = l - 1; k < n; k++)
                        {
                            s += a[j][k] * a[i][k];
                        }

                        for (k = l - 1; k < n; k++)
                        {
                            a[j][k] += s * rv1[k];
                        }
                    }

                    for (k = l - 1; k < n; k++)
                    {
                        a[i][k] *= scale;
                    }
                }
            }

            anorm = Max<float>(anorm, Math::Abs(w[i]) + Math::Abs(rv1[i]));
        }

        for (i = n - 1; i != MCORE_INVALIDINDEX32; i--)
        {
            if (i < n - 1)
            {
                if (Math::IsFloatZero(g) == false)
                {
                    for (j = l; j < n; j++)
                    {
                        v[j][i] = (a[i][j] / a[i][l]) / g;
                    }

                    for (j = l; j < n; j++)
                    {
                        for (s = 0.0f, k = l; k < n; k++)
                        {
                            s += a[i][k] * v[k][j];
                        }

                        for (k = l; k < n; k++)
                        {
                            v[k][j] += s * v[k][i];
                        }
                    }
                }

                for (j = l; j < n; j++)
                {
                    v[i][j] = v[j][i] = 0.0f;
                }
            }
            v[i][i] = 1.0f;
            g = rv1[i];
            l = i;
        }

        for (i = (uint32)Min<float>(static_cast<float>(m), static_cast<float>(n)) - 1; i != MCORE_INVALIDINDEX32; i--)
        {
            l = i + 1;
            g = w[i];

            for (j = l; j < n; j++)
            {
                a[i][j] = 0.0f;
            }

            if (Math::IsFloatZero(g) == false)
            {
                g = 1.0f / g;

                for (j = l; j < n; j++)
                {
                    for (s = 0.0f, k = l; k < m; k++)
                    {
                        s += a[k][i] * a[k][j];
                    }

                    f = (s / a[i][i]) * g;

                    for (k = i; k < m; k++)
                    {
                        a[k][j] += f * a[k][i];
                    }
                }
                for (j = i; j < m; j++)
                {
                    a[j][i] *= g;
                }
            }
            else
            {
                for (j = i; j < m; j++)
                {
                    a[j][i] = 0.0;
                }
            }

            ++a[i][i];
        }

        for (k = n - 1; k != MCORE_INVALIDINDEX32; k--)
        {
            for (its = 0; its < 30; its++)
            {
                flag = true;
                for (l = k; l != MCORE_INVALIDINDEX32; l--)
                {
                    nm = l - 1;
                    if (Math::IsFloatEqual(Math::Abs(rv1[l]) + anorm, anorm))
                    {
                        flag = false;
                        break;
                    }
                    if (Math::IsFloatEqual(Math::Abs(w[nm]) + anorm, anorm))
                    {
                        break;
                    }
                }

                if (flag)
                {
                    c = 0.0f;
                    s = 1.0f;
                    for (i = l - 1; i < k + 1; i++)
                    {
                        f = s * rv1[i];
                        rv1[i] = c * rv1[i];
                        if (Math::IsFloatEqual(Math::Abs(f) + anorm, anorm))
                        {
                            break;
                        }
                        g = w[i];
                        h = Pythag(f, g);
                        w[i] = h;
                        h = 1.0f / h;
                        c = g * h;
                        s = -f * h;
                        for (j = 0; j < m; j++)
                        {
                            y = a[j][nm];
                            z = a[j][i];
                            a[j][nm] = y * c + z * s;
                            a[j][i] = z * c - y * s;
                        }
                    }
                }

                z = w[k];
                if (l == k)
                {
                    if (z < 0.0f)
                    {
                        w[k] = -z;
                        for (j = 0; j < n; j++)
                        {
                            v[j][k] = -v[j][k];
                        }
                    }
                    break;
                }

                if (its == 29)
                {
                    MCORE_ASSERT(0); // no convergence after 30 iterations!
                }
                x = w[l];
                nm = k - 1;
                y = w[nm];
                g = rv1[nm];
                h = rv1[k];
                f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0f * h * y);
                g = Pythag(f, 1.0f);
                f = ((x - z) * (x + z) + h * ((y / (f + Sign(g, f))) - h)) / x;
                c = s = 1.0f;

                for (j = l; j <= nm; j++)
                {
                    i = j + 1;
                    g = rv1[i];
                    y = w[i];
                    h = s * g;
                    g = c * g;
                    z = Pythag(f, h);
                    rv1[j] = z;
                    c = f / z;
                    s = h / z;
                    f = x * c + g * s;
                    g = g * c - x * s;
                    h = y * s;
                    y *= c;

                    for (jj = 0; jj < n; jj++)
                    {
                        x = v[jj][j];
                        z = v[jj][i];
                        v[jj][j] = x * c + z * s;
                        v[jj][i] = z * c - x * s;
                    }

                    z = Pythag(f, h);
                    w[j] = z;

                    if (z > MCore::Math::epsilon)
                    {
                        z = 1.0f / z;
                        c = f * z;
                        s = h * z;
                    }

                    f = c * g + s * y;
                    x = c * y - s * g;

                    for (jj = 0; jj < m; jj++)
                    {
                        y = a[jj][j];
                        z = a[jj][i];
                        a[jj][j] = y * c + z * s;
                        a[jj][i] = z * c - y * s;
                    }
                }

                rv1[l] = 0.0f;
                rv1[k] = f;
                w[k] = x;
            }
        }

        // zero out the tiny w values
        SVDZeroTinyWValues(w);
    }

    /*
    // perform singular value decomposition on the matrix
    void NMatrix::SVD(Array<float>& w, NMatrix& v, Array<float>& rv1)
    {
        NMatrix& a = *this;

        bool flag;
        uint32 i, its, j, jj, k;
        uint32 l  = 0;
        uint32 nm = 0;
        float anorm, c, f, g, h, s;
        float scale, x, y, z;

        uint32 m = mNumRows;
        uint32 n = mNumColumns;
        rv1.Resize(n);

        v.SetDimensions(mNumColumns, mNumColumns);
        w.Resize(mNumColumns);

        g = scale = anorm = 0.0f;

        for (i=0; i<n; ++i)
        {
            l = i+2;
            rv1[i] = scale * g;
            g = s = scale = 0.0f;

            if (i < m)
            {
                for (k=i; k<m; k++)
                    scale += Math::Abs(a[k][i]);

                if (scale != 0.0f)
                {
                    for (k=i; k<m; k++)
                    {
                        a[k][i] /= scale;
                        s += a[k][i] * a[k][i];
                    }

                    f=a[i][i];
                    g = -Sign(Math::Sqrt(s),f);
                    h = f * g - s;
                    a[i][i] = f - g;

                    for (j=l-1; j<n; j++)
                    {
                        for (s=0.0f,k=i; k<m; k++)
                            s += a[k][i] * a[k][j];

                        f =s / h;
                        for (k=i; k<m; k++)
                            a[k][j] += f * a[k][i];
                    }

                    for (k=i; k<m; k++)
                        a[k][i] *= scale;
                }
            }

            w[i] = scale * g;
            g = s = scale = 0.0f;

            if (i+1 <= m && i != n)
            {
                for (k=l-1; k<n; k++)
                    scale += Math::Abs( a[i][k] );

                if (scale != 0.0f)
                {
                    for (k=l-1; k<n; k++)
                    {
                        a[i][k] /= scale;
                        s += a[i][k] * a[i][k];
                    }

                    f = a[i][l-1];
                    g = -Sign(Math::Sqrt(s),f);

                    h = f * g - s;
                    a[i][l-1] = f - g;

                    for (k=l-1; k<n; k++)
                        rv1[k] = a[i][k] / h;

                    for (j=l-1; j<m; j++)
                    {
                        for (s=0.0f, k=l-1; k<n; k++)
                            s += a[j][k] * a[i][k];

                        for (k=l-1; k<n; k++)
                            a[j][k] += s * rv1[k];
                    }

                    for (k=l-1; k<n; k++)
                        a[i][k] *= scale;
                }
            }

            anorm = Max<float>( anorm, Math::Abs(w[i]) + Math::Abs(rv1[i]) );
        }

        for (i=n-1; i != -1; i--)
        {
            if (i < n-1)
            {
                if (g != 0.0f)
                {
                    for (j=l; j<n; j++)
                        v[j][i] = (a[i][j] / a[i][l]) / g;

                    for (j=l;j<n;j++)
                    {
                        for (s=0.0f,k=l; k<n; k++)
                            s += a[i][k]*v[k][j];

                        for (k=l; k<n; k++)
                            v[k][j] += s * v[k][i];
                    }
                }

                for (j=l; j<n; j++)
                    v[i][j] = v[j][i] = 0.0f;
            }
            v[i][i] = 1.0f;
            g = rv1[i];
            l = i;
        }

        for (i=Min<float>(m,n)-1; i != -1; i--)
        {
            l = i + 1;
            g = w[i];

            for (j=l; j<n; j++)
                a[i][j] = 0.0f;

            if (g != 0.0f)
            {
                g= 1.0f / g;

                for (j=l; j<n; j++)
                {
                    for (s=0.0f,k=l; k<m; k++)
                        s += a[k][i] * a[k][j];

                    f = (s / a[i][i]) * g;

                    for (k=i; k<m; k++)
                        a[k][j] += f * a[k][i];
                }
                for (j=i;j<m;j++) a[j][i] *= g;
            } else for (j=i; j<m; j++) a[j][i] = 0.0;

            ++a[i][i];
        }

        for (k=n-1; k != -1; k--)
        {
            for (its=0; its<30; its++)
            {
                flag = true;
                for (l=k; l != -1; l--)
                {
                    nm = l - 1;
                    if (Math::Abs(rv1[l])+anorm == anorm)
                    {
                        flag = false;
                        break;
                    }
                    if (Math::Abs(w[nm])+anorm == anorm) break;
                }

                if (flag)
                {
                    c = 0.0f;
                    s = 1.0f;
                    for (i=l-1;i<k+1;i++)
                    {
                        f = s * rv1[i];
                        rv1[i] = c * rv1[i];
                        if (Math::Abs(f)+anorm == anorm) break;
                        g = w[i];
                        h = Pythag(f,g);
                        w[i] = h;
                        h = 1.0 / h;
                        c = g * h;
                        s = -f*h;
                        for (j=0;j<m;j++)
                        {
                            y = a[j][nm];
                            z = a[j][i];
                            a[j][nm] = y* c + z * s;
                            a[j][i] = z * c - y * s;
                        }
                    }
                }

                z = w[k];
                if (l == k)
                {
                    if (z < 0.0f)
                    {
                        w[k] = -z;
                        for (j=0; j<n; j++)
                            v[j][k] = -v[j][k];
                    }
                    break;
                }

                if (its == 29)
                    MCORE_ASSERT(0);    // no convergence after 30 iterations!

                x = w[l];
                nm = k-1;
                y = w[nm];
                g = rv1[nm];
                h = rv1[k];
                f = ((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
                g = Pythag(f, 1.0f);
                f = ((x-z)*(x+z)+h*((y/(f+Sign(g,f)))-h))/x;
                c = s= 1.0f;

                for (j=l; j<=nm; j++)
                {
                    i = j+1;
                    g = rv1[i];
                    y = w[i];
                    h = s * g;
                    g = c * g;
                    z = Pythag(f, h);
                    rv1[j] = z;
                    c = f / z;
                    s = h / z;
                    f = x*c + g*s;
                    g = g*c - x*s;
                    h = y*s;
                    y *= c;

                    for (jj=0;jj<n;jj++)
                    {
                        x = v[jj][j];
                        z = v[jj][i];
                        v[jj][j] = x*c + z*s;
                        v[jj][i] = z*c - x*s;
                    }

                    z = Pythag(f, h);
                    w[j]=z;

                    if (z)
                    {
                        z = 1.0f / z;
                        c = f*z;
                        s = h*z;
                    }

                    f = c*g + s*y;
                    x = c*y - s*g;

                    for (jj=0; jj<m; jj++)
                    {
                        y = a[jj][j];
                        z = a[jj][i];
                        a[jj][j] = y*c + z*s;
                        a[jj][i] = z*c - y*s;
                    }
                }

                rv1[l] = 0.0f;
                rv1[k] = f;
                w[k] = x;
            }
        }

        // zero out the tiny w values
    //  SVDZeroTinyWValues( w );
    }
    */


    // perform SVD back substitution
    void NMatrix::SVDBackSubstitute(Array<float>& w, const NMatrix& v, const Array<float>& b, Array<float>& x)
    {
        NMatrix& u = *this;

        uint32 m = u.mNumRows;
        uint32 n = u.mNumColumns;

        Array<float> tmp(n, MCORE_MEMCATEGORY_ARRAY);
        float s;

        uint32 j;
        for (j = 0; j < n; ++j)
        {
            s = 0.0f;

            if (Math::IsFloatZero(w[j]) == false)
            {
                for (uint32 i = 0; i < m; ++i)
                {
                    s += u.Get(i, j) * b[i];
                }

                s /= w[j];
            }

            tmp[j] = s;
        }

        for (j = 0; j < n; ++j)
        {
            s = 0.0f;

            for (uint32 jj = 0; jj < n; ++jj)
            {
                s += v.Get(j, jj) * tmp[jj];
            }

            x[j] = s;
        }
    }


    // prepare the w returned by the SVD algorithm for backsubstitution
    void NMatrix::SVDZeroTinyWValues(Array<float>& w)
    {
        uint32 i;
        float maxW = 0.0f;

        // find the maximum w value
        const uint32 numW = w.GetLength();
        for (i = 0; i < numW; ++i)
        {
            if (w[i] > maxW)
            {
                maxW = w[i];
            }
        }

        // make all w values smaller than a given threshold into zero
        const float minW = maxW * 0.000001f;
        for (i = 0; i < numW; ++i)
        {
            if (w[i] < minW)
            {
                w[i] = 0.0f;
            }
        }
    }


    // some pythagoras function used by SVD
    float NMatrix::Pythag(float a, float b)
    {
        const float absA = Math::Abs(a);
        const float absB = Math::Abs(b);

        if (absA > absB)
        {
            return absA * Math::Sqrt(1.0f + Sqr<float>(absB / absA));
        }
        else
        {
            return (Math::IsFloatZero(absB) ? 0.0f : absB* Math::Sqrt(1.0f + Sqr<float>(absA / absB)));
        }
    }


    // calculate the inverse of this matrix using SVD
    void NMatrix::SVDInverse()
    {
        // perform singular value decomposition
        NMatrix V;  // the V matrix
        Array<float> w; // the diagonal values of the W matrix
        Array<float> temp; // a temp array
        SVD(w, V, temp);    // this matrix will now contain the U matrix after the SVD

        // now we have U, the W values and V
        // the inverse is U * 1/w * transpose(V)
        // directly create the matrix W from the inversed w values
        // our SVD method already fixed the w values (setting too small w values to zero)
        NMatrix W(mNumRows, mNumColumns, false);
        W.Zero(); // fill it with zeros

        // create the diagonal values of the W matrix
        const uint32 numW = w.GetLength();
        for (uint32 i = 0; i < numW; ++i)
        {
            if (w[i] > 0.0f)
            {
                W(i, i) = 1.0f / w[i];
            }
        }

        // now transpose the V matrix
        V.Transpose();

        // calculate the final inverse
        // Ax = b
        // inverse(A) = U * W * transpose(V)
        *this *= W * V;
    }
}   // namespace MCore
