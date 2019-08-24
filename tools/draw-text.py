#!/usr/bin/env python3
#
# Draw text with a JSON font to a PNG image.

import argparse
import json
import os
import sys

from PIL import Image, ImageOps


QUICK_BROWN_FOX = 'The quick brown fox jumps over the lazy dog.'

REPLACEMENT_CHARACTER = 0xfffd


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            'Draw text with a JSON font to a PNG image.'))

    parser.add_argument('font', help='Path to a JSON font')
    parser.add_argument('out_image', help='Output PNG image.')

    parser.add_argument(
        '-t', '--text', default=QUICK_BROWN_FOX,
        help=(
            'Text to render, or "-" to read it from stdin. Default '
            'is "The quick brown fox...".'))

    parser.add_argument(
        '-p', '--padding', metavar='EM', type=float, default=1.0,
        help=(
            'Image padding in em units (1 em = font size). '
            'Default is 1.'))

    parser.add_argument(
        '--transparent', action='store_true',
        help='Use transparent background instead of white.')

    args = parser.parse_args()

    if args.padding < 0:
        parser.error('Padding cannot be negative.')

    return args


def lookup_glyph(cp, glyph_lookup):
    for try_cp in (cp, REPLACEMENT_CHARACTER, ord('?')):
        glyph = glyph_lookup.get(try_cp)
        if glyph is not None:
            return glyph

    return None


def load_pages_for_text(text, font, load_dir, glyph_lookup):
    unique_cps = set(ord(c) for c in text)

    unique_page_indices = set()
    for cp in unique_cps:
        glyph = lookup_glyph(cp, glyph_lookup)
        if glyph is None:
            continue

        unique_page_indices.add(glyph['pageIndex'])

    pages = {}
    for page_idx in unique_page_indices:
        page_path = os.path.join(
            load_dir, font['pages'][page_idx]['name'])
        pages[page_idx] = Image.open(page_path)

    return pages


def get_image_size_for_text(text, font, glyph_lookup, kerning_lookup):
    num_lines = 1
    max_w = 0
    x = 0
    prev_cp = 0
    for cp in (ord(c) for c in text):
        if cp == ord('\n'):
            if max_w < x:
                max_w = x

            x = 0
            prev_cp = 0
            num_lines += 1

            continue

        glyph = lookup_glyph(cp, glyph_lookup)
        if glyph is None:
            prev_cp = 0
            continue

        x += kerning_lookup.get((prev_cp, cp), 0)
        prev_cp = cp
        x += glyph['advance']

    if max_w < x:
        max_w = x

    return max_w, num_lines * font['metrics']['lineHeight']


def render_text(
        text, image, pos,
        font, glyph_lookup, page_lookup, kerning_lookup):
    x, y = pos
    prev_cp = 0
    for cp in (ord(c) for c in text):
        if cp == ord('\n'):
            x = pos[0]
            y += font['metrics']['lineHeight']
            prev_cp = 0
            continue

        glyph = lookup_glyph(cp, glyph_lookup)
        if glyph is None:
            prev_cp = 0
            continue

        x += kerning_lookup.get((prev_cp, cp), 0)
        prev_cp = cp

        pos_x = glyph['pagePos']['x']
        pos_y = glyph['pagePos']['y']
        size_w = glyph['size']['w']
        size_h = glyph['size']['h']
        crop_box = (pos_x, pos_y, pos_x + size_w, pos_y + size_h)

        page = page_lookup[glyph['pageIndex']]
        glyph_image = page.crop(crop_box)
        draw_pos = (
            x + glyph['drawOffset']['x'], y + glyph['drawOffset']['y']
        )
        image.paste(glyph_image, draw_pos)

        x += glyph['advance']


def main():
    args = parse_args()

    with open(args.font, 'r', encoding='utf-8') as f:
        font = json.load(f)

    if args.text == '-':
        text = sys.stdin.read().rstrip('\n')
    else:
        text = args.text

    if not text:
        sys.exit('Text is empty.')

    glyph_lookup = {}
    for glyph in font['glyphs']:
        glyph_lookup[glyph['codePoint']] = glyph

    kerning_lookup = {}
    for kerning_pair in font['kerningPairs']:
        cp1 = kerning_pair['codePoint1']
        cp2 = kerning_pair['codePoint2']
        kerning_lookup[(cp1, cp2)] = kerning_pair['amount']

    page_lookup = load_pages_for_text(
        text, font, os.path.dirname(args.font), glyph_lookup)

    min_canvas_size = get_image_size_for_text(
        text, font, glyph_lookup, kerning_lookup)

    padding_px = round(
        font['bakingOptions']['fontPxSize'] * args.padding)
    canvas_size = (
        min_canvas_size[0] + padding_px * 2,
        min_canvas_size[1] + padding_px * 2
    )

    grayscale_mode = all(p.mode == 'L' for p in page_lookup.values())
    if grayscale_mode:
        canvas_mode = 'L'
        fill_color = 0
    else:
        canvas_mode = 'RGBA'
        fill_color = (255, 255, 255, 0)

    canvas = Image.new(canvas_mode, canvas_size, fill_color)
    render_text(
        text, canvas, (padding_px, padding_px),
        font, glyph_lookup, page_lookup, kerning_lookup)

    if grayscale_mode:
        if args.transparent:
            new_canvas = Image.new('LA', canvas.size, (0, 255))
            new_canvas.putalpha(canvas)
            canvas = new_canvas
        else:
            canvas = ImageOps.invert(canvas)
    elif not args.transparent:
        bg = Image.new('RGB', canvas.size, (255 ,255, 255))
        bg.paste(canvas, mask=canvas.split()[-1])
        canvas = bg

    out_image_path = args.out_image
    if os.path.splitext(out_image_path)[1].lower() != '.png':
        out_image_path += '.png'

    out_image_dir = os.path.dirname(out_image_path)
    if out_image_dir:
        os.makedirs(out_image_dir, exist_ok=True)

    canvas.save(out_image_path, optimize=True)


if __name__ == '__main__':
    main()
