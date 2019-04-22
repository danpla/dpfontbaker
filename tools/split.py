#!/usr/bin/env python3
#
# Split glyph pages of a JSON font, putting every glyph on a separate
# PNG image. You can later assemble them back with join.py.

import argparse
import json
import os

from PIL import Image


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            'Split glyph pages of a JSON font, putting every glyph '
            'on a separate PNG image. You can later assemble them '
            'back with join.py.'))

    parser.add_argument('font', help='Path to a JSON font.')

    parser.add_argument(
        '-o', '--out-dir', default='',
        help=(
            'Output directory for images. By default, images are '
            'written to the directory of the font.'))

    parser.add_argument(
        '--name-format', choices=('hex', 'dec'), default='hex',
        help='Output format for image names. Default is "hex".')

    args = parser.parse_args()
    return args


def main():
    args = parse_args()

    with open(args.font, 'r', encoding='utf-8') as f:
        font = json.load(f)

    font_dirname = os.path.dirname(args.font)

    page_images = []
    for page in font['pages']:
        page_path = os.path.join(font_dirname, page['name'])
        page_images.append(Image.open(page_path))

    out_dir = args.out_dir or font_dirname
    os.makedirs(out_dir, exist_ok=True)

    if args.name_format == 'hex':
        name_format = 'U+{:08X}'
    else:
        name_format = '{{:0{}}}'.format(len('10ffff'))

    for glyph in font['glyphs']:
        cp = glyph['codePoint']
        image_path = (
            os.path.join(out_dir, name_format.format(cp)) + '.png')
        if os.path.exists(image_path):
            print('{} already exists'.format(image_path))
            continue

        page_index = glyph['pageIndex']
        pos_x = glyph['pagePos']['x']
        pos_y = glyph['pagePos']['y']
        size_w = glyph['size']['w']
        size_h = glyph['size']['h']
        if size_w == 0 or size_h == 0:
            # Space
            continue

        crop_box = (pos_x, pos_y, pos_x + size_w, pos_y + size_h)
        page_images[page_index].crop(crop_box).save(
            image_path, optimize=True)


if __name__ == '__main__':
    main()
