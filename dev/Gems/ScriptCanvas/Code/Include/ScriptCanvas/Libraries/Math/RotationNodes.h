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

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Math/MathUtils.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    namespace RotationNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        
        AZ_INLINE RotationType Add(RotationType a, RotationType b)
        {
            return a + b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Add, "Math/Rotation", "{D20FAD3C-39CD-4369-BA0D-32AD5E6E23EB}", "returns A + B", "A", "B");

        AZ_INLINE RotationType Conjugate(RotationType source)
        {
            return source.GetConjugate();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Conjugate, "Math/Rotation", "{A1279F70-E211-41F2-8974-84E998206B0D}", "returns the conjugate of the source, (-x, -y, -z, w)", "Source");

        AZ_INLINE RotationType DivideByNumber(RotationType source, NumberType divisor)
        {
            return source / ToVectorFloat(divisor);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(DivideByNumber, "Math/Rotation", "{94C8A813-C20E-4194-98B6-8618CE872BAA}", "returns the Numerator with each element divided by Divisor", "Numerator", "Divisor");

        AZ_INLINE NumberType Dot(RotationType a, RotationType b)
        {
            return FromVectorFloat(a.Dot(b));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Dot, "Math/Rotation", "{01FED020-6EB1-4A69-AFC7-7305FCA7FC97}", "returns the Dot product of A and B", "A", "B");

        AZ_INLINE RotationType FromAxisAngleDegrees(Vector3Type axis, NumberType degrees)
        {
            return RotationType::CreateFromAxisAngle(axis, AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromAxisAngleDegrees, "Math/Rotation", "{109952D1-2DB7-48C3-970D-B8DB4C96FE54}", "returns the rotation created from Axis the angle Degrees", "Axis", "Degrees");

        AZ_INLINE RotationType FromElement(RotationType source, NumberType index, NumberType value)
        {
            source.SetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3), Data::ToVectorFloat(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElement, "Math/Rotation", "{86F85D23-FB6E-4364-AE1E-8260D26988E0}", "returns a rotation with a the element corresponding to the index (0 -> x)(1 -> y)(2 -> z)(3 -> w)", "Source", "Index", "Value");

        AZ_INLINE RotationType FromElements(NumberType x, NumberType y, NumberType z, NumberType w)
        {
            return RotationType(ToVectorFloat(x), ToVectorFloat(y), ToVectorFloat(z), ToVectorFloat(w));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromElements, "Math/Rotation", "{9E5A648C-1378-4EDE-B28C-F867CBC89968}", "returns a rotation from elements", "X", "Y", "Z", "W");

        AZ_INLINE RotationType FromMatrix3x3(const Matrix3x3Type& source)
        {
            return RotationType::CreateFromMatrix3x3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix3x3, "Math/Rotation", "{AFB1A899-D71D-48C8-8C76-086146B7B6EE}", "returns a rotation created from the 3x3 matrix source", "Source");

        AZ_INLINE RotationType FromMatrix4x4(const Matrix4x4Type& source)
        {
            return RotationType::CreateFromMatrix4x4(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMatrix4x4, "Math/Rotation", "{CD6F0D36-EC89-4D3E-920E-267D47F819BE}", "returns a rotation created from the 4x4 matrix source", "Source");

        AZ_INLINE RotationType FromTransform(const TransformType& source)
        {
            return RotationType::CreateFromTransform(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromTransform, "Math/Rotation", "{B6B224CC-7454-4D99-B473-C0A77D4FB885}", "returns a rotation created from the rotation part of the transform source", "Source");

        AZ_INLINE RotationType FromVector3(Vector3Type source)
        {
            return RotationType::CreateFromVector3(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3, "Math/Rotation", "{5FA694EA-B2EA-4403-9144-9171A7AA8636}", "returns a rotation with the imaginary elements set to the Source, and the real element set to 0", "Source");

        AZ_INLINE RotationType FromVector3AndValue(Vector3Type imaginary, NumberType real)
        {
            return RotationType::CreateFromVector3AndValue(imaginary, ToVectorFloat(real));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromVector3AndValue, "Math/Rotation", "{955FE6EB-7C38-4587-BBB7-9C886ACEAF94}", "returns a rotation with the imaginary elements from Imaginary and the real element from Real", "Imaginary", "Real");
        
        AZ_INLINE NumberType GetElement(RotationType source, NumberType index)
        {
            return FromVectorFloat(source.GetElement(AZ::GetClamp(aznumeric_cast<int>(index), 0, 3)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetElement, "Math/Rotation", "{1B1452DA-E23C-43DC-A0AD-37AAC36E38FA}", "returns the element of Source corresponding to the Index (0 -> x)(1 -> y)(2 -> z)(3 -> w)", "Source", "Index");

        AZ_INLINE std::tuple<NumberType, NumberType, NumberType, NumberType> GetElements(const RotationType source)
        {
            return std::make_tuple(FromVectorFloat(source.GetX()), FromVectorFloat(source.GetY()), FromVectorFloat(source.GetZ()), FromVectorFloat(source.GetW()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(GetElements, "Math/Rotation", "{1384FAFE-9435-49C8-941A-F2694A4D3EA4}", "returns the elements of the source", "Source", "X", "Y", "Z", "W");

        AZ_INLINE RotationType InvertFull(RotationType source)
        {
            return source.GetInverseFull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(InvertFull, "Math/Rotation", "{DF936099-48C8-4924-A91D-6B93245D8F30}", "returns the inverse for any rotation, not just unit rotations", "Source");

        AZ_INLINE BooleanType IsClose(RotationType a, RotationType b, NumberType tolerance)
        {
            return a.IsClose(b, ToVectorFloat(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsClose, DefaultToleranceSIMD<2>, "Math/Rotation", "{E0150AD6-6CBE-494E-9A1D-1E7E7C0A114F}", "returns true if A and B are within Tolerance of each other", "A", "B", "Tolerance");

        AZ_INLINE BooleanType IsFinite(RotationType a)
        {
            return a.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, "Math/Rotation", "{503B1229-74E8-40FE-94DE-C4387806BDB0}", "returns true if every element in Source is finite", "Source");

        AZ_INLINE BooleanType IsIdentity(RotationType source, NumberType tolerance)
        {
            return source.IsIdentity(ToVectorFloat(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsIdentity, DefaultToleranceSIMD<1>, "Math/Rotation", "{E7BB6123-E21A-4B51-B35E-BAA3DF239AB8}", "returns true if Source is within Tolerance of the Identity rotation", "Source", "Tolerance");

        AZ_INLINE BooleanType IsZero(RotationType source, NumberType tolerance)
        {
            return source.IsZero(ToVectorFloat(tolerance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(IsZero, DefaultToleranceSIMD<1>, "Math/Rotation", "{8E71A7DC-5FCA-4569-A2C4-3A85B5070AA1}", "returns true if Source is within Tolerance of the Zero rotation", "Source", "Tolerance");

        AZ_INLINE NumberType Length(RotationType source)
        {
            return source.GetLength();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Length, "Math/Rotation", "{61025A32-F17E-4945-95AC-6F12C1A77B7F}", "returns the length of Source", "Source");

        AZ_INLINE NumberType LengthReciprocal(RotationType source)
        {
            return source.GetLengthReciprocal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthReciprocal, "Math/Rotation", "{C4019E78-59F8-4023-97F9-1FC6C2DC94C8}", "returns the reciprocal length of Source", "Source");

        AZ_INLINE NumberType LengthSquared(RotationType source)
        {
            return source.GetLengthSq();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(LengthSquared, "Math/Rotation", "{825A0F09-CDFA-4C80-8177-003B154F213A}", "returns the square of the length of Source", "Source");

        AZ_INLINE RotationType Lerp(RotationType a, RotationType b, NumberType t)
        {
            return a.Lerp(b, ToVectorFloat(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Lerp, "Math/Rotation", "{91CF1C54-89C6-4A00-A53D-20C58454C4EC}", "returns a the linear interpolation between From and To by the amount T", "From", "To", "T");
        
        AZ_INLINE RotationType ModX(RotationType source, NumberType value)
        {
            source.SetX(ToVectorFloat(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModX, "Math/Rotation", "{567CDD18-027E-4DA1-81D1-CDA7FFD9DB8B}", "returns a the rotation(X, Source.Y, Source.Z, Source.W)", "Source", "X");

        AZ_INLINE RotationType ModY(RotationType source, NumberType value)
        {
            source.SetY(ToVectorFloat(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModY, "Math/Rotation", "{64BD2718-D004-40CA-A5B0-4F68A5D823A0}", "returns a the rotation(Source.X, Y, Source.Z, Source.W)", "Source", "Y");

        AZ_INLINE RotationType ModZ(RotationType source, NumberType value)
        {
            source.SetZ(ToVectorFloat(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModZ, "Math/Rotation", "{0CDD1B61-4DC4-480C-A9EE-97251712705B}", "returns a the rotation(Source.X, Source.Y, Z, Source.W)", "Source", "Z");

        AZ_INLINE RotationType ModW(RotationType source, NumberType value)
        {
            source.SetW(ToVectorFloat(value));
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ModW, "Math/Rotation", "{FC2B0283-7530-4927-8AFA-155E0C53C5D9}", "returns a the rotation(Source.X, Source.Y, Source.Z, W)", "Source", "W");

        AZ_INLINE RotationType MultiplyByNumber(RotationType source, NumberType multiplier)
        {
            return source * ToVectorFloat(multiplier);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByNumber, "Math/Rotation", "{B8911827-A1E7-4ECE-8503-9B31DD9C63C8}", "returns the Source with each element multiplied by Multiplier", "Source", "Multiplier");

        AZ_INLINE RotationType MultiplyByRotation(RotationType A, RotationType B)
        {
            return A * B;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyByRotation, "Math/Rotation", "{F4E19446-CBC1-46BF-AEC3-17FCC3FA9DEE}", "returns A * B", "A", "B");

        AZ_INLINE RotationType Negate(RotationType source)
        {
            return -source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Negate, "Math/Rotation", "{5EA770E6-6F6C-4838-B2D8-B2C487BF32E7}", "returns the Source with each element negated", "Source");

        AZ_INLINE RotationType Normalize(RotationType source)
        {
            return source.GetNormalized();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Normalize, "Math/Rotation", "{1B01B185-50E0-4120-BD82-9331FC3117F9}", "returns the normalized version of Source", "Source");

        AZ_INLINE std::tuple<RotationType, NumberType> NormalizeWithLength(RotationType source)
        {
            const AZ::VectorFloat length = source.NormalizeWithLength();
            return std::make_tuple( source, FromVectorFloat(length) );
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NormalizeWithLength, "Math/Rotation", "{E1A7F3F8-854E-4BA1-9DEA-7507BEC6D369}", "returns the normalized version of Source, and the length of Source", "Source", "Normalized", "Length");

        AZ_INLINE RotationType RotationXDegrees(NumberType degrees)
        {
            return RotationType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationXDegrees, "Math/Rotation", "{9A017348-F803-43D7-A2A6-BE01359D5E15}", "creates a rotation of Degrees around the x-axis", "Degrees");

        AZ_INLINE RotationType RotationYDegrees(NumberType degrees)
        {
            return RotationType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationYDegrees, "Math/Rotation", "{6C69AA65-1A83-4C36-B010-ECB621790A6C}", "creates a rotation of Degrees around the y-axis", "Degrees");

        AZ_INLINE RotationType RotationZDegrees(NumberType degrees)
        {
            return RotationType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(RotationZDegrees, "Math/Rotation", "{8BC8B0FE-51A1-4ECC-AFF1-A828A0FC8F8F}", "creates a rotation of Degrees around the z-axis", "Degrees");
        
        AZ_INLINE RotationType ShortestArc(Vector3Type from, Vector3Type to)
        {
            return RotationType::CreateShortestArc(from, to);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ShortestArc, "Math/Rotation", "{00CB739A-6BF9-4160-83F7-A243BD9D5093}", "creates a rotation representing the shortest arc between From and To", "From", "To");

        AZ_INLINE RotationType Slerp(RotationType a, RotationType b, NumberType t)
        {
            return a.Slerp(b, ToVectorFloat(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Slerp, "Math/Rotation", "{26234D44-9BA5-4E1B-8226-224E8A4A15CC}", "returns the spherical linear interpolation between From and To by the amount T, the result is NOT normalized", "From", "To", "T");

        AZ_INLINE RotationType Squad(RotationType from, RotationType to, RotationType in, RotationType out, NumberType t)
        {
            return from.Squad(to, in, out, ToVectorFloat(t));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Squad, "Math/Rotation", "{D354F41E-29E3-49EC-8F0E-C890000D32D6}", "returns the quadratic interpolation, that is: Squad(From, To, In, Out, T) = Slerp(Slerp(From, Out, T), Slerp(To, In, T), 2(1 - T)T)", "From", "To", "In", "Out", "T");

        AZ_INLINE RotationType Subtract(RotationType a, RotationType b)
        {
            return a - b;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Subtract, "Math/Rotation", "{238538F8-D8C9-4348-89CC-E35F5DF11358}", "returns the rotation A - B", "A", "B");

        AZ_INLINE NumberType ToAngleDegrees(RotationType source)
        {
            return aznumeric_caster(AZ::RadToDeg(source.GetAngle()));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToAngleDegrees, "Math/Rotation", "{3EA78793-9AFA-4857-8CB8-CD0D47E97D25}", "returns the angle of angle-axis pair that Source represents in degrees", "Source");
              
        AZ_INLINE Vector3Type ToImaginary(RotationType source)
        {
            return source.GetImaginary();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToImaginary, "Math/Rotation", "{86754CA3-ADBA-4D5C-AAB6-C4AA6B079CFD}", "returns the imaginary portion of Source, that is (x, y, z)", "Source");

        using Registrar = RegistrarGeneric
            < AddNode
            , ConjugateNode
            , DivideByNumberNode
            , DotNode
            , FromAxisAngleDegreesNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromElementNode
            , FromElementsNode
#endif

            , FromMatrix3x3Node
            , FromMatrix4x4Node
            , FromTransformNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromVector3Node
            , FromVector3AndValueNode
            , GetElementNode
            , GetElementsNode
#endif

            , InvertFullNode
            , IsCloseNode
            , IsFiniteNode
            , IsIdentityNode
            , IsZeroNode
            , LengthNode
            , LengthReciprocalNode
            , LengthSquaredNode
            , LerpNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ModXNode
            , ModYNode
            , ModZNode
            , ModWNode
#endif

            , MultiplyByNumberNode
            , MultiplyByRotationNode
            , NegateNode
            , NormalizeNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , NormalizeWithLengthNode
#endif

            , RotationXDegreesNode
            , RotationYDegreesNode
            , RotationZDegreesNode
            , ShortestArcNode
            , SlerpNode
            , SquadNode
            , SubtractNode
            , ToAngleDegreesNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , ToImaginaryNode
#endif
            > ;

    } // namespace Vector2Nodes
} // namespace ScriptCanvas

