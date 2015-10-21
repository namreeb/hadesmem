// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

// TODO: Implement this.
// TODO: Use process reflection on Windows 7 + for scanning while process is suspended. (RtlCreateProcessReflection)
//  Requires extra privileges though… Make it optional?
//  There's newer and better APIs available on W8+. PSS? ProcDump supports them all I think...
//  PSS doesn't support large pages, so can't be used against e.g.SQL.
// TODO: Use a file view with a small memory cache rather than consuming large amounts of RAM.
// TODO: Multi-threaded scanning options.
// TODO: Wildcard support for vector/string scanning.
// TODO: Regex support for string scanning.
// TODO: Memory protection filters(read, write, exec).
// TODO: Memory type filters(private, mapped, image).
// TODO: Support pausing target while scanning.
// TODO: Support injected scanning.
// TODO: Configurable scan buffer size.
// TODO: Pointer scanner.
// TODO: Unknown value scan.
// TODO: Progressive scan filtering based on either value or criteria.
// TODO: Scan history and undo.
// TODO: Support case insensitive string scanning.
// TODO: Binary scanning.
// TODO: Custom scanning via user supplied predicate.
// TODO: Improved floating point support (configurable or 'smart' epsilon).
// TODO: Group search support.
