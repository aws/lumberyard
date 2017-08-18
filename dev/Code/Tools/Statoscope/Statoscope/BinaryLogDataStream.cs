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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace Statoscope
{
	abstract class BinaryLogDataStream : IBinaryLogDataStream
	{
    protected BinaryReader m_binaryReader;
    protected bool m_bSwapEndian;

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			m_binaryReader.Close();
		}

    public EEndian Endianness
    {
      get { return m_bSwapEndian ? EEndian.BIG_ENDIAN : EEndian.LITTLE_ENDIAN; }
    }

    public BinaryLogDataStream()
    {
			m_binaryReader = null;
      m_bSwapEndian = false;
    }

    public void SetEndian(EEndian endian)
    {
      if(endian == EEndian.BIG_ENDIAN)
      {
        m_bSwapEndian=true;
      }
    }

    public int Peek()
    {
      return m_binaryReader.PeekChar();
    }

    public virtual byte[] ReadBytes(int nBytes)
    {
			byte[] data = m_binaryReader.ReadBytes(nBytes);
			if (data.Length != nBytes)
				throw new EndOfStreamException();
			return data;
    }

    public virtual byte ReadByte()
    {
      return m_binaryReader.ReadByte();
    }

		public virtual bool ReadBool()
		{
			return m_binaryReader.ReadBoolean();
		}

    public virtual Int64 ReadInt64()
    {
      Int64 i = m_binaryReader.ReadInt64();

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian8((UInt64*)&i);
        }
      }

      return i;
    }

    public virtual UInt64 ReadUInt64()
    {
      UInt64 i = m_binaryReader.ReadUInt64();

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian8(&i);
        }
      }

      return i;
    }

    public virtual int ReadInt32()
    {
      int i = m_binaryReader.ReadInt32();

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian4((uint*)&i);
        }
      }

      return i;
    }

    public virtual uint ReadUInt32()
    {
      uint i = m_binaryReader.ReadUInt32();

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian4(&i);
        }
      }

      return i;
    }

    public virtual Int16 ReadInt16()
    {
      Int16 i = m_binaryReader.ReadInt16();

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian2((UInt16*)&i);
        }
      }

      return i;
    }

    public virtual UInt16 ReadUInt16()
    {
      UInt16 i = m_binaryReader.ReadUInt16();

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian2(&i);
        }
      }

      return i;
    }

    public virtual float ReadFloat()
    {
      UInt32 p = m_binaryReader.ReadUInt32();
      float r;

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian4(&p);
        }
        r = *(float*)&p;
      }

      return r;
    }

    public virtual double ReadDouble()
    {
      UInt64 p = m_binaryReader.ReadUInt64();
      double r;

      unsafe
      {
        if (m_bSwapEndian)
        {
          EndianBitConverter.SwapEndian8(&p);
        }
        r = *(double*)&p;
      }

      return r;
    }

    public string ReadString()
    {
      int strLen = ReadInt32();

      byte[] s = ReadBytes(strLen);

      System.Text.Encoding enc = System.Text.Encoding.ASCII;
      string str = enc.GetString(s);

      return str;
    }

    public virtual bool IsEndOfStream
    {
      get
      {
        return (m_binaryReader.BaseStream.Position >= m_binaryReader.BaseStream.Length);
      }
    }

		public virtual void FlushWriteStream()
		{
		}

		public virtual void CloseWriteStream()
		{
		}
	}
}
