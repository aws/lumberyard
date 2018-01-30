// Copyright 2017 FragLab Ltd. All rights reserved.

#pragma once

#include <GridMate/GridMate.h>
#include <GridMate/Serialize/Buffer.h>
#include <Cry_Math.h>

namespace FragLab
{
    namespace Marshalers
    {
        template<int MinXY, int MinZ, int MaxXY, int MaxZ, AZStd::size_t BitsXY, AZStd::size_t BitsZ>
        class Vec3OptMarshalerImpl
        {
            static const AZStd::size_t MarshalSizeInBits = BitsXY * 2 + BitsZ;

            static_assert(MinXY < MaxXY, "Enter a Valid RangeXY");
            static_assert(MinZ < MaxZ, "Enter a Valid RangeZ");
            static_assert(MarshalSizeInBits <= (sizeof(AZ::u64) * CHAR_BIT), "Enter a Valid Num Bits");

        public:
            void Marshal(GridMate::WriteBuffer& wb, const AZ::Vector3& vec) const
            {
                static const float xyRatio = ((1U << BitsXY) - 1U) / static_cast<float>(MaxXY - MinXY);
                static const float zRatio = ((1U << BitsZ) - 1U) / static_cast<float>(MaxZ - MinZ);

                AZ::u64 x = static_cast<AZ::u64>((vec.GetX() - static_cast<float>(MinXY)) * xyRatio + 0.5f);
                AZ::u64 y = static_cast<AZ::u64>((vec.GetY() - static_cast<float>(MinXY)) * xyRatio + 0.5f);
                AZ::u64 z = static_cast<AZ::u64>((vec.GetZ() - static_cast<float>(MinZ)) * zRatio + 0.5f);

                AZ::u64 res = x | (y << BitsXY) | (z << (2ULL * BitsXY));

                wb.WriteRaw(&res, GridMate::PackedSize(0, MarshalSizeInBits));
            }
            void Unmarshal(AZ::Vector3& vec, GridMate::ReadBuffer& rb) const
            {
                static const float xyInvRatio = static_cast<float>(MaxXY - MinXY) / ((1U << BitsXY) - 1U);
                static const float zInvRatio = static_cast<float>(MaxZ - MinZ) / ((1U << BitsZ) - 1U);

                AZ::u64 res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, MarshalSizeInBits));

                AZ::u64 mask = (1ULL << BitsXY) - 1ULL;
                AZ::u64 ux = res & mask;

                mask <<= BitsXY;
                AZ::u64 uy = (res & mask) >> BitsXY;

                mask = ((1ULL << BitsZ) - 1ULL) << (2ULL * BitsXY);
                AZ::u64 uz = (res & mask) >> (2ULL * BitsXY);

                vec.Set(ux * xyInvRatio + MinXY, uy * xyInvRatio + MinXY, uz * zInvRatio + MinZ);
            }
        };

        template<AZStd::size_t YawBits, AZStd::size_t ElevBits>
        class Vec3NormOptMarshalerImpl
        {
            static const AZStd::size_t MarshalSizeInBits = YawBits + ElevBits;
            static_assert(MarshalSizeInBits <= (sizeof(AZ::u64) * CHAR_BIT), "Enter a Valid Num Bits");

        public:
            void Marshal(GridMate::WriteBuffer& wb, const AZ::Vector3& dir) const
            {
                // guarantee maximum precision at all Pi / 2 angles:
                // 5 points at interval [-Pi - Pi]
                static const float yawRatio = (((1U << YawBits) - 1U) >> 2 << 2) / (AZ::Constants::Pi * 2.0f);
                static const float elevRatio = (((1U << ElevBits) - 1U) >> 2 << 2) / (AZ::Constants::Pi * 2.0f);

                float yaw = 0.0f;
                float elev = 0.0f;
                float xy = dir.GetX() * dir.GetX() + dir.GetY() * dir.GetY();
                if (xy > AZ_FLT_EPSILON)
                {
                    yaw = atan2_tpl(dir.GetY(), dir.GetX());
                    elev = asin_tpl(AZ::GetClamp(static_cast<float>(dir.GetZ()), -1.0f, 1.0f));
                }
                else
                {
                    elev = AZ::GetSign(dir.GetZ()) * AZ::Constants::HalfPi;
                }

                AZ::u64 uYaw = static_cast<AZ::u64>((yaw + AZ::Constants::Pi) * yawRatio + 0.5f);
                AZ::u64 uElev = static_cast<AZ::u64>((elev + AZ::Constants::Pi) * elevRatio + 0.5f);

                AZ::u64 res = uYaw | (uElev << YawBits);

                wb.WriteRaw(&res, GridMate::PackedSize(0, MarshalSizeInBits));
            }
            void Unmarshal(AZ::Vector3& dir, GridMate::ReadBuffer& rb) const
            {
                static const float yawInvRatio = (AZ::Constants::Pi * 2.0f) / (((1U << YawBits) - 1U) >> 2 << 2);
                static const float elevInvRatio = (AZ::Constants::Pi * 2.0f) / (((1U << ElevBits) - 1U) >> 2 << 2);

                AZ::u64 res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, MarshalSizeInBits));

                AZ::u64 mask = (1ULL << YawBits) - 1ULL;
                AZ::u64 uYaw = res & mask;

                mask = ((1ULL << ElevBits) - 1ULL) << YawBits;
                AZ::u64 uElev = (res & mask) >> YawBits;

                float sinYaw = 0.0f;
                float cosYaw = 0.0f;
                float sinElev = 0.0f;
                float cosElev = 0.0f;
                sincos_tpl(uYaw * yawInvRatio - AZ::Constants::Pi, &sinYaw, &cosYaw);
                sincos_tpl(uElev * elevInvRatio - AZ::Constants::Pi, &sinElev, &cosElev);

                dir.Set(cosYaw * cosElev, sinYaw * cosElev, sinElev);
            }
        };

        template<AZStd::size_t BitsX, AZStd::size_t BitsY, AZStd::size_t BitsZ>
        class QuatOptMarshalerImpl
        {
            static_assert((BitsX + BitsY + BitsZ + 3U) <= (sizeof(AZ::u64) * CHAR_BIT), "Enter a Valid Num Bits");

            enum EFlags
            {
                All = 0,
                OnlyX = 1,
                OnlyY = 2,
                OnlyZ = 3
            };

        public:
            void Marshal(GridMate::WriteBuffer& wb, const AZ::Quaternion& quat) const
            {
                // Range [-1; 1]
                // guarantee maximum precision at 3 points: -1; 0; 1
                static const float xRatio = (((1U << BitsX) - 1U) >> 1 << 1) / 2.0f;
                static const float yRatio = (((1U << BitsY) - 1U) >> 1 << 1) / 2.0f;
                static const float zRatio = (((1U << BitsZ) - 1U) >> 1 << 1) / 2.0f;

                // res = isnegW (1 bit) + flags (2 bits) + [x (BitsX bits)] + [y (BitsY bits)] + [z (BitsZ bits)]
                AZ::u64 res = 0;
                GridMate::PackedSize size(0, 3);
                uint8 flags = 0;
                if (quat.GetX().GetAbs() < AZ_FLT_EPSILON)
                {
                    if (quat.GetY().GetAbs() < AZ_FLT_EPSILON)
                    {
                        flags = OnlyZ;
                        AZ::u64 z = static_cast<AZ::u64>((quat.GetZ() + 1.0f) * zRatio + 0.5f);
                        res = (flags << 1) | (z << 3);
                        size.IncrementBits(BitsZ);
                    }
                    else if (quat.GetZ().GetAbs() < AZ_FLT_EPSILON)
                    {
                        flags = OnlyY;
                        AZ::u64 y = static_cast<AZ::u64>((quat.GetY() + 1.0f) * yRatio + 0.5f);
                        res = (flags << 1) | (y << 3);
                        size.IncrementBits(BitsY);
                    }
                }
                else if (quat.GetY().GetAbs() < AZ_FLT_EPSILON && quat.GetZ().GetAbs() < AZ_FLT_EPSILON)
                {
                    flags = OnlyX;
                    AZ::u64 x = static_cast<AZ::u64>((quat.GetX() + 1.0f) * xRatio + 0.5f);
                    res = (flags << 1) | (x << 3);
                    size.IncrementBits(BitsX);
                }

                if (!flags)
                {
                    AZ::u64 x = static_cast<AZ::u64>((quat.GetX() + 1.0f) * xRatio + 0.5f);
                    AZ::u64 y = static_cast<AZ::u64>((quat.GetY() + 1.0f) * yRatio + 0.5f);
                    AZ::u64 z = static_cast<AZ::u64>((quat.GetZ() + 1.0f) * zRatio + 0.5f);
                    res = (x << 3) | (y << (BitsX + 3)) | (z << (BitsX + BitsY + 3));
                    size.IncrementBits(BitsX + BitsY + BitsZ);
                }

                res |= isneg(quat.GetW());

                wb.WriteRaw(&res, size);
            }
            void Unmarshal(AZ::Quaternion& quat, GridMate::ReadBuffer& rb) const
            {
                // Range [-1; 1]
                static const float xInvRatio = 2.0f / (((1U << BitsX) - 1U) >> 1 << 1);
                static const float yInvRatio = 2.0f / (((1U << BitsY) - 1U) >> 1 << 1);
                static const float zInvRatio = 2.0f / (((1U << BitsZ) - 1U) >> 1 << 1);

                AZ::u64 res = 0;
                rb.ReadRawBits(&res, GridMate::PackedSize(0, 3));

                GridMate::PackedSize size;
                AZ::u64 mask = 1; // (1ULL << 1ULL) - 1ULL;
                AZ::u64 isnegW = res & mask;

                mask = 6; // ((1ULL << 2ULL) - 1ULL) << 1ULL;
                AZ::u64 flags = (res & mask) >> 1ULL;
                switch (flags)
                {
                case OnlyX:
                    size.IncrementBits(BitsX);
                    break;
                case OnlyY:
                    size.IncrementBits(BitsY);
                    break;
                case OnlyZ:
                    size.IncrementBits(BitsZ);
                    break;
                default:
                    size.IncrementBits(BitsX + BitsY + BitsZ);
                    break;
                }

                res = 0;
                rb.ReadRawBits(&res, size);

                switch (flags)
                {
                case OnlyX:
                {
                    float x = res * xInvRatio - 1.0f;
                    float w = sqrtf(AZ::GetMax(0.0f, 1.0f - x * x));
                    quat = AZ::Quaternion(x, 0, 0, isnegW ? -w : w);
                }
                break;
                case OnlyY:
                {
                    float y = res * yInvRatio - 1.0f;
                    float w = sqrtf(AZ::GetMax(0.0f, 1.0f - y * y));
                    quat = AZ::Quaternion(0, y, 0, isnegW ? -w : w);
                }
                break;
                case OnlyZ:
                {
                    float z = res * zInvRatio - 1.0f;
                    float w = sqrtf(AZ::GetMax(0.0f, 1.0f - z * z));
                    quat = AZ::Quaternion(0, 0, z, isnegW ? -w : w);
                }
                break;
                default:
                {
                    mask = (1ULL << BitsX) - 1ULL;
                    AZ::u64 ux = res & mask;
                    float x = ux * xInvRatio - 1.0f;

                    mask = ((1ULL << BitsY) - 1ULL) << BitsX;
                    AZ::u64 uy = (res & mask) >> BitsX;
                    float y = uy * yInvRatio - 1.0f;

                    mask = ((1ULL << BitsZ) - 1ULL) << (BitsX + BitsY);
                    AZ::u64 uz = (res & mask) >> (BitsX + BitsY);
                    float z = uz * zInvRatio - 1.0f;

                    float w = sqrtf(AZ::GetMax(0.0f, 1.0f - x * x - y * y - z * z));
                    quat = AZ::Quaternion(x, y, z, isnegW ? -w : w);
                }
                break;
                }
            }
        };

        using Vec3OptMarshaler = Vec3OptMarshalerImpl<0, 0, 4096, 100, 25, 14>;
        using Vec3NormOptMarshaler = Vec3NormOptMarshalerImpl<20, 20>;
        using QuatOptMarshaler = QuatOptMarshalerImpl<20, 20, 21>;

        //Marshaler with position and rotation only
        class CryMatrixMarshalerNoScale
        {
        public:

            void Marshal(GridMate::WriteBuffer& wb, const Matrix34& value) const
            {
                const Vec3& pos = value.GetTranslation();
                const Quat& ori = Quat(value);

                wb.Write(pos.x);
                wb.Write(pos.y);
                wb.Write(pos.z);
                wb.Write(ori.v.x);
                wb.Write(ori.v.y);
                wb.Write(ori.v.z);
                wb.Write(ori.w);
            }

            void Unmarshal(Matrix34& value, GridMate::ReadBuffer& rb) const
            {
                Vec3 pos;
                Quat ori;
                rb.Read(pos.x);
                rb.Read(pos.y);
                rb.Read(pos.z);
                rb.Read(ori.v.x);
                rb.Read(ori.v.y);
                rb.Read(ori.v.z);
                rb.Read(ori.w);
                value.Set(Vec3(1.0f), ori, pos);
            }
        };
    } // namespace Marshalers
} // namespace FragLab
