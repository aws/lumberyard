class UserComment:
    #
    # Names of encodings that we publicly support.
    #
    ASCII = 'ascii'
    JIS = 'jis'
    UNICODE = 'unicode'
    ENCODINGS = (ASCII, JIS, UNICODE)

    #
    # The actual encodings accepted by the standard library differ slightly from
    # the above.
    #
    _JIS = 'shift_jis'
    _UNICODE = 'utf_16_be'

    _PREFIX_SIZE = 8
    #
    # From Table 9: Character Codes and their Designation
    #
    _ASCII_PREFIX = b'\x41\x53\x43\x49\x49\x00\x00\x00'
    _JIS_PREFIX = b'\x4a\x49\x53\x00\x00\x00\x00\x00'
    _UNICODE_PREFIX = b'\x55\x4e\x49\x43\x4f\x44\x45\x00'
    _UNDEFINED_PREFIX = b'\x00\x00\x00\x00\x00\x00\x00\x00'

    @classmethod
    def load(cls, data):
        """
        Convert "UserComment" value in exif format to str.

        :param bytes data: "UserComment" value from exif
        :return: u"foobar"
        :rtype: str(Unicode)
        :raises: ValueError if the data does not conform to the EXIF specification,
        or the encoding is unsupported.
        """
        if len(data) < cls._PREFIX_SIZE:
            raise ValueError('not enough data to decode UserComment')
        prefix = data[:cls._PREFIX_SIZE]
        body = data[cls._PREFIX_SIZE:]
        if prefix == cls._UNDEFINED_PREFIX:
            raise ValueError('prefix is UNDEFINED, unable to decode UserComment')
        try:
            encoding = {
                cls._ASCII_PREFIX: cls.ASCII, cls._JIS_PREFIX: cls._JIS, cls._UNICODE_PREFIX: cls._UNICODE,
            }[prefix]
        except KeyError:
            raise ValueError('unable to determine appropriate encoding')
        return body.decode(encoding, errors='replace')

    @classmethod
    def dump(cls, data, encoding="ascii"):
        """
        Convert str to appropriate format for "UserComment".

        :param data: Like u"foobar"
        :param str encoding: "ascii", "jis", or "unicode"
        :return: b"ASCII\x00\x00\x00foobar"
        :rtype: bytes
        :raises: ValueError if the encoding is unsupported.
        """
        if encoding not in cls.ENCODINGS:
            raise ValueError('encoding %r must be one of %r' % (encoding, cls.ENCODINGS))
        prefix = {cls.ASCII: cls._ASCII_PREFIX, cls.JIS: cls._JIS_PREFIX, cls.UNICODE: cls._UNICODE_PREFIX}[encoding]
        internal_encoding = {cls.UNICODE: cls._UNICODE, cls.JIS: cls._JIS}.get(encoding, encoding)
        return prefix + data.encode(internal_encoding, errors='replace')
