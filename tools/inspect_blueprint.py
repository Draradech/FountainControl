#!/usr/bin/env python3
"""Inspect native blueprint PNG metadata; check the included real export fixtures.

Only fcRs is decoded. The opaque native building tail is retained in the hash,
not interpreted. Native import still checks the actual building count.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


def inspect(path):
    data = path.read_bytes()
    if len(data) > 134217728 or data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('Not a supported-size PNG')
    canonical = bytearray(data[:8])
    chunks = []
    pos = 8
    while True:
        if pos + 12 > len(data):
            raise ValueError('Truncated PNG')
        length = struct.unpack_from('>I', data, pos)[0]
        end = pos + 12 + length
        if end > len(data):
            raise ValueError('Truncated PNG chunk')
        kind = data[pos + 4:pos + 8]
        if kind == b'fcRs':
            if length > 1048576:
                raise ValueError('Oversized metadata')
            crc = struct.unpack_from('>I', data, end - 4)[0]
            chunks.append((data[pos + 8:end - 4], crc == zlib.crc32(data[pos + 4:end - 4])))
        else:
            canonical.extend(data[pos:end])
        pos = end
        if kind == b'IEND':
            canonical.extend(data[pos:])
            break
    digest = hashlib.sha256(canonical).hexdigest()
    report = {'file': path.name, 'sha256': hashlib.sha256(data).hexdigest(),
              'canonical_sha256': digest, 'native_footer_hex': data[-4:].hex(),
              'metadata_chunks': len(chunks), 'metadata_valid': False}
    if not chunks:
        return report
    if len(chunks) != 1 or not chunks[0][1]:
        raise ValueError('Duplicate metadata or incorrect CRC')
    words = chunks[0][0].decode('utf-8').split()
    if len(words) < 5 or words[:2] != ['FountainControlBlueprint', '1'] or words[2] != digest:
        raise ValueError('Invalid magic, schema or fingerprint')
    buildings, count = int(words[3]), int(words[4])
    if not 0 <= count <= buildings <= 100000 or len(words) != 5 + count * 3:
        raise ValueError('Invalid counts')
    settings = []
    seen = set()
    for pos in range(5, len(words), 3):
        index = int(words[pos])
        drop, horizontal = map(float, words[pos + 1:pos + 3])
        if index in seen or not 0 <= index < buildings:
            raise ValueError('Invalid or duplicate index')
        if not 0 <= drop <= 1000 or drop % 10 or not (horizontal == -1 or 0 <= horizontal <= 500 and horizontal % 10 == 0):
            raise ValueError('Invalid limits')
        seen.add(index)
        settings.append({'index': index, 'drop': drop, 'horizontal': horizontal})
    report.update(metadata_valid=True, building_count=buildings, settings_cm=settings)
    return report


def check_fixtures():
    folder = Path(__file__).resolve().parents[1] / 'tests/fixtures/blueprints'
    manifest = json.loads((folder / 'manifest.json').read_text())
    for name, digest in manifest['files'].items():
        report = inspect(folder / name)
        assert report['sha256'] == digest, name
        assert report['canonical_sha256'] == manifest['canonical_sha256'], name
        assert report['native_footer_hex'] == manifest['native_footer_hex'], name
        if name == 'fountains-with-settings.png':
            assert report['metadata_valid'] and report['building_count'] == manifest['building_count']
            assert report['settings_cm'] == manifest['settings_cm']
        else:
            assert report['metadata_chunks'] == 0
    print('Real native blueprint fixtures: CRC, metadata, canonical fingerprint and unchanged tail passed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('file', nargs='?', type=Path)
    parser.add_argument('--check-fixtures', action='store_true')
    args = parser.parse_args()
    if args.check_fixtures:
        check_fixtures()
    elif args.file:
        try:
            print(json.dumps(inspect(args.file), indent=2))
        except (ValueError, UnicodeError) as error:
            parser.exit(1, f'Invalid blueprint metadata: {error}\n')
    else:
        parser.error('Supply a PNG or --check-fixtures')
