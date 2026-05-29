#!/usr/bin/env python3

import html
import pathlib
import sys


def main() -> int:
    if len(sys.argv) < 5 or len(sys.argv[3:]) % 2 != 0:
        print('Usage: html_generator.py <template> <output> <KEY VALUE>...', file=sys.stderr)
        return 1

    template_path = pathlib.Path(sys.argv[1])
    output_path = pathlib.Path(sys.argv[2])
    replacements = dict(zip(sys.argv[3::2], sys.argv[4::2]))

    for key in ('PAGE_CODE', 'CODE'):
        value = replacements.get(key)
        if value is not None:
            replacements[key] = html.escape(pathlib.Path(value).read_text()).strip()

    content = template_path.read_text()
    for key, value in replacements.items():
        content = content.replace(f'@{key}@', value)

    output_path.write_text(content)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
