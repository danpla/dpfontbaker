#!/usr/bin/env python3
#
# Join glyph images separated by split.py back to pages.

import argparse
import json
import os
import sys

from PIL import Image


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            'Join glyph images separated by split.py back to pages.'))

    parser.add_argument('font', help='Path to a JSON font')

    parser.add_argument(
        '-i', '--in-dir', default='',
        help=(
            'Input directory of images. By default, images will be '
            'loaded from the directory of the font.'))

    parser.add_argument(
        '--name-format', choices=('hex', 'dec'), default='hex',
        help='Input format of image names. Default is "hex".')

    args = parser.parse_args()
    return args


def main():
    args = parse_args()

    with open(args.font, 'r', encoding='utf-8') as f:
        font = json.load(f)

    font_dirname = os.path.dirname(args.font)

    page_images = []
    for page in font['pages']:
        size = (page['size']['w'], page['size']['h'])
        page_images.append(
            Image.new('RGBA', size, (255, 255, 255, 0)))

    in_dir = args.in_dir or font_dirname

    if args.name_format == 'hex':
        name_format = 'U+{:08X}'
    else:
        name_format = '{{:0{}}}'.format(len('10ffff'))

    for glyph in font['glyphs']:
        size_w = glyph['size']['w']
        size_h = glyph['size']['h']
        if size_w == 0 or size_h == 0:
            # Space
            continue

        cp = glyph['codePoint']
        image_path = (
            os.path.join(in_dir, name_format.format(cp)) + '.png')
        if not os.path.isfile(image_path):
            sys.exit('{} doesn\'t exist'.format(image_path))

        im = Image.open(image_path)
        if im.size != (size_w, size_h):
            sys.exit(
                'Size of {} ({}x{}) doesn\'t match size in the font '
                '({}x{})'.format(
                    image_path, im.width, im.height, size_w, size_h))

        page_index = glyph['pageIndex']
        pos_x = glyph['pagePos']['x']
        pos_y = glyph['pagePos']['y']
        page_images[page_index].paste(im, (pos_x, pos_y))

    for page, page_image in zip(font['pages'], page_images):
        page_path = os.path.join(font_dirname, page['name'])
        page_image.save(page_path, optimize=True)


if __name__ == '__main__':
    main()
