#!/usr/bin/python3

import re
import sys

QSTRING_RE = re.compile(r'extern const QString ([A-Za-z0-9_]+)')

def snake_to_camel_case(s: str):
    """Convert snake_case/SNAKE_CASE string to camelCase"""
    parts = s.split('_')
    return parts[0].lower() + ''.join(x.title() for x in parts[1:])

def generate(header: str, source: str):
    fields: list[str] = []
    types: list[str] = []

    with open(header) as f:
        for line in f:
            if line.strip() == "// --- Fields":
                break

        past_types = False
        for line in f:
            if line.strip() == "// --- Types":
                past_types = True
                continue
            if m := QSTRING_RE.match(line):
                (types if past_types else fields).append(m[1])

    text = ''
    with open(source) as f:
        no_auto_generated = True
        for line in f:
            text += line
            if line.strip().startswith('// Auto-generated file part'):
                no_auto_generated = False
                break

        if no_auto_generated:
            text += '// Auto-generated file part\n'

    with open(source, 'w') as f:
        f.write(text)

        write_string = lambda const, value: f.write(f'const QString {const} = QStringLiteral("{value}");\n')

        if fields:
            f.write('\n// Fields\n\n')
            for field in fields:
                write_string(field, field.lower())

        if types:
            f.write('\n// Types\n\n')
            for type in types:
                value = type.lower()
                if value.startswith('type_'):
                    value = value[5:]
                write_string(type, snake_to_camel_case(value))

if __name__ == '__main__':
    if len(sys.argv) == 3:
        generate(*sys.argv[1:])
    else:
        print(f"Usage: {sys.argv[0]} path/to/tdconst.h path/to/tdconst.cpp")