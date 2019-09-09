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


def handler(config, task, updater, main_input, path_input, merge_inputs):
    """
    A simple example of a build handler that downloads a segment of a dictionary from S3, sorts it using the built-in
    sort function, and uploads the results back to S3.
    """
    updater.post_task_update("Downloading data from S3")

    s3_dir = main_input['s3_dir']
    s3_file = main_input['s3_file']
    path_string = "".join([str(x) for x in path_input])
    file_prefix = "%s%s" % (s3_file, path_string)
    text_name = file_prefix + ".txt"
    zip_name = file_prefix + ".zip"
    zip_path = os.path.join(tempfile.gettempdir(), zip_name)

    config.s3_resource.meta.client.download_file(
        Bucket=config.config_bucket,
        Key=util.s3_key_join(s3_dir, zip_name),
        Filename=zip_path
    )

    updater.post_task_update("Processing data")

    with zipfile.ZipFile(zip_path, "r") as zp:
        with zp.open(text_name, "r") as fp:
            lines = fp.readlines()

    lines.sort(key=bytes.lower)

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zp:
        with zp.open(text_name, "w") as fp:
            fp.write(b"".join(lines))

    updater.post_task_update("Uploading results to S3")

    config.s3_resource.meta.client.upload_file(
        Bucket=config.config_bucket,
        Key=util.s3_key_join(s3_dir, zip_name),
        Filename=zip_path
    )

    return {
        'zip_name': zip_name,
        'tail': path_input[-1]
    }
