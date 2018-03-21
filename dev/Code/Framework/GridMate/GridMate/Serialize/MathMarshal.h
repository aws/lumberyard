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
#ifndef GM_UTILS_MATH_MARSHAL
#define GM_UTILS_MATH_MARSHAL

#include <GridMate/Serialize/DataMarshal.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>

namespace GridMate
{
    /**
	* Vector2 Marshaler uses 8 bytes.
	*/
	template<>
	class Marshaler<AZ::Vector2>
	{
	public:
		typedef AZ::Vector2 DataType;

		static const AZStd::size_t MarshalSize = sizeof(float) * 2;

		void Marshal(WriteBuffer& wb, const AZ::Vector2& vec) const
		{
			Marshaler<float> marshaler;
			marshaler.Marshal(wb, vec.GetX());
			marshaler.Marshal(wb, vec.GetY());
		}
		void Unmarshal(AZ::Vector2& vec, ReadBuffer& rb) const
		{
			float x, y;
			Marshaler<float> marshaler;
			marshaler.Unmarshal(x, rb);
			marshaler.Unmarshal(y, rb);
			vec.Set(x, y);
		}
	};
	
	/**
    * Vector3 Marshaler uses 12 bytes.
    */
    template<>
    class Marshaler<AZ::Vector3>
    {
    public:
        typedef AZ::Vector3 DataType;

        static const AZStd::size_t MarshalSize = sizeof(float) * 3;

        void Marshal(WriteBuffer& wb, const AZ::Vector3& vec) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, vec.GetX());
            marshaler.Marshal(wb, vec.GetY());
            marshaler.Marshal(wb, vec.GetZ());
        }
        void Unmarshal(AZ::Vector3& vec, ReadBuffer& rb) const
        {
            float x, y, z;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            vec.Set(x, y, z);
        }
    };

    /**
    * Color Marshaler uses 16 bytes.
    */
    template<>
    class Marshaler<AZ::Color>
    {
    public:
        typedef AZ::Color DataType;

        static const AZStd::size_t MarshalSize = sizeof(float) * 4;

        void Marshal(WriteBuffer& wb, const AZ::Color& color) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, color.GetR());
            marshaler.Marshal(wb, color.GetG());
            marshaler.Marshal(wb, color.GetB());
            marshaler.Marshal(wb, color.GetA());
        }
        void Unmarshal(AZ::Color& color, ReadBuffer& rb) const
        {
            float r, g, b, a;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(r, rb);
            marshaler.Unmarshal(g, rb);
            marshaler.Unmarshal(b, rb);
            marshaler.Unmarshal(a, rb);
            color.Set(r, g, b, a);
        }
    };

    /**
    * Quaternion Marshaler uses 16 bytes
    */
    template<>
    class Marshaler<AZ::Quaternion>
    {
    public:
        typedef AZ::Quaternion DataType;

        static const AZStd::size_t MarshalSize = sizeof(float) * 4;

        void Marshal(WriteBuffer& wb, const AZ::Quaternion& quat) const
        {
            Marshaler<float> marshaler;
            marshaler.Marshal(wb, quat.GetX());
            marshaler.Marshal(wb, quat.GetY());
            marshaler.Marshal(wb, quat.GetZ());
            marshaler.Marshal(wb, quat.GetW());
        }
        void Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const
        {
            float x, y, z, w;
            Marshaler<float> marshaler;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            marshaler.Unmarshal(w, rb);
            quat.Set(x, y, z, w);
        }
    };

    /**
    * Transform Marshaler
    */
    template<>
    class Marshaler<AZ::Transform>
    {
    public:
        typedef AZ::Transform DataType;

        static const AZStd::size_t MarshalSize = Marshaler<AZ::Vector3>::MarshalSize * 4;

        void Marshal(WriteBuffer& wb, const AZ::Transform& value) const
        {
            Marshaler<AZ::Vector3> marshaler;
            marshaler.Marshal(wb, value.GetBasisX());
            marshaler.Marshal(wb, value.GetBasisY());
            marshaler.Marshal(wb, value.GetBasisZ());
            marshaler.Marshal(wb, value.GetTranslation());
        }
        void Unmarshal(AZ::Transform& value, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector3> marshaler;
            AZ::Vector3 x, y, z, pos;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            marshaler.Unmarshal(pos, rb);
            value.SetBasisAndPosition(x, y, z, pos);
        }
    };

    /**
    * Matrix3x3 Marshaler
    */
    template<>
    class Marshaler<AZ::Matrix3x3>
    {
    public:
        typedef AZ::Matrix3x3 DataType;

        static const AZStd::size_t MarshalSize = Marshaler<AZ::Vector3>::MarshalSize * 3;

        void Marshal(WriteBuffer& wb, const AZ::Matrix3x3& value) const
        {
            Marshaler<AZ::Vector3> marshaler;
            marshaler.Marshal(wb, value.GetBasisX());
            marshaler.Marshal(wb, value.GetBasisY());
            marshaler.Marshal(wb, value.GetBasisZ());
        }
        void Unmarshal(AZ::Matrix3x3& value, ReadBuffer& rb) const
        {
            Marshaler<AZ::Vector3> marshaler;
            AZ::Vector3 x, y, z, pos;
            marshaler.Unmarshal(x, rb);
            marshaler.Unmarshal(y, rb);
            marshaler.Unmarshal(z, rb);
            value.SetBasis(x, y, z);
        }
    };
}

#endif // GM_UTILS_MATH_MARSHAL
