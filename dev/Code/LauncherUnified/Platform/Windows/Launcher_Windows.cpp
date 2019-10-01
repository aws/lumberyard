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
#include <Launcher_precompiled.h>
#include <Launcher.h>

#include <CryLibrary.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShellAPI.h>

// We need shell api for Current Root Extraction.
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#if defined(DEDICATED_SERVER)
    static unsigned key[4] = {1339822019, 3471820962, 4179589276, 4119647811};
    static unsigned text[4] = {4114048726, 1217549643, 1454516917, 859556405};
    static unsigned hash[4] = {324609294, 3710280652, 1292597317, 513556273};

    // src and trg can be the same pointer (in place encryption)
    // len must be in bytes and must be multiple of 8 byts (64bits).
    // key is 128bit:  int key[4] = {n1,n2,n3,n4};
    // void encipher(unsigned int *const v,unsigned int *const w,const unsigned int *const k )
    #define TEA_ENCODE(src, trg, len, key) {                                                                                              \
            unsigned int* v = (src), * w = (trg), * k = (key), nlen = (len) >> 3;                                                         \
            unsigned int delta = 0x9E3779B9, a = k[0], b = k[1], c = k[2], d = k[3];                                                      \
            while (nlen--) {                                                                                                              \
                unsigned int y = v[0], z = v[1], n = 32, sum = 0;                                                                         \
                while (n-- > 0) { sum += delta; y += (z << 4) + a ^ z + sum ^ (z >> 5) + b; z += (y << 4) + c ^ y + sum ^ (y >> 5) + d; } \
                w[0] = y; w[1] = z; v += 2, w += 2; }                                                                                     \
    }

    // src and trg can be the same pointer (in place decryption)
    // len must be in bytes and must be multiple of 8 byts (64bits).
    // key is 128bit: int key[4] = {n1,n2,n3,n4};
    // void decipher(unsigned int *const v,unsigned int *const w,const unsigned int *const k)
    #define TEA_DECODE(src, trg, len, key) {                                                                                              \
            unsigned int* v = (src), * w = (trg), * k = (key), nlen = (len) >> 3;                                                         \
            unsigned int delta = 0x9E3779B9, a = k[0], b = k[1], c = k[2], d = k[3];                                                      \
            while (nlen--) {                                                                                                              \
                unsigned int y = v[0], z = v[1], sum = 0xC6EF3720, n = 32;                                                                \
                while (n-- > 0) { z -= (y << 4) + c ^ y + sum ^ (y >> 5) + d; y -= (z << 4) + a ^ z + sum ^ (z >> 5) + b; sum -= delta; } \
                w[0] = y; w[1] = z; v += 2, w += 2; }                                                                                     \
    }

    ILINE unsigned Hash(unsigned a)
    {
        a = (a + 0x7ed55d16) + (a << 12);
        a = (a ^ 0xc761c23c) ^ (a >> 19);
        a = (a + 0x165667b1) + (a << 5);
        a = (a + 0xd3a2646c) ^ (a << 9);
        a = (a + 0xfd7046c5) + (a << 3);
        a = (a ^ 0xb55a4f09) ^ (a >> 16);
        return a;
    }

    // encode size ignore last 3 bits of size in bytes. (encode by 8bytes min)
    #define TEA_GETSIZE(len) ((len) & (~7))
#else
    //Due to some laptops not auto-switching to the discrete GPU correctly we are adding these
    //__declspec as defined in the AMD and NVidia white papers to 'force on' the use of the
    //discrete chips.  This will be overridden by users setting application profiles
    //and may not work on older drivers or bios. In theory this should be enough to always force on
    //the discrete chips.

    //http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
    //https://community.amd.com/thread/169965

    // It is unclear if this is also needed for Linux or macOS at this time (22/02/2017)
    extern "C"
    {
        __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
        __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    }
#endif // defined(DEDICATED_SERVER)

// this extern is necessary in order to keep the include of platform_impl.h in the common Launcher source file instead
// of the platform specific versions.
extern void InitRootDir(char szExeFileName[] = 0, uint nExeSize = 0, char szExeRootName[] = 0, uint nRootSize = 0);


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    InitRootDir();

    using namespace LumberyardLauncher;

    PlatformMainInfo mainInfo;
    mainInfo.m_instance = GetModuleHandle(0);

    // The check for the presence of -load in the command line
    // is to maintain the same behavior as the legacy launcher.
    const bool commandLineContainsLoad = (strstr(lpCmdLine, " -load ") != nullptr);
    bool ret = commandLineContainsLoad ?
               mainInfo.CopyCommandLine(lpCmdLine) :
               mainInfo.CopyCommandLine(GetCommandLineA());

#if defined(DEDICATED_SERVER)
    unsigned buf[4];
    TEA_DECODE((unsigned*)text, buf, 16, (unsigned*)key);

    for (int i = 0; i < 4; i++)
    {
        if (Hash(buf[i]) != hash[i])
        {
            return static_cast<int>(ReturnCode::ErrValidation);
        }
    }

    ret |= mainInfo.AddArgument(reinterpret_cast<char*>(buf));
#endif

    // Prevent allocator from growing in small chunks
    // Pre-create our system allocator and configure it to ask for larger chunks from the OS
    // Creating this here to be consistent with other platforms
    AZ::SystemAllocator::Descriptor sysHeapDesc;
    sysHeapDesc.m_heap.m_systemChunkSize = 64 * 1024 * 1024;
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create(sysHeapDesc);

    ReturnCode status = ret ? 
        Run(mainInfo) : 
        ReturnCode::ErrCommandLine;

#if !defined(_RELEASE)
    bool noPrompt = (strstr(mainInfo.m_commandLine, "-noprompt") != nullptr);
#else
    bool noPrompt = false;
#endif // !defined(_RELEASE)

    if (!noPrompt && status != ReturnCode::Success)
    {
        MessageBoxA(0, GetReturnCodeString(status), "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
    }

#if !defined(AZ_MONOLITHIC_BUILD)
    if (ret)
    {
        // HACK HACK HACK - is this still needed?!?!
        // CrySystem module can get loaded multiple times (even from within CrySystem itself)
        // and currently there is no way to track them (\ref _CryMemoryManagerPoolHelper::Init() in CryMemoryManager_impl.h)
        // so we will release it as many times as it takes until it actually unloads.
        void* hModule = CryLoadLibraryDefName("CrySystem");
        if (hModule)
        {
            // loop until we fail (aka unload the DLL)
            while (CryFreeLibrary(hModule))
            {
                ;
            }
        }
    }
#endif // !defined(AZ_MONOLITHIC_BUILD)

    // there is no way to transfer ownership of the allocator to the component application
    // without altering the app descriptor, so it must be destroyed here
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

    return static_cast<int>(status);
}
