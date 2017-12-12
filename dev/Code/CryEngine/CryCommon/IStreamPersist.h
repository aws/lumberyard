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

// Description : IStreamPersist interface.

#ifndef CRYINCLUDE_CRYCOMMON_ISTREAMPERSIST_H
#define CRYINCLUDE_CRYCOMMON_ISTREAMPERSIST_H
#pragma once

struct IScriptObject;
class CStream;

enum DirtyFlags
{
    DIRTY_NAME      = 0x1,
    DIRTY_POS           = 0x2,
    DIRTY_ANGLES    = 0x4,
};

//!store the stream status per connection(per serverslot)
/*struct StreamStatus
{
    StreamStatus()
    {
        nUserFlags=0;
        nLastUpdate=0;
        nUpdateNumber=0;
    }
    unsigned int nUserFlags;
    unsigned int nLastUpdate;
    unsigned int nUpdateNumber;
};*/


// Description:
//      This interface must be implemented by all objects that must be serialized
//      through the network or file.
//  Remarks:
//      The main purpose of the serialization is reproduce the game remotely
//      or saving and restoring.This mean that the object must not save everything
//      but only what really need to be restored correctly.
struct IStreamPersist
{
    // <interfuscator:shuffle>
    // Description:
    //      Serializes the object to a bitstream(network)
    // Arguments:
    //  stm - the stream class that will store the bitstream
    // Return Value:
    //      True if succeeded,false failed
    // See also:
    //      CStream
    virtual bool Write(CStream&) = 0;
    // Description:
    //      Reads the object from a stream(network)
    // Arguments:
    //      stm - The stream class that store the bitstream.
    // Return Value:
    //      True if succeeded,false failed.
    // See also:
    //      CStream
    virtual bool Read(CStream&) = 0;
    // Description:
    //      Checks if the object must be synchronized since the last serialization.
    // Return Value:
    //      True must be serialized, false the object didn't change.
    virtual bool IsDirty() = 0;
    // Description:
    //      Serializes the object to a bitstream(file persistence)
    // Arguments:
    //      stm - The stream class that will store the bitstream.
    // Return Value:
    //      True if succeeded,false failed
    // See also:
    //      CStream
    virtual bool Save(CStream& stm) = 0;
    // Description:
    //      Reads the object from a stream(file persistence).
    // Arguments:
    //  stm - The stream class that store the bitstream.
    //  pStream - script wrapper for the stream(optional).
    // Return Value:
    //      True if succeeded,false failed.
    // See also:
    //      CStream
    virtual bool Load(CStream& stm, IScriptObject* pStream = NULL) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ISTREAMPERSIST_H
