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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION) && defined(USE_D3DX)

#include <windows.h>
#include "CommonImageReader.h"
#include "d3d9types.h"
#include "comdef.h"
#include "ddraw.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "mmsystem.h"

#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"d3d9.lib ")

#pragma warning (disable : 4127)


const void* NImage::CCommonImageReader::ReadImage
(
    const char* cpImageName,
    uint32& rImageWidth,
    uint32& rImageHeight,
    EImageFileFormat& rFormat,
    const uint32 cBPP
) const
{
    static CSHAllocator<float> sAllocator;

    IDirect3D9Ptr pD3D(0);
    IDirect3DDevice9Ptr pD3DDev(0);
    IDirect3DTexture9Ptr pTex(0);

    HWND hWnd(0);
    // create dummy window (d3d expects a valid window handle upon creation)
    WNDCLASS mywnd;
    mywnd.cbClsExtra = 0;
    mywnd.cbWndExtra = 0;
    mywnd.hbrBackground = 0;
    mywnd.hCursor = 0;
    mywnd.hIcon = 0;
    mywnd.hInstance = (HINSTANCE)::GetModuleHandle(0);
    mywnd.lpfnWndProc = DefWindowProc;
    mywnd.lpszClassName = "Test";
    mywnd.lpszMenuName = 0;
    mywnd.style = 0;

    LPCTSTR strRegisterdWndClassName((LPCTSTR) RegisterClass(&mywnd));

    hWnd = CreateWindow(strRegisterdWndClassName, "Test", WS_POPUP, 0, 0, 100, 100, 0, 0, mywnd.hInstance, 0);
    // create d3d interface
    pD3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    if (0 == pD3D)
    {
        GetSHLog().LogError("Could not create Direct3D interface\n");
        return NULL;
    }
    // set the most basic present parameters (we don't need to set all )
    D3DPRESENT_PARAMETERS sPresentParams;
    ZeroMemory(&sPresentParams, sizeof(sPresentParams));
    sPresentParams.Windowed   = TRUE;
    sPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                D3DDEVTYPE_REF,
                hWnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                &sPresentParams,
                &pD3DDev)))
    {
        GetSHLog().LogError("Could not create Direct3D device\n");
        return NULL;
    }
    // load texture (no mip maps, no scaling to power of two, etc.)
    if (FAILED(D3DXCreateTextureFromFileEx(
                pD3DDev,
                cpImageName,
                D3DX_DEFAULT_NONPOW2,
                D3DX_DEFAULT_NONPOW2,
                1,
                0,
                D3DFMT_A32B32G32R32F,
                D3DPOOL_MANAGED,
                D3DX_DEFAULT,
                D3DX_DEFAULT,
                0,
                0,
                0,
                &pTex)))
    {
        char message[200];
        sprintf(message, "Could not load Texture: %s\n", cpImageName);
        GetSHLog().LogError(message);
        return NULL;
    }
    // get access to surface
    IDirect3DSurface9Ptr pSurface(0);
    if (FAILED(pTex->GetSurfaceLevel(0, &pSurface)))
    {
        char message[200];
        sprintf(message, "Could not access mip map level zero of Texture: %s\n", cpImageName);
        GetSHLog().LogError(message);
        return NULL;
    }
    // lock surface
    D3DLOCKED_RECT rect;
    if (FAILED(pSurface->LockRect(&rect, 0, 0)))
    {
        GetSHLog().LogError("Could not lock surface\n");
        return NULL;
    }
    // get surface description
    D3DSURFACE_DESC desc;
    if (FAILED(pSurface->GetDesc(&desc)))
    {
        GetSHLog().LogError("Could not get surface description\n");
        return NULL;
    }
    rImageWidth     = desc.Width;
    rImageHeight    = desc.Height;
    rFormat             = (cBPP == 4) ? A32B32G32R32F : B32G32R32F;

    // scan and copy surface, everything is float RGB
    float* pOutput = (float*)(sAllocator.new_mem_array(sizeof(float) * rImageWidth * rImageHeight * cBPP));
    assert(pOutput);

    float* pCurrentOutput = pOutput;

    if (cBPP == 4)
    {
        memcpy(pCurrentOutput, rect.pBits, rImageWidth * rImageHeight * cBPP * sizeof(float));
    }
    else
    {
        typedef struct ARGB_F32
        {
            float data[4];
        }ARGB_F32;

        for (unsigned int y = 0; y < desc.Height; ++y)
        {
            ARGB_F32* pData((ARGB_F32*)((size_t)rect.pBits + rect.Pitch * y));
            for (unsigned int x = 0; x < desc.Width; ++x, ++pData)
            {
                for (int i = 0; i < cBPP; ++i)
                {
                    pCurrentOutput[i] = pData->data[i];
                }
                pCurrentOutput += cBPP;
            }
        }
    }
    // unlock surface
    pSurface->UnlockRect();
    CloseWindow(hWnd);
    DestroyWindow(hWnd);
    UnregisterClass(strRegisterdWndClassName, mywnd.hInstance);
    return (void*)pOutput;
}

#pragma warning (default : 4127)

#endif