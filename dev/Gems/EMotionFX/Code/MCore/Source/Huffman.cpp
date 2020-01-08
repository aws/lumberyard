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

// include required headers
#include "Huffman.h"
#include "LogManager.h"
#include "DiskFile.h"
#include "Endian.h"


namespace MCore
{
    // constructor
    HuffmanCompressor::HuffmanCompressor()
    {
        mRootNode = MCORE_INVALIDINDEX16;
    }


    // destructor
    HuffmanCompressor::~HuffmanCompressor()
    {
    }


    // reset the character statistics
    void HuffmanCompressor::BeginProcessData()
    {
        mTreeNodes.SetMemoryCategory(MCORE_MEMCATEGORY_HUFFMAN);
        mCompressMap.SetMemoryCategory(MCORE_MEMCATEGORY_HUFFMAN);
        mTreeNodes.Reserve(515);
        mTreeNodes.Resize(258);

        // reset the stats
        for (uint32 i = 0; i < 258; ++i)
        {
            mTreeNodes[i].mCharacter = static_cast<uint16>(i);
            mTreeNodes[i].mFrequency = 0;
            mTreeNodes[i].mLeftNode  = MCORE_INVALIDINDEX16;
            mTreeNodes[i].mRightNode = MCORE_INVALIDINDEX16;
            mTreeNodes[i].mParent    = MCORE_INVALIDINDEX16;
            mTreeNodes[i].mIsLeaf    = true;
        }

        mPriorityNode = MCORE_INVALIDINDEX16;
        mRootNode = MCORE_INVALIDINDEX16;

        // end of data marker
        mTreeNodes[MARKER_ENDOFDATA].mFrequency = 1;
    }



    // process the character statistics info
    void HuffmanCompressor::ProcessData(const uint8* inputData, uint32 numBytes)
    {
        uint32 maxFreq = 0;
        uint16 maxNode = MCORE_INVALIDINDEX16;

        // process all input data bytes
        for (uint32 i = 0; i < numBytes; ++i)
        {
            const uint32 character = inputData[i];
            mTreeNodes[character].mFrequency++;

            if (mTreeNodes[character].mFrequency > maxFreq)
            {
                maxNode = static_cast<uint16>(character);
                maxFreq = mTreeNodes[character].mFrequency;
            }
        }

        mPriorityNode = maxNode;

        for (uint32 i = 0; i < numBytes; ++i)
        {
            const uint32 character = inputData[i];
            if ((character == mPriorityNode) && (i + 3 < numBytes))
            {
                if (inputData[i + 1] == character && inputData[i + 2] == character && inputData[i + 3] == character)
                {
                    mTreeNodes[mPriorityNode].mFrequency -= 4;
                    mTreeNodes[MARKER_FOURTIMES].mFrequency += 1;
                    i += 3;
                }
            }
        }


        //---------------
        /*
            for (uint32 i=0; i<258; ++i)
            {
                MCore::LogDetailedInfo("%d = %d", i, mTreeNodes[i].mFrequency);
            }
        */
    }


    // build the huffman tree
    void HuffmanCompressor::EndProcessData()
    {
        uint16 leftNode = 0;
        uint16 rightNode = MCORE_INVALIDINDEX16;

        mRootNode = MCORE_INVALIDINDEX16;

        // while there are nodes left
        while (leftNode != MCORE_INVALIDINDEX16)
        {
            // find the smallest number
            leftNode = FindSmallestNode(MCORE_INVALIDINDEX16);
            if (leftNode == MCORE_INVALIDINDEX16)
            {
                break;
            }

            // find the next smallest node
            rightNode = FindSmallestNode(leftNode);
            if (rightNode == MCORE_INVALIDINDEX16)
            {
                break;
            }

            // merge them into a new node
            TreeNode newNode;
            newNode.mCharacter      = 0;
            newNode.mFrequency      = mTreeNodes[leftNode].mFrequency + mTreeNodes[rightNode].mFrequency;
            newNode.mParent         = MCORE_INVALIDINDEX16;
            newNode.mLeftNode       = leftNode;
            newNode.mRightNode      = rightNode;
            newNode.mIsLeaf         = false;
            mTreeNodes[leftNode ].mParent = static_cast<uint16>(mTreeNodes.GetLength());
            mTreeNodes[rightNode].mParent = static_cast<uint16>(mTreeNodes.GetLength());
            mRootNode = static_cast<uint16>(mTreeNodes.GetLength());
            mTreeNodes.Add(newNode);
        }

        // build the compression map for fast compression
        uint32 maxNumBits = 0;
        mCompressMap.Resize(258);
        for (uint32 i = 0; i < 256; ++i)
        {
            mCompressMap[i].mCharacter = (char)i;
            GatherBitInfo(static_cast<uint16>(i), mCompressMap[i].mBits, mCompressMap[i].mNumBits);
            if (mCompressMap[i].mNumBits > maxNumBits)
            {
                maxNumBits = mCompressMap[i].mNumBits;
            }
        }

        mCompressMap[MARKER_FOURTIMES].mCharacter = static_cast<char>(mPriorityNode);
        GatherBitInfo(MARKER_FOURTIMES, mCompressMap[MARKER_FOURTIMES].mBits, mCompressMap[MARKER_FOURTIMES].mNumBits);

        mCompressMap[MARKER_ENDOFDATA].mCharacter = 0;
        GatherBitInfo(MARKER_ENDOFDATA, mCompressMap[MARKER_ENDOFDATA].mBits, mCompressMap[MARKER_ENDOFDATA].mNumBits);

        //  MCore::LogDetailedInfo("Number of nodes = %d (root=%d) (maxbits=%d)", mTreeNodes.GetLength(), mRootNode, maxNumBits);
    }


    // extract the bit info for a given leaf node (the bit code)
    void HuffmanCompressor::GatherBitInfo(uint16 treeNode, uint32& bitData, uint8& outNumBits)
    {
        bitData = 0;
        outNumBits = 0;

        uint16 curNode = treeNode;
        uint16 parent = mTreeNodes[curNode].mParent;
        while (parent != MCORE_INVALIDINDEX16)
        {
            if (mTreeNodes[parent].mRightNode == curNode)
            {
                bitData |= (1 << outNumBits);
            }

            // 0000000000001101 <-- stored code
            // 1101 <-- path to take for the stored code

            outNumBits++;
            curNode = parent;
            parent = mTreeNodes[curNode].mParent;
        }

        MCORE_ASSERT(outNumBits <= 32);

        //if (treeNode == 256)
        //      MCore::LogDetailedInfo("%d = %d bits (freq=%d)", treeNode, outNumBits, mTreeNodes[treeNode].mFrequency);
    }


    // calculate the required output size in bytes
    uint32 HuffmanCompressor::CalcCompressedSize(const uint8* inputData, uint32 numBytes) const
    {
        uint32 totalBits = 0;
        for (uint32 i = 0; i < numBytes; ++i)
        {
            uint32 character = inputData[i];

            if ((character == mPriorityNode) && (i + 4 < numBytes))
            {
                if (inputData[i + 1] == character && inputData[i + 2] == character && inputData[i + 3] == character)
                {
                    totalBits += mCompressMap[MARKER_FOURTIMES].mNumBits;
                    i += 3;
                }
                else
                {
                    totalBits += mCompressMap[character].mNumBits;
                }
            }
            else
            {
                totalBits += mCompressMap[character].mNumBits;
            }
        }

        // add the EOD marker
        totalBits += mCompressMap[MARKER_ENDOFDATA].mNumBits;

        if (totalBits % 8 == 0)
        {
            return totalBits / 8;
        }
        else
        {
            return (totalBits / 8) + 1;
        }
    }


    // find the two smallest nodes
    uint16 HuffmanCompressor::FindSmallestNode(uint16 skipNode) const
    {
        uint16 minNode = MCORE_INVALIDINDEX16;
        uint32 minFreq = MCORE_INVALIDINDEX32;

        const uint32 numNodes = mTreeNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (i == skipNode)
            {
                continue;
            }

            // if it has a lower count and it has no parent yet
            if (mTreeNodes[i].mFrequency < minFreq && mTreeNodes[i].mParent == MCORE_INVALIDINDEX16)
            {
                minFreq = mTreeNodes[i].mFrequency;
                minNode = static_cast<uint16>(i);
            }
        }

        return minNode;
    }


    // compress data
    void HuffmanCompressor::Compress(const uint8* inputData, uint32 inputNumBytes, uint8** outCompressedData, uint32* outCompressedNumBytes, bool processData)
    {
        MCORE_ASSERT(inputData);
        MCORE_ASSERT(outCompressedData);

        // build the character statistics info and the huffman tree
        if (processData)
        {
            BeginProcessData();
            ProcessData(inputData, inputNumBytes);
            EndProcessData();
        }

        // allocate space needed for the compressed data
        *outCompressedNumBytes = CalcCompressedSize(inputData, inputNumBytes);
        *outCompressedData = (uint8*)MCore::Allocate(*outCompressedNumBytes);

        // for all bytes in the input data
        uint32 compressedOffset = 0;
        uint8 compressedByte = 0;
        uint32 bitNumber = 0;
        for (uint32 i = 0; i < inputNumBytes; ++i)
        {
            // get the input byte value
            uint32 byteValue = inputData[i];

            // check if this is a special case
            if ((byteValue == mPriorityNode) && (i + 3 < inputNumBytes))
            {
                if (inputData[i + 1] == byteValue && inputData[i + 2] == byteValue && inputData[i + 3] == byteValue)
                {
                    i += 3;
                    byteValue = 256;
                }
            }

            // write the bits to the file
            const uint32 numBits = mCompressMap[byteValue].mNumBits;
            for (uint32 b = 0; b < numBits; ++b)
            {
                // if its a 1 in the sequence, write it to the compressed byte at the right location
                if ((mCompressMap[byteValue].mBits & (1 << (numBits - b - 1))) != 0)
                {
                    compressedByte |= 1 << bitNumber;
                }

                // if this was the 8th bit, we can write the byte to the compressed data
                bitNumber++;
                if (bitNumber == 8)
                {
                    (*outCompressedData)[compressedOffset++] = compressedByte;
                    compressedByte  = 0;
                    bitNumber       = 0;
                }
            }
        }

        // write end of data marker
        const uint32 byteValue = MARKER_ENDOFDATA;
        const uint32 numBits = mCompressMap[MARKER_ENDOFDATA].mNumBits;
        for (uint32 b = 0; b < numBits; ++b)
        {
            // if its a 1 in the sequence, write it to the compressed byte at the right location
            if ((mCompressMap[byteValue].mBits & (1 << (numBits - b - 1))) != 0)
            {
                compressedByte |= 1 << bitNumber;
            }

            // if this was the 8th bit, we can write the byte to the compressed data
            bitNumber++;
            if (bitNumber == 8)
            {
                (*outCompressedData)[compressedOffset++] = compressedByte;
                compressedByte  = 0;
                bitNumber       = 0;
            }
        }

        // write the remaining data
        if (bitNumber != 0)
        {
            (*outCompressedData)[compressedOffset++] = compressedByte;
        }

        //  MCore::LogDetailedInfo("Num compressed bytes = %d / %d (estcomp=%d)", compressedOffset, inputNumBytes, *outCompressedNumBytes);
    }


    // decompress data
    void HuffmanCompressor::Decompress(const uint8* compressedData, uint8* outDecompressedData, uint32* outDecompressedNumBytes)
    {
        uint32 compressedOffset     = 0;
        uint32 decompressedOffset   = 0;
        uint32 bitNumber            = 0;
        uint32 treeNode             = mRootNode;

        // while not reached 'end of data' marker
        bool done = false;
        uint32 priorityCharacter = mTreeNodes[mPriorityNode].mCharacter;
        uint32 compressedByte = compressedData[compressedOffset++];
        while (done == false)
        {
            // get the bit value
            const uint32 bitValue = compressedByte & (1 << bitNumber);

            // traverse the huffman tree
            if (bitValue != 0) // take the right child
            {
                treeNode = mTreeNodes[treeNode].mRightNode;
            }
            else // take the left child
            {
                treeNode = mTreeNodes[treeNode].mLeftNode;
            }

            // get a shortcut to the node
            //const TreeNode& node = mTreeNodes[treeNode];

            // write the decompressed value
            if (mTreeNodes[treeNode].mIsLeaf)
            {
                if (treeNode != MARKER_FOURTIMES)
                {
                    if (treeNode == MARKER_ENDOFDATA)
                    {
                        break;
                    }

                    outDecompressedData[decompressedOffset++] = static_cast<uint8>(mTreeNodes[treeNode].mCharacter);
                }
                else
                {
                    outDecompressedData[decompressedOffset++] = static_cast<uint8>(priorityCharacter);
                    outDecompressedData[decompressedOffset++] = static_cast<uint8>(priorityCharacter);
                    outDecompressedData[decompressedOffset++] = static_cast<uint8>(priorityCharacter);
                    outDecompressedData[decompressedOffset++] = static_cast<uint8>(priorityCharacter);
                }

                treeNode = mRootNode;
            }

            // get the next byte with bit data
            bitNumber++;
            if (bitNumber == 8)
            {
                compressedByte = compressedData[compressedOffset++];
                bitNumber = 0;
            }
        }

        *outDecompressedNumBytes = decompressedOffset;
    }


    // free memory that was allocated inside this compressor
    void HuffmanCompressor::FreeData(const uint8* data)
    {
        MCore::Free((void*)data);
    }


    // calculate the decompressed size of a compressed buffer
    uint32 HuffmanCompressor::CalcDecompressedSize(const uint8* compressedData, uint32 numBytes) const
    {
        uint32 compressedOffset     = 0;
        uint32 decompressedOffset   = 0;
        uint32 bitNumber            = 0;
        uint32 treeNode             = mRootNode;

        // while not reached 'end of data' marker
        bool done = false;
        uint8 compressedByte = compressedData[compressedOffset++];
        while (done == false)
        {
            // get the bit value
            const uint32 bitValue = compressedByte & (1 << bitNumber);

            // traverse the huffman tree
            if (bitValue == 0) // take the left child
            {
                treeNode = mTreeNodes[treeNode].mLeftNode;
            }
            else // take the right child
            {
                treeNode = mTreeNodes[treeNode].mRightNode;
            }

            // write the decompressed value
            if (mTreeNodes[treeNode].mLeftNode == MCORE_INVALIDINDEX16 && mTreeNodes[treeNode].mRightNode == MCORE_INVALIDINDEX16)
            {
                if (treeNode == MARKER_FOURTIMES)
                {
                    decompressedOffset += 4;
                }
                else
                {
                    // EOD marker
                    if (treeNode == MARKER_ENDOFDATA)
                    {
                        break;
                    }

                    decompressedOffset++;
                }

                treeNode = mRootNode;
            }

            // get the next byte with bit data
            bitNumber++;
            if (bitNumber == 8)
            {
                compressedByte = compressedData[compressedOffset++];
                bitNumber = 0;
            }

            // check if we reached the end of the input buffer
            if (compressedOffset > numBytes)
            {
                return MCORE_INVALIDINDEX32;
            }
        }

        return decompressedOffset;
    }


    // process file data
    void HuffmanCompressor::ProcessFileData(MCore::File& file, bool autoCloseFile)
    {
        // get the size of the file
        const uint32 fileSize = static_cast<uint32>(file.GetFileSize());

        // calculate how many chunks of 256 bytes are inside the file
        const uint32 numBlocks = fileSize / 256;

        // process all blocks of 256 bytes
        uint8 byteData[256];
        for (uint32 i = 0; i < numBlocks; ++i)
        {
            // read the file data
            file.Read(byteData, 256);

            // process it
            ProcessData(byteData, 256);
        }

        // process the remaining bytes
        const uint32 numRemainingBytes = fileSize % 256;
        file.Read(byteData, numRemainingBytes);
        ProcessData(byteData, numRemainingBytes);

        // close the file if we want to
        if (autoCloseFile)
        {
            file.Close();
        }
    }


    // process a file by name
    bool HuffmanCompressor::ProcessFileData(const char* fileName)
    {
        // try to open the file
        MCore::DiskFile file;
        if (file.Open(fileName, MCore::DiskFile::READ) == false)
        {
            return false;
        }

        // process the file and automatically close it
        ProcessFileData(file, true);
        return true;
    }


    // decompress a file on disk
    bool HuffmanCompressor::DecompressFile(const char* compressedFileName, const char* uncompressedFileName, uint32* outCompressedNumBytes, uint32* outUncompressedNumBytes)
    {
        // read the file data into memory, and compress it
        MCore::DiskFile inFile;
        if (inFile.Open(compressedFileName, MCore::DiskFile::READ) == false)
        {
            return false;
        }

        // try to create the output file
        MCore::DiskFile outFile;
        if (outFile.Open(uncompressedFileName, MCore::DiskFile::WRITE) == false)
        {
            inFile.Close();
            return false;
        }

        // read the huffman tree nodes
        Load(inFile);

        // now load the decompressed size
        uint32 storedDecompressedSize;
        inFile.Read(&storedDecompressedSize, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&storedDecompressedSize, MCore::Endian::ENDIAN_BIG);

        // now load the compressed data
        const uint32 compressedDataSize = static_cast<uint32>(inFile.GetFileSize());
        uint8* compressedData = (uint8*)MCore::Allocate(compressedDataSize);
        inFile.Read(compressedData, compressedDataSize);
        inFile.Close();

        // output the compressed size
        if (outCompressedNumBytes)
        {
            *outCompressedNumBytes = compressedDataSize + 258 * sizeof(uint32); // the uint32's are the node frequencies required to build the huffman tree
        }
        // uncompress it
        //uint32 uncompressedNumBytes = CalcDecompressedSize( compressedData, compressedDataSize );
        uint32 uncompressedNumBytes;
        uint8* uncompressedData = (uint8*)MCore::Allocate(storedDecompressedSize);
        Decompress(compressedData, uncompressedData, &uncompressedNumBytes);

        // release compressed memory
        MCore::Free(compressedData);

        // write the uncompressed data to the output file
        outFile.Write(uncompressedData, uncompressedNumBytes);
        outFile.Close();

        // release uncompressed data
        MCore::Free(uncompressedData);

        *outUncompressedNumBytes = uncompressedNumBytes;
        return true;
    }


    // compress a file on disk
    bool HuffmanCompressor::CompressFile(const char* uncompressedFileName, const char* compressedFileName, uint32* outUncompressedNumBytes, uint32* outCompressedNumBytes, bool processData)
    {
        // read the file data into memory, and compress it
        MCore::DiskFile inFile;
        if (inFile.Open(uncompressedFileName, MCore::DiskFile::READ) == false)
        {
            return false;
        }

        // try to create the output file
        MCore::DiskFile outFile;
        if (outFile.Open(compressedFileName, MCore::DiskFile::WRITE) == false)
        {
            inFile.Close();
            return false;
        }

        // get the file size and read it into memory
        const uint32 inFileSize = static_cast<uint32>(inFile.GetFileSize());
        uint8* uncompressedData = (uint8*)MCore::Allocate(inFileSize);
        inFile.Read(uncompressedData, inFileSize);
        inFile.Close();

        // store the uncompressed size
        if (outUncompressedNumBytes)
        {
            *outUncompressedNumBytes = inFileSize;
        }

        // perform the actual compression now
        uint32 compressedNumBytes;
        uint8* compressedData = nullptr;
        Compress(uncompressedData, inFileSize, &compressedData, &compressedNumBytes, processData);

        // release allocated memory of the uncompressed data
        MCore::Free(uncompressedData);

        // store the compressed size
        if (outCompressedNumBytes)
        {
            *outCompressedNumBytes = compressedNumBytes + 258 * sizeof(uint32); // the uint32's are the node frequencies required to build the huffman tree
        }
        // write the compressed data to the compressed file
        Save(outFile);  // write the huffman tree etc

        uint32 decompressedSize = inFileSize;
    #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
        MCore::Endian::ConvertUnsignedInt32(&decompressedSize);     // store it in big endian
    #endif

        outFile.Write(&decompressedSize, sizeof(uint32));   // write the decompressed size
        outFile.Write(compressedData, compressedNumBytes);  // write the compressed data
        outFile.Close();

        // release the compressed data
        MCore::Free(compressedData);
        return true;
    }


    // load the huffman coder info
    bool HuffmanCompressor::Load(MCore::Stream& stream)
    {
        // begin the processing of data
        BeginProcessData();
        MCORE_ASSERT(mTreeNodes.GetLength() == 258);

        // read the stream nodes
        uint32 frequency;
        for (uint32 i = 0; i < 258; ++i)
        {
            // read the node
            stream.Read(&frequency, sizeof(uint32));

            // convert endian, it is stored in big endian
            MCore::Endian::ConvertUnsignedInt32(&frequency, MCore::Endian::ENDIAN_BIG);

            // pass it to the tree node
            mTreeNodes[i].mFrequency = frequency;
        }

        // find the priority node
        mPriorityNode = FindPriorityNode();

        // build the huffman tree based on the statistics we just loaded
        EndProcessData();

        return true;
    }


    // save the huffman coder info
    bool HuffmanCompressor::Save(MCore::Stream& stream) const
    {
        // write the stream nodes
        uint32 frequency;
        for (uint32 i = 0; i < 258; ++i)
        {
            frequency = mTreeNodes[i].mFrequency;

            // convert endian if needed
        #if !defined(AZ_BIG_ENDIAN) // LITTLE_ENDIAN
            MCore::Endian::ConvertUnsignedInt32(&frequency);
        #endif

            stream.Write(&frequency, sizeof(uint32));
        }

        return true;
    }


    // find the priority node
    uint16 HuffmanCompressor::FindPriorityNode() const
    {
        uint32 maxFreq = 0;
        uint16 maxNode = MCORE_INVALIDINDEX16;

        // process all input data bytes
        for (uint32 i = 0; i < 256; ++i)
        {
            if (mTreeNodes[i].mFrequency > maxFreq)
            {
                maxNode = static_cast<uint16>(i);
                maxFreq = mTreeNodes[i].mFrequency;
            }
        }

        return maxNode;
    }


    // init from stats
    void HuffmanCompressor::InitFromStats(uint32* nodeStats, uint32 numNodes)
    {
        // make sure we have enough nodes
        BeginProcessData();
        mTreeNodes.Resize(numNodes);

        // update our node statistics
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mTreeNodes[i].mFrequency =  nodeStats[i];
        }

        // find the priority node
        mPriorityNode = FindPriorityNode();

        // build the tree
        EndProcessData();
    }
}   // namespace MCore
