#!/usr/bin/env python3
#
# Add alpha to grayscale images, threating black as transparent and
# white as opaque. This is the same as applying the input image as a
# mask to a white image of the same size.

import argparse
import os

from PIL import Image


def gray_to_alpha(in_path, out_path, force_rgba):
    in_im = Image.open(in_path)
    if in_im.mode != 'L':
        return

    if force_rgba:
        mode = 'RGBA'
        fill_color = (255, 255, 255, 255)
    else:
        mode = 'LA'
        fill_color = (255, 255)

    out_im = Image.new(mode, in_im.size, fill_color)
    out_im.putalpha(in_im)
    out_im.save(out_path, optimize=True)


def get_output_path(in_path, out_dir, out_format):
    if not out_dir:
        out_dir = os.path.dirname(in_path)

    in_basename = os.path.basename(in_path)
    in_name, in_ext = os.path.splitext(in_basename)

    if out_format:
        out_ext = '.' + out_format.lower()
    else:
        out_ext = in_ext

    return os.path.join(out_dir, in_name + out_ext)


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            'Add alpha to grayscale images, threating black as '
            'transparent and white as opaque. This is the same as '
            'applying the input image as a mask to a white image of '
            'the same size.'))

    parser.add_argument(
        'images', nargs=argparse.REMAINDER, help='List of images')

    parser.add_argument(
        '-o', '--out-dir', default='',
        help=(
            'Output directory for images. By default, every image is '
            'written to the directory of the original.'))

    parser.add_argument(
        '-f', '--format', default='',
        help=(
            'Output format for images. By default, every image will '
            'have the same format as the original.'))

    parser.add_argument(
        '--force-rgba', action='store_true',
        help=(
            'Force image to RGBA. By default, the images will be '
            'in gray+alpha mode (2 bytes per pixel), if supported '
            'by the output image format.'))

    parser.add_argument(
        '--allow-overwrite', action='store_true',
        help='Allow to overwrite existing images.')

    args = parser.parse_args()
    if not args.images:
        parser.error('You must provide at least one image.')

    return args


def main():
    args = parse_args()

    if args.out_dir:
        os.makedirs(args.out_dir, exist_ok=True)

    for in_path in args.images:
        out_path = get_output_path(in_path, args.out_dir, args.format)

        if not args.allow_overwrite and os.path.exists(out_path):
            print(
                '{} already exists. Use --allow-overwrite or choose '
                'a different --out-dir.'.format(out_path))
            continue

        gray_to_alpha(in_path, out_path, args.force_rgba)


if __name__ == '__main__':
    main()
