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

#pragma once

#ifndef MovementRequestID_h
#define MovementRequestID_h

// When you queue a movement request you will get a request id back.
// This ID is used when you want to cancel a queued request.
// (Wrapper around 'unsigned int' but provides type safety.)
// An ID of 0 represents an invalid/uninitialized ID.
struct MovementRequestID
{
    unsigned int id;

    MovementRequestID()
        : id(0) {}
    MovementRequestID(unsigned int _id)
        : id(_id) {}
    MovementRequestID& operator++() { ++id; return *this; }
    bool operator == (unsigned int otherID) const { return id == otherID; }
    operator unsigned int () const {
        return id;
    }

    static MovementRequestID Invalid() { return MovementRequestID(); }
};

#endif // MovementRequestID_h
