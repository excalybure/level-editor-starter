#!/usr/bin/env python3
import argparse
import base64
import re
import struct
import sys
from typing import List

FLOAT_PATTERN = re.compile(
    r"""
    (?P<num>
        [+-]?              # optional sign
        (?:
            (?:\d+\.\d*|\.\d+|\d+)   # decimal forms
        )
        (?:[eE][+-]?\d+)?  # optional exponent
    )
    """,
    re.VERBOSE,
)

def extract_floats(text: str) -> List[float]:
    """Extract floats from arbitrary text (supports spaces, commas, JSON-ish lists)."""
    return [float(m.group("num")) for m in FLOAT_PATTERN.finditer(text)]

def floats_to_bytes(values: List[float], dtype: str = "f32", endian: str = "little") -> bytes:
    if dtype not in ("f32", "f64"):
        raise ValueError("dtype must be 'f32' or 'f64'")
    if endian not in ("little", "big"):
        raise ValueError("endian must be 'little' or 'big'")
    fmt_char = "f" if dtype == "f32" else "d"
    prefix = "<" if endian == "little" else ">"
    fmt = f"{prefix}{len(values)}{fmt_char}"
    return struct.pack(fmt, *values)

def floats_to_data_uri(values: List[float], dtype: str = "f32", endian: str = "little") -> str:
    raw = floats_to_bytes(values, dtype=dtype, endian=endian)
    b64 = base64.b64encode(raw).decode("ascii")
    return f"data:application/octet-stream;base64,{b64}"

def main():
    p = argparse.ArgumentParser(
        description="Convert a sequence of floats to data:application/octet-stream;base64"
    )
    p.add_argument(
        "numbers",
        nargs="*",
        help="Numbers (space- and/or comma-separated). If omitted or '-', read from stdin.",
    )
    p.add_argument("--dtype", choices=["f32", "f64"], default="f32", help="Float width (default: f32)")
    p.add_argument("--endian", choices=["little", "big"], default="little", help="Byte order (default: little)")
    args = p.parse_args()

    # Gather input text
    if not args.numbers or (len(args.numbers) == 1 and args.numbers[0] == "-"):
        text = sys.stdin.read()
    else:
        text = " ".join(args.numbers)

    values = extract_floats(text)
    if not values:
        sys.stderr.write("No numbers found. Provide floats as args or via stdin.\n")
        sys.exit(1)

    uri = floats_to_data_uri(values, dtype=args.dtype, endian=args.endian)
    print(uri)

if __name__ == "__main__":
    main()
