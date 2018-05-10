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

// change file hash again

#pragma once

#include "ITexture.h"
#include <RenderDLL/Common/Textures/PowerOf2BlockPacker.h>
#include <STLPoolAllocator.h>

namespace CloudsGem
{
    struct DynamicTexture;
    struct TextureSetFormat
    {
        TextureSetFormat(ETEX_Format eTF, uint32 nTexFlags) : m_nTexFlags(nTexFlags) {}
        ~TextureSetFormat();

        typedef AZStd::vector<CPowerOf2BlockPacker*> TexturePools;
        TexturePools m_TexPools;                ///< Texture pools
        DynamicTexture* m_pRoot {nullptr};      ///< Root
        ETEX_Type m_eTT { eTT_2D };             ///< Dynamic texture type
        int m_nTexFlags { 0};                   ///< Flags

    };

    struct DynamicTexture
        : public IDynTexture
    {
        typedef std::map<uint32, TextureSetFormat*> TextureSetMap;
        static TextureSetMap s_TexturePool;

        static uint32 s_nMemoryOccupied;
        static uint32 s_SuggestedDynTexAtlasCloudsMaxsize;
        static uint32 s_CurTexAtlasSize;

        DynamicTexture(uint32 nWidth, uint32 nHeight, uint32 nTexFlags, const char* szSource);
        DynamicTexture(const char* szSource);
        ~DynamicTexture() { Remove(); }

        void UnlinkGlobal();
        void LinkGlobal(DynamicTexture*& Before);
        void Link();
        void Unlink();
        bool Remove();
        bool UpdateAtlasSize(int newWidth, int newHeight);

        void ReleaseForce() { delete this; }

        // IDynTexture implementation
        virtual void Release() override { delete this; }
        virtual void GetSubImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight) override;
        virtual void GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight) override;
        virtual int GetTextureID() override { return m_pTexture ? m_pTexture->GetTextureID() : 0; }
        virtual void Lock() override { m_bLocked = true; }
        virtual void UnLock() override { m_bLocked = false; }
        virtual int GetWidth() { return m_nWidth; }
        virtual int GetHeight() { return m_nHeight; }
        virtual bool IsValid();
        virtual uint8 GetFlags() const override { return m_nFlags; }
        virtual void SetFlags(byte flags) override { m_nFlags = flags; }
        virtual bool Update(int nNewWidth, int nNewHeight) override;
        virtual void Apply(int nTUnit, int nTS = -1) override;
        virtual bool ClearRT() override;
        virtual bool SetRT(int nRT, bool bPush, SDepthTexture* pDepthSurf, bool bScreenVP = false) override;
        virtual bool SetRectStates() override;
        virtual bool RestoreRT(int nRT, bool bPop) override;
        virtual ITexture* GetTexture() override { return (ITexture*)m_pTexture; }
        virtual void SetUpdateMask() override;
        virtual void ResetUpdateMask() override;
        virtual bool IsSecondFrame() override { return m_nUpdateMask == 3; }

        // Lifecycle management
        static void Init();
        static void ShutDown();

        // Pool management
        static int GetPoolMaxSize() { return s_SuggestedDynTexAtlasCloudsMaxsize; }
        static void SetPoolMaxSize(int nSize, bool bWarn) { s_SuggestedDynTexAtlasCloudsMaxsize = nSize; }
        static const char* GetPoolName() { return "Clouds"; }

        AZStd::string m_name;                               ///< Name of texture
        TextureSetFormat* m_pOwner { nullptr };             ///< Texture set that maintains this texture
        CPowerOf2BlockPacker* m_pAllocator { nullptr };     ///< Power of 2 block allocator
        CTexture* m_pTexture { nullptr };                   ///< Target texture
        uint32 m_nBlockID { 0xFFFFFFFF };                   ///< Id of this block assigned by allocator
        DynamicTexture* m_Next { nullptr };                 ///< Doubly linked list next pointer
        DynamicTexture** m_PrevLink { nullptr };            ///< Doubly linked list previous pointer
        uint32 m_nX { 0 };                                  ///< x origin of texture
        uint32 m_nY { 0 };                                  ///< y origin of texture
        uint32 m_nWidth { 0 };                              ///< Width of texture
        uint32 m_nHeight { 0 };                             ///< Height of texture
        bool m_bLocked { false };                           ///< Indicates if the texture is currently locked
        byte m_nFlags { 0 };                                ///< Mask
        byte m_nUpdateMask { 0 };                           ///< Crossfire odd/even frames
        uint32 m_nFrameReset { 0 };                         ///< Frame in which texture has been reset
        uint32 m_nAccessFrame { 0 };                        ///< Frame in which texture was accessed
    };
}