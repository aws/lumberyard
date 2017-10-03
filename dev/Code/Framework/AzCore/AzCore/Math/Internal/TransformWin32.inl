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
    AZ_MATH_FORCE_INLINE Transform::Transform(SimdVectorType row0, SimdVectorType row1, SimdVectorType row2)
    {
        m_rows[0] = row0;
        m_rows[1] = row1;
        m_rows[2] = row2;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateIdentity()
    {
        SimdVectorType row0 = *(__m128*)&Internal::g_simd1000;
        SimdVectorType row1 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 1, 0, 1));
        SimdVectorType row2 = _mm_shuffle_ps(row0, row0, _MM_SHUFFLE(3, 0, 1, 1));
        return Transform(row0, row1, row2);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateZero()
    {
        SimdVectorType v0 = _mm_setzero_ps();
        return Transform(v0, v0, v0);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateFromValue(const VectorFloat& value)
    {
        return Transform(value.m_value, value.m_value, value.m_value);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateRotationX(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        return Transform(_mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 2, 3)),
            _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 1, 0, 2)),
            _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 0, 1, 2)));
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateRotationY(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        return Transform(_mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 1, 2, 0)),
            _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 3, 2)),
            _mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 0, 2, 1)));
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateRotationZ(const VectorFloat& angle)
    {
        VectorFloat sin, cos;
        angle.GetSinCos(sin, cos);
        SimdVectorType vCS01 = _mm_setr_ps(cos, sin, 0.0f, 1.0f);
        SimdVectorType vCNegS01 = _mm_xor_ps(vCS01, *(SimdVectorType*)&Internal::g_simdNegateYMask);
        return Transform(_mm_shuffle_ps(vCNegS01, vCNegS01, _MM_SHUFFLE(2, 2, 1, 0)),
            _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 2, 0, 1)),
            _mm_shuffle_ps(vCS01, vCS01, _MM_SHUFFLE(2, 3, 2, 2)));
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateFromQuaternion(const Quaternion& q)
    {
        Transform result;
        result.SetRotationPartFromQuaternion(q);
        result.SetPosition(Vector3::CreateZero());
        return result;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateFromQuaternionAndTranslation(const Quaternion& q, const Vector3& p)
    {
        Transform result;
        result.SetRotationPartFromQuaternion(q);
        result.SetPosition(p);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateScale(const Vector3& scale)
    {
        return CreateDiagonal(scale);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateDiagonal(const Vector3& diagonal)
    {
        SimdVectorType diag0 = _mm_and_ps(diagonal.m_value, *(SimdVectorType*)&Internal::g_simdWMask);
        return Transform(_mm_shuffle_ps(diag0, diag0, _MM_SHUFFLE(3, 3, 3, 0)),
            _mm_shuffle_ps(diag0, diag0, _MM_SHUFFLE(3, 3, 1, 3)),
            _mm_shuffle_ps(diag0, diag0, _MM_SHUFFLE(3, 2, 3, 3)));
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateTranslation(const Vector3& translation)
    {
        SimdVectorType v1000 = *(SimdVectorType*)&Internal::g_simd1000;
        SimdVectorType vXY10 = _mm_shuffle_ps(translation.m_value, v1000, _MM_SHUFFLE(1, 0, 1, 0));
        SimdVectorType vYZ10 = _mm_shuffle_ps(translation.m_value, v1000, _MM_SHUFFLE(1, 0, 2, 1));
        return Transform(_mm_shuffle_ps(vXY10, vXY10, _MM_SHUFFLE(0, 3, 3, 2)),
            _mm_shuffle_ps(vXY10, vXY10, _MM_SHUFFLE(1, 3, 2, 3)),
            _mm_shuffle_ps(vYZ10, vYZ10, _MM_SHUFFLE(1, 2, 3, 3)));
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Transform::GetElement(int row, int col) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return Vector4(m_rows[row]).GetElement(col);
    }
    AZ_MATH_FORCE_INLINE void Transform::SetElement(int row, int col, const VectorFloat& value)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        Vector4 temp(m_rows[row]);
        temp.SetElement(col, value);
        m_rows[row] = temp.m_value;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Transform::GetRow(int row) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        return Vector4(m_rows[row]);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Transform::GetRowAsVector3(int row) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        return Vector3(m_rows[row]);
    }
    AZ_MATH_FORCE_INLINE void Transform::SetRow(int row, const Vector3& v, const VectorFloat& w)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        SimdVectorType vZZWW = _mm_shuffle_ps(v.m_value, w.m_value, _MM_SHUFFLE(0, 0, 2, 2));
        m_rows[row] = _mm_shuffle_ps(v.m_value, vZZWW, _MM_SHUFFLE(2, 0, 1, 0));
    }
    AZ_MATH_FORCE_INLINE void Transform::SetRow(int row, const Vector4& v)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        m_rows[row] = v.m_value;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::GetColumn(int col) const
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
    AZ_MATH_FORCE_INLINE void Transform::SetColumn(int col, const Vector3& v)
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
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
        else if (col == 2)
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(3, 3, 0, 0)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(3, 3, 1, 1)), _MM_SHUFFLE(2, 0, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(3, 3, 2, 2)), _MM_SHUFFLE(2, 0, 1, 0));
        }
        else
        {
            m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(2, 2, 0, 0)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(2, 2, 1, 1)), _MM_SHUFFLE(0, 2, 1, 0));
            m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(0, 2, 1, 0));
        }
        #endif
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::GetPosition() const
    {
        SimdVectorType vXXYY = _mm_shuffle_ps(m_rows[0], m_rows[1], _MM_SHUFFLE(3, 3, 3, 3));
        return Vector3(_mm_shuffle_ps(vXXYY, m_rows[2], _MM_SHUFFLE(3, 3, 2, 0)));
    }
    AZ_MATH_FORCE_INLINE void Transform::SetPosition(const Vector3& v)
    {
        m_rows[0] = _mm_shuffle_ps(m_rows[0], _mm_shuffle_ps(v.m_value, m_rows[0], _MM_SHUFFLE(2, 2, 0, 0)), _MM_SHUFFLE(0, 2, 1, 0));
        m_rows[1] = _mm_shuffle_ps(m_rows[1], _mm_shuffle_ps(v.m_value, m_rows[1], _MM_SHUFFLE(2, 2, 1, 1)), _MM_SHUFFLE(0, 2, 1, 0));
        m_rows[2] = _mm_shuffle_ps(m_rows[2], _mm_shuffle_ps(v.m_value, m_rows[2], _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(0, 2, 1, 0));
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::operator*(const Transform& rhs) const
    {
        Transform out;
        SimdVectorType wMask = *(SimdVectorType*)&Internal::g_simdWMask;
        SimdVectorType rhsRow0 = rhs.m_rows[0];
        SimdVectorType rhsRow1 = rhs.m_rows[1];
        SimdVectorType rhsRow2 = rhs.m_rows[2];
        for (int i = 0; i < 3; ++i)
        {
            SimdVectorType row = m_rows[i];
            out.m_rows[i] = _mm_add_ps(_mm_add_ps(_mm_add_ps(
                            _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(0, 0, 0, 0)), rhsRow0),
                            _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(1, 1, 1, 1)), rhsRow1)),
                        _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(2, 2, 2, 2)), rhsRow2)),
                    _mm_andnot_ps(wMask, row));
        }
        return out;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::operator*(const Vector3& rhs) const
    {
        //Transpose is needed here, for the multiply-add simd implementation. We could store the Transform as column-major, which
        // would remove the transpose but increase the size of the Transform from 48 bytes to 64 bytes.
        //We could also use row vectors instead of column vectors, but that would break X360 which has a dot product instruction // ACCEPTED_USE
        // and prefers the current order.
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = col2;     //don't care about this column, set it to whatever
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        SimdVectorType v = rhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(col0, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(col1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(col2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                col3));
    }

    AZ_MATH_FORCE_INLINE const Vector4 Transform::operator*(const Vector4& rhs) const
    {
        //see comment in operator*(Vector3&) about transpose
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = *(SimdVectorType*)&Internal::g_simd0001;
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);

        SimdVectorType v = rhs.m_value;
        return Vector4(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(col0, _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(col1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(col2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                _mm_mul_ps(col3, _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)))));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::TransposedMultiply3x3(const Vector3& rhs) const
    {
        SimdVectorType v = rhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                    _mm_mul_ps(m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                _mm_mul_ps(m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::Multiply3x3(const Vector3& rhs) const
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

    AZ_MATH_FORCE_INLINE const Transform Transform::GetTranspose() const
    {
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = _mm_setzero_ps(); //position is set to zero in result
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);
        return Transform(col0, col1, col2);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::GetTranspose3x3() const
    {
        SimdVectorType col0 = m_rows[0];
        SimdVectorType col1 = m_rows[1];
        SimdVectorType col2 = m_rows[2];
        SimdVectorType col3 = GetPosition().m_value;
        _MM_TRANSPOSE4_PS(col0, col1, col2, col3);
        return Transform(col0, col1, col2);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::GetInverseFast() const
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
        return Transform(col0, col1, col2);
    }

    AZ_MATH_FORCE_INLINE bool Transform::IsClose(const Transform& rhs, const VectorFloat& tolerance) const
    {
        for (int row = 0; row < 3; ++row)
        {
            if (!Vector4(m_rows[row]).IsClose(Vector4(rhs.m_rows[row]), tolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE bool Transform::operator==(const Transform& rhs) const
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
        return true;
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Transform& rhs)
    {
        SimdVectorType v = lhs.m_value;
        return Vector3(_mm_add_ps(_mm_add_ps(
                    _mm_mul_ps(rhs.m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                    _mm_mul_ps(rhs.m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                _mm_mul_ps(rhs.m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))));
    }

    AZ_MATH_FORCE_INLINE const Vector4 operator*(const Vector4& lhs, const Transform& rhs)
    {
        SimdVectorType v = lhs.m_value;
        return Vector4(_mm_add_ps(_mm_add_ps(_mm_add_ps(
                        _mm_mul_ps(rhs.m_rows[0], _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0))),
                        _mm_mul_ps(rhs.m_rows[1], _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)))),
                    _mm_mul_ps(rhs.m_rows[2], _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)))),
                _mm_andnot_ps(*(SimdVectorType*)&Internal::g_simdWMask, v)));
    }
}
