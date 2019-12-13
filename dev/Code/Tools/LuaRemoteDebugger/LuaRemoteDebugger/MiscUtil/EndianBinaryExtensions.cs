using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MiscUtil.IO
{
	public static class EndianBinaryExtensions
	{
		public static string ReadString(this EndianBinaryReader reader)
		{
			int length;
			length = reader.ReadInt32();
			byte[] data = reader.ReadBytes(length);
			string result = System.Text.Encoding.ASCII.GetString(data);
			if (reader.ReadByte() != 0) // Check NULL terminator
				throw new Exception("Invalid string");
			return result;
		}

		public static void WriteString(this EndianBinaryWriter writer, string str)
		{
			int length = str.Length;
			writer.Write(length);
			byte[] data = System.Text.Encoding.ASCII.GetBytes(str);
			writer.Write(data);
			writer.Write((byte)0);  // NULL terminator
		}
	}
}
