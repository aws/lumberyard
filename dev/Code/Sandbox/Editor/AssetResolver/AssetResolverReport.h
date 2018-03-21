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

#ifndef CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVERREPORT_H
#define CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVERREPORT_H
#pragma once


#include "Util/Variable.h"
#include "IAssetSearcher.h"

#include <QMetaType>

class CMissingAssetMessage;

class CMissingAssetRecord
{
public:
    enum EState
    {
        ESTATE_PENDING,
        ESTATE_NOT_RESOLVED,
        ESTATE_AUTO_RESOLVED,
        ESTATE_ACCEPTED,
        ESTATE_CANCELLED,
    };

    uint32 id;
    EState state;
    int assetTypeId;
    int searcherId;
    QString orgname;
    QString extension;
    QStringList substitutions;
    CMissingAssetMessage* pRecordMessage;
    bool needUpdate;
    bool needRemove;
    std::vector<TMissingAssetResolveCallback> requests;
    IAssetSearcher::TAssetSearchId searchRequestId;

    CMissingAssetRecord(const TMissingAssetResolveCallback& request, const char* _orgname, uint32 _id, int _searcherId, int _assetTypeId)
        : id(_id)
        , searcherId(_searcherId)
        , assetTypeId(_assetTypeId)
        , state(ESTATE_PENDING)
        , orgname(_orgname)
        , pRecordMessage(NULL)
        , needUpdate(false)
        , needRemove(false)
        , searchRequestId(IAssetSearcher::INVALID_ID)
    {
        requests.push_back(request);
        InitExtension(_orgname);
    }

    CMissingAssetRecord()
    {
        state = ESTATE_PENDING;
        assetTypeId = -1;
    }

    inline void UpdateState(EState newState)
    {
        state = newState;
        needUpdate = true;
    }

    inline void SetUpdated() { needUpdate = false; }
    inline bool NeedUpdate() const { return needUpdate; }
    inline bool NeedRemove() const { return needRemove; }

    struct SNotifier
    {
        SNotifier(uint32 _id, const std::vector<TMissingAssetResolveCallback>& reqs, const char* oFile, const char* nFile, bool _success)
            : id(_id)
            , requests(reqs)
            , orgFile(oFile)
            , newFile(nFile)
            , success(_success)
        {}

        inline void Notify()
        {
            for (std::vector<TMissingAssetResolveCallback>::const_iterator it = requests.begin(), end = requests.end(); it != end; ++it)
            {
                (*it)(id, success, orgFile.toUtf8().data(), newFile.toUtf8().data());
            }
        }

    private:
        uint32 id;
        std::vector<TMissingAssetResolveCallback> requests;
        QString orgFile;
        QString newFile;
        bool success;
    };

    SNotifier GetNotifier();

    void InitExtension(const char* file);
};

Q_DECLARE_METATYPE(CMissingAssetRecord*);

class CMissingAssetReport
    : public _i_reference_target_t
{
    typedef std::map<uint32, CMissingAssetRecord> TRecords;
public:
    CMissingAssetReport();

    uint32 AddRecord(const TMissingAssetResolveCallback& request, const char* filename, int searcherId, int assetTypeId);
    void FlagRemoveRecord(uint32 id);
    void FlagRemoveRecord(const TMissingAssetResolveCallback& request, uint32 reportId = 0);
    void FlushRemoved();

    int GetCount() const { return m_records.size(); };
    CMissingAssetRecord* GetRecord(uint32 id);
    CMissingAssetRecord* GetRecord(CMissingAssetMessage* pRecord);

    bool NeedUpdate() const { return m_needUpdate; }
    void SetUpdated(bool updated) { m_needUpdate = !updated; }

    struct SRecordIterator
    {
    public:
        CMissingAssetRecord* Next()
        {
            if (m_iterator != m_end)
            {
                return &((m_iterator++)->second);
            }
            return NULL;
        }

    private:
        friend class CMissingAssetReport;
        SRecordIterator(TRecords& records)
            : m_iterator(records.begin())
            , m_end(records.end())
        {}

        TRecords::iterator m_iterator;
        TRecords::iterator m_end;
        TRecords m_copy;
    };

    SRecordIterator GetIterator() { return SRecordIterator(m_records); }

private:
    bool IsNotInList(const char* filename, uint32& id) const;
    uint32 GetNextFreeId() const;

private:
    TRecords m_records;
    bool m_needUpdate;
};


#endif // CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVERREPORT_H
