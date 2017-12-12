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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEWRITER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEWRITER_H
#pragma once


#include "GeomCache.h"
#include "GeomCacheBlockCompressor.h"
#include "StealingThreadPool.h"

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>

// This contains the position of an asynchronous
// write after it was completed
class DiskWriteFuture
{
    friend class GeomCacheDiskWriteThread;

public:
    DiskWriteFuture()
        : m_position(0)
        , m_size(0) {};

    uint64 GetPosition() const
    {
        return m_position;
    }
    uint32 GetSize() const
    {
        return m_size;
    }

private:
    uint64 m_position;
    uint32 m_size;
};

// This writes the results from the compression job to disk and will
// not run in the thread pool, because it requires almost no CPU.
class GeomCacheDiskWriteThread
{
    static const unsigned int m_kNumBuffers = 8;

public:
    GeomCacheDiskWriteThread(const string& fileName);
    ~GeomCacheDiskWriteThread();

    // Flushes the buffers to disk
    void Flush();

    // Flushes the buffers to disk and exits the thread
    void EndThread();

    // Write with FIFO buffering. This will acquire ownership of the buffer.
    void Write(std::vector<char>& buffer, DiskWriteFuture* pFuture = 0, long offset = 0, int origin = SEEK_CUR);

    size_t GetBytesWritten() { return m_bytesWritten; }

private:
    static unsigned int __stdcall ThreadFunc(void* pParam);
    void Run();

    bool IsReadyToExit();

    AZStd::thread m_thread;
    string m_fileName;

    bool m_bExit;

    AZStd::condition_variable m_dataAvailableCV;
    AZStd::condition_variable m_dataWrittenCV;

    unsigned int m_currentReadBuffer;
    unsigned int m_currentWriteBuffer;

    AZStd::mutex m_outstandingWritesCS;
    AZStd::atomic_long m_outstandingWrites;

    AZStd::mutex m_criticalSections[m_kNumBuffers];
    std::vector<char> m_buffers[m_kNumBuffers];
    DiskWriteFuture* m_futures[m_kNumBuffers];
    long m_offsets[m_kNumBuffers];
    int m_origins[m_kNumBuffers];

    // Stats
    size_t m_bytesWritten;
};

// This class will receive the data from the GeomCacheWriter.
class GeomCacheBlockCompressionWriter
{
    struct JobData
    {
        JobData()
            : m_bFinished(false) {}

        volatile bool m_bFinished;
        volatile bool m_bWritten;
        uint m_blockIndex;
        uint m_dataIndex;
        long m_offset;
        long m_origin;
        JobData* m_pPrevJobData;
        GeomCacheBlockCompressionWriter* m_pCompressionWriter;
        DiskWriteFuture* m_pWriteFuture;
        std::vector<char> m_data;
    };

public:
    GeomCacheBlockCompressionWriter(ThreadUtils::StealingThreadPool& threadPool,
        IGeomCacheBlockCompressor* pBlockCompressor, GeomCacheDiskWriteThread& diskWriteThread);
    ~GeomCacheBlockCompressionWriter();

    // This adds to the current buffer.
    void PushData(const void* data, size_t size);

    // Create job to compress data and write it to disk. Returns size of block.
    uint64 WriteBlock(bool bCompress, DiskWriteFuture* pFuture, long offset = 0, int origin = SEEK_CUR);

    // Flush to write thread
    void Flush();

private:
    static unsigned int __stdcall ThreadFunc(void* pParam);
    void Run();

    void PushCompletedBlocks();
    static void RunJob(JobData* pJobData);
    void CompressJob(JobData* pJobData);

    // Write thread handle
    AZStd::thread m_thread;

    // Flag to exit write thread
    bool m_bExit;

    ThreadUtils::StealingThreadPool& m_threadPool;
    IGeomCacheBlockCompressor* m_pBlockCompressor;
    GeomCacheDiskWriteThread& m_diskWriteThread;
    std::vector<char> m_data;

    AZStd::mutex m_lockWriter;

    // Next block index hat will be used
    uint m_nextBlockIndex;

    // Next job that will be used in the job array
    uint m_nextJobIndex;

    // Index of frame that needs to be written next
    uint m_nextBlockToWrite;

    AZStd::atomic_uint m_numJobsRunning;
    AZStd::mutex m_jobFinishedCS;
    AZStd::condition_variable m_jobFinishedCV;

    // The job data
    std::vector<JobData> m_jobData;
};

struct GeomCacheWriterStats
{
    uint64 m_headerDataSize;
    uint64 m_staticDataSize;
    uint64 m_animationDataSize;
    uint64 m_uncompressedAnimationSize;
};

class GeomCacheWriter
{
public:
    GeomCacheWriter(const string& filename, GeomCacheFile::EBlockCompressionFormat compressionFormat,
        ThreadUtils::StealingThreadPool& threadPool, const uint numFrames, const bool bPlaybackFromMemory,
        const bool b32BitIndices);

    void WriteStaticData(const std::vector<Alembic::Abc::chrono_t>& frameTimes, const std::vector<GeomCache::Mesh*>& meshes, const GeomCache::Node& rootNode);

    void WriteFrame(const uint frameIndex, const AABB& frameAABB, const GeomCacheFile::EFrameType frameType,
        const std::vector<GeomCache::Mesh*>& meshes, GeomCache::Node& rootNode);

    // Flush all buffers, write frame offsets and return the size of the animation stream
    GeomCacheWriterStats FinishWriting();

private:
    void WriteFrameTimes(const std::vector<Alembic::Abc::chrono_t>& frameTimes);
    bool WriteFrameInfos();
    void WriteMeshesStaticData(const std::vector<GeomCache::Mesh*>& meshes);
    void WriteNodeStaticDataRec(const GeomCache::Node& node, const std::vector<GeomCache::Mesh*>& meshes);

    void GetFrameData(GeomCacheFile::SFrameHeader& header, const std::vector<GeomCache::Mesh*>& meshes);
    void WriteNodeFrameRec(GeomCache::Node& node, const std::vector<GeomCache::Mesh*>& meshes, uint32& bytesWritten);

    void WriteMeshFrameData(GeomCache::Mesh& meshData);
    void WriteMeshStaticData(const GeomCache::Mesh& meshData, GeomCacheFile::EStreams streamMask);

    GeomCacheFile::EBlockCompressionFormat m_compressionFormat;

    GeomCacheFile::SHeader m_fileHeader;
    AABB m_animationAABB;
    static const uint m_kNumProgressStatusReports = 10;
    bool m_bShowedStatus[m_kNumProgressStatusReports];
    FILE* m_pFileHandle;
    uint64 m_totalUncompressedAnimationSize;

    std::unique_ptr<IGeomCacheBlockCompressor> m_pBlockCompressor;
    std::unique_ptr<GeomCacheDiskWriteThread> m_pDiskWriteThread;
    std::unique_ptr<GeomCacheBlockCompressionWriter> m_pCompressionWriter;

    DiskWriteFuture m_headerWriteFuture;
    DiskWriteFuture m_frameInfosWriteFuture;
    DiskWriteFuture m_staticNodeDataFuture;
    DiskWriteFuture m_staticMeshDataFuture;
    std::vector<std::unique_ptr<DiskWriteFuture> > m_frameWriteFutures;

    std::vector<Alembic::Abc::chrono_t> m_frameTimes;
    std::vector<uint> m_frameTypes;
};
#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_GEOMCACHEWRITER_H
