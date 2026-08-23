#!/usr/bin/env python3
"""Standalone clone of Clang's ``-verify`` diagnostic checker.

Reference:
    https://clang.llvm.org/docs/InternalsManual.html#verifying-diagnostics

Usage:
    gta3sc ... 2>&1 | verify-diagnostics.py source.sc

Supported annotations (in ``//`` comments):
    // expected-error {{message substring}}
    // expected-warning {{message substring}}
    // expected-error-re {{regex}}
    // expected-warning-re {{regex}}
    // expected-error@+N {{...}}   (relative line offset)
    // expected-error@file:N {{...}}   (explicit file and line)
    // expected-no-diagnostics

Intentionally minimal (e.g. multi-line comments are ignored).

MIT License, Copyright (c) 2016 Denilson das Merces Amorim.
"""
import os
import re
import sys
from dataclasses import dataclass

# Groups: (kind)(-re?)(file?)(@line-or-offset?) {{text}}
# The empty () in ANNOTATION_REL aligns groups with ANNOTATION_ABS.
ANNOTATION_REL = re.compile(
    r"^expected-((?:error)|(?:warning))(-re)?()(@[+-]?\d+)? \{\{(.*)\}\}")
ANNOTATION_ABS = re.compile(
    r"^expected-((?:error)|(?:warning))(-re)?@([^:]+)(:\d+) \{\{(.*)\}\}")
# Groups from a compiler diagnostic line: file:line:col: kind: message
COMPILER_DIAG = re.compile(
    r"^((?:\w:[\\/])?[^:]+):(\d+:)?(\d+:)?( (?:(?:error)|(?:warning)):)? (.*)$")


@dataclass
class Diagnostic:
    """One expected annotation or one compiler diagnostic line."""

    location: str
    line: int | None  # None for some compiler lines; 0 matches any line
    kind: str | None
    text: str | re.Pattern[str]
    raw: str | None = None
    seen: bool = False

    def __post_init__(self) -> None:
        self.path_parts = os.path.normpath(self.location).split(os.sep)

    def location_matches(self, other: "Diagnostic") -> bool:
        if len(self.path_parts) > len(other.path_parts):
            return False
        return all(a == b for a, b in zip(
            reversed(self.path_parts), reversed(other.path_parts)))

    def text_matches(self, other: "Diagnostic") -> bool:
        if isinstance(self.text, str):
            return self.text in other.text
        return self.text.search(other.text) is not None

    def matches(self, other: "Diagnostic") -> bool:
        if not self.location_matches(other):
            return False
        if self.kind != other.kind:
            return False
        if not (self.line == other.line or self.line == 0):
            return False
        return self.text_matches(other)

    def __str__(self) -> str:
        return str(self.raw)


def _error(message: str) -> None:
    sys.stderr.write(f"verify: error: {message}\n")


def _fatal(message: str) -> None:
    _error(message)
    sys.exit(1)


def _expected_line(annotation_line: str | None, current_line: int) -> int:
    if not annotation_line:
        return current_line
    offset = annotation_line[1:]
    if offset.startswith(("+", "-")):
        return current_line + int(offset)
    return int(offset)


def parse_source_annotations(
        lines: list[str], source_name: str) -> list[Diagnostic]:
    expected: list[Diagnostic] = []
    saw_no_diagnostics = False

    for line_no, line in enumerate(lines, start=1):
        comment_pos = line.find("//")
        if comment_pos == -1:
            continue

        comment = line[comment_pos + 2:].strip()
        match = ANNOTATION_REL.match(comment) or ANNOTATION_ABS.match(comment)
        if match is None:
            if comment == "expected-no-diagnostics":
                saw_no_diagnostics = True
            continue

        kind, is_re, diag_file, diag_line, text = match.groups()
        expected.append(Diagnostic(
            location=diag_file or source_name,
            line=_expected_line(diag_line, line_no),
            kind=kind,
            text=re.compile(text) if is_re else text,
            raw=comment,
        ))

    if saw_no_diagnostics and expected:
        _fatal("given 'expected-no-diagnostics' but "
               "diagnostics were expected in the source file.")
    if not saw_no_diagnostics and not expected:
        _fatal("no diagnostics found in the source "
               "file, but 'expected-no-diagnostics' not specified.")

    return expected


def parse_compiler_diagnostics(lines: list[str]) -> list[Diagnostic]:
    actual: list[Diagnostic] = []
    for line in lines:
        match = COMPILER_DIAG.match(line)
        if match is None:
            continue

        location, line_no, _col, kind, text = match.groups()
        if not kind:
            continue

        actual.append(Diagnostic(
            location=location,
            line=int(line_no[:-1]) if line_no else None,
            kind=kind.strip().rstrip(":"),
            text=text,
            raw=line.strip(),
        ))
    return actual


def check_diagnostics(
        expected: list[Diagnostic], actual: list[Diagnostic]) -> int:
    """Pair compiler diagnostics to annotations.

    Greedy first-match, same as Clang's simple -verify: each compiler
    diagnostic is assigned to the first annotation that matches it.
    Annotations are not consumed, so one annotation may cover several
    diagnostics.
    """
    issues = 0
    for diag in actual:
        for exp in expected:
            if exp.matches(diag):
                exp.seen = True
                diag.seen = True
                break
        else:
            issues += 1
            _error(f"unexpected compiler diagnostic: {diag}")

    for exp in expected:
        if not exp.seen:
            issues += 1
            _error(f"expected compiler diagnostic not produced: {exp}")

    return issues


def main(compiler_output, source_lines, source_name):
    output_lines = compiler_output.readlines()
    sys.stderr.writelines(output_lines)

    expected = parse_source_annotations(source_lines, source_name)
    actual = parse_compiler_diagnostics(output_lines)
    if check_diagnostics(expected, actual):
        sys.exit(1)


if __name__ == "__main__":
    with open(sys.argv[1]) as source:
        main(compiler_output=sys.stdin,
             source_lines=source.readlines(),
             source_name=sys.argv[1])
