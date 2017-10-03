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

// include required headers
#include "StandardHeaders.h"
#include "Array.h"
#include "Stream.h"
#include "File.h"


namespace MCore
{
    /**
     *
     *
     *
     */
    class MCORE_API HuffmanCompressor
    {
        MCORE_MEMORYOBJECTCATEGORY(HuffmanCompressor, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_HUFFMAN);

    public:
        HuffmanCompressor();
        ~HuffmanCompressor();

        // compress and decompress data
        void Compress(const uint8* inputData, uint32 inputNumBytes, uint8** outCompressedData, uint32* outCompressedNumBytes, bool processData = false);
        void Decompress(const uint8* compressedData, uint8* outDecompressedData, uint32* outDecompressedNumBytes);
        void FreeData(const uint8* data);
        bool CompressFile(const char* uncompressedFileName, const char* compressedFileName, uint32* outUncompressedNumBytes = nullptr, uint32* outCompressedNumBytes = nullptr, bool processData = true);
        bool DecompressFile(const char* compressedFileName, const char* uncompressedFileName, uint32* outCompressedNumBytes = nullptr, uint32* outUncompressedNumBytes = nullptr);

        // precalculate compressed or decompressed sizes
        uint32 CalcCompressedSize(const uint8* inputData, uint32 numBytes) const;
        uint32 CalcDecompressedSize(const uint8* compressedData, uint32 numBytes) const;

        // manual low-level control
        void BeginProcessData();
        void ProcessData(const uint8* inputData, uint32 numBytes);
        void ProcessFileData(MCore::File& file, bool autoCloseFile = true);
        bool ProcessFileData(const char* fileName);
        void EndProcessData();
        void InitFromStats(uint32* nodeStats, uint32 numNodes = 258);

        MCORE_INLINE uint32 GetNodeFrequency(uint32 nodeNumber) const       { return mTreeNodes[nodeNumber].mFrequency; }

        //
        bool Load(MCore::Stream& stream);
        bool Save(MCore::Stream& stream) const;

    private:
        enum
        {
            MARKER_FOURTIMES = 256, // output 4 times the most frequently occurring character
            MARKER_ENDOFDATA = 257  // end of compressed data
        };

        // compression map entry, to convert an uncompressed byte into compressed bits
        struct MCORE_API MapEntry
        {
            char    mCharacter;
            uint8   mNumBits;
            uint32  mBits;
        };

        // a node in the tree
        struct MCORE_API TreeNode
        {
            uint32  mFrequency;
            uint16  mLeftNode;
            uint16  mRightNode;
            uint16  mParent;
            uint16  mCharacter;
            bool    mIsLeaf;
        };

        MCore::Array<TreeNode>  mTreeNodes;
        MCore::Array<MapEntry>  mCompressMap;
        uint16                  mRootNode;
        uint16                  mPriorityNode;

        // methods
        void GatherBitInfo(uint16 treeNode, uint32& bitData, uint8& outNumBits);
        uint16 FindSmallestNode(uint16 skipNode) const;
        uint16 FindPriorityNode() const;
    };
}   // namespace MCore
