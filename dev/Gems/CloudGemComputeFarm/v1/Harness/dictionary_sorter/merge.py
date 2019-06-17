#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import os
import tempfile
import zipfile

from harness import util


class LineSource(object):
    def __init__(self, lines, list_position):
        self.lines = lines
        self.index = 0
        self.current = bytes.lower(lines[0])
        self.list_position = list_position

    def advance(self):
        self.index += 1
        result = False

        if self.index < len(self.lines):
            self.current = bytes.lower(self.lines[self.index])
            result = True

        return result


def handler(config, task, updater, main_input, path_input, merge_inputs):
    """
    A simple example of a merge handler that downloads sorted segments of a dictionary from S3, merges them together
    into a single sorted dictionary, and uploads the results back to S3.
    """
    s3_dir = main_input['s3_dir']
    s3_file = main_input['s3_file']

    merge_sources = []

    for idx, input in enumerate(merge_inputs):
        updater.post_task_update("Downloading chunk %d of %d from S3" % (idx + 1, len(merge_inputs)))

        zip_name = input['zip_name']
        text_name = zip_name[:-3] + "txt"
        local_zip_path = os.path.join(tempfile.gettempdir(), zip_name)

        config.s3_resource.meta.client.download_file(
            Bucket=config.config_bucket,
            Key=util.s3_key_join(s3_dir, zip_name),
            Filename=local_zip_path
        )

        with zipfile.ZipFile(local_zip_path, "r") as zp:
            with zp.open(text_name, "r") as fp:
                lines = fp.readlines()

        merge_sources.append(LineSource(lines, len(merge_sources)))

    updater.post_task_update("Merging %d data sources" % len(merge_inputs))

    out_lines = []

    # Merge the multiple independently-sorted sources by popping the minimum off of each stack until there are none left
    while len(merge_sources) > 1:
        # Determine which source has the minimum value, and append its line to the output
        min_source = min(merge_sources, key=lambda x: x.current)
        out_lines.append(min_source.lines[min_source.index])

        # Advance that source to the next line
        if not min_source.advance():
            # We are out of lines remaining in this source, so discard it
            del merge_sources[min_source.list_position]

            # Renumber the remaining sources
            for idx, src in enumerate(merge_sources):
                src.list_position = idx

    # With only one source left, we can just dump the remaining lines from it to the output
    out_lines.extend(merge_sources[0].lines[merge_sources[0].index:])

    # Write the output to a file
    out_basename = "%s_sorted%s" % (s3_file, "".join([str(x) for x in path_input]))
    out_zip_name = out_basename + ".zip"
    out_txt_name = out_basename + ".txt"
    out_zip_path = os.path.join(tempfile.gettempdir(), out_zip_name)

    with zipfile.ZipFile(out_zip_path, "w", zipfile.ZIP_DEFLATED) as zp:
        with zp.open(out_txt_name, "w") as fp:
            fp.write(b"".join(out_lines))

    updater.post_task_update("Uploading results to S3")

    config.s3_resource.meta.client.upload_file(
        Filename=out_zip_path,
        Bucket=config.config_bucket,
        Key=util.s3_key_join(s3_dir, out_zip_name)
    )

    return {
        'zip_name': out_zip_name,
        'tail': path_input[-1] if len(path_input) else None
    }
