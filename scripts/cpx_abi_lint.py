#!/usr/bin/env python3
"""Lexically guard CPX callback calls against the native foreign ABI.

This is a source-level check, not a C parser or type-aware analysis. It checks
member spellings from cpxhead.h after removing comments and literals; macro or
preprocessor indirection and aliases formed without a visible member expression
cannot be proven safe. Keep the callback inventory in sync with cpxhead.h.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys

DEFAULT_SOURCES = (
    Path(__file__).resolve().parent.parent / "MagiC/APPS/CPX/MAGXCONF.C",
)

# Function-pointer members in CPXINFO and XCPB (cpxhead.h).
CALLBACKS = frozenset({
    "cpx_call", "cpx_draw", "cpx_wmove", "cpx_timer", "cpx_key",
    "cpx_button", "cpx_m1", "cpx_m2", "cpx_hook", "cpx_close",
    "get_cpx_list", "save_header", "rsh_fix", "rsh_obfix", "Popup",
    "Sl_size", "Sl_x", "Sl_y", "Sl_arrow", "Sl_dragx", "Sl_dragy",
    "Xform_do", "GetFirstRect", "GetNextRect", "Set_Evnt_Mask",
    "XGen_Alert", "CPX_Save", "Get_Buffer", "getcookie", "MFsave",
})
BRIDGES = {
    "Xform_do": "cpx_xform_do",
    "rsh_obfix": "cpx_rsh_obfix",
    "XGen_Alert": "cpx_xgen_alert",
    "GetFirstRect": "cpx_call2",
    "GetNextRect": "cpx_call2",
    "getcookie": "cpx_call2",
    "Get_Buffer": "cpx_call2",
    "CPX_Save": "cpx_call2",
}
BRIDGE_NAMES = frozenset(BRIDGES.values())

@dataclass(frozen=True)
class Token:
    text: str
    line: int

TOKEN_RE = re.compile(r"(?:[A-Za-z_][A-Za-z_0-9]*|(?:0[xX][0-9A-Fa-f]+|[0-9]+)|->|.)", re.S)


def tokenize(source: str) -> list[Token]:
    """Return C-ish tokens, omitting comments and quoted literals."""
    result: list[Token] = []
    i = 0
    line = 1
    n = len(source)
    while i < n:
        c = source[i]
        if c.isspace():
            if c == "\n":
                line += 1
            i += 1
            continue
        if source.startswith("//", i):
            end = source.find("\n", i)
            i = n if end < 0 else end
            continue
        if source.startswith("/*", i):
            end = source.find("*/", i + 2)
            stop = n if end < 0 else end + 2
            line += source[i:stop].count("\n")
            i = stop
            continue
        if c in "\"'":
            quote = c
            i += 1
            while i < n:
                if source[i] == "\\":
                    if i + 1 < n and source[i + 1] == "\n":
                        line += 1
                    i += 2
                elif source[i] == quote:
                    i += 1
                    break
                else:
                    if source[i] == "\n":
                        line += 1
                    i += 1
            continue
        m = TOKEN_RE.match(source, i)
        assert m
        text = m.group()
        result.append(Token(text, line))
        i = m.end()
    return result


def pairs(tokens: list[Token]) -> dict[int, int]:
    matches: dict[int, int] = {}
    stack: list[tuple[str, int]] = []
    close = {")": "(", "]": "[", "}": "{"}
    for i, token in enumerate(tokens):
        if token.text in "([{" and len(token.text) == 1:
            stack.append((token.text, i))
        elif token.text in close:
            if stack and stack[-1][0] == close[token.text]:
                _, start = stack.pop()
                matches[start] = i
                matches[i] = start
    return matches


def lint_text(source: str) -> list[tuple[int, str]]:
    tokens = tokenize(source)
    matching = pairs(tokens)
    approved: set[int] = set()
    # Recognize exact first arguments: bridge((void *) receiver->member, ...).
    for i, token in enumerate(tokens[:-1]):
        if token.text not in BRIDGE_NAMES or tokens[i + 1].text != "(":
            continue
        end = matching.get(i + 1)
        if end is None:
            continue
        # Skip balanced groups so commas inside casts, receivers or indexes do
        # not terminate the first argument.
        comma = i + 2
        while comma < end and tokens[comma].text != ",":
            if tokens[comma].text in ("(", "[", "{"):
                comma = matching.get(comma, end) + 1
            else:
                comma += 1
        start = i + 2
        if [t.text for t in tokens[start:start + 4]] != ["(", "void", "*", ")"]:
            continue
        start += 4
        stop = comma
        while start < stop and tokens[start].text == "(" and matching.get(start) == stop - 1:
            start += 1
            stop -= 1
        member_i = stop - 1
        if member_i <= start or tokens[member_i].text not in CALLBACKS:
            continue
        if tokens[member_i - 1].text not in ("->", "."):
            continue
        if BRIDGES.get(tokens[member_i].text) != token.text:
            continue
        # Permit only receiver/access tokens, not a transformed callback value.
        receiver = tokens[start:member_i - 1]
        if not receiver or any(
            not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*|[0-9]+|->|[.()\[\]*]", t.text)
            for t in receiver
        ):
            continue
        approved.add(member_i)

    errors: list[tuple[int, str]] = []
    for i, token in enumerate(tokens):
        if token.text not in CALLBACKS or i == 0 or tokens[i - 1].text not in ("->", "."):
            continue
        if i in approved:
            continue
        errors.append((token.line, f"callback member {token.text} must be passed as the first argument to {BRIDGES.get(token.text, 'an approved bridge')} with a (void *) cast"))
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="*", type=Path, help="C source paths (default: MAGXCONF.C)")
    args = parser.parse_args(argv)
    files = args.files or DEFAULT_SOURCES
    failures = 0
    for path in files:
        try:
            errors = lint_text(path.read_text(encoding="utf-8", errors="replace"))
        except OSError as exc:
            print(f"{path}: {exc}", file=sys.stderr)
            failures += 1
            continue
        for line, message in errors:
            print(f"{path}:{line}: {message}", file=sys.stderr)
        failures += len(errors)
    if failures:
        return 1
    print(f"CPX ABI lint: {len(files)} file(s) clean")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
