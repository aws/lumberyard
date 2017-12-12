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
    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateIdentity()
    {
        Matrix4x4 result;
        SimdVectorType row0 = *(__m128*)&Internal::g_simd1000;
        result.m_rows[0] = row0;
        result.m_rows[1] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 1, 0, 1));
        result.m_rows[2] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 0, 1, 1));
        result.m_rows[3] = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(0, 1, 1, 1));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateZero()
    {
        Matrix4x4 result;
        SimdVectorType v0 = _mm_setzero_ps();
        result.m_rows[0] = v0;
        result.m_rows[1] = v0;
        result.m_rows[2] = v0;
        result.m_rows[3] = v0;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateFromValue(const VectorFloat& value)
    {
        Matrix4x4 result;
        result.m_rows[0] = value.m_value;
        result.m_rows[1] = value.m_value;
        result.m_rows[2] = value.m_value;
        result.m_rows[3] = value.m_value;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateRotationX(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        Matrix4x4 result;
        result.m_rows[0] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 2, 3));
        result.m_rows[1] = _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 1, 0, 2));
        result.m_rows[2] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 0, 1, 2));
        result.m_rows[3] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(3, 2, 2, 2));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateRotationY(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        Matrix4x4 result;
        result.m_rows[0] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 1, 2, 0));
        result.m_rows[1] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 3, 2));
        result.m_rows[2] = _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 0, 2, 1));
        result.m_rows[3] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(3, 2, 2, 2));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateRotationZ(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        Matrix4x4 result;
        result.m_rows[0] = _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 2, 1, 0));
        result.m_rows[1] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 0, 1));
        result.m_rows[2] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 3, 2, 2));
        result.m_rows[3] = _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(3, 2, 2, 2));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateFromQuaternion(const Quaternion& q)
    {
        Matrix4x4 result;
        result.SetRotationPartFromQuaternion(q);
        result.SetPosition(Vector3::CreateZero());
        result.SetRow(3, Vector4::CreateAxisW());
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateFromQuaternionAndTranslation(const Quaternion& q, const Vector3& p)
    {
        Matrix4x4 result;
        result.SetRotationPartFromQuaternion(q);
        result.SetPosition(p);
        result.SetRow(3, Vector4::CreateAxisW());
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateScale(const Vector3& scale)
    {
        SimdVectorType diag = scale.m_value;
        SimdVectorType v1000 = *(SimdVectorType*)&Internal::g_simd1000;
        SimdVectorType vXY10 = _mm_movelh_ps(diag, v1000);
        SimdVectorType v00ZW = _mm_movehl_ps(diag, v1000);
        Matrix4x4 result;
        result.m_rows[0] = _mm_shuffle_ps(vXY10, vXY10, _MM_SHUFFLE(3, 3, 3, 0));
        result.m_rows[1] = _mm_shuffle_ps(vXY10, vXY10, _MM_SHUFFLE(3, 3, 1, 3));
        result.m_rows[2] = _mm_shuffle_ps(v00ZW, v00ZW, _MM_SHUFFLE(0, 2, 0, 0));
        result.m_rows[3] = _mm_shuffle_ps(v1000, v1000, _MM_SHUFFLE(0, 3, 3, 3));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateDiagonal(const Vector4& diagonal)
    {
        SimdVectorType diag = diagonal.m_value;
        SimdVectorType v0 = _mm_setzero_ps();
        SimdVectorType vXY00 = _mm_movelh_ps(diag, v0);
        SimdVectorType v00ZW = _mm_movehl_ps(diag, v0);
        Matrix4x4 result;
        result.m_rows[0] = _mm_shuffle_ps(vXY00, vXY00, _MM_SHUFFLE(3, 3, 3, 0));
        result.m_rows[1] = _mm_shuffle_ps(vXY00, vXY00, _MM_SHUFFLE(3, 3, 1, 3));
        result.m_rows[2] = _mm_shuffle_ps(v00ZW, v00ZW, _MM_SHUFFLE(0, 2, 0, 0));
        result.m_rows[3] = _mm_shuffle_ps(v00ZW, v00ZW, _MM_SHUFFLE(3, 0, 0, 0));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& translation)
    {
        SimdVectorType v1000 = *(SimdVectorType*)&Internal::g_simd1000;
        SimdVectorType vXY10 = _mm_shuffle_ps(translation.m_value, v1000, _MM_SHUFFLE(1, 0, 1, 0));
        SimdVectorType vYZ10 = _mm_shuffle_ps(translation.m_value, v1000, _MM_SHUFFLE(1, 0, 2, 1));
        Matrix4x4 result;
        result.m_rows[0] = _mm_shuffle_ps(vXY10, vXY10, _MM_SHUFFLE(0, 3, 3, 2));
        result.m_rows[1] = _mm_shuffle_ps(vXY10, vXY10, _MM_SHUFFLE(1, 3, 2, 3));
        result.m_rows[2] = _mm_shuffle_ps(vYZ10, vYZ10, _MM_SHUFFLE(1, 2, 3, 3));
        result.m_rows[3] = _mm_shuffle_ps(vYZ10, vYZ10, _MM_SHUFFLE(2, 3, 3, 3));
        return result;
    }

    AZ_MATH_FORCE_INLINE VectorFloat Matrix4x4::GetElement(int row, int col) const
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return Vector4(m_rows[row]).GetElement(col);
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetElement(int row, int col, const VectorFloat& value)
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        Vector4 temp(m_rows[row]);
        temp.SetElement(col, value);
        m_rows[row] = temp.m_value;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::GetRow(int row) const
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        return Vector4(m_rows[row]);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::GetRowAsVector3(int row) const
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        return Vector3(m_rows[row]);
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetRow(int row, const Vector3& v, const VectorFloat& w)
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        SimdVectorType vZZWW = _mm_shuffle_ps(v.m_value, w.m_value, _MM_SHUFFLE(0, 0, 2, 2));
        m_rows[row] = _mm_shuffle_ps(v.m_value, vZZWW, _MM_SHUFFLE(2, 0, 1, 0));
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetRow(int row, const Vector4& v)
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        m_rows[row] = v.m_value;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::GetColumn(int col) const
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        //Note that here we are optimizing for the case where the column index is a constant, and assuming the compiler will AZ_MATH_FORCE_INLINE this
        //function and remove the unnecessary branches. If col is not a constant it would probably be quicker to read the floats and
        // construct the vector from them directly.
        SimdVectorType vXXYY, vZZWW;
        if (col == 0)
        {
            vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(0, 0, 0, 0));
            vZZWW = _mm_shuffle_ps(m_rows[2], m_rows[3], _MM_SHUFFLE(0, 0, 0, 0));
        }
        else if (col == 1)
        {
            vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(1, 1, 1, 1));
            vZZWW = _mm_shuffle_ps(m_rows[2], m_rows[3], _MM_SHUFFLE(1, 1, 1, 1));
        }
        else if (col == 2)
        {
            vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(2, 2, 2, 2));
            vZZWW = _mm_shuffle_ps(m_rows[2], m_rows[3], _MM_SHUFFLE(2, 2, 2, 2));
        }
        else
        {
            vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(3, 3, 3, 3));
            vZZWW = _mm_shuffle_ps(m_rows[2], m_rows[3], _MM_SHUFFLE(3, 3, 3, 3));
        }
        return Vector4(_mm_shuffle_ps(vXXYY, vZZWW, _MM_SHUFFLE(2, 0, 2, 0)));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::GetColumnAsVector3(int col) const
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
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
        else if (col == 2)
        {
            SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(2, 2, 2, 2));
            return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(2, 2, 2, 0)));
        }
        else
        {
            SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(3, 3, 3, 3));
            return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(3, 3, 2, 0)));
        }
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetColumn(int col, const Vector4& v)
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        //TODO: Profile to see if it's faster to set with shuffles or memory writes... depends on if the matrix is already in registers I guess
#if 0
        SetElement(0, col, v.GetX());
        SetElement(1, col, v.GetY());
        SetElement(2, col, v.GetZ());
        SetElement(3, col, v.GetW());
#else
        //Note we are optimizing for the case where the column index is a constant. See comment in GetColumn for more information.
        if (col == 0)
        {
            m_rows[0] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(1, 1, 0, 0)), m_rows[0], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[1] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(1, 1, 1, 1)), m_rows[1], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[2] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(1, 1, 2, 2)), m_rows[2], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[3] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[3], _MM_SHUFFLE(1, 1, 3, 3)), m_rows[3], _MM_SHUFFLE(3, 2, 2, 0));
        }
        else if (col == 1)
        {
            m_rows[0] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(0, 0, 0, 0)), m_rows[0], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[1] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(0, 0, 1, 1)), m_rows[1], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[2] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(0, 0, 2, 2)), m_rows[2], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[3] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[3], _MM_SHUFFLE(0, 0, 3, 3)), m_rows[3], _MM_SHUFFLE(3, 2, 0, 2));
        }
        else if (col == 2)
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(3, 3, 0, 0)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(3, 3, 1, 1)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(3, 3, 2, 2)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[3] = _mm_shuffle_ps(m_rows[3], _mm_shuffle_ps(v.m_value, m_rows[3], _MM_SHUFFLE(3, 3, 3, 3)), _MM_SHUFFLE(2, 0, 1, 0));
        }
        else
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(2, 2, 0, 0)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(2, 2, 1, 1)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[3] = _mm_shuffle_ps(m_rows[3], _mm_shuffle_ps(v.m_value, m_rows[3], _MM_SHUFFLE(2, 2, 3, 3)), _MM_SHUFFLE(0, 2, 1, 0));
        }
#endif
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetColumn(int col, const Vector3& v, const VectorFloat& w)
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        //TODO: Profile to see if it's faster to set with shuffles or memory writes... depends on if the matrix is already in registers I guess
#if 0
        SetElement(0, col, v.GetX());
        SetElement(1, col, v.GetY());
        SetElement(2, col, v.GetZ());
        SetElement(3, col, w);
#else
        //Note we are optimizing for the case where the column index is a constant. See comment in GetColumn for more information.
        if (col == 0)
        {
            m_rows[0] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(1, 1, 0, 0)), m_rows[0], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[1] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(1, 1, 1, 1)), m_rows[1], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[2] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(1, 1, 2, 2)), m_rows[2], _MM_SHUFFLE(3, 2, 2, 0));
            m_rows[3] = _mm_shuffle_ps(_mm_shuffle_ps(w.m_value, m_rows[3], _MM_SHUFFLE(1, 1, 0, 0)), m_rows[3], _MM_SHUFFLE(3, 2, 2, 0));
        }
        else if (col == 1)
        {
            m_rows[0] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(0, 0, 0, 0)), m_rows[0], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[1] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(0, 0, 1, 1)), m_rows[1], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[2] = _mm_shuffle_ps(_mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(0, 0, 2, 2)), m_rows[2], _MM_SHUFFLE(3, 2, 0, 2));
            m_rows[3] = _mm_shuffle_ps(_mm_shuffle_ps(w.m_value, m_rows[3], _MM_SHUFFLE(0, 0, 0, 0)), m_rows[3], _MM_SHUFFLE(3, 2, 0, 2));
        }
        else if (col == 2)
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(3, 3, 0, 0)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(3, 3, 1, 1)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(3, 3, 2, 2)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[3] = _mm_shuffle_ps(m_rows[3], _mm_shuffle_ps(w.m_value, m_rows[3], _MM_SHUFFLE(3, 3, 0, 0)), _MM_SHUFFLE(2, 0, 1, 0));
        }
        else
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(2, 2, 0, 0)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(2, 2, 1, 1)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[3] = _mm_shuffle_ps(m_rows[3], _mm_shuffle_ps(w.m_value, m_rows[3], _MM_SHUFFLE(2, 2, 0, 0)), _MM_SHUFFLE(0, 2, 1, 0));
        }
#endif
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::GetPosition() const
    {
        SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(3, 3, 3, 3));
        return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(3, 3, 2, 0)));
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetPosition(const Vector3& v)
    {
        m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(2, 2, 0, 0)), _MM_SHUFFLE(0, 2, 1, 0));
        m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(2, 2, 1, 1)), _MM_SHUFFLE(0, 2, 1, 0));
        m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(0, 2, 1, 0));
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
    {
        Matrix4x4 out;
        SimdVectorType rhsRow0 = rhs.m_rows[0];
        SimdVectorType rhsRow1 = rhs.m_rows[1];
        SimdVectorType rhsRow2 = rhs.m_rows[2];
        SimdVectorType rhsRow3 = rhs.m_rows[3];
        for (int i = 0; i < 4; ++i)
        {
            SimdVectorType row = m_rows[i];
            out.m_rows[i] = _mm_add_ps(_mm_add_ps(_mm_add_ps(
                            _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(0, 0, 0, 0)), rhsRow0),
                            _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(1, 1, 1, 1)), rhsRow1)),
                        _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(2, 2, 2, 2)), rhsRow2)),
                    _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(3, 3, 3, 3)), rhsRow3));
        }
        return out;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::operator*(const Vector3& rhs) const
    {
        //Transpose is needed here, for the multiply-add simd implementation. We should store the Matrix4x4 as
        // column-major instead, unlike the Transform there's no real reason not to do this.
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = m_rows[3];
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        SimdVectorType v = rhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(col0, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(col1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(col2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                col3));
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::operator*(const Vector4& rhs) const
    {
        //see comment in operator*(Vector3&) about transpose
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = m_rows[3];
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        SimdVectorType v = rhs.m_value;
        return Vector4(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(col0, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(col1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(col2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                _mm_mul_ps(col3, _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)))));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::TransposedMultiply3x3(const Vector3& rhs) const
    {
        SimdVectorType v = rhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                    _mm_mul_ps(m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                _mm_mul_ps(m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::Multiply3x3(const Vector3& rhs) const
    {
        //see comment in operator* about transpose
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = col2;     //don't care about this column, set it to whatever
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        SimdVectorType v = rhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(col0, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                    _mm_mul_ps(col1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                _mm_mul_ps(col2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))));
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::GetTranspose() const
    {
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = m_rows[3];
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        Matrix4x4 result;
        result.m_rows[0] = col0;
        result.m_rows[1] = col1;
        result.m_rows[2] = col2;
        result.m_rows[3] = col3;
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::GetInverseFast() const
    {
        SimdVectorType pos = _mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(m_rows[0], _mm_shuffle_ps(m_rows[0], m_rows[0], _MM_SHUFFLE(3, 3, 3, 3))),
                    _mm_mul_ps(m_rows[1], _mm_shuffle_ps(m_rows[1], m_rows[1], _MM_SHUFFLE(3, 3, 3, 3)))),
                _mm_mul_ps(m_rows[2], _mm_shuffle_ps(m_rows[2], m_rows[2], _MM_SHUFFLE(3, 3, 3, 3))));
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = _mm_xor_ps(pos, *(SimdVectorType*)&Internal::g_simdNegateMask);
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        Matrix4x4 result;
        result.m_rows[0] = col0;
        result.m_rows[1] = col1;
        result.m_rows[2] = col2;
        result.m_rows[3] = *(__m128*)&Internal::g_simd0001;
        return result;
    }

    AZ_MATH_FORCE_INLINE bool Matrix4x4::IsClose(const Matrix4x4& rhs, const VectorFloat& tolerance) const
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!Vector4(m_rows[row]).IsClose(Vector4(rhs.m_rows[row]), tolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE bool Matrix4x4::operator==(const Matrix4x4& rhs) const
    {
        if (Vector4(m_rows[0]) != Vector4(rhs.m_rows[0]))
        {
            return false;
        }
        if (Vector4(m_rows[1]) != Vector4(rhs.m_rows[1]))
        {
            return false;
        }
        if (Vector4(m_rows[2]) != Vector4(rhs.m_rows[2]))
        {
            return false;
        }
        if (Vector4(m_rows[3]) != Vector4(rhs.m_rows[3]))
        {
            return false;
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::GetDiagonal() const
    {
        SimdVectorType vX__Y = _mm_movelh_ps(m_rows[0], m_rows[1]);
        SimdVectorType v_WZ_ = _mm_movehl_ps(m_rows[2], m_rows[3]);
        return Vector4(_mm_shuffle_ps(vX__Y, v_WZ_, _MM_SHUFFLE(1, 2, 3, 0)));
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs)
    {
        SimdVectorType v = lhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(rhs.m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(rhs.m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(rhs.m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                rhs.m_rows[3]));
    }

    AZ_MATH_FORCE_INLINE const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs)
    {
        SimdVectorType v = lhs.m_value;
        return Vector4(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(rhs.m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(rhs.m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(rhs.m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                _mm_mul_ps(rhs.m_rows[3], _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)))));
    }
}
