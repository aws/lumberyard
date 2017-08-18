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
using System.IO;
using System.Reflection;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;

namespace Aga.Controls
{
    public static class ResourceHelper
    {
        // VSpilt Cursor with Innerline (symbolisize hidden column)
        private static Cursor _dVSplitCursor = GetCursor(Properties.Resources.DVSplit);
        public static Cursor DVSplitCursor
        {
            get { return _dVSplitCursor; }
        }

		private static GifDecoder _loadingIcon = GetGifDecoder(Properties.Resources.loading_icon);
		public static GifDecoder LoadingIcon
		{
			get { return _loadingIcon; }
		}

        /// <summary>
        /// Help function to convert byte[] from resource into Cursor Type 
        /// </summary>
        /// <param name="data"></param>
        /// <returns></returns>
        private static Cursor GetCursor(byte[] data)
        {
            using (MemoryStream s = new MemoryStream(data))
                return new Cursor(s);
        }

		/// <summary>
		/// Help function to convert byte[] from resource into GifDecoder Type 
		/// </summary>
		/// <param name="data"></param>
		/// <returns></returns>
		private static GifDecoder GetGifDecoder(byte[] data)
		{
			using(MemoryStream ms = new MemoryStream(data))
				return new GifDecoder(ms, true);
		}

    }
}
