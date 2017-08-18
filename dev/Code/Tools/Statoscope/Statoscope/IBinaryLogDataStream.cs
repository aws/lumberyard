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

using System;

namespace Statoscope
{
  interface IBinaryLogDataStream : IDisposable
  {
    int Peek();
    byte ReadByte();
    byte[] ReadBytes(int nBytes);
		bool ReadBool();
    Int64 ReadInt64();
    UInt64 ReadUInt64();
    int ReadInt32();
    uint ReadUInt32();
    Int16 ReadInt16();
    UInt16 ReadUInt16();
    float ReadFloat();
    double ReadDouble();
    string ReadString();
    bool IsEndOfStream { get; }
    EEndian Endianness { get; }
    void SetEndian(EEndian endian);
		// used for when writing out the stream that's being processed (e.g. when network logging)
		void FlushWriteStream();
		void CloseWriteStream();
  }
}
