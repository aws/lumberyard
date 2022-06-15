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

import base64
import hashlib
import os
from shutil import copyfile
from time import strftime

import dynamic_content_settings
import show_manifest
from lib.Crypto.Hash import SHA256
from lib.Crypto.PublicKey import RSA
from lib.Crypto.Signature import PKCS1_v1_5
from resource_manager.errors import HandledError


def backup_files(file_list):
    for file_to_backup in file_list:
        backup_file_name = file_to_backup + '.backup.' + strftime("%Y%m%d-%H%M%S")
        copyfile(file_to_backup, backup_file_name)


def command_generate_keys(context, args):
    public_key_file, private_key_file = get_key_pair_paths(context, args.key_name)

    found_file_list = []
    if os.path.exists(public_key_file):
        found_file_list.append(public_key_file)

    if os.path.exists(private_key_file):
        found_file_list.append(private_key_file)

    if len(found_file_list) > 0:
        if args.gui is True:
            backup_files(found_file_list)
        else:
            context.view.confirm_file_replacements(found_file_list)

    generate_keys(context, args.key_name)


def command_show_signature(context, args):
    file_path = os.path.join(context.config.game_directory_path, args.file_name)

    show_signature(context, file_path)


def get_file_signature(context, file_path):
    if not os.path.exists(file_path):
        raise HandledError('File not found: {}'.format(file_path))

    this_file = open(file_path, 'rb')
    hashdigest = hashlib.md5(this_file.read()).hexdigest()

    hash_value = SHA256.new()
    hash_value.update(hashdigest.encode('utf8'))

    signature_string = get_signature(context, hash_value)
    show_manifest.show_file_signature(file_path, hashdigest, signature_string)
    return signature_string


def show_signature(context, file_path):
    signature = get_file_signature(context, file_path)
    show_manifest.show_signature(file_path, signature)


def get_key_pair_paths(context, base_name):
    if base_name is None:
        base_name = dynamic_content_settings.get_default_keyname()

    public_out_folder = dynamic_content_settings.get_public_certificate_game_folder(context.config.game_directory_path)
    public_name = dynamic_content_settings.get_public_keyname(base_name, 'pem')
    out_public = os.path.join(public_out_folder, public_name)

    private_out_folder = dynamic_content_settings.get_private_certificate_game_folder(
        context.config.game_directory_path)
    private_name = dynamic_content_settings.get_private_keyname(base_name, 'pem')
    out_private = os.path.join(private_out_folder, private_name)

    return out_public, out_private


def generate_keys(context, base_name):
    new_key = RSA.generate(dynamic_content_settings.get_keysize())

    if base_name is None:
        base_name = dynamic_content_settings.get_default_keyname()

    public_out_folder = dynamic_content_settings.get_public_certificate_game_folder(context.config.game_directory_path)

    out_public, out_private = get_key_pair_paths(context, base_name)

    public_out_folder, public_name = os.path.split(out_public)

    if not os.path.exists(public_out_folder):
        os.makedirs(public_out_folder)

    public_key = new_key.publickey()
    with open(out_public, 'w') as public_file:
        public_file.write(public_key.exportKey('PEM').decode("utf-8"))
        public_file.close()

    private_out_folder, private_name = os.path.split(out_private)

    if not os.path.exists(private_out_folder):
        os.makedirs(private_out_folder)

    with open(out_private, 'w') as private_file:
        private_file.write(new_key.exportKey().decode("utf-8"))
        private_file.close()


def _load_key(context, base_name=None):
    keypath = os.path.join(
        dynamic_content_settings.get_private_certificate_game_folder(context.config.game_directory_path),
        dynamic_content_settings.get_private_keyname(base_name, 'pem'))

    if not os.path.exists(keypath):
        raise HandledError('Private Key File not found: {}'.format(keypath))

    import_key = RSA.importKey(open(keypath).read())

    signing_key = PKCS1_v1_5.new(import_key)
    return signing_key


def _load_public_key(context, base_name=None):
    keypath = os.path.join(
        dynamic_content_settings.get_public_certificate_game_folder(context.config.game_directory_path),
        dynamic_content_settings.get_public_keyname(base_name, 'pem'))

    if not os.path.exists(keypath):
        raise HandledError('Public Key File not found: {}'.format(keypath))

    import_key = RSA.importKey(open(keypath).read())

    signing_key = PKCS1_v1_5.new(import_key)
    return signing_key


def test_signature(context, args):
    signature = base64.b64decode(args.signature)
    to_sign = args.to_sign

    public_key = _load_public_key(context)
    if public_key is None:
        return

    hashedMessage = SHA256.new(to_sign.encode('utf8'))

    public_key = _load_public_key(context)

    show_manifest.show_signature_check(args.signature, to_sign, public_key.verify(hashedMessage, signature))


def get_signature(context, to_sign):
    signing_key = _load_key(context)

    if signing_key is None:
        raise HandledError('Unable to generate signing key')

    signature = signing_key.sign(to_sign)

    base64sig = base64.b64encode(signature)

    return base64sig
