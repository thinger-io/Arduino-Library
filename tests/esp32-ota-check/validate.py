"""Compile metadata declarations, including expected failures, without hardware.

Usage: python validate.py --compiler /path/to/xtensa-esp32-elf-g++
"""
import argparse
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--compiler", required=True)
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]

cases = [
    ("legacy", None, None),
    ("valid", [('PCB_MODEL', 'SENSOR_CURRENT'), ('PCB_VERSION', 'V2')], None),
    ("empty value", [('key', '')], None),
    ("boundary lengths", [('k' * 64, 'v' * 256)], None),
    ("32 fields", [(f'k{i}', 'v') for i in range(32)], None),
    ("duplicate keys", [('key', 'a'), ('key', 'b')], 'keys must be unique'),
    ("empty key", [('', 'value')], 'keys must contain'),
    ("long key", [('k' * 65, 'v')], 'keys must contain'),
    ("long value", [('k', 'v' * 257)], 'values must contain'),
    ("embedded NUL", [('key', r'v\0hidden')], 'cannot contain NUL'),
    ("empty fields", [], 'requires 1 to 32'),
    ("too many fields", [(f'k{i}', 'v') for i in range(33)], 'requires 1 to 32'),
    ("oversized payload", [(f'k{i}', 'v' * 256) for i in range(20)], 'cannot exceed 4096'),
]

with tempfile.TemporaryDirectory() as temp:
    source = pathlib.Path(temp) / 'check.cpp'
    output = pathlib.Path(temp) / 'check.o'
    for name, fields, expected in cases:
        declaration = '' if fields is None else '#define THINGER_OTA_CHECK(FIELD) ' + ' '.join(
            f'FIELD("{key}", "{value}")' for key, value in fields) + '\n'
        source.write_text(declaration + '#include "ThingerOTACheck.h"\n', encoding='utf-8')
        result = subprocess.run([args.compiler, '-std=gnu++11', '-I', str(root / 'src'),
                                 '-c', str(source), '-o', str(output)], capture_output=True, text=True)
        if expected is None:
            assert result.returncode == 0, f'{name}: {result.stderr}'
        else:
            assert result.returncode != 0 and expected in result.stderr, f'{name}: {result.stderr}'
        print(f'PASS: {name}')

    # Multiple source files must not create multiple discoverable manifests.
    objects = []
    for index in range(2):
        source.write_text(
            '#define THINGER_OTA_CHECK(FIELD) FIELD("PCB_MODEL", "SENSOR_CURRENT")\n'
            '#include "ThingerOTACheck.h"\n'
            f'const char* get_manifest_{index}() {{ return thinger_ota_check::definition<>::manifest; }}\n',
            encoding='utf-8')
        obj = pathlib.Path(temp) / f'check{index}.o'
        result = subprocess.run([args.compiler, '-std=gnu++11', '-I', str(root / 'src'),
                                 '-c', str(source), '-o', str(obj)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        objects.append(str(obj))
    result = subprocess.run([args.compiler, '-r', *objects, '-o', str(output)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert output.read_bytes().count(b'@THINGER_OTA_CHECK:1@\0') == 1
    print('PASS: one manifest across multiple translation units')
