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
        result.SetRow(0, 1.0f, 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, 1.0f, 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, 1.0f, 0.0f);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateZero()
    {
        return Matrix4x4::CreateFromValue(0.0f);
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateFromValue(const VectorFloat& value)
    {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                result.SetElement(i, j, value);
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateRotationX(const VectorFloat& angle)
    {
        Matrix4x4 result;
        VectorFloat s, c;
        angle.GetSinCos(s, c);
        result.SetRow(0, 1.0f, 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, c, -s, 0.0f);
        result.SetRow(2, 0.0f, s, c, 0.0f);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }


    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateRotationY(const VectorFloat& angle)
    {
        Matrix4x4 result;
        VectorFloat s, c;
        angle.GetSinCos(s, c);
        result.SetRow(0, c, 0.0f, s, 0.0f);
        result.SetRow(1, 0.0f, 1.0f, 0.0f, 0.0f);
        result.SetRow(2, -s, 0.0f, c, 0.0f);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateRotationZ(const VectorFloat& angle)
    {
        Matrix4x4 result;
        VectorFloat s, c;
        angle.GetSinCos(s, c);
        result.SetRow(0, c, -s, 0.0f, 0.0f);
        result.SetRow(1, s, c, 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, 1.0f, 0.0f);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateFromQuaternion(const Quaternion& q)
    {
        Matrix4x4 result;
        result.SetRotationPartFromQuaternion(q);
        result.SetPosition(Vector3(0.0f));
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateFromQuaternionAndTranslation(const Quaternion& q, const Vector3& p)
    {
        Matrix4x4 result;
        result.SetRotationPartFromQuaternion(q);
        result.SetPosition(p);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateScale(const Vector3& scale)
    {
        return CreateDiagonal(Vector4(scale.GetX(), scale.GetY(), scale.GetZ(), 1.0f));
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateDiagonal(const Vector4& diagonal)
    {
        Matrix4x4 result;
        result.SetRow(0, diagonal.GetX(), 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, diagonal.GetY(), 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, diagonal.GetZ(), 0.0f);
        result.SetRow(3, 0.0f, 0.0f, 0.0f, diagonal.GetW());
        return result;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& translation)
    {
        Matrix4x4 result;
        result.SetRow(0, 1.0f, 0.0f, 0.0f, translation.GetX());
        result.SetRow(1, 0.0f, 1.0f, 0.0f, translation.GetY());
        result.SetRow(2, 0.0f, 0.0f, 1.0f, translation.GetZ());
        result.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE VectorFloat Matrix4x4::GetElement(int row, int col) const
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return m_values[col][row];
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetElement(int row, int col, const VectorFloat& value)
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        m_values[col][row] = value;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::GetRow(int row) const
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        return Vector4(m_values[0][row], m_values[1][row], m_values[2][row], m_values[3][row]);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::GetRowAsVector3(int row) const
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        return Vector3(m_values[0][row], m_values[1][row], m_values[2][row]);
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetRow(int row, const Vector3& v, const VectorFloat& w)
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        m_values[0][row] = v.GetX();
        m_values[1][row] = v.GetY();
        m_values[2][row] = v.GetZ();
        m_values[3][row] = w;
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetRow(int row, const Vector4& v)
    {
        AZ_Assert((row >= 0) && (row < 4), "Invalid index for component access!\n");
        m_values[0][row] = v.GetX();
        m_values[1][row] = v.GetY();
        m_values[2][row] = v.GetZ();
        m_values[3][row] = v.GetW();
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::GetColumn(int col) const
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return *reinterpret_cast<const Vector4*>(&m_values[col][0]);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::GetColumnAsVector3(int col) const
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return *reinterpret_cast<const Vector3*>(&m_values[col][0]);
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetColumn(int col, const Vector3& v, const VectorFloat& w)
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        m_values[col][0] = v.GetX();
        m_values[col][1] = v.GetY();
        m_values[col][2] = v.GetZ();
        m_values[col][3] = w;
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetColumn(int col, const Vector4& v)
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        m_values[col][0] = v.GetX();
        m_values[col][1] = v.GetY();
        m_values[col][2] = v.GetZ();
        m_values[col][3] = v.GetW();
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::GetPosition() const
    {
        return Vector3(m_values[3][0], m_values[3][1], m_values[3][2]);
    }

    AZ_MATH_FORCE_INLINE void Matrix4x4::SetPosition(const Vector3& v)
    {
        m_values[3][0] = v.GetX();
        m_values[3][1] = v.GetY();
        m_values[3][2] = v.GetZ();
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
    {
        Matrix4x4 out;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; col++)
            {
                out.m_values[col][row] = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
                    out.m_values[col][row] += m_values[k][row] * rhs.m_values[col][k];
                }
            }
        }
        return out;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::operator*(const Vector3& rhs) const
    {
        return Vector3(m_values[0][0] * rhs(0) + m_values[1][0] * rhs(1) + m_values[2][0] * rhs(2) + m_values[3][0],
            m_values[0][1] * rhs(0) + m_values[1][1] * rhs(1) + m_values[2][1] * rhs(2) + m_values[3][1],
            m_values[0][2] * rhs(0) + m_values[1][2] * rhs(1) + m_values[2][2] * rhs(2) + m_values[3][2]);
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::operator*(const Vector4& rhs) const
    {
        return Vector4(m_values[0][0] * rhs(0) + m_values[1][0] * rhs(1) + m_values[2][0] * rhs(2) + m_values[3][0] * rhs(3),
            m_values[0][1] * rhs(0) + m_values[1][1] * rhs(1) + m_values[2][1] * rhs(2) + m_values[3][1] * rhs(3),
            m_values[0][2] * rhs(0) + m_values[1][2] * rhs(1) + m_values[2][2] * rhs(2) + m_values[3][2] * rhs(3),
            m_values[0][3] * rhs(0) + m_values[1][3] * rhs(1) + m_values[2][3] * rhs(2) + m_values[3][3] * rhs(3));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::TransposedMultiply3x3(const Vector3& v) const
    {
        return Vector3(v(0) * m_values[0][0] + v(1) * m_values[0][1] + v(2) * m_values[0][2],
            v(0) * m_values[1][0] + v(1) * m_values[1][1] + v(2) * m_values[1][2],
            v(0) * m_values[2][0] + v(1) * m_values[2][1] + v(2) * m_values[2][2]);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Matrix4x4::Multiply3x3(const Vector3& v) const
    {
        return Vector3(v(0) * m_values[0][0] + v(1) * m_values[1][0] + v(2) * m_values[2][0],
            v(0) * m_values[0][1] + v(1) * m_values[1][1] + v(2) * m_values[2][1],
            v(0) * m_values[0][2] + v(1) * m_values[1][2] + v(2) * m_values[2][2]);
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::GetTranspose() const
    {
        Matrix4x4 ret;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                ret.m_values[col][row] = m_values[row][col];
            }
        }
        return ret;
    }

    AZ_MATH_FORCE_INLINE const Matrix4x4 Matrix4x4::GetInverseFast() const
    {
        Matrix4x4 out;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; j++)
            {
                out.m_values[i][j] = m_values[j][i];
            }
        }
        out.m_values[3][0] = -(GetPosition().Dot(GetColumnAsVector3(0)));
        out.m_values[3][1] = -(GetPosition().Dot(GetColumnAsVector3(1)));
        out.m_values[3][2] = -(GetPosition().Dot(GetColumnAsVector3(2)));
        out.m_values[3][3] = 1.0f;
        out.m_values[0][3] = 0.0f;
        out.m_values[1][3] = 0.0f;
        out.m_values[2][3] = 0.0f;
        return out;
    }

    AZ_MATH_FORCE_INLINE bool Matrix4x4::IsClose(const Matrix4x4& rhs, const VectorFloat& tolerance) const
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                if (fabsf(rhs(row, col) - GetElement(row, col)) > tolerance)
                {
                    return false;
                }
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE bool Matrix4x4::operator==(const Matrix4x4& rhs) const
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                if ((float)rhs.GetElement(row, col) != (float)GetElement(row, col))
                {
                    return false;
                }
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Matrix4x4::GetDiagonal() const
    {
        return Vector4(GetElement(0, 0), GetElement(1, 1), GetElement(2, 2), GetElement(3, 3));
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs)
    {
        return Vector3(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0) + rhs(3, 0),
            lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1) + rhs(3, 1),
            lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2) + rhs(3, 2));
    }

    AZ_MATH_FORCE_INLINE const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs)
    {
        return Vector4(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0) + lhs(3) * rhs(3, 0),
            lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1) + lhs(3) * rhs(3, 1),
            lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2) + lhs(3) * rhs(3, 2),
            lhs(0) * rhs(0, 3) + lhs(1) * rhs(1, 3) + lhs(2) * rhs(2, 3) + lhs(3) * rhs(3, 3));
    }
}
