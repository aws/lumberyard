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
    AZ_MATH_FORCE_INLINE const Transform Transform::CreateIdentity()
    {
        Transform result;
        result.SetRow(0, 1.0f, 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, 1.0f, 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, 1.0f, 0.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateZero()
    {
        return Transform::CreateFromValue(0.0f);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateFromValue(const VectorFloat& value)
    {
        Transform result;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                result.SetElement(i, j, value);
            }
        }
        return result;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateRotationX(const VectorFloat& angle)
    {
        Transform result;
        VectorFloat s, c;
        angle.GetSinCos(s, c);
        result.SetRow(0, 1.0f, 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, c, -s, 0.0f);
        result.SetRow(2, 0.0f, s, c, 0.0f);
        return result;
    }


    AZ_MATH_FORCE_INLINE const Transform Transform::CreateRotationY(const VectorFloat& angle)
    {
        Transform result;
        VectorFloat s, c;
        angle.GetSinCos(s, c);
        result.SetRow(0, c, 0.0f, s, 0.0f);
        result.SetRow(1, 0.0f, 1.0f, 0.0f, 0.0f);
        result.SetRow(2, -s, 0.0f, c, 0.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateRotationZ(const VectorFloat& angle)
    {
        Transform result;
        VectorFloat s, c;
        angle.GetSinCos(s, c);
        result.SetRow(0, c, -s, 0.0f, 0.0f);
        result.SetRow(1, s, c, 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, 1.0f, 0.0f);
        return result;
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
        Transform result;
        result.SetRow(0, diagonal.GetX(), 0.0f, 0.0f, 0.0f);
        result.SetRow(1, 0.0f, diagonal.GetY(), 0.0f, 0.0f);
        result.SetRow(2, 0.0f, 0.0f, diagonal.GetZ(), 0.0f);
        return result;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::CreateTranslation(const Vector3& translation)
    {
        Transform result;
        result.SetRow(0, 1.0f, 0.0f, 0.0f, translation.GetX());
        result.SetRow(1, 0.0f, 1.0f, 0.0f, translation.GetY());
        result.SetRow(2, 0.0f, 0.0f, 1.0f, translation.GetZ());
        return result;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Transform::GetElement(int row, int col) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return m_values[col][row];
    }

    AZ_MATH_FORCE_INLINE void Transform::SetElement(int row, int col, const VectorFloat& value)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        m_values[col][row] = value;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Transform::GetRow(int row) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        return Vector4(m_values[0][row], m_values[1][row], m_values[2][row], m_values[3][row]);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Transform::GetRowAsVector3(int row) const
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        return Vector3(m_values[0][row], m_values[1][row], m_values[2][row]);
    }
    AZ_MATH_FORCE_INLINE void Transform::SetRow(int row, const Vector3& v, const VectorFloat& w)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        m_values[0][row] = v.GetX();
        m_values[1][row] = v.GetY();
        m_values[2][row] = v.GetZ();
        m_values[3][row] = w;
    }
    AZ_MATH_FORCE_INLINE void Transform::SetRow(int row, const Vector4& v)
    {
        AZ_Assert((row >= 0) && (row < 3), "Invalid index for component access!\n");
        m_values[0][row] = v.GetX();
        m_values[1][row] = v.GetY();
        m_values[2][row] = v.GetZ();
        m_values[3][row] = v.GetW();
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::GetColumn(int col) const
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        return Vector3::CreateFromFloat3(&m_values[col][0]);
    }
    AZ_MATH_FORCE_INLINE void Transform::SetColumn(int col, const Vector3& v)
    {
        AZ_Assert((col >= 0) && (col < 4), "Invalid index for component access!\n");
        m_values[col][0] = v.GetX();
        m_values[col][1] = v.GetY();
        m_values[col][2] = v.GetZ();
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::GetPosition() const
    {
        return GetColumn(3);
    }
    AZ_MATH_FORCE_INLINE void Transform::SetPosition(const Vector3& v)
    {
        SetColumn(3, v);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::operator*(const Transform& rhs) const
    {
        Transform out;
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
            out.m_values[3][row] = 0.0f;
            for (int k = 0; k < 3; ++k)
            {
                out.m_values[3][row] += m_values[k][row] * rhs.m_values[3][k];
            }
            out.m_values[3][row] += m_values[3][row];
        }
        return out;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::operator*(const Vector3& rhs) const
    {
        return Vector3(m_values[0][0] * rhs(0) + m_values[1][0] * rhs(1) + m_values[2][0] * rhs(2) + m_values[3][0],
            m_values[0][1] * rhs(0) + m_values[1][1] * rhs(1) + m_values[2][1] * rhs(2) + m_values[3][1],
            m_values[0][2] * rhs(0) + m_values[1][2] * rhs(1) + m_values[2][2] * rhs(2) + m_values[3][2]);
    }

    AZ_MATH_FORCE_INLINE const Vector4 Transform::operator*(const Vector4& rhs) const
    {
        return Vector4(m_values[0][0] * rhs(0) + m_values[1][0] * rhs(1) + m_values[2][0] * rhs(2) + m_values[3][0] * rhs(3),
            m_values[0][1] * rhs(0) + m_values[1][1] * rhs(1) + m_values[2][1] * rhs(2) + m_values[3][1] * rhs(3),
            m_values[0][2] * rhs(0) + m_values[1][2] * rhs(1) + m_values[2][2] * rhs(2) + m_values[3][2] * rhs(3),
            rhs(3));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::TransposedMultiply3x3(const Vector3& rhs) const
    {
        return Vector3(rhs(0) * m_values[0][0] + rhs(1) * m_values[0][1] + rhs(2) * m_values[0][2],
            rhs(0) * m_values[1][0] + rhs(1) * m_values[1][1] + rhs(2) * m_values[1][2],
            rhs(0) * m_values[2][0] + rhs(1) * m_values[2][1] + rhs(2) * m_values[2][2]);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Transform::Multiply3x3(const Vector3& rhs) const
    {
        return Vector3(rhs(0) * m_values[0][0] + rhs(1) * m_values[1][0] + rhs(2) * m_values[2][0],
            rhs(0) * m_values[0][1] + rhs(1) * m_values[1][1] + rhs(2) * m_values[2][1],
            rhs(0) * m_values[0][2] + rhs(1) * m_values[1][2] + rhs(2) * m_values[2][2]);
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::GetTranspose() const
    {
        Transform ret;
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                ret.m_values[col][row] = m_values[row][col];
            }
        }
        ret.SetPosition(Vector3::CreateZero());
        return ret;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::GetTranspose3x3() const
    {
        Transform ret;
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                ret.m_values[col][row] = m_values[row][col];
            }
        }
        ret.SetPosition(GetPosition());
        return ret;
    }

    AZ_MATH_FORCE_INLINE const Transform Transform::GetInverseFast() const
    {
        Transform out;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; j++)
            {
                out.m_values[i][j] = m_values[j][i];
            }
        }
        out.m_values[3][0] = -(GetPosition().Dot(GetColumn(0)));
        out.m_values[3][1] = -(GetPosition().Dot(GetColumn(1)));
        out.m_values[3][2] = -(GetPosition().Dot(GetColumn(2)));
        return out;
    }

    AZ_MATH_FORCE_INLINE bool Transform::IsClose(const Transform& rhs, const VectorFloat& tolerance) const
    {
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                if (fabsf(rhs.m_values[col][row] - m_values[col][row]) > tolerance)
                {
                    return false;
                }
            }
        }
        return true;
    }

    AZ_MATH_FORCE_INLINE bool Transform::operator==(const Transform& rhs) const
    {
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                if (m_values[col][row] != rhs.m_values[col][row])
                {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Pre-multiplies the matrix by a vector. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
     * a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
     * Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Transform& rhs)
    {
        return Vector3(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0),
            lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1),
            lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2));
    }

    /**
     * Pre-multiplies the matrix by a vector. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector4 operator*(const Vector4& lhs, const Transform& rhs)
    {
        return Vector4(lhs(0) * rhs(0, 0) + lhs(1) * rhs(1, 0) + lhs(2) * rhs(2, 0),
            lhs(0) * rhs(0, 1) + lhs(1) * rhs(1, 1) + lhs(2) * rhs(2, 1),
            lhs(0) * rhs(0, 2) + lhs(1) * rhs(1, 2) + lhs(2) * rhs(2, 2),
            lhs(0) * rhs(0, 3) + lhs(1) * rhs(1, 3) + lhs(2) * rhs(2, 3) + lhs(3));
    }
}
