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

// Description : NULL font functions.


#include "StdAfx.h"
#include "NULL_Renderer.h"
#include "Common/Textures/TextureManager.h"

#include "../CryFont/FBitmap.h"

bool CNULLRenderer::FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte* pData)
{
    return true;
}

bool CNULLRenderer::FontUploadTexture(class CFBitmap* pBmp, ETEX_Format eTF)
{
    return true;
}
void CNULLRenderer::FontReleaseTexture(class CFBitmap* pBmp)
{
}

void CNULLRenderer::FontSetTexture(class CFBitmap* pBmp, int nTexFiltMode)
{
}

void CNULLRenderer::FontSetTexture(int nTexId, int nFilterMode)
{
}

int CNULLRenderer::FontCreateTexture(int Width, int Height, byte* pData, ETEX_Format eTF, bool genMips, const char* textureName)
{
    return CTextureManager::Instance()->GetNoTexture()->GetTextureID();
}

void CNULLRenderer::DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType)
{
}

void CNULLRenderer::DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices)
{
}