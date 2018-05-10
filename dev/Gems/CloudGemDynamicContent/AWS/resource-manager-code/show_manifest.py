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
# $Revision: #1 $

import datetime
import staging

''' Print calls for now '''
def output_message(message):
    print message

def invalid_file(filePath):
    output_message('Invalid File: {}'.format(filePath))

def skipping_invalid_file(filePath):
    output_message('Skipping Invalid File: {}'.format(filePath))

def updating_file_hashes(manifestPath):
    output_message('Updating file hashes for {}'.format(manifestPath))

def hash_comparison_disk(filePath, manifestHash, diskHash):
    output_message('Comparing hashes for {}'.format(filePath))
    if manifestHash == diskHash:
        output_message(' MATCH Manifest: {}  Disk: {}'.format(manifestHash, diskHash))
    else:
        output_message(' UPDATE Manifest: {}  Disk: {}'.format(manifestHash, diskHash))

def hash_comparison_bucket(filePath, manifestHash, bucketHash):
    output_message('Comparing hashes for {}'.format(filePath))
    if manifestHash == bucketHash:
        output_message(' MATCH Manifest: {}  Bucket: {}'.format(manifestHash, bucketHash))
    else:
        output_message(' UPDATE Manifest: {}  Bucket: {}'.format(manifestHash, bucketHash))

def found_stack(stack_id):
    output_message('Found stack_id {}'.format(stack_id))

def found_bucket(bucketName):
    output_message('Found bucket {}'.format(bucketName))

def new_local_key(keyName):
    output_message('New Local Key {}'.format(keyName))

def found_updated_item(keyName):
    output_message('Found updated item {}'.format(keyName))

def key_not_found(keyName):
    output_message('Key not found {}'.format(keyName))

def done_uploading(filePath):
    output_message('Done uploading {}'.format(filePath))
    
def show_staging_status(filePath, status_info):
    output_message('StagingStatus of {} now set to {} - Parent {}'.format(filePath, status_info.get('StagingStatus'), status_info.get('Parent')))
    
    if status_info['StagingStatus'] == 'WINDOW':
    
        current_time = datetime.datetime.utcnow()
        output_message('Current UTC time: {}'.format(staging.get_formatted_time_string(current_time)))
        
        staging_start = status_info.get('StagingStart') 
        
        if staging_start != None:
            output_message('StagingStart is {}'.format(staging_start))
            start_time = staging.get_struct_time(staging_start)
            
            if start_time < current_time:
                output_message('Which is in the past')
            else:
                time_delta = start_time - current_time
                output_message('Which is {} days, {} hours, {} minutes from now '.format(time_delta.days, time_delta.seconds / 3600, (time_delta.seconds % 3600) / 60))
                
        staging_end = status_info.get('StagingEnd')    
         
        if staging_end != None:
            output_message('StagingEnd is {}'.format(staging_end))
            end_time = staging.get_struct_time(staging_end)           
            if end_time < current_time:
                output_message('Which is in the past')
            else:
                time_delta = end_time - current_time
                output_message('Which is {} days, {} hours, {} minutes from now'.format(time_delta.days, time_delta.seconds / 3600, (time_delta.seconds % 3600) / 60))

def finding_updated_content(manifestPath):
    output_message('Searching for updated content from {}'.format(manifestPath))

def manifest_not_writable(manifestPath):
    output_message('Manifest is not writable: {}'.format(manifestPath))

def list_manifests(files):
    for file in files:
        output_message(file)
        
def show_file_signature(file_path, to_sign, result):
    output_message('Signature of file {} to_sign {} returns {}'.format(file_path,to_sign, result))
    
def show_signature_check(signature, to_sign, result):
    output_message('Signature {} to_sign {} returns {}'.format(signature,to_sign, result))

def show_signature(file_path, signature):
    output_message('Signature of {} is {}'.format(file_path, signature))  

def skipping_unmodified_pak(file_path):
    output_message('No updated content in {} - skipping'.format(file_path))