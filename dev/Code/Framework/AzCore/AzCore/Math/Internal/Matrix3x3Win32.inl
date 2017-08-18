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
namespace AZ
{
    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateIdentity()
    {
        Matrix3x3 result;
        SimdVectorType row0 = *(__m128*)&Internal::g_simd1000;
        result.m_rows[0] = row0;
        result.m_rows[1] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 1, 0, 1));
        result.m_rows[2] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 0, 1, 1));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateZero()
    {
        Matrix3x3 result;
        SimdVectorType v0 = _mm_setzero_ps();
        result.m_rows[0] = v0;
        result.m_rows[1] = v0;
        result.m_rows[2] = v0;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromValue(const VectorFloat& value)
    {
        Matrix3x3 result;
        result.m_rows[0] = value.m_value;
        result.m_rows[1] = value.m_value;
        result.m_rows[2] = value.m_value;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateRotationX(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        Matrix3x3 result;
        result.m_rows[0] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 2, 3));
        result.m_rows[1] = _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 1, 0, 2));
        result.m_rows[2] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 0, 1, 2));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateRotationY(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        Matrix3x3 result;
        result.m_rows[0] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 1, 2, 0));
        result.m_rows[1] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 3, 2));
        result.m_rows[2] = _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 0, 2, 1));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateRotationZ(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        Matrix3x3 result;
        result.m_rows[0] = _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 2, 1, 0));
        result.m_rows[1] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 0, 1));
        result.m_rows[2] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 3, 2, 2));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromTransform(const Transform& t)
    {
        Matrix3x3 result;
        result.m_rows[0] = t.m_rows[0];
        result.m_rows[1] = t.m_rows[1];
        result.m_rows[2] = t.m_rows[2];
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromMatrix4x4(const Matrix4x4& m)
    {
        Matrix3x3 result;
        result.m_rows[0] = m.m_rows[0];
        result.m_rows[1] = m.m_rows[1];
        result.m_rows[2] = m.m_rows[2];
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromQuaternion(const Quaternion& q)
    {
        Matrix3x3 result;
        result.SetRotationPartFromQuaternion(q);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateDiagonal(const Vector3& diagonal)
    {
        SimdVectorType diag = diagonal.m_value;
        SimdVectorType v0 = _mm_setzero_ps();
        SimdVectorType vXY00 = _mm_movelh_ps(diag, v0);
        SimdVectorType v00ZW = _mm_movehl_ps(diag, v0);
        Matrix3x3 result;
        result.m_rows[0] = _mm_shuffle_ps(vXY00, vXY00, _MM_SHUFFLE(3, 3, 3, 0));
        result.m_rows[1] = _mm_shuffle_ps(vXY00, vXY00, _MM_SHUFFLE(3, 3, 1, 3));
        result.m_rows[2] = _mm_shuffle_ps(v00ZW, v00ZW, _MM_SHUFFLE(0, 2, 0, 0));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateCrossProduct(const Vector3& p)
    {
        Matrix3x3 result;
        SimdVectorType vXYZ0 = _mm_and_ps(p.m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        result.m_rows[0] = _mm_xor_ps(_mm_shuffle_ps(vXYZ0, vXYZ0, _MM_SHUFFLE(3, 1, 2, 3)), *(SimdVectorType*)&Internal::g_simdNegateYMask);
        result.m_rows[1] = _mm_xor_ps(_mm_shuffle_ps(vXYZ0, vXYZ0, _MM_SHUFFLE(3, 0, 3, 2)), *(SimdVectorType*)&Internal::g_simdNegateZWMask);
        result.m_rows[2] = _mm_xor_ps(_mm_shuffle_ps(vXYZ0, vXYZ0, _MM_SHUFFLE(3, 3, 0, 1)), *(SimdVectorType*)&Internal::g_simdNegateXMask);
        return result;
    }

    AZ_MATH_FORCE_INLINE VectorFloat Matrix3x3::GetElement(int row, int col) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        return Vector3(m_rows[row]).GetElement(col);
    }

    AZ_MATH_FORCE_INLINE void Matrix3x3::SetElement(int row, int col, const VectorFloat& value)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        Vector3 temp(m_rows[row]);
        temp.SetElement(col, value);
        m_rows[row] = temp.m_value;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::GetRow(int row) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        return Vector3(m_rows[row]);
    }

    AZ_MATH_FORCE_INLINE void Matrix3x3::SetRow(int row, const Vector3& v)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        m_rows[row] = v.m_value;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::GetColumn(int col) const
    {
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        //Note that here we are optimizing for the case where the column index is a constant, and assuming the compiler will AZ_MATH_FORCE_INLINE this
        //function and remove the unnecessary branches. If col is not a constant it would probably be quicker to read the floats and
        // construct the vector from them directly.
        if (col == 0)
        {
            SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(0, 0, 0, 0));
            return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(0, 0, 2, 0)));
        }
        else if (col == 1)
        {
            SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(1, 1, 1, 1));
            return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(1, 1, 2, 0)));
        }
        else
        {
            SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(2, 2, 2, 2));
            return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(2, 2, 2, 0)));
        }
    }

    AZ_MATH_FORCE_INLINE void Matrix3x3::SetColumn(int col, const Vector3& v)
    {
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        //TODO: Profile to see if it's faster to set with shuffles or memory writes... depends on if the matrix is already in registers I guess
#if 0
        SetElement(0, col, v.GetX());
        SetElement(1, col, v.GetY());
        SetElement(2, col, v.GetZ());
#else
        //Note we are optimizing for the case where the column index is a constant. See comment in GetColumn for more information.
        if (col == 0)
        {
            m_rows[0] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(1, 1, 0, 0)), m_rows[0], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[1] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(1, 1, 1, 1)), m_rows[1], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[2] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(1, 1, 2, 2)), m_rows[2], _MM_SHUFFLE(3, 2, 2, 0));
        }
        else if (col == 1)
        {
            m_rows[0] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(0, 0, 0, 0)), m_rows[0], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[1] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(0, 0, 1, 1)), m_rows[1], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[2] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(0, 0, 2, 2)), m_rows[2], _MM_SHUFFLE(3, 2, 0, 2));
        }
        else
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], v.m_value, _MM_SHUFFLE(0, 0, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], v.m_value, _MM_SHUFFLE(0, 1, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], v.m_value, _MM_SHUFFLE(0, 2, 1, 0));
        }
#endif
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        SimdVectorType rhsRow0 = rhs.m_rows[0];
        SimdVectorType rhsRow1 = rhs.m_rows[1];
        SimdVectorType rhsRow2 = rhs.m_rows[2];
        for (int i = 0; i < 3; ++i)
        {
            SimdVectorType row = m_rows[i];
            result.m_rows[i] = _mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(0, 0, 0, 0)), rhsRow0),
                        _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(1, 1, 1, 1)), rhsRow1)),
                    _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(2, 2, 2, 2)), rhsRow2));
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::operator*(const Vector3& rhs) const
    {
        //Transpose is needed here, for the multiply-add simd implementation. We should store the Matrix3x3 as
        // column-major instead, unlike the Transform there's no real reason not to do this.
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = col2;      //don't care about this column, set it to whatever
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        SimdVectorType v = rhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(col0, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                    _mm_mul_ps(col1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                _mm_mul_ps(col2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))));
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator+(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        result.m_rows[0] = _mm_add_ps(m_rows[0], rhs.m_rows[0]);
        result.m_rows[1] = _mm_add_ps(m_rows[1], rhs.m_rows[1]);
        result.m_rows[2] = _mm_add_ps(m_rows[2], rhs.m_rows[2]);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator-(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        result.m_rows[0] = _mm_sub_ps(m_rows[0], rhs.m_rows[0]);
        result.m_rows[1] = _mm_sub_ps(m_rows[1], rhs.m_rows[1]);
        result.m_rows[2] = _mm_sub_ps(m_rows[2], rhs.m_rows[2]);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator*(const VectorFloat& multiplier) const
    {
        Matrix3x3 result;
        result.m_rows[0] = _mm_mul_ps(m_rows[0], multiplier.m_value);
        result.m_rows[1] = _mm_mul_ps(m_rows[1], multiplier.m_value);
        result.m_rows[2] = _mm_mul_ps(m_rows[2], multiplier.m_value);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator/(const VectorFloat& divisor) const
    {
        Matrix3x3 result;
        result.m_rows[0] = _mm_div_ps(m_rows[0], divisor.m_value);
        result.m_rows[1] = _mm_div_ps(m_rows[1], divisor.m_value);
        result.m_rows[2] = _mm_div_ps(m_rows[2], divisor.m_value);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator-() const
    {
        Matrix3x3 result;
        SimdVectorType v0 = _mm_setzero_ps();
        result.m_rows[0] = _mm_sub_ps(v0, m_rows[0]);
        result.m_rows[1] = _mm_sub_ps(v0, m_rows[1]);
        result.m_rows[2] = _mm_sub_ps(v0, m_rows[2]);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::GetTranspose() const
    {
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = col2;      //don't care about this column, set it to whatever
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        Matrix3x3 result;
        result.m_rows[0] = col0;
        result.m_rows[1] = col1;
        result.m_rows[2] = col2;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::TransposedMultiply(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        result.m_rows[0] = _mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(0, 0, 0, 0)), rhs.m_rows[0]),
                    _mm_mul_ps(_mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 0, 0, 0)), rhs.m_rows[1])),
                _mm_mul_ps(_mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 0, 0, 0)), rhs.m_rows[2]));
        result.m_rows[1] = _mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(1, 1, 1, 1)), rhs.m_rows[0]),
                    _mm_mul_ps(_mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(1, 1, 1, 1)), rhs.m_rows[1])),
                _mm_mul_ps(_mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(1, 1, 1, 1)), rhs.m_rows[2]));
        result.m_rows[2] = _mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(2, 2, 2, 2)), rhs.m_rows[0]),
                    _mm_mul_ps(_mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(2, 2, 2, 2)), rhs.m_rows[1])),
                _mm_mul_ps(_mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(2, 2, 2, 2)), rhs.m_rows[2]));
        return result;
    }

    AZ_MATH_FORCE_INLINE bool Matrix3x3::IsClose(const Matrix3x3& rhs, const VectorFloat& tolerance) const
    {
        for (int row = 0; row < 3; ++row)
        {
            if (!Vector3(m_rows[row]).IsClose(Vector3(rhs.m_rows[row]), tolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE bool Matrix3x3::operator==(const Matrix3x3& rhs) const
    {
        if (Vector3(m_rows[0]) != Vector3(rhs.m_rows[0]))
        {
            return false;
        }
        if (Vector3(m_rows[1]) != Vector3(rhs.m_rows[1]))
        {
            return false;
        }
        if (Vector3(m_rows[2]) != Vector3(rhs.m_rows[2]))
        {
            return false;
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::GetDiagonal() const
    {
        SimdVectorType vX__Y = _mm_movelh_ps(m_rows[0], m_rows[1]);
        return Vector3(_mm_shuffle_ps(vX__Y, m_rows[2], _MM_SHUFFLE(0, 2, 3, 0)));
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::GetInverseFull() const
    {
        SimdVectorType row0YZX = _mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row0ZXY = _mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(0, 1, 0, 2));
        SimdVectorType row1YZX = _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row1ZXY = _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 1, 0, 2));
        SimdVectorType row2YZX = _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row2ZXY = _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 1, 0, 2));

        SimdVectorType col0 = _mm_sub_ps(_mm_mul_ps(row1YZX, row2ZXY), _mm_mul_ps(row1ZXY, row2YZX));
        SimdVectorType col1 = _mm_sub_ps(_mm_mul_ps(row2YZX, row0ZXY), _mm_mul_ps(row2ZXY, row0YZX));
        SimdVectorType col2 = _mm_sub_ps(_mm_mul_ps(row0YZX, row1ZXY), _mm_mul_ps(row0ZXY, row1YZX));
        SimdVectorType col3 = col2; //don't care about this column, set it to whatever

        SimdVectorType detXYZ = _mm_mul_ps(m_rows[0], col0);
        SimdVectorType detTmp = _mm_add_ps(detXYZ, _mm_shuffle_ps(detXYZ, detXYZ, _MM_SHUFFLE(0, 1, 0, 1)));
        SimdVectorType det = _mm_add_ps(detTmp, _mm_shuffle_ps(detXYZ, detXYZ, _MM_SHUFFLE(0, 0, 2, 2)));

        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        Matrix3x3 result;
        result.m_rows[0] = _mm_div_ps(col0, det);
        result.m_rows[1] = _mm_div_ps(col1, det);
        result.m_rows[2] = _mm_div_ps(col2, det);
        return result;
    }

    AZ_MATH_FORCE_INLINE VectorFloat Matrix3x3::GetDeterminant() const
    {
        SimdVectorType row1YZX = _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row1ZXY = _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 1, 0, 2));
        SimdVectorType row2YZX = _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row2ZXY = _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 1, 0, 2));
        SimdVectorType cofactors = _mm_sub_ps(_mm_mul_ps(row1YZX, row2ZXY), _mm_mul_ps(row1ZXY, row2YZX));

        //multiply and horizontal sum
        SimdVectorType x2 = _mm_mul_ps(m_rows[0], cofactors);
        SimdVectorType x = _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(0, 0, 0, 0));
        SimdVectorType xy = _mm_add_ps(x, _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)));
        SimdVectorType xyz = _mm_add_ps(xy, _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)));
        return VectorFloat(xyz);
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::GetAdjugate() const
    {
        SimdVectorType row0YZX = _mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row0ZXY = _mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(0, 1, 0, 2));
        SimdVectorType row1YZX = _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row1ZXY = _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(0, 1, 0, 2));
        SimdVectorType row2YZX = _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 0, 2, 1));
        SimdVectorType row2ZXY = _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(0, 1, 0, 2));

        SimdVectorType col0 = _mm_sub_ps(_mm_mul_ps(row1YZX, row2ZXY), _mm_mul_ps(row1ZXY, row2YZX));
        SimdVectorType col1 = _mm_sub_ps(_mm_mul_ps(row2YZX, row0ZXY), _mm_mul_ps(row2ZXY, row0YZX));
        SimdVectorType col2 = _mm_sub_ps(_mm_mul_ps(row0YZX, row1ZXY), _mm_mul_ps(row0ZXY, row1YZX));
        SimdVectorType col3 = col2; //don't care about this column, set it to whatever

        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);
        Matrix3x3 result;
        result.m_rows[0] = col0;
        result.m_rows[1] = col1;
        result.m_rows[2] = col2;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs)
    {
        SimdVectorType v = lhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(rhs.m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                    _mm_mul_ps(rhs.m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                _mm_mul_ps(rhs.m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))));
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 operator*(const VectorFloat& lhs, const Matrix3x3& rhs)
    {
        Matrix3x3 result;
        result.m_rows[0] = _mm_mul_ps(lhs.m_value, rhs.m_rows[0]);
        result.m_rows[1] = _mm_mul_ps(lhs.m_value, rhs.m_rows[1]);
        result.m_rows[2] = _mm_mul_ps(lhs.m_value, rhs.m_rows[2]);
        return result;
    }
}
