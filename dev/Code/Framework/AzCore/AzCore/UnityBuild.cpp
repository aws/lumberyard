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
/**
 * Unity Build module! Used to generate smaller lib sizes (when an SDK is released)
 * in addition we get a speed up (no PCH needed on any platform). The build machine will define
 * AZ_UNITY_BUILD when needed. If you want to test it just define it on the top and compile this file.
 */
#ifdef AZ_UNITY_BUILD

#undef AZ_UNITY_BUILD // allow cpps to compile
#include "std/string/string.cpp" ///<- must be first string include on MSVC to handle properly explicit specialization!
#include "std/string/regex.cpp"
#include "std/string/alphanum.cpp"
#include "std/allocator.cpp"
#include "std/hash.cpp"
#include "std/time.cpp"


#if AZ_TRAIT_OS_USE_WINDOWS_THREADS
#   include "std/parallel/internal/thread_win.cpp"
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#   include "std/parallel/internal/thread_linux.cpp"
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(UnityBuild_cpp, AZ_RESTRICTED_PLATFORM)
#endif

#include "Debug/Trace.cpp"
#include "Debug/TraceMessagesDriller.cpp"
#include "Debug/Profiler.cpp"
#include "Debug/ProfilerDriller.cpp"
#include "Debug/FrameProfilerComponent.cpp"
#include "Debug/StackTracer.cpp"

#include "IO/Device.cpp"

#if !defined(AZCORE_EXCLUDE_ZLIB)
#include "IO/Compressor.cpp"
#include "IO/CompressorZLib.cpp"
#else
#pragma message("Compression is excluded from build.")
#endif // #if !defined(AZCORE_EXCLUDE_ZLIB)

#include "IO/Streamer.cpp"
#include "IO/StreamerLayoutHelper.cpp"
#include "IO/StreamerDriller.cpp"
#include "IO/SystemFile.cpp"
#include "IO/GenericStreams.cpp"
#include "IO/StreamerComponent.cpp"
#include "IO/FileIO.cpp"

#include "Math/Aabb.cpp"
#include "Math/Crc.cpp"
#include "Math/IntersectSegment.cpp"
#include "Math/Matrix3x3.cpp"
#include "Math/Matrix4x4.cpp"
#include "Math/Obb.cpp"
#include "Math/Quaternion.cpp"
#include "Math/Transform.cpp"
#include "Math/Vector2.cpp"
#include "Math/Vector3.cpp"
#include "Math/VectorFloat.cpp"
#include "Math/Sfmt.cpp"
#include "Math/Uuid.cpp"
#include "Math/UuidToDebugString.cpp"
#include "Math/MathReflection.cpp"
#include "Math/Random.cpp"

#include "State/HSM.cpp"

#if !defined(AZCORE_EXCLUDE_ZLIB)
#include "Compression/Compression.cpp"
#endif // #if !defined(AZCORE_EXCLUDE_ZLIB)

#include "Memory/AllocationRecords.cpp"
#include "Memory/AllocatorBase.cpp"
#include "Memory/AllocatorManager.cpp"
#include "Memory/BestFitExternalMapAllocator.cpp"
#include "Memory/BestFitExternalMapSchema.cpp"
#include "Memory/OSAllocator.cpp"
//#include "Memory/HeapSchema.cpp" deprecated
#include "Memory/HphaSchema.cpp"
#include "Memory/Memory.cpp"
#include "Memory/PoolAllocator.cpp"
#include "Memory/PoolSchema.cpp"
#include "Memory/SystemAllocator.cpp"
#include "Memory/MemoryComponent.cpp"
#include "Memory/MemoryDriller.cpp"

#include "Module/Environment.cpp"
#include "Module/Module.cpp"
#include "Module/ModuleWindows.cpp"

#include "Component/ComponentApplication.cpp"
#include "Component/Component.cpp"
#include "Component/ComponentBus.cpp"
#include "Component/Entity.cpp"
#include "Component/EntityUtils.cpp"
#include "Component/TransformComponent.cpp"
#include "UserSettings/UserSettingsComponent.cpp"

#include "Jobs/JobContext.cpp"
#include "Jobs/JobManager.cpp"
#include "Jobs/Internal/JobManagerBase.cpp"
#include "Jobs/Internal/JobManagerDefault.cpp"
#include "Jobs/Internal/JobManagerSynchronous.cpp"
#include "Jobs/Internal/JobManagerWorkStealing.cpp"
#include "Jobs/JobManagerComponent.cpp"

#include "Driller/Driller.cpp"
#include "Driller/DrillerBus.cpp"
#include "Driller/Stream.cpp"

#include "IPC/DirectSocket.cpp"
#include "IPC/SharedMemory.cpp"

#if !defined(AZCORE_EXCLUDE_LUA)
#include "Script/ScriptContext.cpp"
#include "Script/ScriptContextDebug.cpp"
#include "Script/ScriptComponent.cpp"
#include "Script/ScriptSystemComponent.cpp"
#include "Script/ScriptAsset.cpp"
#else
#pragma message("LUA script support is excluded from build.")
#endif // #if !defined(AZCORE_EXCLUDE_LUA)

#include "Serialization/SerializeContext.cpp"
#include "Serialization/EditContext.cpp"
#include "Serialization/ObjectStream.cpp"
#include "Serialization/ObjectStreamComponent.cpp"
#include "Serialization/DataOverlayProviderMsgs.cpp"
#include "Serialization/DynamicSerializableField.cpp"
#include "Serialization/DataPatch.cpp"
#include "Serialization/Utils.cpp"

#include "Asset/AssetCommon.cpp"
#include "Asset/AssetDatabase.cpp"
#include "Asset/AssetDatabaseComponent.cpp"
#include "Asset/AssetSerializer.cpp"

//extern "C" {
//#include "compression/minilzo/minilzo.c"
//#include "compression/quicklz/quicklz.c"
//#include "compression/zlib/adler32.c"
//#include "compression/zlib/compress.c"
//#include "compression/zlib/crc32.c"
//#include "compression/zlib/deflate.c"
//#include "compression/zlib/infback.c"
//#include "compression/zlib/inffast.c"
//#include "compression/zlib/inflate.c"
//#include "compression/zlib/inftrees.c"
//#include "compression/zlib/trees.c"
//#include "compression/zlib/uncompr.c"
//#include "compression/zlib/zutil.c"
//}

#define AZ_UNITY_BUILD

#endif // AZ_UNITY_BUILD




