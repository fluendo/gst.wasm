#!/usr/bin/env python3
import sys
import html

if len(sys.argv) < 4 or len(sys.argv[3:]) % 2 != 0:
    print("Usage: ./gen_page.py template.html output.html KEY1 VAL1 [KEY2 VAL2 ...]")
    sys.exit(1)

template_file = sys.argv[1]
output_file = sys.argv[2]
args = sys.argv[3:]

with open(template_file) as f:
    template = f.read()

for i in range(0, len(args), 2):
    key = args[i]
    value = args[i + 1].strip()

    try:
        with open(value) as f:
            value = f.read().strip()
            if key != "PAGE_CODE":
                value = html.escape(value)
    except:
        pass

    template = template.replace(f"@{key}@", value)

with open(output_file, "w") as f:
    f.write(template)
