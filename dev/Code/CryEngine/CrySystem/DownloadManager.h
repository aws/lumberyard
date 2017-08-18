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

#ifndef CRYINCLUDE_CRYSYSTEM_DOWNLOADMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_DOWNLOADMANAGER_H
#pragma once

#ifdef WIN32

class CHTTPDownloader;


class CDownloadManager
{
public:
    CDownloadManager();
    virtual ~CDownloadManager();

    void Create(ISystem* pSystem);
    CHTTPDownloader* CreateDownload();
    void RemoveDownload(CHTTPDownloader* pDownload);
    void Update();
    void Release();

private:

    ISystem* m_pSystem;
    std::list<CHTTPDownloader*>    m_lDownloadList;
};


#endif //LINUX
#endif // CRYINCLUDE_CRYSYSTEM_DOWNLOADMANAGER_H
