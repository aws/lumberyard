import json
import os
import zipfile
import wave

import resource_manager.cli
from resource_manager.errors import HandledError

CACHE_FOLDER = 'ttscache'
CHARACTER_MAPPINGS_FILE_NAME = 'character_mapping.json'

def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("cloud-gem-tts", help="Commands to help import content packages from CloudGemTextToSpeech back end")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    tts_subparsers = subparser.add_subparsers(dest = 'subparser_name')
    subparser = tts_subparsers.add_parser('import-tts-zip', help='Import files that were generated using CloudGemTextToSpeech')
    subparser.add_argument('--download-path', required=True, help='Absolute path to the zip file downloaded from CloudGemTextToSpeech CGP')
    subparser.add_argument('--import-as-wav', required=False, action='store_true', help="Convert audio files to .wav format when extracting")
    subparser.set_defaults(func=import_zip)

def import_zip(context, args):
    print "extracting zip at {} into {} project assets".format(args.download_path, context.config.get_game_directory_name())
    # make sure we have a zip
    if not os.path.exists(args.download_path):
        raise HandledError("Provided path does not exist")

    # make sure directory is there for extraction
    out_path = os.path.join(context.config.game_directory_path, CACHE_FOLDER)
    if not os.path.isdir(out_path):
        os.makedirs(out_path)
    
    # hold onto character mappings so as to not lose existing ones in an overwrite
    existing_mappings = {}
    mappings_file_path = os.path.join(out_path, CHARACTER_MAPPINGS_FILE_NAME)
    if os.path.exists(mappings_file_path):
        with open(mappings_file_path, 'r') as mappings_file:
            existing_mappings = json.load(mappings_file)

    # extract
    if args.import_as_wav:
        extract_as_wav(args.download_path, out_path)
    else:
        extract_all(args.download_path, out_path)

    # If we had mappings before extraction merge them with the new ones
    if existing_mappings:
        __merge_character_mappings(existing_mappings, mappings_file_path)

def __merge_character_mappings(mappings, file_path):
    new_mappings = {}
    if os.path.exists(file_path):
        with open(file_path, 'r') as mappings_file:
            new_mappings = json.load(mappings_file)

    has_edits = False
    for key in mappings:
        if not key in new_mappings:
            new_mappings[key] = mappings[key]
            has_edits = True

    if has_edits:
        with open(file_path, 'w') as out_file:
            json.dump(new_mappings, out_file, indent=4)

def extract_all(download_path, out_path):
    with open(download_path, 'rb') as file_handle:
        zip_file = zipfile.ZipFile(file_handle)
        zip_file.extractall(out_path)

def extract_as_wav(download_path, out_path):
    zip_file = zipfile.ZipFile(download_path)
    for archive_file in zip_file.namelist():
        if not '.pcm' in archive_file:
            zip_file.extract(archive_file, out_path)
        else:
            write_wav_from_pcm(zip_file, archive_file, out_path)

def write_wav_from_pcm(zip, file_name, out_path):
    print "converting {}".format(file_name)

    wav_name = file_name.replace(".pcm", ".wav")
    out_path = os.path.join(out_path, wav_name)
    wav_file = wave.open(out_path, "w")

    data_file = zip.open(file_name)
    data = data_file.read()
    data_file.close()


    wav_file.setnchannels(1)
    wav_file.setframerate(16000)
    wav_file.setsampwidth(2)
    wav_file.writeframes(data)
    wav_file.close()