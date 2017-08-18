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

namespace Statoscope
{
	public abstract class SessionInfo
	{
		int		NumFrames;
		float TotalTime;
		float StartTime;
		float EndTime;

		public abstract string LogType { get; }
		public abstract string Summary { get; }
		public virtual string[,] Items
		{
			get
			{
				int mins = (int)(TotalTime/60.0f);
				float secs = TotalTime - (mins * 60.0f);
				return new string[,] {
					{ "Log Type", LogType },
					{ "Num Frames", NumFrames.ToString() },
					{ "Total Time", mins > 0 ? string.Format("{0}m{1:d2}s", mins, (int)secs) : string.Format("{0:n2}s", secs) },
					{ "Start Time", string.Format("{0:n2}s", StartTime) },
					{ "End Time", string.Format("{0:n2}s", EndTime) }
				};
			}
		}

		public void Update(int numFrames, float startTime, float endTime)
		{
			NumFrames = numFrames;
			StartTime = startTime;
			EndTime = endTime;
			TotalTime = EndTime - StartTime;
		}

		protected string[,] ConcatItems(string[,] baseItems, string[,] newItems)
		{
			string[,] items = new string[baseItems.GetUpperBound(0) + 1 + newItems.GetUpperBound(0) + 1, 2];
			Array.Copy(baseItems, 0, items, 0, baseItems.Length);
			Array.Copy(newItems, 0, items, baseItems.Length, newItems.Length);
			return items;
		}
	}

	class FileSessionInfo : SessionInfo
	{
		public readonly string LocalLogFile;
		public override string LogType { get { return "File"; } }
		public override string Summary { get { return LocalLogFile; } }
		public override string[,] Items
		{
			get
			{
				return ConcatItems(base.Items, new string[,] {
					{ "Local File", LocalLogFile }
				});
			}
		}

		public FileSessionInfo(string localLogFile)
		{
			LocalLogFile = localLogFile;
		}
	}

	class TelemetrySessionInfo : FileSessionInfo
	{
		public readonly string SourceLogFile;
		public readonly string PeerName;
		public readonly string Platform;
		public readonly string PeerType;
		public override string LogType { get { return "Telemetry"; } }
		public override string Summary { get { return string.Format("{0} ({1})", PeerName, Platform); } }
		public override string[,] Items
		{
			get
			{
				return ConcatItems(base.Items, new string[,] {
					{ "Source File", SourceLogFile },
					{ "Peer Name", PeerName },
					{ "Platform", Platform },
					{ "Peer Type", PeerType },
				});
			}
		}

		public TelemetrySessionInfo(string localLogFile, string sourceLogFile, string peerName, string platform, string peerType)
			: base(localLogFile)
		{
			SourceLogFile = sourceLogFile;
			PeerName = peerName;
			Platform = platform;
			PeerType = peerType;
		}
	}

	class SocketSessionInfo : FileSessionInfo
	{
		public readonly string ConnectionName;
		public override string LogType { get { return "Socket"; } }
		public override string Summary { get { return ConnectionName; } }
		public override string[,] Items
		{
			get
			{
				return ConcatItems(base.Items, new string[,] {
					{ "Connected To", ConnectionName }
				});
			}
		}

		public SocketSessionInfo(string connectionName, string localLogFile)
			: base(localLogFile)
		{
			ConnectionName = connectionName;
		}
	}
}