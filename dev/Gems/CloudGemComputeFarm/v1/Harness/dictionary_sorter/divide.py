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

import copy
import os
import tempfile
import zipfile

import harness
from harness import divide_response
from harness import util

def handler(config, task, updater, main_input, path_input, merge_inputs):
    """
    A simple example of a divide handler that downloads a dictionary of words from S3, splits it in half, and uploads
    the results back to S3.
    """
    max_level = main_input['max_level']
    s3_dir = main_input['s3_dir']
    s3_file = main_input['s3_file']

    updater.begin_task("Dividing input range at level {}".format(len(path_input)))

    response = harness.divide_response.DivideResponse(path_input)
    response.merge_task.task_input = main_input

    if len(path_input) < max_level:
        updater.begin_task("Fetching data to divide from S3")

        path_string = "".join([str(x) for x in path_input])
        zip_name = "%s%s.zip" % (s3_file, path_string)
        text_name = "%s%s.txt" % (s3_file, path_string)
        in_zip_path = os.path.join(tempfile.gettempdir(), zip_name)

        config.s3_resource.meta.client.download_file(
            Bucket=config.config_bucket,
            Key=util.s3_key_join(s3_dir, zip_name),
            Filename=in_zip_path
        )

        # Split the data into separate files
        with zipfile.ZipFile(in_zip_path, "r") as in_zip:
            with in_zip.open(text_name, "r") as in_data:
                lines = in_data.readlines()

        split_count = len(lines) >> 1
        batches = (lines[:split_count], lines[split_count:])

        for idx, batch in enumerate(batches):
            updater.begin_task("Creating and uploading segment %d of 2" % (idx + 1))

            file_prefix = "%s%s%d" % (s3_file, path_string, idx)
            out_text_name = file_prefix + ".txt"
            out_zip_name = file_prefix + ".zip"
            out_zip_path = os.path.join(tempfile.gettempdir(), out_zip_name)

            with zipfile.ZipFile(out_zip_path, "w", zipfile.ZIP_DEFLATED) as out_zip:
                with out_zip.open(out_text_name, "w") as out_text:
                    out_text.write(b''.join(batch))

            config.s3_resource.meta.client.upload_file(
                Filename=out_zip_path,
                Bucket=config.config_bucket,
                Key=util.s3_key_join(s3_dir, out_zip_name)
            )

        for x in [0, 1]:
            child_input = copy.deepcopy(main_input)
            response.add_divide_task(child_input, x)

    return response.as_dict()
