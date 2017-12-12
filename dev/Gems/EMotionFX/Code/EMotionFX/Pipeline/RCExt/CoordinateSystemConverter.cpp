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

#include <RCExt/CoordinateSystemConverter.h>

namespace EMotionFX
{
    namespace Pipeline
    {

        // Default constructor, initializes so that it basically does no conversion.
        CoordinateSystemConverter::CoordinateSystemConverter()
            : m_sourceTransform(AZ::Transform::CreateIdentity())
            , m_targetTransform(AZ::Transform::CreateIdentity())
            , m_conversionTransform(AZ::Transform::CreateIdentity())
            , m_conversionTransformInversed(AZ::Transform::CreateIdentity())
            , m_needsConversion(false)
            , m_sourceRightHanded(CheckIfIsRightHanded(m_sourceTransform))
            , m_targetRightHanded(CheckIfIsRightHanded(m_targetTransform))
        {
            m_targetBasisIndices[0] = 0;
            m_targetBasisIndices[1] = 1;
            m_targetBasisIndices[2] = 2;
        }


        CoordinateSystemConverter::CoordinateSystemConverter(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3])
        {
            m_sourceTransform = AZ::Transform::CreateIdentity();
            m_sourceTransform.SetBasisAndPosition(sourceBasisVectors[0], sourceBasisVectors[1], sourceBasisVectors[2], AZ::Vector3(0.0f, 0.0f, 0.0f));
            AZ_Assert(m_sourceTransform.IsOrthogonal(), "Invalid source transformation, basis vectors have to be orthogonal.");

            m_targetTransform = AZ::Transform::CreateIdentity();
            m_targetTransform.SetBasisAndPosition(targetBasisVectors[0], targetBasisVectors[1], targetBasisVectors[2], AZ::Vector3(0.0f, 0.0f, 0.0f));
            AZ_Assert(m_targetTransform.IsOrthogonal(), "Invalid target transformation, basis vectors have to be orthogonal.");

            m_conversionTransform = m_targetTransform * m_sourceTransform.GetInverseFull();
            m_conversionTransformInversed = m_conversionTransform.GetInverseFull();

            m_targetBasisIndices[0] = targetBasisIndices[0];
            m_targetBasisIndices[1] = targetBasisIndices[1];
            m_targetBasisIndices[2] = targetBasisIndices[2];

            m_needsConversion   = (m_sourceTransform != m_targetTransform);
            m_sourceRightHanded = CheckIfIsRightHanded(m_sourceTransform);
            m_targetRightHanded = CheckIfIsRightHanded(m_targetTransform);
        }


        const CoordinateSystemConverter CoordinateSystemConverter::CreateFromBasisVectors(const AZ::Vector3 sourceBasisVectors[3], const AZ::Vector3 targetBasisVectors[3], const AZ::u32 targetBasisIndices[3])
        {
            return CoordinateSystemConverter(sourceBasisVectors, targetBasisVectors, targetBasisIndices);
        }


        const CoordinateSystemConverter CoordinateSystemConverter::CreateFromTransforms(const AZ::Transform& sourceTransform, const AZ::Transform& targetTransform, const AZ::u32 targetBasisIndices[3])
        {
            AZ::Vector3 sourceBasisVectors[3];
            sourceBasisVectors[0] = sourceTransform.GetBasisX();
            sourceBasisVectors[1] = sourceTransform.GetBasisY();
            sourceBasisVectors[2] = sourceTransform.GetBasisZ();

            AZ::Vector3 targetBasisVectors[3];
            targetBasisVectors[0] = targetTransform.GetBasisX();
            targetBasisVectors[1] = targetTransform.GetBasisY();
            targetBasisVectors[2] = targetTransform.GetBasisZ();

            return CoordinateSystemConverter(sourceBasisVectors, targetBasisVectors, targetBasisIndices);
        }


        bool CoordinateSystemConverter::CheckIfIsRightHanded(const AZ::Transform& transform) const
        {
            const AZ::Vector3 right   = transform.GetBasisX();
            const AZ::Vector3 up      = transform.GetBasisY();
            const AZ::Vector3 forward = transform.GetBasisZ();
            return ((right.Cross(up)).Dot(forward) <= 0.0f);
        }


        //-------------------------------------------------------------------------
        //  Conversions
        //-------------------------------------------------------------------------
        AZ::Quaternion CoordinateSystemConverter::ConvertQuaternion(const AZ::Quaternion& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            const AZ::Vector3 vec = ConvertVector3( input.GetImaginary() );
            float w = input.GetW();
            if (m_sourceRightHanded != m_targetRightHanded)
            {
                w = -w;
            }

            return AZ::Quaternion(vec.GetX(), vec.GetY(), vec.GetZ(), AZ::VectorFloat(w));
        }


        AZ::Vector3 CoordinateSystemConverter::ConvertVector3(const AZ::Vector3& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            return m_conversionTransform * input;
        }


        AZ::Transform CoordinateSystemConverter::ConvertTransform(const AZ::Transform& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            return input * m_conversionTransform;
        }


        // Convert a scale value, which never flips some axis or so, just switches them.
        // Think of two coordinate systems, where for example the Z axis is inverted in one of them, the scale will remain the same, in both systems.
        // However, if we swap Y and Z, the scale Y and Z still have to be swapped.
        AZ::Vector3 CoordinateSystemConverter::ConvertScale(const AZ::Vector3& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            return AZ::Vector3( input.GetElement(m_targetBasisIndices[0]), 
                                input.GetElement(m_targetBasisIndices[1]), 
                                input.GetElement(m_targetBasisIndices[2]) );
        }



        //-------------------------------------------------------------------------
        //  Inverse Conversions
        //-------------------------------------------------------------------------
        AZ::Quaternion CoordinateSystemConverter::InverseConvertQuaternion(const AZ::Quaternion& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            const AZ::Vector3 vec = InverseConvertVector3( input.GetImaginary() );
            float w = input.GetW();
            if (m_sourceRightHanded != m_targetRightHanded)
            {
                w = -w;
            }

            return AZ::Quaternion(vec.GetX(), vec.GetY(), vec.GetZ(), AZ::VectorFloat(w));
        }


        AZ::Vector3 CoordinateSystemConverter::InverseConvertVector3(const AZ::Vector3& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            return m_conversionTransformInversed * input;
        }


        AZ::Transform CoordinateSystemConverter::InverseConvertTransform(const AZ::Transform& input) const
        {
            if (!m_needsConversion)
            {
                return input;
            }

            return input * m_conversionTransformInversed;
        }


        AZ::Vector3 CoordinateSystemConverter::InverseConvertScale(const AZ::Vector3& input) const
        {
            return ConvertScale(input);
        }

    }   // namespace Pipeline
}   // namespace EMotionFX
