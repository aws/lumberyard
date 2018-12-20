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

from __future__ import print_function

import os
from collections import namedtuple
import magic
from PIL import Image
Image.warnings.simplefilter('error', Image.DecompressionBombWarning)
from hurry.filesize import size
import piexif
import CloudCanvas
import defect_reporter_constants as constants
import defect_reporter_sanitization_settings as settings
import defect_reporter_s3 as s3

FileStatus = namedtuple("FileStatus", "key, is_allowed, reason, exception")

def main(event, request):
    ''' Main entry point for the sanitization lambda. '''

    __validate_event(event)

    s3_client = s3.get_client()
    attachment_bucket = CloudCanvas.get_setting(constants.ATTACHMENT_BUCKET)
    sanitized_bucket = CloudCanvas.get_setting(constants.SANITIZED_BUCKET)

    sanitizer = Sanitizer(s3_client, attachment_bucket)

    records = event['Records']
    for record in records:
        __validate_record(record)

        key = record['s3']['object']['key']

        if sanitizer.validate(key):
            sanitizer.move_to_target(key, sanitized_bucket)

    if sanitizer.rejected_files:
        print("[REJECTED FILES]")
        for status in sanitizer.rejected_files:
            print("Key: {} is_allowed: {} Reason: {} Exception: {}".format(status.key, status.is_allowed, status.reason, status.exception))


def __validate_event(event):
    ''' Validate content of event received. '''

    if 'Records' not in event:
        raise RuntimeError('Malformed event received. (Records)')


def __validate_record(record):
    ''' Extracts the data specific to the file from records in event. '''

    if 'eventVersion' not in record:
        raise RuntimeError('Malformed event recieved. (eventVersion)')

    if 's3' not in record:
        raise RuntimeError('Malformed event received. (s3)')

    s3 = record['s3']

    if 'object' not in s3:
        raise RuntimeError('Malformed event received. (object)')

    object_ = s3['object']

    if 'key' not in object_:
        raise RuntimeError('Malformed event received. (key)')


class Sanitizer:

    def __init__(self, s3_client, source_bucket):
        self.s3_client = s3_client
        self.source_bucket = source_bucket
        self.rejected_files = []


    def validate(self, key):
        ''' Validates that file does not contain know threats. '''

        download_path = self.__get_download_path(key)

        self.s3_client.download_file(self.source_bucket, key, download_path)

        content_type = self.__get_mime_type(download_path)

        if not self.__validate_mime_type(key, content_type):
            return False

        if content_type in (constants.MIME_TYPE_IMAGE_JPEG, constants.MIME_TYPE_IMAGE_PNG) and self.__validate_image(key, content_type, download_path):
            return True
        elif content_type == constants.MIME_TYPE_TEXT_PLAIN and self.__validate_file(key, download_path):
            return True
        else:
            return False


    def move_to_target(self, key, target):
        ''' Moves object with key to target bucket. '''

        is_successfully_copied = self.__copy_object_to_target(key, target)

        if is_successfully_copied:
            self.__delete_object_from_source(key)


    def __copy_object_to_target(self, key, target_bucket):
        ''' Copies object with key to target bucket. Keeps encryption of original file. '''

        encryption_type = self.__get_serverside_encryption(key)

        if encryption_type == 'aws:kms':
            raise RuntimeError('KMS Encryption not supported.')

        is_encrypted = True if encryption_type == 'AES256' else False

        try:
            copy_source = {'Bucket': self.source_bucket, 'Key': key}
            if is_encrypted:
                self.s3_client.copy_object(
                    Bucket=target_bucket,
                    CopySource=copy_source,
                    ServerSideEncryption='AES256',
                    Key=key
                    )
            else:
                self.s3_client.copy_object(
                    Bucket=target_bucket,
                    CopySource=copy_source,
                    Key=key
                    )
        except Exception as error:
            self.rejected_files.append(FileStatus(key, True, "Could not upload to sanitized bucket.", error))
            return False
        else:
            return True


    def __delete_object_from_source(self,key):
        ''' Deletes object with key from source bucket. '''

        try:
            self.s3_client.delete_object(Bucket=self.source_bucket, Key=key)
        except Exception as error:
            self.rejected_files.append(FileStatus(key, True, "Could not delete object after moving object to target bucket.", error))


    def __get_download_path(self, key):
        ''' Returns local path in lambda for download. '''

        return '/tmp/unsanitized-{}'.format(key)


    def __validate_image(self, key, content_type, file_path):
        ''' Validates image is within the allowed parameters defined in the settings. '''
        if content_type == constants.MIME_TYPE_IMAGE_JPEG:
            self.__remove_metadata(file_path)

        if not self.__validate_file_size(file_path):
            self.rejected_files.append(FileStatus(key, False, "Invalid file size.", ''))
            return False

        try:
            image_object = Image.open(file_path)
        except Image.DecompressionBombWarning as error:
            self.rejected_files.append(FileStatus(key, False, "Decompression Bomb Warning triggered.", error))
            return False

        if not self.__validate_image_dimensions(image_object):
            self.rejected_files.append(FileStatus(key, False, "Invalid file dimensions.", ''))
            return False

        if self.__process_image(key, image_object):
            return True
        else:
            return False


    def __validate_file_size(self, file_path):
        ''' Validate file size is less than the maximum allowed in settings. '''

        size_in_bytes = os.path.getsize(file_path)

        if size_in_bytes > settings.MAXIMUM_IMAGE_SIZE_IN_BYTES:
            return False
        else:
            return True


    def __validate_image_dimensions(self, image_object):
        ''' Validates image dimensions are less than the maximum allowed in settings. '''

        width, height = image_object.size

        if (width < settings.MAXIMUM_IMAGE_WIDTH) and (height < settings.MAXIMUM_IMAGE_HEIGHT):
            return True
        else:
            return False


    def __process_image(self, key, image_object):
        ''' Shrink image by a pixel and expand to original size.  '''

        width, height = image_object.size

        try:
            image_object.resize((width - 1 , height - 1))
            image_object.resize((width , height))
        except Exception as error:
            self.rejected_files.append(FileStatus(key, False, "Could not resize image.", error))
            return False
        else:
            return True


    def __remove_metadata(self, file_path):
        ''' Removes metadata from image. '''

        piexif.remove(file_path)


    def __get_mime_type(self, file_path):
        ''' Returns the mime type for the file. '''

        return magic.from_file(file_path, mime=True)


    def __validate_mime_type(self, key, content_type):
        ''' Validates mime type is of expected type. '''

        if content_type in (constants.MIME_TYPE_IMAGE_JPEG, constants.MIME_TYPE_TEXT_PLAIN, constants.MIME_TYPE_IMAGE_PNG):
            return True
        else:
            self.rejected_files.append(FileStatus(key, False, "Content-type not valid. (Mime)", ''))
            return False


    def __validate_file(self, key, file_path):
        ''' Validates plain text files like logs. '''

        if self.__validate_file_size(file_path):
            return True
        else:
            self.rejected_files.append(FileStatus(key, False, "Invalid file size.", ''))
            return False


    def __get_serverside_encryption(self, key):
        ''' Get encryption for object from metadata in S3. '''

        try:
            response = self.s3_client.head_object(
                Bucket=self.source_bucket,
                Key=key
                )
        except Exception as e:
            raise RuntimeError('Cannot determine encryption on object {}. {} '.format(key,e))

        if 'ServerSideEncryption' in response:
            return response['ServerSideEncryption']
        else:
            return ''
