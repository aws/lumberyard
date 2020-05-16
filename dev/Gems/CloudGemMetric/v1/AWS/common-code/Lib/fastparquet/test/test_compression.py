from fastparquet.compression import (compress_data, decompress_data,
        compressions, decompressions)

import pytest


@pytest.mark.parametrize('fmt', compressions)
def test_compress_decompress_roundtrip(fmt):
    data = b'123' * 1000
    compressed = compress_data(data, compression=fmt)
    if fmt.lower() == 'uncompressed':
        assert compressed is data
    else:
        assert len(compressed) < len(data)

    decompressed = decompress_data(compressed, len(data), algorithm=fmt)
    assert data == decompressed


def test_compress_decompress_roundtrip_args_gzip():
    data = b'123' * 1000
    compressed = compress_data(
        data,
        compression={
            "type": "gzip",
            "args": {
                "compresslevel": 5,
            }
        }
    )
    assert len(compressed) < len(data)

    decompressed = decompress_data(compressed, len(data), algorithm="gzip")
    assert data == decompressed

def test_compress_decompress_roundtrip_args_lz4():
    pytest.importorskip('lz4')
    data = b'123' * 1000
    compressed = compress_data(
        data,
        compression={
            "type": "lz4",
            "args": {
                "compression": 5,
                "store_size": False,
            }
        }
    )
    assert len(compressed) < len(data)

    decompressed = decompress_data(compressed, len(data), algorithm="lz4")
    assert data == decompressed

def test_compress_decompress_roundtrip_args_zstd():
    pytest.importorskip('zstd')
    data = b'123' * 1000
    compressed = compress_data(
        data,
        compression={
            "type": "zstd",
            "args": {
                "level": 5,
                "threads": 0,
                "write_checksum": True,
                "write_dict_id": True,
                "write_content_size": False,
            }
        }
    )
    assert len(compressed) < len(data)

    decompressed = decompress_data(compressed, len(data), algorithm="zstd")
    assert data == decompressed

def test_errors():
    with pytest.raises(RuntimeError) as e:
        compress_data(b'123', compression='not-an-algorithm')

    assert 'not-an-algorithm' in str(e.value)
    assert 'gzip' in str(e.value).lower()


def test_not_installed():
    compressions.pop('BROTLI', None)
    with pytest.raises(RuntimeError) as e:
        compress_data(b'123', compression=4)
    assert 'brotli' in str(e.value).lower()
