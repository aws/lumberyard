// Copyright 2017 FragLab Ltd. All rights reserved.

#include "StdAfx.h"
#include "MarshalerTest.h"
#include <Marshalers/FloatMarshaler.h>
#include <Marshalers/IntMarshaler.h>
#include <Marshalers/OptimizedMarshal.h>
#include <Marshalers/ContainerMarshaler.h>
#include <Marshalers/StringMarshal.h>

#include <Utils/SummaryInfo.h>
#include <Utils/Trace.h>

#include <GridMate/Serialize/CompressionMarshal.h>
#include <GridMate/Serialize/ContainerMarshal.h>
#include <AzCore/std/containers/map.h>

namespace FragLab
{
    namespace Marshalers
    {
        namespace Test
        {
            template<typename TMarshaler>
            void TestMarshaler(const char* szMarshalerName, float xyMin, float xyMax, float zMin, float zMax, int numSteps, bool bNormalize = false, bool bTestAngles = false)
            {
                float stepXY = AZ::GetMax((xyMax - xyMin) / numSteps, AZ_FLT_EPSILON);
                float stepZ = AZ::GetMax((zMax - zMin) / numSteps, AZ_FLT_EPSILON);

                GridMate::WriteBufferStatic<1024> wb(GridMate::EndianType::IgnoreEndian);
                TMarshaler marshaler;
                SSummaryInfo summaryX;
                SSummaryInfo summaryY;
                SSummaryInfo summaryZ;
                SSummaryInfo summarySize;
                AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
                for (float x = xyMin; x < xyMax; x += stepXY)
                {
                    for (float y = xyMin; y < xyMax; y += stepXY)
                    {
                        for (float z = zMin; z < zMax; z += stepZ)
                        {
                            AZ::Vector3 testVec(x, y, z);
                            if (bNormalize)
                            {
                                if (testVec.GetLengthSq() < AZ_FLT_EPSILON)
                                {
                                    continue;
                                }
                                testVec.NormalizeExact();
                            }
                            marshaler.Marshal(wb, testVec);
                            GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                            AZ::Vector3 testVec1;
                            marshaler.Unmarshal(testVec1, rb);
                            if (bTestAngles)
                            {
                                Ang3 angDiff(ZERO);
                                if (testVec.GetX() * testVec.GetX() + testVec.GetY() * testVec.GetY() > AZ_FLT_EPSILON && testVec1.GetX() * testVec1.GetX() + testVec1.GetY() * testVec1.GetY() > AZ_FLT_EPSILON)
                                {
                                    angDiff = Ang3(Quat::CreateRotationVDir(Vec3(testVec.GetX(), testVec.GetY(), testVec.GetZ())).GetNormalized() * !Quat::CreateRotationVDir(Vec3(testVec1.GetX(), testVec1.GetY(), testVec1.GetZ())).GetNormalized());
                                }
                                else
                                {
                                    angDiff.x = (AZ::GetSign(testVec.GetZ()) - AZ::GetSign(testVec1.GetZ())) * AZ::Constants::Pi;
                                }
                                summaryX.Add(AZ::RadToDeg(abs(angDiff.x)));
                                summaryY.Add(AZ::RadToDeg(abs(angDiff.y)));
                                summaryZ.Add(AZ::RadToDeg(abs(angDiff.z)));
                            }
                            else
                            {
                                summaryX.Add(abs(testVec1.GetX() - testVec.GetX()));
                                summaryY.Add(abs(testVec1.GetY() - testVec.GetY()));
                                summaryZ.Add(abs(testVec1.GetZ() - testVec.GetZ()));
                            }
                            summarySize.Add(static_cast<float>(wb.Size()));

                            wb.Clear();
                        }
                    }
                }

                const AZ::u64 dt = (AZStd::chrono::system_clock::now() - startTime).count();
                GLogUnformatted("| %-36s | %12.7f {%2.0f - %2.0f} | %8f {%8f} | %8f {%8f} | %8f {%8f} | %9u | %13.3f | %9.3f |", szMarshalerName, summarySize.avg, summarySize.min, summarySize.max,
                                summaryX.avg, summaryX.max, summaryY.avg, summaryY.max, summaryZ.avg, summaryZ.max, summaryX.count, dt / 1000000.0, static_cast<double>(dt) / summaryX.count);
            }

            template<typename TMarshaler>
            void TestRotationMarshaler(const char* szMarshalerName, float xmin, float xmax, float ymin, float ymax, float zmin, float zmax, int numSteps)
            {
                float stepX = AZ::GetMax((xmax - xmin) / numSteps, AZ_FLT_EPSILON);
                float stepY = AZ::GetMax((ymax - ymin) / numSteps, AZ_FLT_EPSILON);
                float stepZ = AZ::GetMax((zmax - zmin) / numSteps, AZ_FLT_EPSILON);

                GridMate::WriteBufferStatic<1024> wb(GridMate::EndianType::IgnoreEndian);
                TMarshaler marshaler;
                SSummaryInfo summaryX;
                SSummaryInfo summaryY;
                SSummaryInfo summaryZ;
                SSummaryInfo summarySize;
                AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
                for (float x = xmin; x <= xmax; x += stepX)
                {
                    for (float y = ymin; y <= ymax; y += stepY)
                    {
                        for (float z = zmin; z <= zmax; z += stepZ)
                        {
                            AZ::Quaternion testQuat(AZ::Quaternion::CreateRotationX(x) * AZ::Quaternion::CreateRotationY(y) * AZ::Quaternion::CreateRotationZ(z));
                            testQuat.NormalizeExact();
                            marshaler.Marshal(wb, testQuat);
                            GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                            AZ::Quaternion testQuat1;
                            marshaler.Unmarshal(testQuat1, rb);

                            Ang3 angDiff(Quat(testQuat.GetW(), testQuat.GetX(), testQuat.GetY(), testQuat.GetZ()).GetNormalized() * !Quat(testQuat1.GetW(), testQuat1.GetX(), testQuat1.GetY(), testQuat1.GetZ()).GetNormalized());
                            summaryX.Add(AZ::RadToDeg(abs(angDiff.x)));
                            summaryY.Add(AZ::RadToDeg(abs(angDiff.y)));
                            summaryZ.Add(AZ::RadToDeg(abs(angDiff.z)));
                            summarySize.Add(static_cast<float>(wb.Size()));
                            wb.Clear();
                        }
                    }
                }

                const AZ::u64 dt = (AZStd::chrono::system_clock::now() - startTime).count();
                GLogUnformatted("| %-36s | %12.7f {%2.0f - %2.0f} | %8f {%8f} | %8f {%8f} | %8f {%8f} | %9u | %13.3f | %9.3f |", szMarshalerName, summarySize.avg, summarySize.min, summarySize.max,
                                summaryX.avg, summaryX.max, summaryY.avg, summaryY.max, summaryZ.avg, summaryZ.max, summaryX.count, dt / 1000000.0, static_cast<double>(dt) / summaryX.count);
            }

            template<typename TMarshaler>
            void TestStringMarshaler(const char* szMarshalerName, int count)
            {
                GridMate::WriteBufferStatic<1024> wb(GridMate::EndianType::IgnoreEndian);
                TMarshaler marshaler;
                SSummaryInfo marshalTime;
                SSummaryInfo unmarshalTime;
                SSummaryInfo summarySize;
                AZStd::string testStr = "TEST0000";
                AZStd::string testStr1;
                AZStd::chrono::system_clock::time_point startMarshalTime;
                AZStd::chrono::system_clock::time_point startUnmarshalTime;
                AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
                for (int i = 0; i < count; ++i)
                {
                    testStr[4] = i / 1000 % 10 + '0';
                    testStr[5] = i / 100 % 10 + '0';
                    testStr[6] = i / 10 % 10 + '0';
                    testStr[7] = i % 10 + '0';

                    startMarshalTime = AZStd::chrono::system_clock::now();
                    marshaler.Marshal(wb, testStr);
                    marshalTime.Add(static_cast<float>((AZStd::chrono::system_clock::now() - startMarshalTime).count()));
                    GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                    startUnmarshalTime = AZStd::chrono::system_clock::now();
                    marshaler.Unmarshal(testStr1, rb);
                    unmarshalTime.Add(static_cast<float>((AZStd::chrono::system_clock::now() - startUnmarshalTime).count()));
                    summarySize.Add(static_cast<float>(wb.Size()));
                    wb.Clear();
                }

                const AZ::u64 dt = (AZStd::chrono::system_clock::now() - startTime).count();
                GLogUnformatted("| %-36s | %12.7f {%2.0f - %2.0f} | %17.7f {%3.0f - %3.0f} | %19.7f {%3.0f - %3.0f} | %9u | %13.3f | %9.3f |", szMarshalerName, summarySize.avg, summarySize.min, summarySize.max,
                                marshalTime.avg, marshalTime.min, marshalTime.max, unmarshalTime.avg, unmarshalTime.min, unmarshalTime.max, summarySize.count, dt / 1000000.0, static_cast<double>(dt) / summarySize.count);
            }

            void MarshalPosition()
            {
                const int numSteps = 200;
                GLogUnformatted("================== Vector3 Position {0 - 4096; 0 - 4096; 0 - 100} =========================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-19s | %-19s | %-19s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "dx, m: avg { max  }", "dy, m: avg { max  }", "dz, m: avg { max  }", "count", "total time, s", "time, mcs");
                TestMarshaler<Vec3OptMarshalerImpl<0, 0, 4096, 100, 12, 8>>("Vec3PosMarshaler_x12_y12_z8", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<Vec3OptMarshalerImpl<0, 0, 4096, 100, 11, 10>>("Vec3PosMarshaler_x11_y11_z10", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<Vec3OptMarshalerImpl<0, 0, 4096, 100, 15, 10>>("Vec3PosMarshaler_x15_y15_z10", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<Vec3OptMarshalerImpl<0, 0, 4096, 100, 18, 12>>("Vec3PosMarshaler_x18_y18_z12", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<Vec3OptMarshalerImpl<0, 0, 4096, 100, 21, 14>>("Vec3PosMarshaler_x21_y21_z14", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<Vec3OptMarshaler>("Vec3OptMarshaler", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<Vec3OptMarshalerImpl<0, 0, 4096, 100, 24, 16>>("Vec3PosMarshaler_x24_y24_z16", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<GridMate::Vec3CompMarshaler>("GridMate::Vec3CompMarshaler(half)", 0, 4096.0f, 0, 100, numSteps);
                TestMarshaler<GridMate::Marshaler<AZ::Vector3>>("GridMate::Marshaler<AZ::Vector3>", 0, 4096.0f, 0, 100, numSteps);
                GLogUnformatted("===========================================================================================================================================================================");
            }

            void MarshalDirection()
            {
                const int numSteps = 100;
                GLogUnformatted("================== Vector3 Direction Normalized ===========================================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-19s | %-19s | %-19s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "dx, m: avg { max  }", "dy, m: avg { max  }", "dz, m: avg { max  }", "count", "total time, s", "time, mcs");
                TestMarshaler<Vec3NormOptMarshalerImpl<12, 12>>("Vec3NormOptMarshaler_12_12", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true);
                TestMarshaler<Vec3NormOptMarshalerImpl<16, 16>>("Vec3NormOptMarshaler_16_16", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true);
                TestMarshaler<Vec3NormOptMarshalerImpl<24, 24>>("Vec3NormOptMarshaler_24_24", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true);
                TestMarshaler<Vec3NormOptMarshaler>("Vec3NormOptMarshaler", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true);
                TestMarshaler<GridMate::Vec3CompNormMarshaler>("GridMate::Vec3CompNormMarshaler(f16)", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true);
                TestMarshaler<GridMate::Marshaler<AZ::Vector3>>("GridMate::Marshaler<AZ::Vector3>", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true);
                GLogUnformatted("================== Vector3 Direction Normalized ===========================================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-19s | %-19s | %-19s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "dx, deg: avg {max} ", "dy, deg: avg {max} ", "dz, deg: avg {max} ", "count", "total time, s", "time, mcs");
                TestMarshaler<Vec3NormOptMarshalerImpl<12, 12>>("Vec3NormOptMarshaler_12_12", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true, true);
                TestMarshaler<Vec3NormOptMarshalerImpl<16, 16>>("Vec3NormOptMarshaler_16_16", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true, true);
                TestMarshaler<Vec3NormOptMarshalerImpl<24, 24>>("Vec3NormOptMarshaler_24_24", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true, true);
                TestMarshaler<Vec3NormOptMarshaler>("Vec3NormOptMarshaler", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true, true);
                TestMarshaler<GridMate::Vec3CompNormMarshaler>("GridMate::Vec3CompNormMarshaler(f16)", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true, true);
                TestMarshaler<GridMate::Marshaler<AZ::Vector3>>("GridMate::Marshaler<AZ::Vector3>", -1.0f, 1.0f, -1.0f, 1.0f, numSteps, true, true);
                GLogUnformatted("===========================================================================================================================================================================");
            }

            void MarshalRotation()
            {
                int numSteps = 1000000;
                GLogUnformatted("================== Z Rotation =============================================================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-19s | %-19s | %-19s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "dx, deg: avg {max} ", "dy, deg: avg {max} ", "dz, deg: avg {max} ", "count", "total time, s", "time, mcs");
                TestRotationMarshaler<QuatOptMarshalerImpl<12, 12, 12>>("QuatOptMarshalerImpl_x12_y12_z12", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<15, 15, 15>>("QuatOptMarshalerImpl_x15_y15_z15", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<16, 16, 17>>("QuatOptMarshalerImpl_x16_y16_z17", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<18, 17, 18>>("QuatOptMarshalerImpl_x18_y17_z18", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<19, 19, 19>>("QuatOptMarshalerImpl_x19_y19_z19", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshaler>("QuatOptMarshaler", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::QuatCompNormMarshaler>("GridMate::QuatCompNormMarshaler", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::QuatCompMarshaler>("GridMate::QuatCompMarshaler", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::Marshaler<AZ::Quaternion>>("GridMate::Marshaler<AZ::Quaternion>", 0, 0, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);

                numSteps = 1000;
                GLogUnformatted("================== X + Z Rotation =========================================================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-19s | %-19s | %-19s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "dx, deg: avg {max} ", "dy, deg: avg {max} ", "dz, deg: avg {max} ", "count", "total time, s", "time, mcs");
                TestRotationMarshaler<QuatOptMarshalerImpl<12, 12, 12>>("QuatOptMarshalerImpl_x12_y12_z12", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<15, 15, 15>>("QuatOptMarshalerImpl_x15_y15_z15", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<16, 16, 17>>("QuatOptMarshalerImpl_x16_y16_z17", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<18, 17, 18>>("QuatOptMarshalerImpl_x18_y17_z18", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<19, 19, 19>>("QuatOptMarshalerImpl_x19_y19_z19", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshaler>("QuatOptMarshaler", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::QuatCompNormMarshaler>("GridMate::QuatCompNormMarshaler", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::QuatCompMarshaler>("GridMate::QuatCompMarshaler", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::Marshaler<AZ::Quaternion>>("GridMate::Marshaler<AZ::Quaternion>", -AZ::Constants::Pi, AZ::Constants::Pi, 0, 0, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);

                numSteps = 100;
                GLogUnformatted("================== X + Y + Z Rotation =====================================================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-19s | %-19s | %-19s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "dx, deg: avg {max} ", "dy, deg: avg {max} ", "dz, deg: avg {max} ", "count", "total time, s", "time, mcs");
                TestRotationMarshaler<QuatOptMarshalerImpl<12, 12, 12>>("QuatOptMarshalerImpl_x12_y12_z12", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<15, 15, 15>>("QuatOptMarshalerImpl_x15_y15_z15", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<16, 16, 17>>("QuatOptMarshalerImpl_x16_y16_z17", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<18, 17, 18>>("QuatOptMarshalerImpl_x18_y17_z18", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshalerImpl<19, 19, 19>>("QuatOptMarshalerImpl_x19_y19_z19", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<QuatOptMarshaler>("QuatOptMarshaler", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::QuatCompNormMarshaler>("GridMate::QuatCompNormMarshaler", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::QuatCompMarshaler>("GridMate::QuatCompMarshaler", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                TestRotationMarshaler<GridMate::Marshaler<AZ::Quaternion>>("GridMate::Marshaler<AZ::Quaternion>", -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, -AZ::Constants::Pi, AZ::Constants::Pi, numSteps);
                GLogUnformatted("===========================================================================================================================================================================");
            }

            void MarshalString()
            {
                GLogUnformatted("================== String ================================================================================================================================================");
                GLogUnformatted("| %-36s | %-22s | %-29s | %-31s | %-9s | %-13s | %-9s |", "marshalerName", "size, B: avg {min-max}", "Marshal, mcs: avg {min - max}", "Unmarshal, mcs: avg {min - max}", "count", "total time, s", "time, mcs");
                TestStringMarshaler<StringMarshaler>("StringMarshaler", 1000000);
                TestStringMarshaler<GridMate::Marshaler<AZStd::string>>("GridMate::Marshaler<AZStd::string>", 1000000);
                GLogUnformatted("==========================================================================================================================================================================");
            }

            void MarshalInt()
            {
                GridMate::WriteBufferStatic<1024> wb(GridMate::EndianType::IgnoreEndian);
                IntMarshaler<int, -10, 10> marshaler;
                GLogUnformatted("[IntMarshaler] interval: [-10, 10] %zu bits", marshaler.GetNumBits());
                for (int i = -10; i <= 10; i += 5)
                {
                    marshaler.Marshal(wb, i);
                }

                GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                int res;
                for (int i = -10; i <= 10; i += 5)
                {
                    marshaler.Unmarshal(res, rb);
                    GLogUnformatted("[IntMarshaler] %d => %d (%s)", i, res, i == res ? "ok" : "failed");
                }
                GLogUnformatted("===========================================================================================================================");
            }

            void MarshalEnum()
            {
                enum ETest
                {
                    Test0,
                    Test1,
                    Test2,
                    Test3,
                    Count
                };

                GridMate::WriteBufferStatic<1024> wb(GridMate::EndianType::IgnoreEndian);
                EnumMarshaler<ETest> marshaler;
                GLogUnformatted("[EnumMarshaler] interval: [0, %d) %zu bits", static_cast<int>(ETest::Count), marshaler.GetNumBits());
                for (int i = 0; i < static_cast<int>(ETest::Count); ++i)
                {
                    marshaler.Marshal(wb, static_cast<ETest>(i));
                }

                GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                ETest res;
                for (int i = 0; i < static_cast<int>(ETest::Count); ++i)
                {
                    marshaler.Unmarshal(res, rb);
                    GLogUnformatted("[EnumMarshaler] %d => %d (%s)", i, static_cast<int>(res), i == static_cast<int>(res) ? "ok" : "failed");
                }
                GLogUnformatted("===========================================================================================================================");

                enum class ETestClass
                {
                    Test0,
                    Test1,
                    Count
                };

                GridMate::WriteBufferStatic<1024> wb2(GridMate::EndianType::IgnoreEndian);
                EnumMarshaler<ETestClass> marshaler2;
                GLogUnformatted("[EnumClassMarshaler] interval: [0, %d) %zu bits", static_cast<int>(ETestClass::Count), marshaler2.GetNumBits());
                marshaler2.Marshal(wb2, ETestClass::Test0);
                marshaler2.Marshal(wb2, ETestClass::Test1);

                GridMate::ReadBuffer rb2(GridMate::EndianType::IgnoreEndian, wb2.Get(), wb2.Size());
                ETestClass res2;
                for (int i = 0; i < static_cast<int>(ETestClass::Count); ++i)
                {
                    marshaler2.Unmarshal(res2, rb2);
                    GLogUnformatted("[EnumClassMarshaler] %d => %d (%s)", i, static_cast<int>(res2), i == static_cast<int>(res2) ? "ok" : "failed");
                }
                GLogUnformatted("===========================================================================================================================");
            }

            void MarshalFloat()
            {
                GridMate::WriteBufferStatic<1024> wb(GridMate::EndianType::IgnoreEndian);
                FloatMarshaler<7> marshaler(-1.0f, 1.0f);
                float precision = marshaler.GetPrecision();
                GLogUnformatted("[FloatMarshaler] interval: [-1.0, 1.0] 7 bits with precision(%f) = 0.5 * (maxValue - minValue) / ((1 << numBits) - 1)", precision);
                for (float i = -1.0f; i <= 1.0f; i += 0.5f)
                {
                    marshaler.Marshal(wb, i);
                }

                GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                float res;
                for (float i = -1.0f; i <= 1.0f; i += 0.5f)
                {
                    marshaler.Unmarshal(res, rb);
                    GLogUnformatted("[FloatMarshaler] %f => %f delta: %f (%s)", i, res, res - i, fabs_tpl(res - i) <= precision ? "ok" : "failed");
                }
                GLogUnformatted("===========================================================================================================================");
            }

            template<typename TMarshaler, typename TContainer>
            void ContainerTest(const char* marshalerName, const TContainer& testContainer, AZStd::size_t numTests)
            {
                TMarshaler marshaler;
                GridMate::WriteBufferDynamic wb(GridMate::EndianType::IgnoreEndian);
                bool bTestFailed = false;

                SSummaryInfo marshalTime;
                SSummaryInfo unmarshalTime;
                SSummaryInfo summarySize;
                AZStd::chrono::system_clock::time_point startMarshalTime;
                AZStd::chrono::system_clock::time_point startUnmarshalTime;
                AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();

                for (AZStd::size_t i = 0; i < numTests; ++i)
                {
                    AZStd::chrono::system_clock::time_point startMarshalTime;
                    startMarshalTime = AZStd::chrono::system_clock::now();
                    marshaler.Marshal(wb, testContainer);
                    marshalTime.Add(static_cast<float>((AZStd::chrono::system_clock::now() - startMarshalTime).count()));

                    TContainer res;
                    GridMate::ReadBuffer rb(GridMate::EndianType::IgnoreEndian, wb.Get(), wb.Size());
                    startUnmarshalTime = AZStd::chrono::system_clock::now();
                    marshaler.Unmarshal(res, rb);
                    unmarshalTime.Add(static_cast<float>((AZStd::chrono::system_clock::now() - startUnmarshalTime).count()));
                    summarySize.Add(static_cast<float>(wb.GetExactSize().GetTotalSizeInBits()));
                    wb.Clear();

                    if (res.size() != testContainer.size())
                    {
                        bTestFailed = true;
                    }
                }

                const AZ::u64 dt = (AZStd::chrono::system_clock::now() - startTime).count();
                GLogUnformatted("| %-32s | %11.3f {%4.0f - %4.0f} | %17.7f {%3.0f - %3.0f} | %19.7f {%3.0f - %3.0f} | %9u | %13.3f | %9.3f |", marshalerName, summarySize.avg, summarySize.min, summarySize.max,
                                marshalTime.avg, marshalTime.min, marshalTime.max, unmarshalTime.avg, unmarshalTime.min, unmarshalTime.max, summarySize.count, dt / 1000000.0, static_cast<double>(dt) / summarySize.count);

                if (bTestFailed)
                {
                    GWarning("%s failed to read size: %zu", marshalerName, testContainer.size());
                }
            }

            void MarshalContainer()
            {
                const AZStd::size_t numTests = 100000;
                for (AZStd::size_t size = 0; size <= 16; size += 8)
                {
                    AZStd::vector<int> testVector(size, 123456789);
                    AZStd::map<int, int> testMap;
                    for (AZStd::size_t i = 0; i < size; ++i)
                    {
                        testMap.insert(AZStd::make_pair(i, 123456789));
                    }

                    GLogUnformatted("================== Container (elements: %zu) =============================================================================================================================", size);
                    GLogUnformatted("| %-32s | %-22s | %-29s | %-31s | %-9s | %-13s | %-9s |", "marshalerName", "size, Bits: avg {min-max}", "Marshal, mcs: avg {min - max}", "Unmarshal, mcs: avg {min - max}", "count", "total time, s", "time, mcs");
                    ContainerTest<ContainerMarshaler<AZStd::vector<int>, 6>>("ContainerMarshaler<6>", testVector, numTests);
                    ContainerTest<GridMate::Marshaler<AZStd::vector<int>>>("GridMate::Marshaler<vector<int>>", testVector, numTests);
                    GLogUnformatted("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
                    ContainerTest<ContainerMarshaler<AZStd::vector<int>, 7>>("ContainerMarshaler<7>", testVector, numTests);
                    ContainerTest<GridMate::Marshaler<AZStd::vector<int>>>("GridMate::Marshaler<vector<int>>", testVector, numTests);
                    GLogUnformatted("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
                    ContainerTest<MapMarshaler<AZStd::map<int, int>, 16>>("MapMarshaler<16>", testMap, numTests);
                    ContainerTest<GridMate::MapContainerMarshaler<AZStd::map<int, int>>>("GridMate::MapContainerMarshaler", testMap, numTests);
                }
                GLogUnformatted("=========================================================================================================================================================================");
            }

            void MarshalersTest(const char* params)
            {
                if (params && *params)
                {
                    size_t numCommands = strlen(params);
                    for (size_t i = 0; i < numCommands; ++i)
                    {
                        switch (params[i])
                        {
                        case 'c':
                            MarshalContainer();
                            break;
                        case 'd':
                            MarshalDirection();
                            break;
                        case 'e':
                            MarshalEnum();
                            break;
                        case 'f':
                            MarshalFloat();
                            break;
                        case 'i':
                            MarshalInt();
                            break;
                        case 'p':
                            MarshalPosition();
                            break;
                        case 'r':
                            MarshalRotation();
                            break;
                        case 's':
                            MarshalString();
                            break;
                        default:
                            break;
                        }
                    }
                }
                else
                {
                    MarshalPosition();
                    MarshalDirection();
                    MarshalRotation();
                    MarshalString();
                    MarshalInt();
                    MarshalEnum();
                    MarshalFloat();
                    MarshalContainer();
                }
            }
        }
    } // namespace Marshalers
}     // namespace FragLab
