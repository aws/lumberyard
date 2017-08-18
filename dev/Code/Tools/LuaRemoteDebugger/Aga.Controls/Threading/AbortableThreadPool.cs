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
using System.Text;
using System.Threading;

namespace Aga.Controls.Threading
{
	public class AbortableThreadPool
	{
		private LinkedList<WorkItem> _callbacks = new LinkedList<WorkItem>();
		private Dictionary<WorkItem, Thread> _threads = new Dictionary<WorkItem, Thread>();

		public WorkItem QueueUserWorkItem(WaitCallback callback)
		{
			return QueueUserWorkItem(callback, null);
		}

		public WorkItem QueueUserWorkItem(WaitCallback callback, object state)
		{
			if (callback == null) throw new ArgumentNullException("callback");

			WorkItem item = new WorkItem(callback, state, ExecutionContext.Capture());
			lock (_callbacks)
			{
				_callbacks.AddLast(item);
			}
			ThreadPool.QueueUserWorkItem(new WaitCallback(HandleItem));
			return item;
		}

		private void HandleItem(object ignored)
		{
			WorkItem item = null;
			try
			{
				lock (_callbacks)
				{
					if (_callbacks.Count > 0)
					{
						item = _callbacks.First.Value;
						_callbacks.RemoveFirst();
					}
					if (item == null)
						return;
					_threads.Add(item, Thread.CurrentThread);

				}
				ExecutionContext.Run(item.Context,
					delegate { item.Callback(item.State); }, null);
			}
			finally
			{
				lock (_callbacks)
				{
					if (item != null)
						_threads.Remove(item);
				}
			}
		}

		public bool IsMyThread(Thread thread)
		{
			lock (_callbacks)
			{
				foreach (Thread t in _threads.Values)
				{
					if (t == thread)
						return true;
				}
				return false;
			}
		}

		public WorkItemStatus Cancel(WorkItem item, bool allowAbort)
		{
			if (item == null)
				throw new ArgumentNullException("item");
			lock (_callbacks)
			{
				LinkedListNode<WorkItem> node = _callbacks.Find(item);
				if (node != null)
				{
					_callbacks.Remove(node);
					return WorkItemStatus.Queued;
				}
				else if (_threads.ContainsKey(item))
				{
					if (allowAbort)
					{
						_threads[item].Abort();
						_threads.Remove(item);
						return WorkItemStatus.Aborted;
					}
					else
						return WorkItemStatus.Executing;
				}
				else
					return WorkItemStatus.Completed;
			}
		}

		public void CancelAll(bool allowAbort)
		{
			lock (_callbacks)
			{
				_callbacks.Clear();
				if (allowAbort)
				{
					foreach (Thread t in _threads.Values)
						t.Abort();
				}
			}
		}
	}
}
