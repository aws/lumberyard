"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

RemoteConsole: Used to interact with Lumberyard Launchers throught the Remote Console
to execute console commands.
"""

import socket
import time
import threading
import platform
import os

import logging

logger = logging.getLogger(__name__)


# List of all console command types, some are not being used in this
CONSOLE_MESSAGE_MAP = {
    'NOOP': '0',
    'REQ': '1',
    'LOGMESSAGE': '2',
    'LOGWARNING': '3',
    'LOGERROR': '4',
    'COMMAND': '5',
    'AUTOCOMPLETELIST': '6',
    'AUTOCOMPLETELISTDONE': '7',
    'GAMEPLAYEVENT': '22'
}


def in_game_screenshot(remote_console_instance, screenshot_name):
    # type: (RemoteConsole, str) -> None
    """
    Uses the remote console's screenshot command.
    :param remote_console_instance: RemoteConsole instance
    :param screenshot_name: Name you want the screenshot to be called
    :return: None
    """
    send_screenshot_command(remote_console_instance, 'capture_file_name {}'.format(screenshot_name),
                              '$3capture_file_name = $6{} $5[]$4'.format(screenshot_name))

    send_screenshot_command(remote_console_instance, 'capture_frame_once 1', '$3capture_frame_once = $61 $5[]$4')

    send_screenshot_command(remote_console_instance, 'capture_frames 1', '$3capture_frames = $61 $5[]$4')

    send_screenshot_command(remote_console_instance, 'capture_frames 0', '$3capture_frames = $60 $5[]$4')

    logger.info("Screenshot for {} has been taken".format(screenshot_name))


def capture_screenshot_command(remote_console_instance):
    # type: (RemoteConsole) -> None
    """
    Used for an in-game screenshot
    :param remote_console_instance: RemoteConsole instance
    :return: None
    """
    screenshot_command = 'Screenshot: ' + os.path.join('@user@', 'screenshots')
    send_screenshot_command(remote_console_instance, 'r_GetScreenShot 1', screenshot_command)
    logger.info("Screenshot has been taken.")


def send_screenshot_command(remote_console_instance, command_to_run, expected_log_line):
    # type: (RemoteConsole, str, str) -> None
    """
    This is just a helper function to help send and validate against screenshot console commands.
    :param remote_console_instance: RemoteConsole instance
    :param command_to_run: The screenshot command that you wish to run
    :param expected_log_line:  The console log line to expect in order to set the event to true
    :return: None
    """
    screenshotCommandSuccess = \
        remote_console_instance.expect_log_line(expected_log_line, 60)
    remote_console_instance.send_command(command_to_run)

    assert screenshotCommandSuccess(), '{} command failed. Was looking for {} in the log but did not find it.'.format(command_to_run, expected_log_line)


def _default_on_message_received(raw):
    # type: (str) -> None
    """
    This will just print the raw data from the message received.  We are striping white spaces before and after the
    message and logging it.  We are passing this function to the remote console instance as a default.  On any received
    message a user can overwrite the functionality with any function that they would like.
    :param raw: Raw string returned from the remote console
    :return: None
    """
    refined = raw.strip()
    if len(refined):
        logger.info(raw)


def _default_disconnect():
    # type: () -> None
    """
    On a disconnect a user can overwrite the functionality with any function, this one will just print to the
    logger a line 'Disconnecting from the Port.'
    :return: None
    """
    logger.info('Disconnecting from the Port')


class RemoteConsole:
    def __init__(self, addr='127.0.0.1', port=4600, on_disconnect=_default_disconnect,
                 on_message_received=_default_on_message_received):
        # type: (str, int, func, func) -> None
        """
        Creates a port connection using port 4600 to issue console commands and poll for specific console log lines.
        :param addr: The ip address where the launcher lives that we want to connect to
        :param port: The port where the remote console will be connecting to.  This usually starts at 4600, and is
        increased by one for each additional launcher that is opened
        :param on_disconnect: User can supply their own disconnect functionality if they would like
        :param on_message_received: on_message_received function in case they want their logging info handled in a
        different way
        """
        self.handlers = {}
        self.connected = False
        self.addr = addr
        self.port = port
        self.on_disconnect = on_disconnect
        self.on_display = on_message_received
        self.pump_thread = threading.Thread(target=self.pump)
        self.stop_pump = threading.Event()
        self.ready = threading.Event()

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def start(self, timeout=10):
        # type: (int) -> None
        """
        Starts the socket connection to the Launcher instance.
        :param timeout: The timeout in seconds for the pump thread to get ready before raising an exception.
        """
        if self.connected:
            logger.warning('RemoteConsole is already connected.')
            return

        self.socket.connect((self.addr, self.port))
        self.pump_thread.start()
        if not self.ready.wait(timeout):
            raise Exception("Remote console connection never became ready")
        self.connected = True
        logger.info('Remote Console Started')

    def stop(self):
        # type: () -> None
        """
        Stops and closes the socket connection to the Launcher instance.
        """
        if not self.connected:
            logger.warning('RemoteConsole is not connected, cannot stop.')
            return

        self.stop_pump.set()
        self.socket.shutdown(socket.SHUT_WR)
        self.socket.close()
        self.pump_thread.join()
        self.connected = False

    def send_command(self, command):
        # type: (str) -> None
        """
        Transforms and sends commands to the Launcher instance.
        :param command: The command to be sent to the Launcher instance
        """
        message = self._create_message(CONSOLE_MESSAGE_MAP['COMMAND'], command)
        try:
            self._send_message(message)
        except:
            self.on_disconnect()

    def pump(self):
        # type: () -> None
        """
        Pump function that is used by the pump_thread. Listens to receive messages from the socket
        and disconnects during an exception.
        """
        while not self.stop_pump.is_set():
            # Sending a NOOP message in order to get log lines
            self._send_message(self._create_message(CONSOLE_MESSAGE_MAP['NOOP']))
            try:
                self._handle_message(self.socket.recv(4096))
            except:
                self.on_disconnect()
                self.stop_pump.set()

    def expect_log_line(self, match_string, timeout=30):
        # type: (str, int) -> bool
        """
        Looks for a log line event to expect within a time frame. Returns False is timeout is reached.
        :param match_string: The string to match that acts as a key
        :param timeout: The timeout to wait for the log line in seconds
        """
        evt = threading.Event()
        self.handlers[match_string] = evt

        def log_event():
            logger.info("waiting for event '{0}'".format(match_string))
            return evt.wait(timeout)

        return log_event

    def _create_message(self, message_type, message_body=''):
        # type: (int, str) -> bytearray
        """
        Transforms a message to be sent to the launcher. The string is converted to a bytearray and
        is prepended with the message type and appended with an ending 0.
        :param message_type: Uses CONSOLE_MESSAGE_MAP to prepend the bytearray message
        :param message_body: The message string to be converted
        """
        message = bytearray(0)
        message.append(message_type)

        for char in message_body:
            message.append(char)

        message.append(0)
        return message

    def _send_message(self, message):
        # type: (bytearray) -> None
        """
        Sends console commands through the socket connection to the launcher. The message string should
        first be transformed into a bytearray.
        :param message: The message to be sent to the Launcher instance
        """
        self.socket.sendall(message)

    def _handle_message(self, message):
        # type: (bytearray) -> None
        """
        Handles the messages and and will poll for expected console messages that we are looking for and set() events to True.
        Displays the message if we determine it is a logging message.
        :param message: The message received to be handled in various ways
        """
        # message[0] is the representation of the message type inside of the message received from our launchers
        message_type = message[0]

        # ignoring the first byte and the last byte.  The first byte is the message type and the last byte is a
        # Null terminator
        message_body = message[1:-1]

        # display the message if it's a logging message type
        if CONSOLE_MESSAGE_MAP['LOGMESSAGE'] <= message_type <= CONSOLE_MESSAGE_MAP['LOGERROR']:
            self.on_display(message_body)
            for key in self.handlers.keys():
                # message received, set flag handler as True for success
                if key in message_body:
                    self.handlers[key].set()
                    continue

        # The very first connection using the socket will return all of the auto complete items, turned off so no one
        # wouldn't need to see them
        elif message_type == CONSOLE_MESSAGE_MAP['AUTOCOMPLETELIST']:
            pass

        # The after the autocompletelists finishes we will be ready to send console commands we determine that by
        # looking at for an autocompletelistdone message
        elif message_type == CONSOLE_MESSAGE_MAP['AUTOCOMPLETELISTDONE']:
            self.ready.set()

        # cleanup expect_log_line handers if the matching string was found or timeout happened.
        for key in self.handlers.keys():
            if self.handlers[key].is_set():
                del self.handlers[key]
