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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Math/MathReflection.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>


#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

#include <AzCore/Math/Quaternion.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Math/VectorFloat.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/InterpolationSample.h>

namespace AZ
{
    namespace Internal
    {
        // uuid
        class UuidSerializer
            : public SerializeContext::IDataSerializer
        {
            /// Store the class data into a binary buffer
            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;

                const Uuid* uuidPtr = reinterpret_cast<const Uuid*>(classPtr);

                return static_cast<size_t>(stream.Write(16, reinterpret_cast<const void*>(uuidPtr->data)));
            }

            /// Convert binary data to text
            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;

                if (in.GetLength() < 16)
                {
                    return 0;
                }

                Uuid value;
                void* dataPtr = reinterpret_cast<void*>(&value.data);
                in.Read(16, dataPtr);

                char str[128];
                value.ToString(str, 128);
                AZStd::string outText = str;

                return static_cast<size_t>(out.Write(outText.size(), outText.data()));
            }

            /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
            size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)textVersion;
                (void)isDataBigEndian;
                AZ_Assert(textVersion == 0, "Unknown char, short, int version!");

                Uuid uuid = Uuid::CreateString(text);
                stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                return static_cast<size_t>(stream.Write(16, uuid.data));
            }

            /// Load the class data from a stream.
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;
                if (stream.GetLength() < 16)
                {
                    return false;
                }

                Uuid* uuidPtr = reinterpret_cast<Uuid*>(classPtr);
                if (stream.Read(16, reinterpret_cast<void*>(&uuidPtr->data)) == 16)
                {
                    return true;
                }

                return false;
            }

            bool CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<AZ::Uuid>::CompareValues(lhs, rhs);
            }
        };

        class FloatArrayTextSerializer
        {
        public:
            static size_t DataToText(const float* floats, size_t numFloats, char* textBuffer, size_t textBufferSize, bool isDataBigEndian)
            {
                size_t numWritten = 0;
                for (size_t i = 0; i < numFloats; ++i)
                {
                    float value = floats[i];
                    AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
                    int result;
                    char* textData = &textBuffer[numWritten];
                    size_t textDataSize = textBufferSize - numWritten;
                    if (i != 0)
                    {
                        result = azsnprintf(textData, textDataSize, " %.7f", value);
                    }
                    else
                    {
                        result = azsnprintf(textData, textDataSize, "%.7f", value);
                    }
                    if (result == -1)
                    {
                        break;
                    }
                    numWritten += result;
                }
                return numWritten ? numWritten + 1 : 0;
            }

            static size_t TextToData(const char* text, float* floats, size_t numFloats, bool isDataBigEndian)
            {
                size_t nextNumberIndex = 0;
                while (text != nullptr && nextNumberIndex < numFloats)
                {
                    char* end;
                    floats[nextNumberIndex] = static_cast<float>(strtod(text, &end));
                    AZ_SERIALIZE_SWAP_ENDIAN(floats[nextNumberIndex], isDataBigEndian);
                    text = end;
                    ++nextNumberIndex;
                }
                return nextNumberIndex * sizeof(float);
            }
        };

        /**
         * Custom template to cover all fundamental AZMATH classes.
         */
        template <class T, const T CreateFromFloats(const float*), void (T::* StoreToFloat)(float*) const, float GetEpsilon(), size_t NumFloats = sizeof(T) / sizeof(float)>
        class FloatBasedContainerSerializer
            : public SerializeContext::IDataSerializer
        {
            /// Store the class data into a stream.
            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                float tempData[NumFloats];

                (reinterpret_cast<const T*>(classPtr)->*StoreToFloat)(tempData);
                for (unsigned int i = 0; i < AZ_ARRAY_SIZE(tempData); ++i)
                {
                    AZ_SERIALIZE_SWAP_ENDIAN(tempData[i], isDataBigEndian);
                }

                return static_cast<size_t>(stream.Write(sizeof(tempData), reinterpret_cast<void*>(&tempData)));
            }

            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                if (in.GetLength() < NumFloats * sizeof(float))
                {
                    return 0;
                }

                float tempData[NumFloats];
                in.Read(NumFloats * sizeof(float), reinterpret_cast<void*>(&tempData));
                char textBuffer[256];
                FloatArrayTextSerializer::DataToText(tempData, NumFloats, textBuffer, sizeof(textBuffer), isDataBigEndian);
                AZStd::string outText = textBuffer;

                return static_cast<size_t>(out.Write(outText.size(), outText.data()));
                ;
            }

            size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)textVersion;

                float floats[NumFloats];
                size_t numRead = FloatArrayTextSerializer::TextToData(text, floats, NumFloats, isDataBigEndian);
                stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                return static_cast<size_t>(stream.Write(numRead * sizeof(float), reinterpret_cast<void*>(&floats)));
            }

            bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                float tempData[NumFloats];
                if (stream.GetLength() < sizeof(tempData))
                {
                    return false;
                }

                stream.Read(sizeof(tempData), reinterpret_cast<void*>(tempData));

                for (unsigned int i = 0; i < AZ_ARRAY_SIZE(tempData); ++i)
                {
                    AZ_SERIALIZE_SWAP_ENDIAN(tempData[i], isDataBigEndian);
                }
                *reinterpret_cast<T*>(classPtr) = CreateFromFloats(tempData);
                return true;
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                float tempDataLhs[NumFloats];
                float tempDataRhs[NumFloats];
                (reinterpret_cast<const T*>(lhs)->*StoreToFloat)(tempDataLhs);
                (reinterpret_cast<const T*>(rhs)->*StoreToFloat)(tempDataRhs);

                const float epsilon = GetEpsilon();

                bool match = true;
                for (size_t i = 0; i < NumFloats && match; ++i)
                {
                    match &= AZ::IsClose(tempDataLhs[i], tempDataRhs[i], epsilon);
                }

                return match;
            }
        };
    }

    namespace ScriptCanvas
    {
        AZ::Vector2 ConstructVector2(VectorFloat xValue, VectorFloat yValue)
        {
            return AZ::Vector2(xValue, yValue);
        }

        AZ::Vector3 ConstructVector3(VectorFloat xValue, VectorFloat yValue, VectorFloat zValue)
        {
            return AZ::Vector3(xValue, yValue, zValue);
        }

        AZ::Vector4 ConstructVector4(VectorFloat xValue, VectorFloat yValue, VectorFloat zValue, VectorFloat wValue)
        {
            return AZ::Vector4(xValue, yValue, zValue, wValue);
        }

        AZ::Color ConstructColor(int red, int blue, int green, int alpha)
        {
            return AZ::Color((red/255.0f), (blue/255.0f), (green/255.0f), (alpha/255.0f));
        }

        AZ::Matrix3x3 ConstructMatrix3x3(AZ::Vector3 row0, AZ::Vector3 row1, AZ::Vector3 row2)
        {
            AZ::Matrix3x3 result;
            result.SetRows(row0, row1, row2);
            return result;
        }

        AZ::Matrix4x4 ConstructMatrix4x4(AZ::Vector4 row0, AZ::Vector4 row1, AZ::Vector4 row2, AZ::Vector4 row3)
        {
            AZ::Matrix4x4 result;
            result.SetRows(row0, row1, row2, row3);
            return result;
        }
    }

    float GetTransformEpsilon()
    {
        // For scale, rotation, and translation, use a larger epsilon to accommodate precision loss.
        // Note: This will likely become configurable through serialization attributes, but for now we'll
        // apply globally to vector and transform types.
        return 0.0001f; 
    }

    void MathReflect(SerializeContext& context)
    {

        context.Class<VectorFloat>()->
            Serializer<Internal::FloatBasedContainerSerializer<VectorFloat, & VectorFloat::CreateFromFloat, & VectorFloat::StoreToFloat, &GetTransformEpsilon, 1> >();

        context.Class<Vector2>()->
            Serializer<Internal::FloatBasedContainerSerializer<Vector2, & Vector2::CreateFromFloat2, & Vector2::StoreToFloat2, &GetTransformEpsilon, 2> >();

        context.Class<Vector3>()->
            Serializer<Internal::FloatBasedContainerSerializer<Vector3, & Vector3::CreateFromFloat3, & Vector3::StoreToFloat3, &GetTransformEpsilon, 3> >();

        context.Class<Vector4>()->
            Serializer<Internal::FloatBasedContainerSerializer<Vector4, & Vector4::CreateFromFloat4, & Vector4::StoreToFloat4, &GetTransformEpsilon, 4> >();

        context.Class<Color>()->
            Serializer<Internal::FloatBasedContainerSerializer<Color, &Color::CreateFromFloat4, &Color::StoreToFloat4, &GetTransformEpsilon, 4> >();

        context.Class<Transform>()->
            Serializer<Internal::FloatBasedContainerSerializer<Transform, & Transform::CreateFromColumnMajorFloat12, & Transform::StoreToColumnMajorFloat12, &GetTransformEpsilon, 12> >();

        context.Class<Matrix3x3>()->
            Serializer<Internal::FloatBasedContainerSerializer<Matrix3x3, & Matrix3x3::CreateFromColumnMajorFloat9, & Matrix3x3::StoreToColumnMajorFloat9, &GetTransformEpsilon, 9> >();

        context.Class<Matrix4x4>()->
            Serializer<Internal::FloatBasedContainerSerializer<Matrix4x4, & Matrix4x4::CreateFromColumnMajorFloat16, & Matrix4x4::StoreToColumnMajorFloat16, &GetTransformEpsilon, 16> >();

        context.Class<Quaternion>()->
            Serializer<Internal::FloatBasedContainerSerializer<Quaternion, & Quaternion::CreateFromFloat4, & Quaternion::StoreToFloat4, &GetTransformEpsilon, 4> >();


        // aggregates
        context.Class<Aabb>()
            ->Field("min", &Aabb::m_min)
            ->Field("max", &Aabb::m_max);

        context.Class<Obb>()
            ;

        context.Class<Plane>()
            ->Field("plane", &Plane::m_plane);

        context.Class<Uuid>()->
            Serializer<Internal::UuidSerializer>();

        Crc32::Reflect(context);
    }


    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        void Vector2ScriptConstructor(Vector2* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Vector2(number);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Vector2(), it must be a number!");
                }
            } break;
            case 2:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1))
                {
                    float x = 0;
                    float y = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    *thisPtr = Vector2(x, y);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 2 arguments to Vector2(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Vector2() accepts only 1 or 2 arguments, not %d!", numArgs);
            } break;
            }
        }

        void Vector2DefaultConstructor(Vector2* thisPtr)
        {
            new (thisPtr) Vector2(Vector2::CreateZero());
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string Vector2ToString(const Vector2* thisPtr)
        {
            return AZStd::string::format("(x=%.7f,y=%.7f)", static_cast<float>(thisPtr->GetX()), static_cast<float>(thisPtr->GetY()));
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector2MultiplyGeneric(const Vector2* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector2 result = *thisPtr * scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector2>(0))
                {
                    Vector2 vector2 = Vector2::CreateZero();
                    dc.ReadArg(0, vector2);
                    Vector2 result = *thisPtr * vector2;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector2 multiply should have only 1 argument - Vector2 or a Float/Number!");
                dc.PushResult(Vector2::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector2DivideGeneric(const Vector2* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector2 result = *thisPtr / VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector2>(0))
                {
                    Vector2 vector2 = Vector2::CreateZero();
                    dc.ReadArg(0, vector2);
                    Vector2 result = *thisPtr / vector2;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector2 divide should have only 1 argument - Vector2 or a Float/Number!");
                dc.PushResult(Vector2::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector2SetGeneric(Vector2* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;
            if (dc.GetNumArguments() == 1 && dc.IsNumber(0))
            {
                float setValue = 0;
                dc.ReadArg(0, setValue);
                thisPtr->Set(setValue);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 2 && dc.IsNumber(0) && dc.IsNumber(1))
            {
                float x = 0;
                float y = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                thisPtr->Set(x, y);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Vector2.Set only supports Set(number,number), Set(number)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector3ScriptConstructor(Vector3* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Vector3(number);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Vector3(), it must be a number!");
                }
            } break;
            case 3:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2))
                {
                    float x = 0;
                    float y = 0;
                    float z = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    dc.ReadArg(2, z);
                    *thisPtr = Vector3(x, y, z);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 3 arguments to Vector3(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Vector3() accepts only 1 or 3 arguments, not %d!", numArgs);
            } break;
            }
        }

        void Vector3DefaultConstructor(Vector3* thisPtr)
        {
            new (thisPtr) Vector3(Vector3::CreateZero());
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector3MultiplyGeneric(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector3 result = *thisPtr * VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Matrix3x3>(0))
                {
                    Matrix3x3 matrix3x3 = Matrix3x3::CreateZero();
                    dc.ReadArg(0, matrix3x3);
                    Vector3 result = *thisPtr * matrix3x3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Matrix4x4>(0))
                {
                    Matrix4x4 matrix4x4 = Matrix4x4::CreateZero();
                    dc.ReadArg(0, matrix4x4);
                    Vector3 result = *thisPtr * matrix4x4;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Transform>(0))
                {
                    Transform transform = Transform::CreateZero();
                    dc.ReadArg(0, transform);
                    Vector3 result = *thisPtr * transform;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector3 multiply should have only 1 argument - Vector3, Float/Number, Matrix3x3, Matrix4x4, or Transform!");
                dc.PushResult(Vector3::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector3SetGeneric(Vector3* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1 && dc.IsNumber(0))
            {
                float setValue = 0;
                dc.ReadArg(0, setValue);
                thisPtr->Set(setValue);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 3 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                thisPtr->Set(x, y, z);
                valueWasSet = true;
            }
            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Vector3.Set only supports Set(number,number,number), Set(number)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector3DivideGeneric(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector3 result = *thisPtr / VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr / vector3;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector3 divide should have only 1 argument - Vector3 or a Float/Number!");
                dc.PushResult(Vector3::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector3GetSinCosMultipleReturn(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 sin, cos;
            thisPtr->GetSinCos(sin, cos);
            dc.PushResult(sin);
            dc.PushResult(cos);
        }

        /// Implements multiple return values for BuildTangetBasis in Lua.
        void Vector3BuildTangentBasis(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 v1, v2;
            thisPtr->BuildTangentBasis(v1, v2);
            dc.PushResult(v1);
            dc.PushResult(v2);
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string Vector3ToString(const Vector3& v)
        {
            return AZStd::string::format("(x=%.7f,y=%.7f,z=%.7f)", static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector4ScriptConstructor(Vector4* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Vector4(number);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Vector4(), it must be a number!");
                }
            } break;
            case 4:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                {
                    float x = 0;
                    float y = 0;
                    float z = 0;
                    float w = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    dc.ReadArg(2, z);
                    dc.ReadArg(3, w);
                    *thisPtr = Vector4(x, y, z, w);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to Vector4(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Vector4() accepts only 1 or 4 arguments, not %d!", numArgs);
            } break;
            }
        }

        void Vector4DefaultConstructor(Vector4* thisPtr)
        {
            new (thisPtr) Vector4(Vector4::CreateZero());
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector4MultiplyGeneric(const Vector4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector4 result = *thisPtr * VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr * vector4;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Matrix4x4>(0))
                {
                    Matrix4x4 matrix4x4 = Matrix4x4::CreateZero();
                    dc.ReadArg(0, matrix4x4);
                    Vector4 result = *thisPtr * matrix4x4;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Transform>(0))
                {
                    Transform transform = Transform::CreateZero();
                    dc.ReadArg(0, transform);
                    Vector4 result = *thisPtr * transform;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector4 multiply should have only 1 argument - Vector4, Matrix4x4, Transform, or a Float/Number!");
                dc.PushResult(Vector4::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector4DivideGeneric(const Vector4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector4 result = *thisPtr / VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr / vector4;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector4 divide should have only 1 argument - Vector4 or a Float/Number!");
                dc.PushResult(Vector4::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Vector4SetGeneric(Vector4* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float setValue = 0;
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 setValue = Vector3::CreateZero();
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 vector3 = Vector3::CreateZero();
                float w = 0;
                dc.ReadArg(0, vector3);
                dc.ReadArg(1, w);
                thisPtr->Set(vector3, w);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                float w = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                dc.ReadArg(3, w);
                thisPtr->Set(x, y, z, w);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Vector4.Set only supports Set(number,number,number,number), Set(number), Set(Vector3), Set(Vector3,float)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string Vector4ToString(const Vector4& v)
        {
            return AZStd::string::format("(x=%.7f,y=%.7f,z=%.7f,w=%.7f)", static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()), static_cast<float>(v.GetW()));
        }

        void ColorScriptConstructor(Color* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Color(number);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Color(), it must be a number!");
                }
            } break;
            case 3:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2))
                {
                    float r = 0;
                    float g = 0;
                    float b = 0;
                    dc.ReadArg(0, r);
                    dc.ReadArg(1, g);
                    dc.ReadArg(2, b);
                    *thisPtr = Color(r, g, b, 1.0);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 3 arguments to Color(), all must be numbers!");
                }
            } break;
            case 4:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                {
                    float r = 0;
                    float g = 0;
                    float b = 0;
                    float a = 0;
                    dc.ReadArg(0, r);
                    dc.ReadArg(1, g);
                    dc.ReadArg(2, b);
                    dc.ReadArg(3, a);
                    *thisPtr = Color(r, g, b, a);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to Color(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Color() accepts only 1, 3, or 4 arguments, not %d!", numArgs);
            } break;
            }
        }

        void ColorDefaultConstructor(AZ::Color* thisPtr)
        {
            new (thisPtr) AZ::Color(AZ::Color::CreateZero());
        }

        //////////////////////////////////////////////////////////////////////////
        void ColorMultiplyGeneric(const Color* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Color result = *thisPtr * VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Color>(0))
                {
                    Color color = Color::CreateZero();
                    dc.ReadArg(0, color);
                    Color result = *thisPtr * color;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Color multiply should have only 1 argument - Color or a Float/Number!");
                dc.PushResult(Color::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ColorDivideGeneric(const Color* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Color result = *thisPtr / VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Color>(0))
                {
                    Color color = Color::CreateZero();
                    dc.ReadArg(0, color);
                    Color result = *thisPtr / color;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Color divide should have only 1 argument - Color or a Float/Number!");
                dc.PushResult(Color::CreateZero());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ColorSetGeneric(Color* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float setValue = 0;
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
                else if (dc.IsClass<Color>(0))
                {
                    Color setValue = Color::CreateZero();
                    dc.ReadArg(0, setValue);
                    *thisPtr = setValue;
                    valueWasSet = true;
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 setValue = Vector3::CreateZero();
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 setValue = Vector4::CreateZero();
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue.GetX(), setValue.GetY(), setValue.GetZ(), setValue.GetW());
                    valueWasSet = true;
                }
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 vector3 = Vector3::CreateZero();
                float a = 0;
                dc.ReadArg(0, vector3);
                dc.ReadArg(1, a);
                thisPtr->Set(vector3, a);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float r = 0;
                float g = 0;
                float b = 0;
                float a = 0;
                dc.ReadArg(0, r);
                dc.ReadArg(1, g);
                dc.ReadArg(2, b);
                dc.ReadArg(3, a);
                thisPtr->Set(r, g, b, a);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Color.Set only supports Set(number,number,number,number), Set(number), Set(Vector4), Set(Vector3,float)");
            }
        }

        AZStd::string ColorToString(const Color& c)
        {
            return AZStd::string::format("(r=%.7f,g=%.7f,b=%.7f,a=%.7f)", static_cast<float>(c.GetR()), static_cast<float>(c.GetG()), static_cast<float>(c.GetB()), static_cast<float>(c.GetA()));
        }

        //////////////////////////////////////////////////////////////////////////
        void QuaternionScriptConstrcutor(Quaternion* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Quaternion(number);
                }
                else
                {
                    AZ_Warning("Script", false, "You should provde only 1 number/float to be splatted!");
                }
            } break;
            case 4:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                {
                    float x = 0;
                    float y = 0;
                    float z = 0;
                    float w = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    dc.ReadArg(2, z);
                    dc.ReadArg(3, w);
                    *thisPtr = Quaternion(x, y, z, w);
                }
            } break;
            }
        }

        void QuaternionDefaultConstructor(AZ::Quaternion* thisPtr)
        {
            new (thisPtr) AZ::Quaternion(AZ::Quaternion::CreateIdentity());
        }

        //////////////////////////////////////////////////////////////////////////
        void QuaternionMultiplyGeneric(const Quaternion* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Quaternion result = *thisPtr * VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Quaternion>(0))
                {
                    Quaternion quaternion = Quaternion::CreateZero();
                    dc.ReadArg(0, quaternion);
                    Quaternion result = *thisPtr * quaternion;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3(Vector3::CreateZero());
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Quaternion multiply should have only 1 argument - Quaternion or a Float/Number!");
                dc.PushResult(Quaternion::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void QuaternionDivideGeneric(const Quaternion* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Quaternion result = *thisPtr / VectorFloat(scalar);
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Quaternion divide should have only 1 argument - Float/Number!");
                dc.PushResult(Quaternion::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void QuaternionSetGeneric(Quaternion* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1 && dc.IsNumber(0))
            {
                float setValue = 0;
                dc.ReadArg(0, setValue);
                thisPtr->Set(setValue);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 vector3 = Vector3::CreateZero();
                float w = 0;
                dc.ReadArg(0, vector3);
                dc.ReadArg(1, w);
                thisPtr->Set(vector3, w);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                float w = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                dc.ReadArg(3, w);
                thisPtr->Set(x, y, z, w);
                valueWasSet = true;
            }
            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Quaternion.Set only supports Set(number), Set(number, number, number, number), Set(Vector3, number)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string QuaternionToString(const Quaternion& v)
        {
            return AZStd::string::format("(x=%.7f,y=%.7f,z=%.7f,w=%.7f)", static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()), static_cast<float>(v.GetW()));
        }

        void Matrix3x3DefaultConstructor(AZ::Matrix3x3* thisPtr)
        {
            new (thisPtr) AZ::Matrix3x3(AZ::Matrix3x3::CreateIdentity());
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3GetBasisMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 basisX, basisY, basisZ;
            thisPtr->GetBasis(&basisX, &basisY, &basisZ);
            dc.PushResult(basisX);
            dc.PushResult(basisY);
            dc.PushResult(basisZ);
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3SetRowGeneric(Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
            if (dc.GetNumArguments() >= 4)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        thisPtr->SetRow(index, Vector3(x, y, z));
                        rowIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector3>(1))
                    {
                        Vector3 vector3 = Vector3::CreateZero();
                        dc.ReadArg(1, vector3);
                        thisPtr->SetRow(index, vector3);
                        rowIsSet = true;
                    }
                }
            }

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Matrix3x3 SetRow should have two signatures SetRow(index,Vector3) or SetRaw(index,x,y,z)!");
                dc.PushResult(Matrix3x3::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3SetColumnGeneric(Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
            if (dc.GetNumArguments() >= 4)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        thisPtr->SetColumn(index, Vector3(x, y, z));
                        rowIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector3>(1))
                    {
                        Vector3 vector3 = Vector3::CreateZero();
                        dc.ReadArg(1, vector3);
                        thisPtr->SetColumn(index, vector3);
                        rowIsSet = true;
                    }
                }
            }

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Matrix3x3 SetColumn should have two signatures SetColumn(index,Vector3) or SetColumn(index,x,y,z)!");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3GetRowsMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 row0, row1, row2;
            thisPtr->GetRows(&row0, &row1, &row2);
            dc.PushResult(row0);
            dc.PushResult(row1);
            dc.PushResult(row2);
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3GetColumnsMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 column0, column1, column2;
            thisPtr->GetColumns(&column0, &column1, &column2);
            dc.PushResult(column0);
            dc.PushResult(column1);
            dc.PushResult(column2);
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3GetPolarDecompositionMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Matrix3x3 orthogonal, symmetric;
            thisPtr->GetPolarDecomposition(&orthogonal, &symmetric);
            dc.PushResult(orthogonal);
            dc.PushResult(symmetric);
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3MultiplyGeneric(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Matrix3x3 result = *thisPtr * VectorFloat(scalar);
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Matrix3x3>(0))
                {
                    Matrix3x3 mat3x3 = Matrix3x3::CreateZero();
                    dc.ReadArg(0, mat3x3);
                    Matrix3x3 result = *thisPtr * mat3x3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Matrix3x3 multiply should have only 1 argument - Matrix3x3, Vector3 or a Float/Number!");
                dc.PushResult(Matrix3x3::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix3x3DivideGeneric(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() != 1 || !dc.IsNumber(0))
            {
                AZ_Error("Script", false, "Matrix3x3 divide should have only 1 argument - a Float/Number!");
                dc.PushResult(Matrix3x3::CreateIdentity());
            }
            float divisor = 0;
            dc.ReadArg(0, divisor);
            dc.PushResult(*thisPtr / divisor);
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string   Matrix3x3ToString(const Matrix3x3& m)
        {
            return AZStd::string::format("%s,%s,%s", Vector3ToString(m.GetBasisX()).c_str(), Vector3ToString(m.GetBasisY()).c_str(), Vector3ToString(m.GetBasisZ()).c_str());
        }

        /**
        * Script Wrapper for Matrix4x4
        */
        void Matrix4x4DefaultConstructor(AZ::Matrix4x4* thisPtr)
        {
            new (thisPtr) AZ::Matrix4x4(AZ::Matrix4x4::CreateIdentity());
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix4x4SetRowGeneric(Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
            if (dc.GetNumArguments() >= 5)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3) && dc.IsNumber(4))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        float w = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        dc.ReadArg(4, w);
                        thisPtr->SetRow(index, Vector4(x, y, z, w));
                        rowIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() == 3)
            {
                if (dc.IsNumber(0) && dc.IsClass<Vector3>(1) && dc.IsNumber(2))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(1, vector3);

                    float w = 0;
                    dc.ReadArg(2, w);
                    thisPtr->SetRow(index, vector3, w);
                    rowIsSet = true;
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector4>(1))
                    {
                        Vector4 vector4 = Vector4::CreateZero();
                        dc.ReadArg(1, vector4);
                        thisPtr->SetRow(index, vector4);
                        rowIsSet = true;
                    }
                }
            }

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Matrix4x4 SetRow only supports the following signatures: SetRow(index,Vector4), SetRow(index,Vector3,number) or SetRow(index,x,y,z,w)!");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix4x4SetColumnGeneric(Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            bool columnIsSet = false;
            if (dc.GetNumArguments() >= 5)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3) && dc.IsNumber(4))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        float w = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        dc.ReadArg(4, w);
                        thisPtr->SetColumn(index, Vector4(x, y, z, w));
                        columnIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() == 3)
            {
                if (dc.IsNumber(0) && dc.IsClass<Vector3>(1) && dc.IsNumber(2))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(1, vector3);

                    float w = 0;
                    dc.ReadArg(2, w);
                    thisPtr->SetColumn(index, vector3, w);
                    columnIsSet = true;
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector4>(1))
                    {
                        Vector4 vector4 = Vector4::CreateZero();
                        dc.ReadArg(1, vector4);
                        thisPtr->SetColumn(index, vector4);
                        columnIsSet = true;
                    }
                }
            }

            if (!columnIsSet)
            {
                AZ_Error("Script", false, "Matrix4x4 SetColumn only supports the following signatures: SetColumn(index,Vector4), SetColumn(index,Vector3,number) or SetColumn(index,x,y,z,w)!");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix4x4SetPositionGeneric(Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            bool positionIsSet = false;

            if (dc.GetNumArguments() == 3 &&
                dc.IsNumber(0) &&
                dc.IsNumber(1) &&
                dc.IsNumber(2))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                thisPtr->SetPosition(x, y, z);
                positionIsSet = true;
            }
            else if (dc.GetNumArguments() == 1 &&
                dc.IsClass<Vector3>(0))
            {
                Vector3 position = Vector3::CreateZero();
                dc.ReadArg(0, position);
                thisPtr->SetPosition(position);
                positionIsSet = true;
            }

            if (!positionIsSet)
            {
                AZ_Error("Script", false, "Matrix4x4 SetPosition/SetTranslation only supports the following signatures: SetPosition(Vector3), SetPosition(number,number,number)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix4x4GetRowsMultipleReturn(const Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            Vector4 row0(Vector4::CreateZero()), row1(Vector4::CreateZero()), row2(Vector4::CreateZero()), row3(Vector4::CreateZero());
            thisPtr->GetRows(&row0, &row1, &row2, &row3);
            dc.PushResult(row0);
            dc.PushResult(row1);
            dc.PushResult(row2);
            dc.PushResult(row3);
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix4x4GetColumnsMultipleReturn(const Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            Vector4 column0(Vector4::CreateZero()), column1(Vector4::CreateZero()), column2(Vector4::CreateZero()), column3(Vector4::CreateZero());
            thisPtr->GetColumns(&column0, &column1, &column2, &column3);
            dc.PushResult(column0);
            dc.PushResult(column1);
            dc.PushResult(column2);
            dc.PushResult(column3);
        }

        //////////////////////////////////////////////////////////////////////////
        void Matrix4x4MultiplyGeneric(const Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<Matrix4x4>(0))
                {
                    Matrix4x4 mat4x4 = Matrix4x4::CreateZero();
                    dc.ReadArg(0, mat4x4);
                    Matrix4x4 result = *thisPtr * mat4x4;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr * vector4;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Matrix4x4 multiply should have only 1 argument - Matrix4x4, Vector3 or Vector4!");
                dc.PushResult(Matrix4x4::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string Matrix4x4ToString(const Matrix4x4& m)
        {
            return AZStd::string::format("%s,%s,%s,%s",
                Vector4ToString(m.GetColumn(0)).c_str(), Vector4ToString(m.GetColumn(1)).c_str(),
                Vector4ToString(m.GetColumn(2)).c_str(), Vector4ToString(m.GetColumn(3)).c_str());
        }

        /**
        * Script Wrapper for Transform
        */
        void TransformDefaultConstructor(AZ::Transform* thisPtr)
        {
            new (thisPtr) AZ::Transform(AZ::Transform::CreateIdentity());
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformSetRowGeneric(Transform* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
            if (dc.GetNumArguments() >= 5)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3) && dc.IsNumber(4))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        float w = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        dc.ReadArg(4, w);
                        thisPtr->SetRow(index, Vector4(x, y, z, w));
                        rowIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() == 3)
            {
                if (dc.IsNumber(0) && dc.IsClass<Vector3>(1) && dc.IsNumber(2))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(1, vector3);

                    float w = 0;
                    dc.ReadArg(2, w);
                    thisPtr->SetRow(index, vector3, w);
                    rowIsSet = true;
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector4>(1))
                    {
                        Vector4 vector4 = Vector4::CreateZero();
                        dc.ReadArg(1, vector4);
                        thisPtr->SetRow(index, vector4);
                        rowIsSet = true;
                    }
                }
            }

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Transform SetRow only supports the following signatures: SetRow(index,Vector4), SetRow(index,Vector3,number) or SetRow(index,x,y,z,w)!");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformSetColumnGeneric(Transform* thisPtr, ScriptDataContext& dc)
        {
            bool columnIsSet = false;
            if (dc.GetNumArguments() >= 4)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        thisPtr->SetColumn(index, Vector3(x, y, z));
                        columnIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector3>(1))
                    {
                        Vector3 vector3 = Vector3::CreateZero();
                        dc.ReadArg(1, vector3);
                        thisPtr->SetColumn(index, vector3);
                        columnIsSet = true;
                    }
                }
            }

            if (!columnIsSet)
            {
                AZ_Error("Script", false, "Transform SetColumn only supports the following signatures: SetColumn(index,Vector4), SetColumn(index,Vector3,number) or SetColumn(index,x,y,z,w)!");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformSetPositionGeneric(Transform* thisPtr, ScriptDataContext& dc)
        {
            bool positionIsSet = false;

            if (dc.GetNumArguments() == 3 &&
                dc.IsNumber(0) &&
                dc.IsNumber(1) &&
                dc.IsNumber(2))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                thisPtr->SetPosition(x, y, z);
                positionIsSet = true;
            }
            else if (dc.GetNumArguments() == 1 &&
                dc.IsClass<Vector3>(0))
            {
                Vector3 position = Vector3::CreateZero();
                dc.ReadArg(0, position);
                thisPtr->SetPosition(position);
                positionIsSet = true;
            }

            if (!positionIsSet)
            {
                AZ_Error("Script", false, "Transform SetPosition/SetTranslation only supports the following signatures: SetPosition(Vector3), SetPosition(number,number,number)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformMultiplyGeneric(const Transform* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<Transform>(0))
                {
                    Transform tm = Transform::CreateZero();
                    dc.ReadArg(0, tm);
                    Transform result = *thisPtr * tm;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr * vector4;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Obb>(0))
                {
                    Obb obb;
                    dc.ReadArg(0, obb);
                    Obb result = *thisPtr * obb;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "ScriptTransform multiply should have only 1 argument - Matrix4x4, Vector3 or a Vector4!");
                dc.PushResult(Transform::CreateIdentity());
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformGetRowsMultipleReturn(const Transform* thisPtr, ScriptDataContext& dc)
        {
            Vector4 row0(Vector4::CreateZero()), row1(Vector4::CreateZero()), row2(Vector4::CreateZero());
            thisPtr->GetRows(&row0, &row1, &row2);
            dc.PushResult(row0);
            dc.PushResult(row1);
            dc.PushResult(row2);
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformGetColumnsMultipleReturn(const Transform* thisPtr, ScriptDataContext& dc)
        {
            Vector3 column0(Vector3::CreateZero()), column1(Vector3::CreateZero()), column2(Vector3::CreateZero()), column3(Vector3::CreateZero());
            thisPtr->GetColumns(&column0, &column1, &column2, &column3);
            dc.PushResult(column0);
            dc.PushResult(column1);
            dc.PushResult(column2);
            dc.PushResult(column3);
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformGetBasisAndPositionMultipleReturn(const Transform* thisPtr, ScriptDataContext& dc)
        {
            Vector3 basisX(Vector3::CreateZero()), basisY(Vector3::CreateZero()), basisZ(Vector3::CreateZero()), position(Vector3::CreateZero());
            thisPtr->GetBasisAndPosition(&basisX, &basisY, &basisZ, &position);
            dc.PushResult(basisX);
            dc.PushResult(basisY);
            dc.PushResult(basisZ);
            dc.PushResult(position);
        }

        //////////////////////////////////////////////////////////////////////////
        void TransformGetPolarDecompositionMultipleReturn(const Transform* thisPtr, ScriptDataContext& dc)
        {
            Transform orthogonal(Transform::CreateZero()), symmetric(Transform::CreateZero());
            thisPtr->GetPolarDecomposition(&orthogonal, &symmetric);
            dc.PushResult(orthogonal);
            dc.PushResult(symmetric);
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string TransformToString(const Transform& t)
        {
            return AZStd::string::format("%s,%s,%s,%s",
                Vector3ToString(t.GetColumn(0)).c_str(), Vector3ToString(t.GetColumn(1)).c_str(),
                Vector3ToString(t.GetColumn(2)).c_str(), Vector3ToString(t.GetColumn(3)).c_str());
        }

        /**
         * Script Wrapper for UUID
         */

         //////////////////////////////////////////////////////////////////////////
        void ScriptUuidConstructor(Uuid* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsString(0))
                {
                    const char* value = nullptr;
                    if (dc.ReadArg(0, value))
                    {
                        *thisPtr = Uuid(value);
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "You should provde only 1 string containing the Uuid in allowed formats!");
                }
            } break;
            default:
                *thisPtr = Uuid::CreateNull();
                break;
            }
        }

        void UuidDefaultConstructor(AZ::Uuid* thisPtr)
        {
            new (thisPtr) AZ::Uuid(AZ::Uuid::CreateNull());
        }

        //////////////////////////////////////////////////////////////////////////
        void UuidCreateStringGeneric(ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsString(0))
                {
                    const char* value = nullptr;
                    if (dc.ReadArg(0, value))
                    {
                        dc.PushResult(Uuid(value));
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "CreateString can have one parameter (null teminated string) or two parameters string and stringLength CreateString(string 'uuid', number stringLen = 0)!");
                }
            } break;
            case 2:
            {
                if (dc.IsString(0) && dc.IsNumber(1))
                {
                    const char* uuidString = nullptr;
                    unsigned int uuidStringLength = 0;
                    if (dc.ReadArg(0, uuidString) && dc.ReadValue(1, uuidStringLength))
                    {
                        dc.PushResult(Uuid(uuidString, uuidStringLength));
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "CreateString can have one parameter (null teminated string) or two parameters string and stringLength CreateString(string 'uuid', number stringLen = 0)!");
                }
            }
            default:
                break;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string UuidToString(const Uuid& id)
        {
            char buffer[64];
            AZStd::string result;
            if (id.ToString(buffer, AZ_ARRAY_SIZE(buffer)))
            {
                result = buffer;
            }
            return result;
        }

        /**
         * Script Wrapper for Crc32
         */

         //////////////////////////////////////////////////////////////////////////
        void ScriptCrc32Constructor(Crc32* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() > 0)
            {
                if (dc.IsString(0))
                {
                    const char* str = nullptr;
                    if (dc.ReadArg(0, str))
                    {
                        *thisPtr = Crc32(str);
                    }
                }
                else if (dc.IsClass<Crc32>(0))
                {
                    Crc32 crc32;
                    dc.ReadArg(0, crc32);
                    *thisPtr = crc32;
                }
                else if (dc.IsNumber(0))
                {
                    u32 value = 0;
                    dc.ReadArg(0, value);
                    *thisPtr = value;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Crc32AddGeneric(Crc32* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() > 0)
            {
                if (dc.IsString(0))
                {
                    const char* str = nullptr;
                    if (dc.ReadArg(0, str))
                    {
                        thisPtr->Add(str);
                    }
                }
                //else if ()
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string Crc32ToString(const Crc32& crc32)
        {
            return AZStd::string::format("0x%08x", static_cast<u32>(crc32));
        }

        /**
        * Script Wrapper for Random
        */

        //////////////////////////////////////////////////////////////////////////
        void ScriptRandomConstructor(SimpleLcgRandom* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number;
                    if (dc.ReadArg(0, number))
                    {
                        AZ::u64 seed = static_cast<AZ::u64>(number);
                        *thisPtr = SimpleLcgRandom(seed);
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "You should provde only 1 number containing the random seed!");
                }
            } break;
            default:
                *thisPtr = SimpleLcgRandom();
                break;
            }
        }

        /**
        * Script Wrapper for Axis Aligned Bounding Box
        */
        void AabbDefaultConstructor(AZ::Aabb* thisPtr)
        {
            new (thisPtr) AZ::Aabb(AZ::Aabb::CreateNull());
        }

        //////////////////////////////////////////////////////////////////////////
        void AabbSetGeneric(Aabb* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 2 &&
                dc.IsClass<Vector3>(0) &&
                dc.IsClass<Vector3>(1))
            {
                Vector3 min, max;
                dc.ReadArg<Vector3>(0, min);
                dc.ReadArg<Vector3>(1, max);
                thisPtr->Set(min, max);
            }
            else
            {
                AZ_Error("Script", false, "ScriptAabb Set only supports two arguments: Vector3 min, Vector3 max");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void AabbGetAsSphereMultipleReturn(const Aabb* thisPtr, ScriptDataContext& dc)
        {
            Vector3 center;
            VectorFloat radius;
            thisPtr->GetAsSphere(center, radius);
            dc.PushResult(center);
            dc.PushResult(radius);
        }

        //////////////////////////////////////////////////////////////////////////
        void AabbContainsGeneric(const Aabb* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<Vector3>(0))
                {
                    Vector3 v = Vector3::CreateZero();
                    dc.ReadArg<Vector3>(0, v);
                    dc.PushResult<bool>(thisPtr->Contains(v));
                }
                else if (dc.IsClass<Aabb>(0))
                {
                    Aabb aabb = Aabb::CreateNull();
                    dc.ReadArg<Aabb>(0, aabb);
                    dc.PushResult<bool>(thisPtr->Contains(aabb));
                }
            }

            if (dc.GetNumResults() == 0)
            {
                AZ_Error("Script", false, "ScriptAabb Contains expects one argument: Either Vector3 or Aabb");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string AabbToString(const Aabb& aabb)
        {
            return AZStd::string::format("Min %s Max %s",
                Vector3ToString(aabb.GetMin()).c_str(),
                Vector3ToString(aabb.GetMax()).c_str());
        }

        /**
        * Script Wrapper for Oriented Bounding Box
        */
        void ObbDefaultConstructor(Obb* thisPtr)
        {
            new (thisPtr) Obb(Obb::CreateFromPositionAndAxes(Vector3::CreateZero(),
                Vector3(1, 0, 0), 0.5,
                Vector3(0, 1, 0), 0.5,
                Vector3(0, 0, 1), 0.5));
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string ObbToString(const Obb& obb)
        {
            return AZStd::string::format("Position %s AxisX %s AxisY %s AxisZ %s halfLengthX=%.7f halfLengthY=%.7f halfLengthZ=%.7f",
                Vector3ToString(obb.GetPosition()).c_str(),
                Vector3ToString(obb.GetAxisX()).c_str(),
                Vector3ToString(obb.GetAxisY()).c_str(),
                Vector3ToString(obb.GetAxisZ()).c_str(),
                obb.GetHalfLengthX(),
                obb.GetHalfLengthY(),
                obb.GetHalfLengthZ());
        }

        /**
        * Script Wrapper for Plane
        */
        void PlaneDefaultConstructor(Plane* thisPtr)
        {
            new (thisPtr) Plane(Plane::CreateFromNormalAndPoint(Vector3(0, 0, 1), Vector3::CreateZero()));
        }

        //////////////////////////////////////////////////////////////////////////
        void PlaneSetGeneric(Plane* thisPtr, ScriptDataContext& dc)
        {
            // Plane.SetGeneric has three overloads:
            // Set(Vector4)
            // Set(Vector3, float)
            // Set(float, float, float, float)
            bool valueWasSet = false;
            if (dc.GetNumArguments() == 1 && dc.IsClass<Vector4>(0))
            {
                Vector4 vector4 = Vector4::CreateZero();
                dc.ReadArg(0, vector4);
                thisPtr->Set(vector4);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 normal = Vector3::CreateZero();
                float d = 0;
                dc.ReadArg(0, normal);
                dc.ReadArg(1, d);
                thisPtr->Set(normal, d);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 &&
                dc.IsNumber(0) &&
                dc.IsNumber(1) &&
                dc.IsNumber(2) &&
                dc.IsNumber(3))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                float d = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                dc.ReadArg(3, d);
                thisPtr->Set(x, y, z, d);
                valueWasSet = true;
            }
            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Plane.Set only supports Set(Vector4), Set(Vector3, number), Set(number, number, number, number)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void PlaneCastRayMultipleReturn(const Plane* thisPtr, ScriptDataContext& dc)
        {
            // CastRay as implemented for Planes has two overloads,
            // each with the same pair of inputs but different return values.
            // As one return value (hit point) is based on the other (hit time), for simplicity, the Lua implementation
            // just returns all three values: does the ray hit? When does it hit? Where does it hit?
            if (!dc.IsClass<Vector3>(0) || !dc.IsClass<Vector3>(1))
            {
                AZ_Error("Script", false, "ScriptPlane CastRay requires two ScriptVector3s as arguments.");
                return;
            }

            Vector3 start(Vector3::CreateZero()), dir(Vector3::CreateZero()), hitPoint;
            VectorFloat hitTime(VectorFloat::CreateZero());

            dc.ReadArg(0, start);
            dc.ReadArg(1, dir);


            bool result = thisPtr->CastRay(start, dir, hitTime);
            hitPoint = start + dir * hitTime;

            dc.PushResult(result);
            dc.PushResult(hitPoint);
            dc.PushResult(hitTime);
        }

        //////////////////////////////////////////////////////////////////////////
        void PlaneIntersectSegmentMultipleReturn(const Plane* thisPtr, ScriptDataContext& dc)
        {
            // IntersectSegment as implemented for Planes has two overloads,
            // each with the same pair of inputs but different return values.
            // As one return value (hit point) is based on the other (hit time), for simplicity, the Lua implementation
            // just returns all three values: does the ray hit? When does it hit? Where does it hit?

            if (!dc.IsClass<Vector3>(0) || !dc.IsClass<Vector3>(0))
            {
                AZ_Error("Script", false, "ScriptPlane CastRay requires two ScriptVector3s as arguments.");
                return;
            }

            Vector3 start(Vector3::CreateZero()), end(Vector3::CreateZero()), hitPoint;
            VectorFloat hitTime(VectorFloat::CreateZero());

            dc.ReadArg(0, start);
            dc.ReadArg(1, end);

            bool result = thisPtr->IntersectSegment(start, end, hitTime);
            Vector3 dir = end - start;
            hitPoint = start + dir * hitTime;

            dc.PushResult(result);
            dc.PushResult(hitPoint);
            dc.PushResult(hitTime);
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string PlaneToString(const Plane& p)
        {
            return AZStd::string::format("%s",
                Vector4ToString(p.GetPlaneEquationCoefficients()).c_str());
        }

        void VectorFloatGetSinCosMultipleReturn(const VectorFloat* thisPtr, ScriptDataContext& dc)
        {
            VectorFloat sin, cos;
            thisPtr->GetSinCos(sin, cos);
            dc.PushResult(sin);
            dc.PushResult(cos);
        }

    } // namespace Internal

    class MathGlobals
    {
    public:
        AZ_TYPE_INFO(MathGlobals, "{35D44724-7470-42F2-A0E3-4E4349793B98}");
        AZ_CLASS_ALLOCATOR(MathGlobals, SystemAllocator, 0);

        MathGlobals() = default;
        ~MathGlobals() = default;
    };

    void MathReflect(BehaviorContext& context)
    {
        context.Constant("FloatEpsilon", BehaviorConstant(static_cast<float>(g_fltEps)));
        context.Constant("FloatEpsilonSq", BehaviorConstant(static_cast<float>(g_fltEpsSq)));

         context.Class<MathGlobals>("Math")
            ->Method("DegToRad", &DegToRad, {{{"Degrees", ""}}})
            ->Method("RadToDeg", &RadToDeg, {{{"Radians", ""}}})
            ->Method("Sin", &sinf)
            ->Method("Cos", &cosf)
            ->Method("Tan", &tanf)
            ->Method("ArcSin", &asinf)
            ->Method("ArcCos", &acosf)
            ->Method("ArcTan", &atanf)
            ->Method("ArcTan2", &atan2f)
            ->Method("Ceil", &ceilf)
            ->Method("Floor", &floorf)
            ->Method("Round", &roundf)
            ->Method("Sqrt", &sqrtf)
            ->Method("Pow", &powf)
            ->Method("Sign", &GetSign)
            ->Method("Min", &GetMin<float>)
            ->Method("Max", &GetMax<float>)
            ->Method<float(float, float, float)>("Lerp", &AZ::Lerp,               { { { "a", "" },{ "b", "" },{ "t", "" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns a linear interpolation between two values 'a' and 'b'")
            ->Method<float(float, float, float)>("LerpInverse", &AZ::LerpInverse, { { { "a", "" },{ "b", "" },{ "value", "" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns a value t where Lerp(a,b,t)==value (or 0 if a==b)")
            ->Method("Clamp", &GetClamp<float>)
            ->Method("IsEven", &IsEven<int>)
            ->Method<bool(int)>("IsOdd", &IsOdd<int>, {{{"Value",""}}})
            ->Method<float(float,float)>("Mod", &GetMod)
            ->Method<void(VectorFloat, VectorFloat&, VectorFloat&)>("GetSinCos", &GetSinCos<VectorFloat>)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::VectorFloatGetSinCosMultipleReturn)
            ->Method<bool(double, double, double)>("IsClose", &AZ::IsClose, context.MakeDefaultValues(static_cast<double>(g_fltEps)))
             ;

        // Vector2
        context.Class<Vector2>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Constructor<float>()->
            Constructor<float, float>()->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::Vector2ScriptConstructor)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::Vector2DefaultConstructor)->
            Property("x", &Vector2::GetX, &Vector2::SetX)->
            Property("y", &Vector2::GetY, &Vector2::SetY)->
            Method<const Vector2(Vector2::*)() const>("Unary", &Vector2::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
            Method("ToString",&Internal::Vector2ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Vector2(Vector2::*)(float) const>("MultiplyFloat",&Vector2::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector2MultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method<const Vector2(Vector2::*)(const Vector2&) const>("MultiplyVector2",&Vector2::operator*)->
                Attribute(AZ::Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic multiply above
            Method<const Vector2(Vector2::*)(float) const>("DivideFloat",&Vector2::operator/)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector2DivideGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method<const Vector2(Vector2::*)(const Vector2&) const>("DivideVector2",&Vector2::operator/)->
                Attribute(AZ::Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic divide above
            Method("Equal",&Vector2::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessThan",&Vector2::IsLessThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessEqualThan",&Vector2::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Add",&Vector2::operator+)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
            Method<const Vector2(Vector2::*)(const Vector2&) const>("Subtract",&Vector2::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
            Method<void (Vector2::*)(float,float)>("Set", &Vector2::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector2SetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Clone", [](const Vector2& rhs) -> Vector2 { return rhs; }, {{{"Vector2",""}}})->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Vector2::GetElement)->
            Method("SetElement", &Vector2::SetElement, { { {"Index", ""}, {"Value",""} } })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetLength", &Vector2::GetLength)->
            Method("GetLengthSq", &Vector2::GetLengthSq)->
            Method("SetLength", &Vector2::SetLength)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetNormalized", &Vector2::GetNormalized)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetNormalizedSafe", &Vector2::GetNormalizedSafe)->
            Method("Normalize", &Vector2::Normalize)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeWithLength", &Vector2::NormalizeWithLength)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeSafe", &Vector2::NormalizeSafe, context.MakeDefaultValues(static_cast<float>(g_simdTolerance)))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeSafeWithLength", &Vector2::NormalizeSafeWithLength, context.MakeDefaultValues(static_cast<float>(g_simdTolerance)))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsNormalized", &Vector2::IsNormalized, context.MakeDefaultValues(static_cast<float>(g_simdTolerance)))->
            Method("GetDistance", &Vector2::GetDistance)->
            Method("GetDistanceSq", &Vector2::GetDistanceSq)->
            Method("Lerp", &Vector2::Lerp)->
            Method("Slerp", &Vector2::Slerp)->
            Method("Dot", &Vector2::Dot)->
            Method("GetPerpendicular", &Vector2::GetPerpendicular)->
            Method("IsClose", &Vector2::IsClose, context.MakeDefaultValues(static_cast<float>(g_simdTolerance)))->
            Method("IsZero", &Vector2::IsZero, context.MakeDefaultValues(static_cast<float>(g_fltEps)))->
            Method("IsLessThan", &Vector2::IsLessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsLessEqualThan", &Vector2::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterThan", &Vector2::IsGreaterThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterEqualThan", &Vector2::IsGreaterEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetMin", &Vector2::GetMin)->
            Method("GetMax", &Vector2::GetMax)->
            Method("GetClamp", &Vector2::GetClamp)->
            Method("GetAbs", &Vector2::GetAbs)->
            Method("Project", &Vector2::Project)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ProjectOnNormal", &Vector2::ProjectOnNormal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetProjected", &Vector2::GetProjected)->
            Method("GetProjectedOnNormal", &Vector2::GetProjectedOnNormal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsFinite", &Vector2::IsFinite)->
            Method("CreateAxisX", &Vector2::CreateAxisX, context.MakeDefaultValues(1.0f))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateAxisY", &Vector2::CreateAxisY, context.MakeDefaultValues(1.0f))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromAngle", &Vector2::CreateFromAngle, context.MakeDefaultValues(0.0f))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateOne", &Vector2::CreateOne)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateZero", &Vector2::CreateZero)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateSelectCmpEqual", &Vector2::CreateSelectCmpEqual)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateSelectCmpGreaterEqual", &Vector2::CreateSelectCmpGreaterEqual)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateSelectCmpGreater", &Vector2::CreateSelectCmpGreater)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetSelect", &Vector2::GetSelect)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Select", &Vector2::Select)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetMadd", &Vector2::GetMadd)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Madd", &Vector2::Madd)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ConstructFromValues", &ScriptCanvas::ConstructVector2)
            ;
            ;

        // Vector3
        context.Class<Vector3>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Constructor<const VectorFloat&>()->
            Constructor<const VectorFloat&, const VectorFloat&, const VectorFloat&>()->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::Vector3ScriptConstructor)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::Vector3DefaultConstructor)->
            Property("x", &Vector3::GetX, &Vector3::SetX)->
            Property("y", &Vector3::GetY, &Vector3::SetY)->
            Property("z", &Vector3::GetZ, &Vector3::SetZ)->
            Method<const Vector3(Vector3::*)() const>("Unary", &Vector3::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
            Method("ToString",&Internal::Vector3ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Vector3(Vector3::*)(const VectorFloat&) const>("MultiplyFloat",&Vector3::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector3MultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method<const Vector3(Vector3::*)(const Vector3&) const>("MultiplyVector3",&Vector3::operator*)->
                Attribute(AZ::Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic multiply above
            Method<const Vector3(Vector3::*)(const VectorFloat&) const>("DivideFloat",&Vector3::operator/)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector3DivideGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method<const Vector3(Vector3::*)(const Vector3&) const>("DivideVector3",&Vector3::operator/)->
                Attribute(AZ::Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic divide above
            Method("Clone", [](const Vector3& rhs) -> Vector3 { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Equal",&Vector3::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessThan",&Vector3::IsLessThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessEqualThan",&Vector3::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Add",&Vector3::operator+)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
            Method<const Vector3(Vector3::*)(const Vector3&) const>("Subtract",&Vector3::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
            Method<void (Vector3::*)(const VectorFloat&, const VectorFloat&, const VectorFloat&)>("Set", &Vector3::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector3SetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Vector3::GetElement)->
            Method("SetElement", &Vector3::SetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetLength", &Vector3::GetLengthExact)->
            Method("GetLengthSq", &Vector3::GetLengthSq)->
            Method("GetLengthReciprocal", &Vector3::GetLengthReciprocalExact)->
            Method("GetNormalized", &Vector3::GetNormalizedExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Normalize", &Vector3::NormalizeExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeWithLength", &Vector3::NormalizeWithLengthExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetNormalizedSafe", &Vector3::GetNormalizedSafeExact, context.MakeDefaultValues(g_simdTolerance))->
            Method("NormalizeSafe", &Vector3::NormalizeSafeExact, context.MakeDefaultValues(g_simdTolerance))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeSafeWithLength", &Vector3::NormalizeSafeWithLengthExact, context.MakeDefaultValues(g_simdTolerance))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsNormalized", &Vector3::IsNormalized, context.MakeDefaultValues(g_simdTolerance))->
            Method("SetLength", &Vector3::SetLength)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetLengthApprox", &Vector3::SetLengthApprox)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetLengthExact", &Vector3::SetLengthExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetDistance", &Vector3::GetDistanceExact)->
            Method("GetDistanceSq", &Vector3::GetDistanceSq)->
            Method("Lerp", &Vector3::Lerp)->
            Method("Slerp", &Vector3::Slerp)->
            Method("Dot", &Vector3::Dot)->
            Method("Cross", &Vector3::Cross)->
            Method("CrossXAxis", &Vector3::CrossXAxis)->
            Method("CrossYAxis", &Vector3::CrossYAxis)->
            Method("CrossZAxis", &Vector3::CrossZAxis)->
            Method("XAxisCross", &Vector3::XAxisCross)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("YAxisCross", &Vector3::YAxisCross)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ZAxisCross", &Vector3::ZAxisCross)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsClose", &Vector3::IsClose, context.MakeDefaultValues(g_simdTolerance))->
            Method("IsZero", &Vector3::IsZero, context.MakeDefaultValues(g_fltEps))->
            Method("IsLessThan", &Vector3::IsLessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsLessEqualThan", &Vector3::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterThan", &Vector3::IsGreaterThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterEqualThan", &Vector3::IsGreaterEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetMin", &Vector3::GetMin)->
            Method("GetMax", &Vector3::GetMax)->
            Method("GetClamp", &Vector3::GetClamp)->
            Method("GetSin", &Vector3::GetSin)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetCos", &Vector3::GetCos)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetSinCos", &Vector3::GetSinCos)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector3GetSinCosMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetAngleMod", &Vector3::GetAngleMod)->
            Method("GetAbs", &Vector3::GetAbs)->
            Method("GetReciprocal", &Vector3::GetReciprocalExact)->
            Method("BuildTangentBasis", &Vector3::BuildTangentBasis)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector3BuildTangentBasis)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetMadd", &Vector3::GetMadd)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Madd", &Vector3::Madd)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsPerpendicular", &Vector3::IsPerpendicular, context.MakeDefaultValues(g_simdTolerance))->
            Method("Project", &Vector3::Project)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ProjectOnNormal", &Vector3::ProjectOnNormal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetProjected", &Vector3::GetProjected)->
            Method("GetProjectedOnNormal", &Vector3::GetProjectedOnNormal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsFinite", &Vector3::IsFinite)->
            Method("CreateAxisX", &Vector3::CreateAxisX, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateAxisY", &Vector3::CreateAxisY, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateAxisZ", &Vector3::CreateAxisZ, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateOne", &Vector3::CreateOne)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateZero", &Vector3::CreateZero)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ConstructFromValues", &ScriptCanvas::ConstructVector3)
            ;
            
        // Vector4
        context.Class<Vector4>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Constructor<const VectorFloat&>()->
            Constructor<const VectorFloat&, const VectorFloat&, const VectorFloat&, const VectorFloat&>()->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::Vector4ScriptConstructor)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::Vector4DefaultConstructor)->
            Property("x", &Vector4::GetX, &Vector4::SetX)->
            Property("y", &Vector4::GetY, &Vector4::SetY)->
            Property("z", &Vector4::GetZ, &Vector4::SetZ)->
            Property("w", &Vector4::GetW, &Vector4::SetW)->
            Method<const Vector4(Vector4::*)() const>("Unary", &Vector4::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
            Method("ToString", &Internal::Vector4ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Vector4(Vector4::*)(const VectorFloat&) const>("MultiplyFloat", &Vector4::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector4MultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method<const Vector4(Vector4::*)(const Vector4&) const>("MultiplyVector4", &Vector4::operator*)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
            Method<const Vector4(Vector4::*)(const VectorFloat&) const>("DivideFloat", &Vector4::operator/)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector4DivideGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method<const Vector4(Vector4::*)(const Vector4&) const>("DivideVector4", &Vector4::operator/)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic divide above
            Method("Clone", [](const Vector4& rhs) -> Vector4 { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Equal", &Vector4::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessThan", &Vector4::IsLessThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessEqualThan", &Vector4::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Add", &Vector4::operator+)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
            Method<const Vector4(Vector4::*)(const Vector4&) const>("Subtract", &Vector4::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
            Method<void (Vector4::*)(const VectorFloat&, const VectorFloat&, const VectorFloat&, const VectorFloat&)>("Set", &Vector4::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Vector4SetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Vector4::GetElement)->
            Method("SetElement", &Vector4::SetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetLength", &Vector4::GetLengthExact)->
            Method("GetLengthSq", &Vector4::GetLengthSq)->
            Method("GetLengthReciprocal", &Vector4::GetLengthReciprocalExact)->
            Method("GetNormalized", &Vector4::GetNormalizedExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Normalize", &Vector4::NormalizeExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeWithLength", &Vector4::NormalizeWithLengthExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetNormalizedSafe", &Vector4::GetNormalizedSafeExact, context.MakeDefaultValues(g_simdTolerance))->
            Method("NormalizeSafe", &Vector4::NormalizeSafeExact, context.MakeDefaultValues(g_simdTolerance))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeSafeWithLength", &Vector4::NormalizeSafeWithLengthExact, context.MakeDefaultValues(g_simdTolerance))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsNormalized", &Vector4::IsNormalized, context.MakeDefaultValues(g_simdTolerance))->
            Method("Dot", &Vector4::Dot)->
            Method("Dot3", &Vector4::Dot3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Homogenize", &Vector4::Homogenize)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetHomogenized", &Vector4::GetHomogenized)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsClose", &Vector4::IsClose, context.MakeDefaultValues(g_simdTolerance))->
            Method("IsZero", &Vector4::IsZero, context.MakeDefaultValues(g_fltEps))->
            Method("IsLessThan", &Vector4::IsLessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsLessEqualThan", &Vector4::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterThan", &Vector4::IsGreaterThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterEqualThan", &Vector4::IsGreaterEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetAbs", &Vector4::GetAbs)->
            Method("GetReciprocal", &Vector4::GetReciprocal)->
            Method("IsFinite", &Vector4::IsFinite)->
            Method("CreateAxisX", &Vector4::CreateAxisX, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateAxisY", &Vector4::CreateAxisY, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateAxisZ", &Vector4::CreateAxisZ, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateAxisW", &Vector4::CreateAxisW, context.MakeDefaultValues(VectorFloat::CreateOne()))->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromVector3", &Vector4::CreateFromVector3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromVector3AndFloat", &Vector4::CreateFromVector3AndFloat)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateOne", &Vector4::CreateOne)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateZero", &Vector4::CreateZero)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ConstructFromValues", &ScriptCanvas::ConstructVector4)
            ;
        
        // Color
        context.Class<Color>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Constructor<const VectorFloat&>()->
            Constructor<const VectorFloat&, const VectorFloat&, const VectorFloat&, const VectorFloat&>()->
                Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ColorScriptConstructor)->
                Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::ColorDefaultConstructor)->
            Property("r", &Color::GetR, &Color::SetR)->
            Property("g", &Color::GetG, &Color::SetG)->
            Property("b", &Color::GetB, &Color::SetB)->
            Property("a", &Color::GetA, &Color::SetA)->
            Method<const Color(Color::*)() const>("Unary", &Color::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ToString", &Internal::ColorToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Color(Color::*)(const VectorFloat&) const>("MultiplyFloat", &Color::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::ColorMultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method<const Color(Color::*)(const Color&) const>("MultiplyColor", &Color::operator*)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<const Color(Color::*)(const VectorFloat&) const>("DivideFloat", &Color::operator/)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::ColorDivideGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method<const Color(Color::*)(const Color&) const>("DivideColor", &Color::operator/)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic divide above
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Clone", [](const Color& rhs) -> Color { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Equal", &Color::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessThan", &Color::IsLessThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LessEqualThan", &Color::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Add", &Color::operator+)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<const Color(Color::*)(const Color&) const>("Subtract", &Color::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void (Color::*)(const VectorFloat&, const VectorFloat&, const VectorFloat&, const VectorFloat&)>("Set", &Color::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::ColorSetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Color::GetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetElement", &Color::SetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Dot", &Color::Dot)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Dot3", &Color::Dot3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsClose", &Color::IsClose, context.MakeDefaultValues(g_simdTolerance))->
            Method("IsZero", &Color::IsZero, context.MakeDefaultValues(g_fltEps))->
            Method("IsLessThan", &Color::IsLessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsLessEqualThan", &Color::IsLessEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterThan", &Color::IsGreaterThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsGreaterEqualThan", &Color::IsGreaterEqualThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromVector3", &Color::CreateFromVector3)->
            Method("CreateFromVector3AndFloat", &Color::CreateFromVector3AndFloat)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateOne", &Color::CreateOne)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateZero", &Color::CreateZero)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ToU32", &Color::ToU32)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ToU32LinearToGamma", &Color::ToU32LinearToGamma)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("FromU32", &Color::FromU32)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("FromU32GammaToLinear", &Color::FromU32GammaToLinear)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("LinearToGamma", &Color::LinearToGamma)->
            Method("GammaToLinear", &Color::GammaToLinear)->
            Method("ConstructFromValues", &ScriptCanvas::ConstructColor)
            ;

        // Quaternion
        context.Class<Quaternion>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Constructor<const VectorFloat&>()->
            Constructor<const VectorFloat&, const VectorFloat&, const VectorFloat&, const VectorFloat&>()->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::QuaternionScriptConstrcutor)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::QuaternionDefaultConstructor)->
            Property("x", &Quaternion::GetX, &Quaternion::SetX)->
            Property("y", &Quaternion::GetY, &Quaternion::SetY)->
            Property("z", &Quaternion::GetZ, &Quaternion::SetZ)->
            Property("w", &Quaternion::GetW, &Quaternion::SetW)->
            Method<const Quaternion(Quaternion::*)() const>("Unary", &Quaternion::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
            Method("ToString", &Internal::QuaternionToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Quaternion(Quaternion::*)(const VectorFloat&) const>("MultiplyFloat", &Quaternion::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::QuaternionMultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method<const Quaternion(Quaternion::*)(const Quaternion&) const>("MultiplyQuaternion", &Quaternion::operator*)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<const Quaternion(Quaternion::*)(const VectorFloat&) const>("DivideFloat", &Quaternion::operator/)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::QuaternionDivideGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method("Clone", [](const Quaternion& rhs) -> Quaternion { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Equal", &Quaternion::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Add", &Quaternion::operator+)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<const Quaternion(Quaternion::*)(const Quaternion&) const>("Subtract", &Quaternion::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void (Quaternion::*)(const VectorFloat&, const VectorFloat&, const VectorFloat&, const VectorFloat&)>("Set", &Quaternion::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::QuaternionSetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Quaternion::GetElement)->
            Method("SetElement", &Quaternion::SetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetLength", &Quaternion::GetLengthExact)->
            Method("GetLengthSq", &Quaternion::GetLengthSq)->
            Method("GetLengthReciprocal", &Quaternion::GetLengthReciprocalExact)->
            Method("GetNormalized", &Quaternion::GetNormalizedExact)->
            Method("Normalize", &Quaternion::NormalizeExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("NormalizeWithLength", &Quaternion::NormalizeWithLengthExact)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Lerp", &Quaternion::Lerp)->
            Method("Slerp", &Quaternion::Slerp)->
            Method("Squad", &Quaternion::Squad)->
            Method("GetConjugate", &Quaternion::GetConjugate)->
            Method("GetInverseFast", &Quaternion::GetInverseFast)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("InvertFast", &Quaternion::InvertFast)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetInverseFull", &Quaternion::GetInverseFull)->                
            Method("InvertFull", &Quaternion::InvertFull)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Dot", &Quaternion::Dot)->
            Method("IsClose", &Quaternion::IsClose, context.MakeDefaultValues(g_simdTolerance))->
            Method("IsIdentity", &Quaternion::IsIdentity, context.MakeDefaultValues(g_simdTolerance))->
            Method("IsZero", &Quaternion::IsZero, context.MakeDefaultValues(g_fltEps))->
            Method("GetImaginary", &Quaternion::GetImaginary)->
            Method("IsFinite", &Quaternion::IsFinite)->
            Method("GetAngle", &Quaternion::GetAngle)->
            Method("CreateIdentity", &Quaternion::CreateIdentity)->
            Method("CreateZero", &Quaternion::CreateZero)->
            Method("CreateRotationX", &Quaternion::CreateRotationX)->
            Method("CreateRotationY", &Quaternion::CreateRotationY)->
            Method("CreateRotationZ", &Quaternion::CreateRotationZ)->
            Method("CreateFromVector3", &Quaternion::CreateFromVector3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromVector3AndValue", &Quaternion::CreateFromVector3AndValue)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromTransform", &Quaternion::CreateFromTransform)->
            Method("CreateFromMatrix3x3", &Quaternion::CreateFromMatrix3x3)->
            Method("CreateFromMatrix4x4", &Quaternion::CreateFromMatrix4x4)->
            Method("CreateFromAxisAngle", &Quaternion::CreateFromAxisAngle)->
            Method("CreateShortestArc", &Quaternion::CreateShortestArc)
            ;

        // Matrix3x3
        context.Class<Matrix3x3>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::Matrix3x3DefaultConstructor)->
            Property<const Vector3(Matrix3x3::*)() const, void (Matrix3x3::*)(const Vector3&)>("basisX", &Matrix3x3::GetBasisX, &Matrix3x3::SetBasisX)->
            Property<const Vector3(Matrix3x3::*)() const, void (Matrix3x3::*)(const Vector3&)>("basisY", &Matrix3x3::GetBasisY, &Matrix3x3::SetBasisY)->
            Property<const Vector3(Matrix3x3::*)() const, void (Matrix3x3::*)(const Vector3&)>("basisZ", &Matrix3x3::GetBasisZ, &Matrix3x3::SetBasisZ)->
            Method<const Matrix3x3(Matrix3x3::*)() const>("Unary", &Matrix3x3::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
            Method("ToString", &Internal::Matrix3x3ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Matrix3x3(Matrix3x3::*)(const VectorFloat&) const>("MultiplyFloat", &Matrix3x3::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3MultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method<const Vector3(Matrix3x3::*)(const Vector3&) const>("MultiplyVector3", &Matrix3x3::operator*)->            
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
            Method<const Matrix3x3(Matrix3x3::*)(const Matrix3x3&) const>("MultiplyMatrix3x3", &Matrix3x3::operator*)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
            Method<const Matrix3x3(Matrix3x3::*)(const VectorFloat&) const>("DivideFloat", &Matrix3x3::operator/)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3DivideGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method("Clone", [](const Matrix3x3& rhs) -> Matrix3x3 { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Equal", &Matrix3x3::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Add", &Matrix3x3::operator+)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->                
            Method<const Matrix3x3(Matrix3x3::*)(const Matrix3x3&) const>("Subtract", &Matrix3x3::operator-)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->                
            Method("GetBasis", &Matrix3x3::GetBasis)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3GetBasisMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetBasis", &Matrix3x3::SetBasis)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Matrix3x3::GetElement)->
            Method("SetElement", &Matrix3x3::SetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetRow", &Matrix3x3::GetRow)->
            Method("GetRows", &Matrix3x3::GetRows)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3GetRowsMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void (Matrix3x3::*)(int, const Vector3&)>("SetRow", &Matrix3x3::SetRow)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3SetRowGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetRows", &Matrix3x3::SetRows)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetColumn", &Matrix3x3::GetColumn)->
            Method("GetColumns", &Matrix3x3::GetColumn)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3GetColumnsMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void (Matrix3x3::*)(int, const Vector3&)>("SetColumn", &Matrix3x3::SetColumn)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3SetColumnGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetColumns", &Matrix3x3::SetColumns)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("TransposedMultiply", &Matrix3x3::TransposedMultiply)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetTranspose", &Matrix3x3::GetTranspose)->
            Method("Transpose", &Matrix3x3::Transpose)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetInverseFull", &Matrix3x3::GetInverseFull)->
            Method("InvertFull", &Matrix3x3::InvertFull)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetInverseFast", &Matrix3x3::GetInverseFast)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("InvertFast", &Matrix3x3::InvertFast)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("RetrieveScale", &Matrix3x3::RetrieveScale)->                
            Method("ExtractScale", &Matrix3x3::ExtractScale)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("MultiplyByScale", &Matrix3x3::MultiplyByScale)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<const Matrix3x3(Matrix3x3::*)()const>("GetPolarDecomposition", &Matrix3x3::GetPolarDecomposition)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x3GetPolarDecompositionMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsOrthogonal", &Matrix3x3::IsOrthogonal, context.MakeDefaultValues(g_simdTolerance))->
            Method("GetOrthogonalized", &Matrix3x3::GetOrthogonalized)->
            Method("Orthogonalize", &Matrix3x3::Orthogonalize)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsClose", &Matrix3x3::IsClose, context.MakeDefaultValues(g_simdTolerance))->
            Method("SetRotationPartFromQuaternion", &Matrix3x3::SetRotationPartFromQuaternion)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetDiagonal", &Matrix3x3::GetDiagonal)->
            Method("GetDeterminant", &Matrix3x3::GetDeterminant)->
            Method("GetAdjugate", &Matrix3x3::GetAdjugate)->
            Method("IsFinite", &Matrix3x3::IsFinite)->
            Method("CreateIdentity", &Matrix3x3::CreateIdentity)->
            Method("CreateZero", &Matrix3x3::CreateZero)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromValue", &Matrix3x3::CreateFromValue)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromRows", &Matrix3x3::CreateFromRows)->
            Method("CreateFromColumns", &Matrix3x3::CreateFromColumns)->
            Method("CreateRotationX", &Matrix3x3::CreateRotationX)->
            Method("CreateRotationY", &Matrix3x3::CreateRotationY)->
            Method("CreateRotationZ", &Matrix3x3::CreateRotationZ)->
            Method("CreateFromTransform", &Matrix3x3::CreateFromTransform)->
            Method("CreateFromMatrix4x4", &Matrix3x3::CreateFromMatrix4x4)->
            Method("CreateFromQuaternion", &Matrix3x3::CreateFromQuaternion)->
            Method("CreateScale", &Matrix3x3::CreateScale)->
            Method("CreateDiagonal", &Matrix3x3::CreateDiagonal)->
            Method("CreateCrossProduct", &Matrix3x3::CreateCrossProduct)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ConstructFromValues", &ScriptCanvas::ConstructMatrix3x3)
            ;

        // Matrix4x4
        context.Class<Matrix4x4>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::Matrix4x4DefaultConstructor)->
            Property<const Vector4(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector4&)>("basisX", &Matrix4x4::GetBasisX, &Matrix4x4::SetBasisX)->
            Property<const Vector4(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector4&)>("basisY", &Matrix4x4::GetBasisY, &Matrix4x4::SetBasisY)->
            Property<const Vector4(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector4&)>("basisZ", &Matrix4x4::GetBasisZ, &Matrix4x4::SetBasisZ)->
            Property<const Vector3(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector3&)>("position", &Matrix4x4::GetPosition, &Matrix4x4::SetPosition)->
            Method("ToString", &Internal::Matrix4x4ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method<const Vector3(Matrix4x4::*)(const Vector3&) const>("MultiplyVector3", &Matrix4x4::operator*)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4MultiplyGeneric)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<const Vector4(Matrix4x4::*)(const Vector4&) const>("MultiplyVector4", &Matrix4x4::operator*)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
            Method<const Matrix4x4(Matrix4x4::*)(const Matrix4x4&) const>("MultiplyMatrix4x4", &Matrix4x4::operator*)->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
            Method("Equal", &Matrix4x4::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Clone", [](const Matrix4x4& rhs) -> Matrix4x4 { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetElement", &Matrix4x4::GetElement)->
            Method("SetElement", &Matrix4x4::SetElement)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetRow", &Matrix4x4::GetRow)->
            Method("GetRowAsVector3", &Matrix4x4::GetRowAsVector3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetColumnAsVector3", &Matrix4x4::GetColumnAsVector3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetRows", &Matrix4x4::GetRows)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4GetRowsMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void (Matrix4x4::*)(int, const Vector4&)>("SetRow", &Matrix4x4::SetRow)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4SetRowGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetRows", &Matrix4x4::SetRows)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetColumn", &Matrix4x4::GetColumn)->
            Method("GetColumns", &Matrix4x4::GetColumn)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4GetColumnsMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void (Matrix4x4::*)(int, const Vector4&)>("SetColumn", &Matrix4x4::SetColumn)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4SetColumnGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetColumns", &Matrix4x4::SetColumns)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetBasisAndPosition", &Matrix4x4::SetBasisAndPosition)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("TransposedMultiply3x3", &Matrix4x4::TransposedMultiply3x3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Multiply3x3", &Matrix4x4::Multiply3x3)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetPosition", &Matrix4x4::GetPosition)->
            Method<void (Matrix4x4::*)(const Vector3&)>("SetPosition", &Matrix4x4::SetPosition)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4SetPositionGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetTranslation", &Matrix4x4::GetTranslation)->
            Method<void (Matrix4x4::*)(const Vector3&)>("SetTranslation", &Matrix4x4::SetPosition)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix4x4SetPositionGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetTranspose", &Matrix4x4::GetTranspose)->
            Method("Transpose", &Matrix4x4::Transpose)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetInverseFull", &Matrix4x4::GetInverseFull)->
            Method("InvertFull", &Matrix4x4::InvertFull)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetInverseTransform", &Matrix4x4::GetInverseTransform)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("InvertTransform", &Matrix4x4::InvertTransform)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetInverseFast", &Matrix4x4::GetInverseFast)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("InvertFast", &Matrix4x4::InvertFast)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("RetrieveScale", &Matrix4x4::RetrieveScale)->
            Method("ExtractScale", &Matrix4x4::ExtractScale)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("MultiplyByScale", &Matrix4x4::MultiplyByScale)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsClose", &Matrix4x4::IsClose, context.MakeDefaultValues(g_simdTolerance))->
            Method("SetRotationPartFromQuaternion", &Matrix4x4::SetRotationPartFromQuaternion)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetDiagonal", &Matrix4x4::GetDiagonal)->
            Method("IsFinite", &Matrix4x4::IsFinite)->
            Method("CreateIdentity", &Matrix4x4::CreateIdentity)->
            Method("CreateZero", &Matrix4x4::CreateZero)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromValue", &Matrix4x4::CreateFromValue)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromRows", &Matrix4x4::CreateFromRows)->
            Method("CreateFromColumns", &Matrix4x4::CreateFromColumns)->
            Method("CreateRotationX", &Matrix4x4::CreateRotationX)->
            Method("CreateRotationY", &Matrix4x4::CreateRotationY)->
            Method("CreateRotationZ", &Matrix4x4::CreateRotationZ)->
            Method("CreateFromQuaternion", &Matrix4x4::CreateFromQuaternion)->
            Method("CreateFromQuaternionAndTranslation", &Matrix4x4::CreateFromQuaternionAndTranslation)->
            Method("CreateScale", &Matrix4x4::CreateScale)->
            Method("CreateDiagonal", &Matrix4x4::CreateDiagonal)->
            Method("CreateTranslation", &Matrix4x4::CreateTranslation)->
            Method("CreateProjection", &Matrix4x4::CreateProjection)->
            Method("CreateProjectionFov", &Matrix4x4::CreateProjectionFov)->
            Method("CreateProjectionOffset", &Matrix4x4::CreateProjectionOffset)->
            Method("ConstructFromValues", &ScriptCanvas::ConstructMatrix4x4)
            ;

        // Transform
            context.Class<Transform>()->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
                Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::TransformDefaultConstructor)->
                Property<const Vector3(Transform::*)() const, void (Transform::*)(const Vector3&)>("basisX", &Transform::GetBasisX, &Transform::SetBasisX)->
                Property<const Vector3(Transform::*)() const, void (Transform::*)(const Vector3&)>("basisY", &Transform::GetBasisY, &Transform::SetBasisY)->
                Property<const Vector3(Transform::*)() const, void (Transform::*)(const Vector3&)>("basisZ", &Transform::GetBasisZ, &Transform::SetBasisZ)->
                Property<const Vector3(Transform::*)() const, void (Transform::*)(const Vector3&)>("position", &Transform::GetPosition, &Transform::SetPosition)->
                Method("ToString", &Internal::TransformToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
                Method<const Vector3(Transform::*)(const Vector3&) const>("Multiply", &Transform::operator*)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformMultiplyGeneric)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
                Method<const Vector4(Transform::*)(const Vector4&) const>("MultiplyVector4", &Transform::operator*)->
                    Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<const Transform(Transform::*)(const Transform&) const>("MultiplyTransform", &Transform::operator*)->
                    Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Equal", &Transform::operator==)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Clone", [](const Transform& rhs) -> Transform { return rhs; })->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Transform::GetElement)->
                Method("SetElement", &Transform::SetElement)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetRow", &Transform::GetRow)->
                Method("GetRows", &Transform::GetRows)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformGetRowsMultipleReturn)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetRowAsVector3", &Transform::GetRowAsVector3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<void (Transform::*)(int, const Vector4&)>("SetRow", &Transform::SetRow)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformSetRowGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("SetRows", &Transform::SetRows)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetColumn", &Transform::GetColumn)->
                Method("GetColumns", &Transform::GetColumns)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformGetColumnsMultipleReturn)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<void (Transform::*)(int, const Vector3&)>("SetColumn", &Transform::SetColumn)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformSetColumnGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("SetColumns", &Transform::SetColumns)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetTranslation", &Transform::GetTranslation)->
                Method("SetBasisAndPosition", &Transform::SetBasisAndPosition)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetBasisAndPosition", &Transform::GetBasisAndPosition)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformGetBasisAndPositionMultipleReturn)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("TransposedMultiply3x3", &Transform::TransposedMultiply3x3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Multiply3x3", &Transform::Multiply3x3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetTranspose3x3", &Transform::GetTranspose3x3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Transpose3x3", &Transform::Transpose3x3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetPosition", &Transform::GetPosition)->
                Method<void (Transform::*)(const Vector3&)>("SetPosition", &Transform::SetPosition)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformSetPositionGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<void (Transform::*)(const Vector3&)>("SetTranslation", &Transform::SetPosition)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformSetPositionGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetTranspose", &Transform::GetTranspose)->
                Method("Transpose", &Transform::Transpose)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFull", &Transform::GetInverseFull)->
                Method("InvertFull", &Transform::InvertFull)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFast", &Transform::GetInverseFast)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("InvertFast", &Transform::InvertFast)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("RetrieveScale", &Transform::RetrieveScale)->
                Method("ExtractScale", &Transform::ExtractScaleExact)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("MultiplyByScale", &Transform::MultiplyByScale)->
                Method<const Transform(Transform::*)() const>("GetPolarDecomposition", &Transform::GetPolarDecomposition)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::TransformGetPolarDecompositionMultipleReturn)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsOrthogonal", &Transform::IsOrthogonal, context.MakeDefaultValues(g_simdTolerance))->
                Method("GetOrthogonalized", &Transform::GetOrthogonalized)->
                Method("Orthogonalize", &Transform::Orthogonalize)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsClose", &Transform::IsClose, context.MakeDefaultValues(g_simdTolerance))->
                Method("SetRotationPartFromQuaternion", &Transform::SetRotationPartFromQuaternion)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetDeterminant3x3", &Transform::GetDeterminant3x3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsFinite", &Transform::IsFinite)->
                Method("CreateIdentity", &Transform::CreateIdentity)->
                Method("CreateZero", &Transform::CreateZero)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromValue", &Transform::CreateFromValue)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromRows", &Transform::CreateFromRows)->
                Method("CreateFromColumns", &Transform::CreateFromColumns)->
                Method("CreateRotationX", &Transform::CreateRotationX)->
                Method("CreateRotationY", &Transform::CreateRotationY)->
                Method("CreateRotationZ", &Transform::CreateRotationZ)->
                Method("CreateFromQuaternion", &Transform::CreateFromQuaternion)->
                Method("CreateFromQuaternionAndTranslation", &Transform::CreateFromQuaternionAndTranslation)->
                Method("CreateFromMatrix3x3", &Transform::CreateFromMatrix3x3)->
                Method("CreateFromMatrix3x3AndTranslation", &Transform::CreateFromMatrix3x3AndTranslation)->
                Method("CreateScale", &Transform::CreateScale)->
                Method("CreateDiagonal", &Transform::CreateDiagonal)->
                Method("CreateTranslation", &Transform::CreateTranslation);

        // Uuid
        context.Class<Uuid>("Uuid")->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ScriptUuidConstructor)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::UuidDefaultConstructor)->
            Method("ToString", &Internal::UuidToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("LessThan", &Uuid::operator<)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<bool (Uuid::*)(const Uuid&) const>("Equal", &Uuid::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
            Method("Clone", [](const Uuid& rhs) -> Uuid { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsNull", &Uuid::IsNull)->
            Method("Create", &Uuid::Create)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateNull", &Uuid::CreateNull)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateString", &Uuid::CreateString)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::UuidCreateStringGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateName", &Uuid::CreateName)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateRandom", &Uuid::CreateRandom);

        // Random
        context.Class<SimpleLcgRandom>("Random")->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ScriptRandomConstructor)->
            Method("SetSeed", &SimpleLcgRandom::SetSeed, {{{ "Seed", "" }}})->
            Method("GetRandom", &SimpleLcgRandom::GetRandom)->
            Method("GetRandomFloat", &SimpleLcgRandom::GetRandomFloat);

        // Crc
        context.Class<Crc32>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ScriptCrc32Constructor)->
            Property("value", &Crc32::operator u32, nullptr)->
            Method("ToString", &Internal::Crc32ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("Equal", &Crc32::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Clone", [](const Crc32& rhs) -> Crc32 { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void(Crc32::*)(const char*)>("Add", &Crc32::Add)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Crc32AddGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Property("stringValue", nullptr, [](Crc32* thisPtr, AZStd::string_view value) { *thisPtr  = Crc32(value.data()); })->
            Method("CreateCrc32", [](AZStd::string_view value) -> Crc32 { return Crc32(value.data()); }, { { { "value", "String to compute to Crc32" } } })
            ;

        // Aabb
        context.Class<Aabb>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::AabbDefaultConstructor)->
            Property("min", &Aabb::GetMin, &Aabb::SetMin)->
            Property("max", &Aabb::GetMax, &Aabb::SetMax)->
            Method("ToString", &Internal::AabbToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("CreateNull", &Aabb::CreateNull)->
            Method("IsValid", &Aabb::IsValid)->
            Method("CreateFromPoint", &Aabb::CreateFromPoint)->
            Method("CreateFromMinMax", &Aabb::CreateFromMinMax)->
            Method("CreateCenterHalfExtents", &Aabb::CreateCenterHalfExtents)->
            Method("CreateCenterRadius", &Aabb::CreateCenterRadius)->
            Method("GetExtents", &Aabb::GetExtents)->
            Method("GetCenter", &Aabb::GetCenter)->
            Method("Set", &Aabb::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::AabbSetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromObb", &Aabb::CreateFromObb)->
            Method("GetWidth", &Aabb::GetWidth)->
            Method("GetHeight", &Aabb::GetHeight)->
            Method("GetDepth", &Aabb::GetDepth)->
            Method("GetAsSphere", &Aabb::GetAsSphere, nullptr, "() -> Vector3(center) and float(radius)")->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::AabbGetAsSphereMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<bool (Aabb::*)(const Aabb&) const>("Contains", &Aabb::Contains, nullptr, "const Vector3& or const Aabb&")->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::AabbContainsGeneric)->
            Method<bool (Aabb::*)(const Vector3&) const>("ContainsVector3", &Aabb::Contains, nullptr, "const Vector3&")->
                Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic contains above
            Method("Overlaps", &Aabb::Overlaps)->
            Method("Expand", &Aabb::Expand)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetExpanded", &Aabb::GetExpanded)->
            Method("AddPoint", &Aabb::AddPoint)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("AddAabb", &Aabb::AddAabb)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetDistance", &Aabb::GetDistance)->
            Method("GetClamped", &Aabb::GetClamped)->
            Method("Clamp", &Aabb::Clamp)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetNull", &Aabb::SetNull)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Translate", &Aabb::Translate)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetTranslated", &Aabb::GetTranslated)->
            Method("GetSurfaceArea", &Aabb::GetSurfaceArea)->
            Method("GetTransformedObb", &Aabb::GetTransformedObb)->
            Method("GetTransformedAabb", &Aabb::GetTransformedAabb)->
            Method("ApplyTransform", &Aabb::ApplyTransform)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Clone", [](const Aabb& rhs) -> Aabb { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsFinite", &Aabb::IsFinite)->
            Method("Equal", &Aabb::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ;

        // Obb
        context.Class<Obb>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::ObbDefaultConstructor)->
            Property("axisX", &Obb::GetAxisX, &Obb::SetAxisX)->
            Property("axisY", &Obb::GetAxisY, &Obb::SetAxisY)->
            Property("axisZ", &Obb::GetAxisZ, &Obb::SetAxisZ)->
            Property("halfLengthX", &Obb::GetHalfLengthX, &Obb::SetHalfLengthX)->
            Property("halfLengthY", &Obb::GetHalfLengthY, &Obb::SetHalfLengthY)->
            Property("halfLengthZ", &Obb::GetHalfLengthZ, &Obb::SetHalfLengthZ)->
            Property("position", &Obb::GetPosition, &Obb::SetPosition)->
            Method("ToString", &Internal::ObbToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("CreateFromPositionAndAxes", &Obb::CreateFromPositionAndAxes)->
            Method("CreateFromAabb", &Obb::CreateFromAabb)->
            Method("GetAxis", &Obb::GetAxis)->
            Method("SetAxis", &Obb::SetAxis)->
            Method("GetHalfLength", &Obb::GetHalfLength)->
            Method("SetHalfLength", &Obb::SetHalfLength)->
            Method("Clone", [](const Obb& rhs) -> Obb { return rhs; })->
            Method("IsFinite", &Obb::IsFinite)->
            Method("Equal", &Obb::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal);

        // Plane
        context.Class<Plane>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::PlaneDefaultConstructor)->
            Method("ToString", &Internal::PlaneToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("Clone", [](const Plane& rhs) -> Plane { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateFromCoefficients", &Plane::CreateFromCoefficients)->                
            Method("CreateFromNormalAndPoint", &Plane::CreateFromNormalAndPoint)->
            Method("CreateFromNormalAndDistance", &Plane::CreateFromNormalAndDistance)->
            Method<void(Plane::*)(const Vector4&)>("Set", &Plane::Set)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::PlaneSetGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetNormal", &Plane::SetNormal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("SetDistance", &Plane::SetDistance)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("ApplyTransform", &Plane::ApplyTransform)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("GetTransform", &Plane::GetTransform)->
            Method("GetPlaneEquationCoefficients", &Plane::GetPlaneEquationCoefficients)->
            Method("GetNormal", &Plane::GetNormal)->
            Method("GetDistance", &Plane::GetDistance)->
            Method("GetPointDist", &Plane::GetPointDist)->
            Method("GetProjected", &Plane::GetProjected)->
            Method<bool(Plane::*)(const Vector3&,const Vector3&,VectorFloat&) const>("CastRay", &Plane::CastRay)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::PlaneCastRayMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<bool(Plane::*)(const Vector3&,const Vector3&,VectorFloat&) const>("IntersectSegment", &Plane::IntersectSegment)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::PlaneIntersectSegmentMultipleReturn)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsFinite", &Plane::IsFinite);

        // Interpolation
        context.Class<InterpolationMode>()->
            Enum<(int)AZ::InterpolationMode::NoInterpolation>("NoInterpolation")->
            Enum<(int)AZ::InterpolationMode::LinearInterpolation>("LinearInterpolation");

        // Other math function
    }



    void MathReflect(ReflectContext* context)
    {
        if (context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                MathReflect(*serializeContext);
            }

            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                MathReflect(*behaviorContext);
            }
        }
    }
}

#endif // #ifndef AZ_UNITY_BUILD
