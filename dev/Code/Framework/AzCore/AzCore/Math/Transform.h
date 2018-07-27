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
#ifndef AZCORE_MATH_TRANSFORM_H
#define AZCORE_MATH_TRANSFORM_H 1

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    /**
     * The basic transformation class, stores a matrix with 3 rows and 4 columns, so behaves just like Matrix4x4 but
     * uses less memory. The last row is always set implicitly to (0,0,0,1). Cannot be used for projection matrices.
     * See Matrix4x4 for more information about the matrices in general.
     */
    class Transform
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Transform);
        AZ_TYPE_INFO(Transform, "{5D9958E9-9F1E-4985-B532-FFFDE75FEDFD}")

        enum class Axis : AZ::u8
        {
            XPositive,
            XNegative,
            YPositive,
            YNegative,
            ZPositive,
            ZNegative
        };

        //===============================================================
        // Constructors
        //===============================================================

        ///Default constructor does not initialize the matrix
        Transform()     { }

        ///For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Transform(SimdVectorType row0, SimdVectorType row1, SimdVectorType row2);


        //===============================================================
        // Static functions to initialize various types of matrices
        //===============================================================
        static const Transform CreateIdentity();
        static const Transform CreateZero();

        ///Constructs a matrix with all components set to the specified value
        static const Transform CreateFromValue(const VectorFloat& value);

        ///Constructs from an array of 12 floats stored in row-major order
        static AZ_MATH_FORCE_INLINE const Transform CreateFromRowMajorFloat12(const float* values)  { return CreateFromRows(Vector4::CreateFromFloat4(&values[0]), Vector4::CreateFromFloat4(&values[4]), Vector4::CreateFromFloat4(&values[8])); }
        static AZ_MATH_FORCE_INLINE const Transform CreateFromRows(const Vector4& row0, const Vector4& row1, const Vector4& row2) { Transform t; t.SetRows(row0, row1, row2); return t; }

        ///Constructs from an array of 12 floats stored in column-major order
        static AZ_MATH_FORCE_INLINE const Transform CreateFromColumnMajorFloat12(const float* values) { return CreateFromColumns(Vector3::CreateFromFloat3(&values[0]), Vector3::CreateFromFloat3(&values[3]), Vector3::CreateFromFloat3(&values[6]), Vector3::CreateFromFloat3(&values[9])); }
        static AZ_MATH_FORCE_INLINE const Transform CreateFromColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2, const Vector3& col3) { Transform t; t.SetColumns(col0, col1, col2, col3); return t; }

        ///Constructs from an array of 16 floats stored in column-major order
        static AZ_MATH_FORCE_INLINE const Transform CreateFromColumnMajorFloat16(const float* values) { return CreateFromColumns(Vector3::CreateFromFloat3(&values[0]), Vector3::CreateFromFloat3(&values[4]), Vector3::CreateFromFloat3(&values[8]), Vector3::CreateFromFloat3(&values[12])); }

        ///Sets the matrix to be a rotation around a specified axis.
        /*@{*/
        static const Transform CreateRotationX(const VectorFloat& angle);
        static const Transform CreateRotationY(const VectorFloat& angle);
        static const Transform CreateRotationZ(const VectorFloat& angle);
        /*@}*/

        ///Sets the matrix from a quaternion, translation is set to zero
        static const Transform CreateFromQuaternion(const class Quaternion& q);

        ///Sets the matrix from a quaternion and a translation
        static const Transform CreateFromQuaternionAndTranslation(const class Quaternion& q, const Vector3& p);

        ///Constructs from a Matrix3x3, translation is set to zero
        static const Transform CreateFromMatrix3x3(const class Matrix3x3& value);

        ///Constructs from a Matrix3x3, translation is set to zero
        static const Transform CreateFromMatrix3x3AndTranslation(const class Matrix3x3& value, const Vector3& p);

        ///Sets the matrix to be a scale matrix, translation is set to zero
        static const Transform CreateScale(const Vector3& scale);

        ///Sets the matrix to be a diagonal matrix, translation is set to zero
        static const Transform CreateDiagonal(const Vector3& diagonal);

        ///Sets the matrix to be a translation matrix, rotation part is set to identity
        static const Transform CreateTranslation(const Vector3& translation);

        //! Create a "look-at" transform, given a source position and target position,
        //! make a transform at the source position that points toward the target along a chosen
        //! local-space axis.
        //! @param Vector3 from The source position (world-space).
        //! @param Vector3 to The target position (world-space).
        //! @param Axis forwardAxis The local-space basis axis that should "look-at" the target.
        //! @return Transform The resulting transform, or identity transform if from == to.
        static AZ::Transform CreateLookAt(const AZ::Vector3& from, const AZ::Vector3& to, AZ::Transform::Axis forwardAxis = AZ::Transform::Axis::YPositive);

        /// Return a reference to the Identity transform.
        static inline const Transform&  Identity();

        //===============================================================
        // Store
        //===============================================================

        ///Stores the matrix into to an array of 12 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToRowMajorFloat12(float* values) const
        {
            GetRow(0).StoreToFloat4(values);
            GetRow(1).StoreToFloat4(values + 4);
            GetRow(2).StoreToFloat4(values + 8);
        }

        ///Stores the matrix into to an array of 12 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToColumnMajorFloat12(float* values) const
        {
            GetColumn(0).StoreToFloat4(values);
            GetColumn(1).StoreToFloat4(values + 3);
            GetColumn(2).StoreToFloat4(values + 6);
            GetColumn(3).StoreToFloat3(values + 9);
        }

        ///Stores the matrix into to an array of 16 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToColumnMajorFloat16(float* values) const
        {
            GetColumn(0).StoreToFloat4(values);
            GetColumn(1).StoreToFloat4(values + 4);
            GetColumn(2).StoreToFloat4(values + 8);
            GetColumn(3).StoreToFloat4(values + 12);
        }

        //===============================================================
        // Accessors
        //===============================================================

        ///Indexed accessor functions.
        ///Prefer the GetRow/SetRow functions to these, as these functions can be slower.
        /*@{*/
        const VectorFloat GetElement(int row, int col) const;
        void SetElement(int row, int col, const VectorFloat& value);
        /*@}*/

        ///Indexed access using operator().
        ///Prefer the GetRow/SetRow functions to these, as these functions can be slower.
        /*@{*/
        AZ_MATH_FORCE_INLINE const VectorFloat operator()(int row, int col) const   { return GetElement(row, col); }
        /*@}*/

        ///Row access functions
        /*@{*/
        const Vector4 GetRow(int row) const;
        const Vector3 GetRowAsVector3(int row) const;
        AZ_MATH_FORCE_INLINE void SetRow(int row, const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w) { SetRow(row, Vector4(x, y, z, w)); }
        void SetRow(int row, const Vector3& v, const VectorFloat& w);
        void SetRow(int row, const Vector4& v);
        AZ_MATH_FORCE_INLINE void GetRows(Vector4* row0, Vector4* row1, Vector4* row2) const { *row0 = GetRow(0); *row1 = GetRow(1); *row2 = GetRow(2); }
        AZ_MATH_FORCE_INLINE void SetRows(const Vector4& row0, const Vector4& row1, const Vector4& row2) { SetRow(0, row0); SetRow(1, row1); SetRow(2, row2); }
        /*@}*/

        ///Column access functions
        /*@{*/
        const Vector3 GetColumn(int col) const;
        AZ_MATH_FORCE_INLINE void SetColumn(int col, const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(col, Vector3(x, y, z)); }
        void SetColumn(int col, const Vector3& v);
        AZ_MATH_FORCE_INLINE void GetColumns(Vector3* col0, Vector3* col1, Vector3* col2, Vector3* col3) const { *col0 = GetColumn(0); *col1 = GetColumn(1); *col2 = GetColumn(2); *col3 = GetColumn(3); }
        AZ_MATH_FORCE_INLINE void SetColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2, const Vector3& col3) { SetColumn(0, col0); SetColumn(1, col1); SetColumn(2, col2); SetColumn(3, col3); }
        /*@}*/

        ///Basis (column) access functions
        /*@{*/
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisX() const { return GetColumn(0); }
        AZ_MATH_FORCE_INLINE void SetBasisX(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(0, x, y, z); }
        AZ_MATH_FORCE_INLINE void SetBasisX(const Vector3& v) { SetColumn(0, v); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisY() const { return GetColumn(1); }
        AZ_MATH_FORCE_INLINE void SetBasisY(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(1, x, y, z); }
        AZ_MATH_FORCE_INLINE void SetBasisY(const Vector3& v) { SetColumn(1, v); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisZ() const { return GetColumn(2); }
        AZ_MATH_FORCE_INLINE void SetBasisZ(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(2, x, y, z); }
        AZ_MATH_FORCE_INLINE void SetBasisZ(const Vector3& v) { SetColumn(2, v); }
        AZ_MATH_FORCE_INLINE void GetBasisAndPosition(Vector3* basisX, Vector3* basisY, Vector3* basisZ, Vector3* pos) const { GetColumns(basisX, basisY, basisZ, pos); }
        AZ_MATH_FORCE_INLINE void SetBasisAndPosition(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ, const Vector3& pos) { SetColumns(basisX, basisY, basisZ, pos); }
        /*@}*/

        /**
        * Position (last column) convenience access functions
        */
        /*@{*/
        const Vector3 GetPosition() const;
        AZ_MATH_FORCE_INLINE void SetPosition(float x, float y, float z)            { SetPosition(Vector3(x, y, z)); }
        void SetPosition(const Vector3& v);
        AZ_MATH_FORCE_INLINE const Vector3 GetTranslation() const                   { return GetPosition(); }
        AZ_MATH_FORCE_INLINE void SetTranslation(float x, float y, float z)         { SetPosition(x, y, z); }
        AZ_MATH_FORCE_INLINE void SetTranslation(const Vector3& v)                  { SetPosition(v); }
        /*@}*/

        //---------------------------------------------
        // Multiplication operators and functions
        //---------------------------------------------

        const Transform operator*(const Transform& rhs) const;
        AZ_MATH_FORCE_INLINE Transform& operator*=(const Transform& rhs)            { *this = *this * rhs; return *this; }

        /**
        * Post-multiplies the matrix by a vector.  Assumes that the w-component of the Vector3 is 1.0.
        */
        const Vector3 operator*(const Vector3& rhs) const;

        /**
        * Post-multiplies the matrix by a vector.
        */
        const Vector4 operator*(const Vector4& rhs) const;

        /**
        * Pre-multiplies the matrix by a vector, using only the upper 3x3 submatrix. Note that this is NOT the
        * usual multiplication order for transformations.
        */
        const Vector3 TransposedMultiply3x3(const Vector3& rhs) const;

        /**
        * Post-multiplies the matrix by a vector, using only the upper 3x3 submatrix.
        */
        const Vector3 Multiply3x3(const Vector3& rhs) const;

        //-------------------------------------------------------
        // Transposing
        //-------------------------------------------------------

        /**
         * Transpose calculation, i.e. flipping the rows and columns. Note that this resets the last column (position)
         * of the matrix, since the last row is always fixed. Use Transpose3x3 to transpose just the upper 3x3 submatrix.
         */
        /*@{*/
        const Transform GetTranspose() const;
        AZ_MATH_FORCE_INLINE void Transpose()                                       { *this = GetTranspose(); }
        /*@}*/

        /**
         * Transposes the upper 3x3 submatrix, leaves the last column untouched.
         */
        /*@{*/
        const Transform GetTranspose3x3() const;
        AZ_MATH_FORCE_INLINE void Transpose3x3()                                    { *this = GetTranspose3x3(); }
        /*@}*/

        //-------------------------------------------------------
        // Inverses
        //-------------------------------------------------------

        /**
         * Gets the inverse of the matrix, assumes that the last row is (0,0,0,1). This function works for any matrix, even if
         * they have scaling or skew. If the upper 3x3 matrix is orthogonal then use GetInverse instead, it is much faster.
         */
        /*@{*/
        const Transform GetInverseFull() const;
        AZ_MATH_FORCE_INLINE void InvertFull()                                      { *this = GetInverseFull(); }
        /*@}*/

        /**
         * Fast inversion assumes the matrix consists of an upper 3x3 orthogonal matrix (i.e. a rotation)
         * and a translation in the last row. We call this GetInverse since this should be the default for Transforms.
         */
        /*@{*/
        const Transform GetInverseFast() const;
        AZ_MATH_FORCE_INLINE void InvertFast()                                      { *this = GetInverseFast(); }
        /*@}*/

        //-------------------------------------------------------
        // Scale access and decomposition
        //-------------------------------------------------------

        ///Gets the scale part of the transformation, i.e. the length of the scale components
        const Vector3 RetrieveScale() const;

        /**
         * Same as @ref RetrieveScale but execute with full precision.
         */
        const Vector3 RetrieveScaleExact() const;

        ///Gets the scale part of the transformation as in RetrieveScale, and also removes this scaling from the matrix
        const Vector3 ExtractScale();

        /// Same as \ref ExtractScale but executed with full precision.
        const Vector3 ExtractScaleExact();

        ///Quick multiplication by a scale matrix, equivalent to m = Transform::CreateScale(scale) * m
        void MultiplyByScale(const Vector3& scale);

        ///Polar decomposition, M=U*H, U is orthogonal (unitary) and H is symmetric (hermitian), this function
        ///returns the orthogonal part only, translation is included.
        const Transform GetPolarDecomposition() const;

        ///Polar decomposition, M=U*H, U is orthogonal (unitary) and H is symmetric (hermitian), translation is stored
        ///in the orthogonal part also.
        void GetPolarDecomposition(Transform* orthogonalOut, Transform* symmetricOut) const;

        //-------------------------------------------------------
        // Orthgonality
        //-------------------------------------------------------

        ///Tests if the upper 3x3 part of the matrix is orthgonal
        bool IsOrthogonal(const VectorFloat& tolerance = g_simdTolerance) const;

        ///Adjusts an almost orthogonal matrix to be orthogonal
        const Transform GetOrthogonalized() const;

        ///Adjusts an almost orthogonal matrix to be orthogonal
        AZ_MATH_FORCE_INLINE void Orthogonalize()                                   { *this = GetOrthogonalized(); }

        //-------------------------------------------------------
        // Compare
        //-------------------------------------------------------

        bool IsClose(const Transform& rhs, const VectorFloat& tolerance = g_simdTolerance) const;

        bool operator==(const Transform& rhs) const;
        AZ_MATH_FORCE_INLINE bool operator!=(const Transform& rhs) const    { return !operator==(rhs); }


        //-------------------------------------------------------
        // Euler transforms
        //-------------------------------------------------------

        //! Converts the transform to corresponding component-wise Euler angles.
        //! @return Vector3 A vector containing component-wise rotation angles in degrees.
        // Technique from published work available here
        // https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2012/07/euler-angles1.pdf (Extracting Euler Angles from a Rotation Matrix - Mike Day, Insomniac Games mday@insomniacgames.com)
        Vector3 GetEulerDegrees() const;
        Vector3 GetEulerRadians() const;

        //! Create the transform from Euler Angles (e.g. rotation angles in X, Y, and Z)
        //!         This version uses an approxmimation for sin and cos in GetSinCos()
        //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in degrees.
        //! @return Transform made from the rotational components.
        void SetFromEulerDegrees(const AZ::Vector3& eulerDegrees);

        //! Create a rotation transform from Euler angles in radian around each base axis.
        //!        This version uses precise sin/cos for a more accurate conversion.
        //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in radian.
        //! @return Transform made from the composite of rotations first around z-axis, and y-axis and then x-axis.
        void SetFromEulerRadiansPrecise(const AZ::Vector3& eulerRadians);

        //! Create a rotation transform from Euler angles in degree around each base axis.
        //!        This version uses precise sin/cos for a more accurate conversion.
        //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in degree.
        //! @return Transform made from the composite of rotations first around z-axis, and y-axis and then x-axis.
        void SetFromEulerDegreesPrecise(const AZ::Vector3& eulerDegrees);

        //------------------------------------
        // Misc. stuff
        //------------------------------------

        ///sets the upper 3x3 rotation part of the matrix from a quaternion
        void SetRotationPartFromQuaternion(const Quaternion& q);

        ///calculates the determinant of the upper 3x3 rotation part
        const VectorFloat GetDeterminant3x3() const;

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return GetRow(0).IsFinite() && GetRow(1).IsFinite() && GetRow(2).IsFinite(); }

#if defined(AZ_SIMD)
        AZ_MATH_FORCE_INLINE Transform(const Transform& tm)
        {
            m_rows[0] = tm.m_rows[0];
            m_rows[1] = tm.m_rows[1];
            m_rows[2] = tm.m_rows[2];
        }

        AZ_MATH_FORCE_INLINE Transform& operator= (const Transform& rhs)
        {
            m_rows[0] = rhs.m_rows[0];
            m_rows[1] = rhs.m_rows[1];
            m_rows[2] = rhs.m_rows[2];
            return *this;
        }
#endif

    private:
        friend class Matrix3x3;
        friend const Vector3 operator*(const Vector3& lhs, const Transform& rhs);
        friend const Vector4 operator*(const Vector4& lhs, const Transform& rhs);

        #if defined(AZ_SIMD)
        SimdVectorType m_rows[3];
        #else
        //matrix elements, stored in COLUMN-MAJOR format
        AZ_ALIGN(float m_values[4][3], 16);
        #endif
    };

    /**
     * Pre-multiplies the matrix by a vector. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
     * a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
     * Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Transform& rhs);

    /**
     * Pre-multiplies the matrix by a vector. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector4 operator*(const Vector4& lhs, const Transform& rhs);

    /**
     * Pre-multiplies the matrix by a vector in-place. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
     * a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
     * Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE Vector3& operator*=(Vector3& lhs, const Transform& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    /**
     * Pre-multiplies the matrix by a vector in-place. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE Vector4& operator*=(Vector4& lhs, const Transform& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    extern const Transform g_transformIdentity;

    inline const Transform& Transform::Identity()
    {
        return g_transformIdentity;
    }

    //! Non-member functionality belonging to the AZ namespace
    //!
    //! Converts a transform to corresponding component-wise Euler angles.
    //! @param Transform transform The input transform to decompose.
    //! @return Vector3 A vector containing component-wise rotation angles in degrees.
    AZ::Vector3 ConvertTransformToEulerDegrees(const AZ::Transform& transform);
    AZ::Vector3 ConvertTransformToEulerRadians(const AZ::Transform& transform);

    //! Create a transform from Euler Angles (e.g. rotation angles in X, Y, and Z)
    //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in degrees.
    //! @return Transform A transform made from the rotational components.
    AZ::Transform ConvertEulerDegreesToTransform(const AZ::Vector3& eulerDegrees);

    //! Create a rotation transform from Euler angles in radian around each base axis.
    //!        This version uses precise sin/cos for a more accurate conversion.
    //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in radian.
    //! @return Transform A transform made from the composite of rotations first around z-axis, and y-axis and then x-axis.
    AZ::Transform ConvertEulerRadiansToTransformPrecise(const AZ::Vector3& eulerRadians);

    //! Create a rotation transform from Euler angles in degree around each base axis.
    //!        This version uses precise sin/cos for a more accurate conversion.
    //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in degree.
    //! @return Transform A transform made from the composite of rotations first around z-axis, 
    AZ::Transform ConvertEulerDegreesToTransformPrecise(const AZ::Vector3& eulerDegrees);
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Transform);
#endif

#if AZ_TRAIT_USE_PLATFORM_SIMD
    #include <AzCore/Math/Internal/TransformWin32.inl>
#else
    #include <AzCore/Math/Internal/TransformFpu.inl>
#endif

#endif
#pragma once
