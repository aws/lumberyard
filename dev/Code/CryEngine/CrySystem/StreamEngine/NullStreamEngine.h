// Description : Streaming Engine null realization


#ifndef CRYINCLUDE_CRYSYSTEM_STREAMENGINE_NULL_STREAMENGINE_H
#define CRYINCLUDE_CRYSYSTEM_STREAMENGINE_NULL_STREAMENGINE_H
#pragma once


#include "IStreamEngine.h"

//////////////////////////////////////////////////////////////////////////
class CNullStreamEngine
    : public IStreamEngine
{
public:
    CNullStreamEngine()
    {
#ifdef STREAMENGINE_ENABLE_STATS
        m_Statistics.nPendingReadBytes = 0;

        m_Statistics.nCurrentAsyncCount = 0;
        m_Statistics.nCurrentDecryptCount = 0;
        m_Statistics.nCurrentDecompressCount = 0;
        m_Statistics.nCurrentFinishedCount = 0;
#endif
    }

    virtual IReadStreamPtr StartRead (const EStreamTaskType tSource, const char* szFile, IStreamCallback* pCallback = NULL, const StreamReadParams* pParams = NULL) { return NULL; }

    // Pass a callback to preRequestCallback if you need to execute code right before the requests get enqueued; the callback is called only once per execution
    virtual size_t StartBatchRead(IReadStreamPtr* pStreamsOut, const StreamReadBatchParams* pReqs, size_t numReqs, AZStd::function<void ()>* preRequestCallback = nullptr) { return 0; }

    // Call this methods before/after submitting large number of new requests.
    virtual void BeginReadGroup() {}
    virtual void EndReadGroup() {}

    // Pause/resumes streaming of specific data types.
    // nPauseTypesBitmask is a bit mask of data types (ex, 1<<eStreamTaskTypeGeometry)
    virtual void PauseStreaming(bool bPause, uint32 nPauseTypesBitmask) {}

    // Get pause bit mask
    virtual uint32 GetPauseMask() const { return 0; }

    // Pause/resumes any IO active from the streaming engine
    virtual void PauseIO(bool bPause) {}

    // Description:
    //   Is the streaming data available on harddisc for fast streaming
    virtual bool IsStreamDataOnHDD() const { return false; }

    // Description:
    //   Inform streaming engine that the streaming data is available on HDD
    virtual void SetStreamDataOnHDD(bool bFlag) {}

    // Description:
    //   Per frame update ofthe streaming engine, synchronous events are dispatched from this function.
    virtual void Update() {}

    // Description:
    //   Per frame update of the streaming engine, synchronous events are dispatched from this function, by particular TypesBitmask.
    virtual void Update(uint32 nUpdateTypesBitmask) {}

    // Description:
    //   Waits until all submitted requests are complete. (can abort all reads which are currently in flight)
    virtual void UpdateAndWait(bool bAbortAll = false) {}

    // Description:
    //   Puts the memory statistics into the given sizer object.
    //   According to the specifications in interface ICrySizer.
    // See also:
    //   ICrySizer
    virtual void GetMemoryStatistics(ICrySizer* pSizer) {}

#if defined(STREAMENGINE_ENABLE_STATS)
    // Description:
    //   Returns the streaming statistics collected from the previous call.
    virtual SStreamEngineStatistics& GetStreamingStatistics() { return m_Statistics; }
    virtual void ClearStatistics() {}

    // Description:
    //   returns the bandwidth used for the given type of streaming task
    virtual void GetBandwidthStats(EStreamTaskType type, float* bandwidth) {}
#endif

    // Description:
    //   Returns the counts of open streaming requests.
    virtual void GetStreamingOpenStatistics(SStreamEngineOpenStats& openStatsOut) {}

    virtual const char* GetStreamTaskTypeName(EStreamTaskType type) { return ""; }

#if defined(STREAMENGINE_ENABLE_LISTENER)
    // Description:
    //   Sets up a listener for stream events (used for statoscope)
    virtual void SetListener(IStreamEngineListener* pListener) {}
    virtual IStreamEngineListener* GetListener() { return nullptr; }
#endif

#ifdef STREAMENGINE_ENABLE_STATS
    SStreamEngineStatistics m_Statistics;
#endif
};

#endif // CRYINCLUDE_CRYSYSTEM_STREAMENGINE_NULL_STREAMENGINE_H
