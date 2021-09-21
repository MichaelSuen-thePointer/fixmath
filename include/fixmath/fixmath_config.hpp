/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#ifndef FIXMATH_USE_ASSERT
#  define FIXMATH_USE_ASSERT 1
#endif

#if defined(__GNUC__)
#  define FIXMATH_LIKELY(expr) __builtin_expect(!!(expr), 1)
#  define FIXMATH_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#  define FIXMATH_LIKELY(expr) (expr)
#  define FIXMATH_UNLIKELY(expr) (expr)
#endif

#if FIXMATH_USE_ASSERT
#  include <cassert>
#  define FIXMATH_ERROR(msg) assert(((void)msg, 0))
#  define FIXMATH_ASSERT(cond, msg) assert(((void)msg, cond))
#else
#  define FIXMATH_ERROR(msg) (void)0
#  define FIXMATH_ASSERT(cond, msg) (void)0
#endif

#if defined(_MSC_VER)
#  define FIXMATH_FORCEINLINE __forceinline
#  define FIXMATH_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#  define FIXMATH_FORCEINLINE __attribute__((always_inline))
#  define FIXMATH_NOINLINE __attribute__((noinline))
#else
#  define FIXMATH_FORCEINLINE
#  define FIXMATH_NOINLINE
#endif

#define FIXMATH_LINUX 0
#define FIXMATH_LINUX_X64 0
#define FIXMATH_LINUX_X86 0
#define FIXMATH_LINUX_ARM32 0
#define FIXMATH_LINUX_ARM64 0

#define FIXMATH_WIN 0
#define FIXMATH_WIN_X64 0
#define FIXMATH_WIN_X86 0
#define FIXMATH_WIN_ARM32 0
#define FIXMATH_WIN_ARM64 0

#define FIXMATH_64BIT 0
#define FIXMATH_32BIT 0
#define FIXMATH_GENERIC 1

#if defined(__GNUC__)
#  undef FIXMATH_LINUX
#  define FIXMATH_LINUX 1
#  if defined(__x86_64__)
#    undef FIXMATH_LINUX_X64
#    define FIXMATH_LINUX_X64 1
#    undef FIXMATH_64BIT
#    define FIXMATH_64BIT 1
#  elif defined(__i386__)
#    undef FIXMATH_LINUX_X86
#    define FIXMATH_LINUX_X86 1
#    undef FIXMATH_32BIT
#    define FIXMATH_32BIT 1
#  elif defined(__arm__)
#    undef FIXMATH_LINUX_ARM32
#    define FIXMATH_LINUX_ARM32 1
#    undef FIXMATH_32BIT
#    define FIXMATH_32BIT 1
#  elif defined(__aarch64__)
#    undef FIXMATH_LINUX_ARM64
#    define FIXMATH_LINUX_ARM64 1
#    undef FIXMATH_64BIT
#    define FIXMATH_64BIT 1
#  else
#    undef FIXMATH_GENERIC
#    define FIXMATH_GENERIC 1
#  endif
#elif defined(_MSC_VER)
#  undef FIXMATH_WIN
#  define FIXMATH_WIN 1
#  if defined(_M_X64)
#    undef FIXMATH_WIN_X64
#    define FIXMATH_WIN_X64 1
#    undef FIXMATH_64BIT
#    define FIXMATH_64BIT 1
#  elif defined(_M_IX86)
#    undef FIXMATH_WIN_X86
#    define FIXMATH_WIN_X86 1
#    undef FIXMATH_32BIT
#    define FIXMATH_32BIT 1
#  elif defined(_M_ARM)
#    undef FIXMATH_WIN_ARM32
#    define FIXMATH_WIN_ARM32 1
#    undef FIXMATH_32BIT
#    define FIXMATH_32BIT 1
#  elif defined(_M_ARM64)
#    undef FIXMATH_WIN_ARM64
#    define FIXMATH_WIN_ARM64 1
#    undef FIXMATH_64BIT
#    define FIXMATH_64BIT 1
#  else
#    undef FIXMATH_GENERIC
#    define FIXMATH_GENERIC 1
#  endif
#else
#  undef FIXMATH_GENERIC
#  define FIXMATH_GENERIC 1
#endif
