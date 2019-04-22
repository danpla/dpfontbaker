#!/usr/bin/env python3
#
# Extract Unicode code points from text files.

import argparse
import sys
import io
from collections import namedtuple
import enum


# [first, last]
CpRange = namedtuple('CpRange', 'first last')


@enum.unique
class CpFormat(enum.Enum):
    HEX = 0
    DEC = 1


def cp_to_str(cp, cp_format):
    if cp_format == CpFormat.HEX:
        return 'U+{:04X}'.format(cp)
    else:
        return str(cp)


def cp_range_to_str(cp_range, cp_format):
    assert cp_range.first <= cp_range.last

    if cp_range.first == cp_range.last:
        return cp_to_str(cp_range.first, cp_format)
    else:
        return '{}-{}'.format(
            cp_to_str(cp_range.first, cp_format),
            cp_to_str(cp_range.last, cp_format))


def cp_range_list_to_str(cp_range_list, cp_format):
    return ', '.join(
        cp_range_to_str(c, cp_format) for c in cp_range_list)


def create_cp_range_list(cp_set):
    cp_list = list(cp_set)
    cp_list.sort()

    cp_range_list = []
    i = 0
    while i < len(cp_list):
        cp_first = cp_list[i]
        cp_second = cp_first
        i += 1

        while i < len(cp_list):
            cp = cp_list[i]
            if cp > cp_second + 1:
                break

            cp_second = cp
            i += 1

        cp_range_list.append(CpRange(cp_first, cp_second))

    return cp_range_list


def parse_args():
    parser = argparse.ArgumentParser(
        description='Extract Unicode code points from text files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            'The full list of supported encodings can be found at\n'
            '    https://docs.python.org/3.{}/library/codecs.html'
            '#standard-encodings\n'
            'Be aware that the encoding option is intentionally '
            'ignored for stdin,\n'
            'as its encoding depends on the platform.'
            ''.format(sys.version_info.minor)))

    parser.add_argument(
        'files', nargs=argparse.REMAINDER,
        help='List of files or "-" to read from stdin.')

    parser.add_argument(
        '-e', '--encoding', default='utf-8',
        help='Encoding of files. Default is "utf-8".')

    parser.add_argument(
        '-f', '--cp-format', choices=('hex', 'dec'), default='hex',
        help='Format of code points. Default is "hex".')

    args = parser.parse_args()
    if not args.files:
        parser.error('You must provide at least one file.')

    return args


def main():
    args = parse_args()

    cp_set = set()
    for file in args.files:
        if file == '-':
            cp_set.update(ord(c) for c in sys.stdin.read())
        else:
            with open(file, 'r', encoding=args.encoding) as f:
                cp_set.update(ord(c) for c in f.read())

    cp_set.discard(ord('\n'))

    print(
        cp_range_list_to_str(
            create_cp_range_list(cp_set),
            CpFormat[args.cp_format.upper()]))


if __name__ == '__main__':
    main()
