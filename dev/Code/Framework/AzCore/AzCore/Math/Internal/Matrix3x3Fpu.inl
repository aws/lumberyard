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
        result.SetRow(0, 1.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, 1.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateZero()
    {
        return Matrix3x3::CreateFromValue(0.0f);
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromValue(const VectorFloat& value)
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, value);
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateRotationX(const VectorFloat& angle)
    {
        Matrix3x3 result;
        float c = cosf(angle);
        float s = sinf(angle);
        result.SetRow(0, 1.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, c, -s);
        result.SetRow(2, 0.0f, s, c);
        return result;
    }


    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateRotationY(const VectorFloat& angle)
    {
        Matrix3x3 result;
        float c = cosf(angle);
        float s = sinf(angle);
        result.SetRow(0, c, 0.0f, s);
        result.SetRow(1, 0.0f, 1.0f, 0.0f);
        result.SetRow(2, -s, 0.0f, c);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateRotationZ(const VectorFloat& angle)
    {
        Matrix3x3 result;
        float c = cosf(angle);
        float s = sinf(angle);
        result.SetRow(0, c, -s, 0.0f);
        result.SetRow(1, s, c, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromTransform(const Transform& t)
    {
        Matrix3x3 result;
        result.SetRow(0, t.GetRowAsVector3(0));
        result.SetRow(1, t.GetRowAsVector3(1));
        result.SetRow(2, t.GetRowAsVector3(2));
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateFromMatrix4x4(const Matrix4x4& m)
    {
        Matrix3x3 result;
        result.SetRow(0, m.GetRowAsVector3(0));
        result.SetRow(1, m.GetRowAsVector3(1));
        result.SetRow(2, m.GetRowAsVector3(2));
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
        Matrix3x3 result;
        result.SetRow(0, diagonal.GetX(), 0.0f, 0.0f);
        result.SetRow(1, 0.0f, diagonal.GetY(), 0.0f);
        result.SetRow(2, 0.0f, 0.0f, diagonal.GetZ());
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::CreateCrossProduct(const Vector3& p)
    {
        Matrix3x3 result;
        result.SetRow(0, 0.0f, -p.GetZ(), p.GetY());
        result.SetRow(1, p.GetZ(), 0.0f, -p.GetX());
        result.SetRow(2, -p.GetY(), p.GetX(), 0.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE VectorFloat Matrix3x3::GetElement(int row, int col) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        return m_values[col][row];
    }
    AZ_MATH_FORCE_INLINE void Matrix3x3::SetElement(int row, int col, const VectorFloat& value)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        m_values[col][row] = value;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::GetRow(int row) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        return Vector3(m_values[0][row], m_values[1][row], m_values[2][row]);
    }

    AZ_MATH_FORCE_INLINE void Matrix3x3::SetRow(int row, const Vector3& v)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        m_values[0][row] = v.GetX();
        m_values[1][row] = v.GetY();
        m_values[2][row] = v.GetZ();
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::GetColumn(int col) const
    {
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        return Vector3::CreateFromFloat3(&m_values[col][0]);
    }

    AZ_MATH_FORCE_INLINE void Matrix3x3::SetColumn(int col, const Vector3& v)
    {
        AZ_Assert((col >= 0) && (col < 3), "Invalid index for component access!\n");
        m_values[col][0] = v.GetX();
        m_values[col][1] = v.GetY();
        m_values[col][2] = v.GetZ();
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) const
    {
        Matrix3x3 out;
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; col++)
            {
                out.m_values[col][row] = 0.0f;
                for (int k = 0; k < 3; ++k)
                {
                    out.m_values[col][row] += m_values[k][row] * rhs.m_values[col][k];
                }
            }
        }
        return out;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::operator*(const Vector3& rhs) const
    {
        return Vector3(m_values[0][0] * rhs(0) + m_values[1][0] * rhs(1) + m_values[2][0] * rhs(2),
            m_values[0][1] * rhs(0) + m_values[1][1] * rhs(1) + m_values[2][1] * rhs(2),
            m_values[0][2] * rhs(0) + m_values[1][2] * rhs(1) + m_values[2][2] * rhs(2));
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator+(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, GetElement(i, j) + rhs.GetElement(i, j));
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator-(const Matrix3x3& rhs) const
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, GetElement(i, j) - rhs.GetElement(i, j));
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator*(const VectorFloat& multiplier) const
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, GetElement(i, j) * multiplier);
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator/(const VectorFloat& divisor) const
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, GetElement(i, j) / divisor);
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::operator-() const
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, -GetElement(i, j));
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::GetTranspose() const
    {
        Matrix3x3 ret;
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                ret.m_values[col][row] = m_values[row][col];
            }
        }
        return ret;
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::TransposedMultiply(const Matrix3x3& m) const
    {
        // \ref bullet btMatrix3x3::transposeTimes - practically this is this.Inverse * m;
        Matrix3x3 ret;

        Vector3 mc0 = m.GetColumn(0);
        Vector3 mc1 = m.GetColumn(1);
        Vector3 mc2 = m.GetColumn(2);

        Vector3 cc0 = GetColumn(0);
        Vector3 cc1 = GetColumn(1);
        Vector3 cc2 = GetColumn(2);

        ret.SetRow(0, cc0.Dot(mc0), cc0.Dot(mc1), cc0.Dot(mc2));
        ret.SetRow(1, cc1.Dot(mc0), cc1.Dot(mc1), cc1.Dot(mc2));
        ret.SetRow(2, cc2.Dot(mc0), cc2.Dot(mc1), cc2.Dot(mc2));

        return ret;
    }

    AZ_MATH_FORCE_INLINE bool Matrix3x3::IsClose(const Matrix3x3& rhs, const VectorFloat& tolerance) const
    {
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                if (fabsf(rhs(row, col) - GetElement(row, col)) > tolerance)
                {
                    return false;
                }
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE bool Matrix3x3::operator==(const Matrix3x3& rhs) const
    {
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                if ((float)rhs.GetElement(row, col) != (float)GetElement(row, col))
                {
                    return false;
                }
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix3x3::GetDiagonal() const
    {
        return Vector3(GetElement(0, 0), GetElement(1, 1), GetElement(2, 2));
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::GetInverseFull() const
    {
        Matrix3x3 out = GetAdjugate();

        //calculate the determinant
        float det = m_values[0][0] * out.m_values[0][0] + m_values[0][1] * out.m_values[1][0] + m_values[0][2] * out.m_values[2][0];

        //divide cofactors by determinant
        float f = (det == 0.0f) ? 10000000.0f : 1.0f / det;
        out.m_values[0][0] *= f;
        out.m_values[1][0] *= f;
        out.m_values[2][0] *= f;
        out.m_values[0][1] *= f;
        out.m_values[1][1] *= f;
        out.m_values[2][1] *= f;
        out.m_values[0][2] *= f;
        out.m_values[1][2] *= f;
        out.m_values[2][2] *= f;

        return out;
    }

    AZ_MATH_FORCE_INLINE VectorFloat Matrix3x3::GetDeterminant() const
    {
        return m_values[0][0] * (m_values[1][1] * m_values[2][2] - m_values[2][1] * m_values[1][2])
               + m_values[0][1] * (m_values[1][2] * m_values[2][0] - m_values[2][2] * m_values[1][0])
               + m_values[0][2] * (m_values[1][0] * m_values[2][1] - m_values[2][0] * m_values[1][1]);
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 Matrix3x3::GetAdjugate() const
    {
        Matrix3x3 out;

        out.m_values[0][0] = (m_values[1][1] * m_values[2][2] - m_values[2][1] * m_values[1][2]);
        out.m_values[0][1] = (m_values[2][1] * m_values[0][2] - m_values[0][1] * m_values[2][2]);
        out.m_values[0][2] = (m_values[0][1] * m_values[1][2] - m_values[1][1] * m_values[0][2]);

        out.m_values[1][0] = (m_values[1][2] * m_values[2][0] - m_values[2][2] * m_values[1][0]);
        out.m_values[1][1] = (m_values[2][2] * m_values[0][0] - m_values[0][2] * m_values[2][0]);
        out.m_values[1][2] = (m_values[0][2] * m_values[1][0] - m_values[1][2] * m_values[0][0]);

        out.m_values[2][0] = (m_values[1][0] * m_values[2][1] - m_values[2][0] * m_values[1][1]);
        out.m_values[2][1] = (m_values[2][0] * m_values[0][1] - m_values[0][0] * m_values[2][1]);
        out.m_values[2][2] = (m_values[0][0] * m_values[1][1] - m_values[1][0] * m_values[0][1]);

        return out;
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs)
    {
        return Vector3(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0),
            lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1),
            lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2));
    }

    AZ_MATH_FORCE_INLINE const Matrix3x3 operator*(const VectorFloat& lhs, const Matrix3x3& rhs)
    {
        Matrix3x3 result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                result.SetElement(i, j, lhs * rhs(i, j));
            }
        }
        return result;
    }
}
