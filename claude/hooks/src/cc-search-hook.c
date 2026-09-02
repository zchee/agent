// cc-search-hook: Claude Code PreToolUse(Bash) hook.
//
// Rewrites the Bash tool's command so grep/find/rg resolve to configurable
// full-featured binaries, by prepending shell function overrides. The overrides
// win because the command is evaluated AFTER Claude Code sources its shell
// snapshot, so they shadow the snapshot's own grep/find functions. Each search
// execs the selected binary DIRECTLY: no cc-applet trampoline, no
// CLAUDE_CODE_EXECPATH indirection.
//
// Cost model. A hook invocation is dominated by process startup, so the wins
// are in what it EMITS, and they are large:
//
//   * inject nothing unless the command actually names grep/find/rg
//   * define only the functions the command actually names
//   * the wrapper is a direct `command "$bin" $args "$@"`: no dispatcher
//     function, no `local`, no arrays, no `eval`
//     (39.2 us -> 5.9 us per search call, 27 us -> 8..17 us of prefix parse)
//
// How low startup goes is a platform property, not a code property:
//
//   macOS/arm64  ~1.5 ms, irreducible. The kernel SIGKILLs non-dyld static
//                executables and ld refuses dynamic ones that do not link
//                libSystem, so dyld + libSystem init is mandatory. Measured,
//                this program is indistinguishable from `int main(){return 0;}`.
//   Linux        a freestanding static build has no interpreter at all, so the
//                floor is just execve + page-in: 343 us p50 for the glibc
//                dynamic build, 217 us for an empty static-glibc main, 76 us
//                here (Xeon 8481C). Ship the freestanding build there.
//
// The program itself carries zero libc symbol imports on every supported
// target (raw syscalls), has no output buffer at all (one writev(2) of
// constant and input slices), and scans JSON with the widest vector unit the
// build targets: NEON, SSE2, AVX2 or AVX-512BW. Measured string scan:
// 10.7 GB/s SSE2, 12.8 AVX2, 15.3 AVX-512BW, 40.0 NEON on Apple silicon.
//
// The source lives in hooks/src/ and the binary is deployed to scripts/, which
// is the path settings.json registers; both commands below run from the Claude
// config dir (~/.claude, or $CLAUDE_CONFIG_DIR).
//
// Build, macOS arm64:
//   clang -O3 -fno-stack-protector -fno-unwind-tables \
//     -fno-asynchronous-unwind-tables -Wl,-dead_strip -Wl,-x \
//     -o /opt/local/bin/cc-search-hook hooks/src/cc-search-hook.c
//   llvm-strip --strip-all /opt/local/bin/cc-search-hook
//   codesign -f -s - /opt/local/bin/cc-search-hook
//
// Build, Linux (freestanding: no libc, no dynamic loader). The link flags
// collapse the four PT_LOAD segments into two and drop the section headers,
// worth ~4% of startup and 5256 -> 4056 bytes:
//   clang -O3 -march=native -DCC_FREESTANDING -ffreestanding -nostdlib -static \
//     -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
//     -fno-ident -fuse-ld=lld -Wl,--build-id=none -Wl,--no-rosegment \
//     -Wl,-z,noseparate-code -Wl,-z,norelro -Wl,-z,nosectionheader -Wl,-s \
//     -o /opt/local/bin/cc-search-hook hooks/src/cc-search-hook.c
//
// -z norelro costs nothing here: a static binary with no relocations has no
// .data.rel.ro worth protecting, and dropping the segment is what takes the
// PT_LOAD count from three to two. Segment count trades against buffer layout,
// and the crossover sits at 2..20 KB of payload: two PT_LOADs win by 3 us at
// the 300 B payloads Claude Code actually sends, and lose 5 us at 100 KB and
// 44 us at 900 KB. -Wl,-z,nosectionheader leaves objdump and gdb unable to see
// sections, so drop it and -Wl,-s while debugging; with GNU ld drop
// --no-rosegment and -z nosectionheader as well. Two further options were
// measured and rejected: -Wl,-N collapses to a single PT_LOAD but makes the
// mapping RWX and loses section alignment, and -flto=full saves 4 us of
// startup while costing 35 us of scan.
//
// -march=native only widens the JSON scan; drop it for a portable binary and
// the SSE2/NEON baseline still applies. On macOS strip invalidates the linker's
// ad-hoc signature, so the codesign step is mandatory, not cosmetic: arm64
// SIGKILLs unsigned binaries. Any other target falls back to libc read/writev.
//
// CLAUDE_CODE_{UGREP,BFS,RG} select executables; unset or empty values use the
// paths below. The matching *_ARGS variables hold trusted shell words. They are
// word-split by the shell, not re-parsed by eval, so quotes inside them stay
// literal and glob characters inside them are expanded against the cwd. Unset
// *_ARGS uses the defaults below; an explicitly empty value passes none.

#include <stdint.h>

// ---------------------------------------------------------------- syscalls --
// EINTR is 4 on every target here, so -4 is the retry sentinel everywhere.
#define CC_EINTR 4

#if defined(__APPLE__) && defined(__aarch64__)
  #define CC_RAW        1
  #define CC_SYS_READ   3
  #define CC_SYS_WRITEV 121
  #define CC_SYS_EXIT   1
// arm64 macOS: number in x16, errors flagged by the carry bit.
static inline long cc_syscall(long num, long a0, long a1, long a2) {
  register long x0 __asm__("x0") = a0;
  register long x1 __asm__("x1") = a1;
  register long x2 __asm__("x2") = a2;
  register long x16 __asm__("x16") = num;
  __asm__ volatile("svc #0x80\n\tb.cc 1f\n\tneg x0, x0\n1:"
                   : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x16)
                   :
                   : "cc", "memory", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
                     "x17");
  return x0;
}

#elif defined(__linux__) && defined(__x86_64__)
  #define CC_RAW        1
  #define CC_SYS_READ   0
  #define CC_SYS_WRITEV 20
  #define CC_SYS_EXIT   231 /* exit_group */
// x86-64 Linux: number in rax, args in rdi/rsi/rdx, -errno in rax.
static inline long cc_syscall(long num, long a0, long a1, long a2) {
  long ret;
  __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(a0), "S"(a1), "d"(a2) : "rcx", "r11", "memory");
  return ret;
}

#elif defined(__linux__) && defined(__aarch64__)
  #define CC_RAW        1
  #define CC_SYS_READ   63
  #define CC_SYS_WRITEV 66
  #define CC_SYS_EXIT   94 /* exit_group */
static inline long cc_syscall(long num, long a0, long a1, long a2) {
  register long x0 __asm__("x0") = a0;
  register long x1 __asm__("x1") = a1;
  register long x2 __asm__("x2") = a2;
  register long x8 __asm__("x8") = num;
  __asm__ volatile("svc #0" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x8) : : "cc", "memory");
  return x0;
}
#endif

struct cc_iov {
  const void *base;
  unsigned long len;
};

#ifdef CC_RAW
static inline long cc_read(void *buf, unsigned long n) {
  return cc_syscall(CC_SYS_READ, 0, (long)buf, (long)n);
}
static inline long cc_writev(const struct cc_iov *v, long n) {
  return cc_syscall(CC_SYS_WRITEV, 1, (long)v, n);
}
__attribute__((noreturn)) static void cc_exit(void) {
  cc_syscall(CC_SYS_EXIT, 0, 0, 0);
  __builtin_unreachable();
}
#else
  // Portable fallback. struct cc_iov is layout-compatible with struct iovec.
  #include <errno.h>
  #include <sys/uio.h>
  #include <unistd.h>
static inline long cc_read(void *buf, unsigned long n) {
  long r = (long)read(0, buf, (size_t)n);
  return r < 0 ? (errno == EINTR ? -CC_EINTR : -1) : r;
}
static inline long cc_writev(const struct cc_iov *v, long n) {
  long r = (long)writev(1, (const struct iovec *)v, (int)n);
  return r < 0 ? (errno == EINTR ? -CC_EINTR : -1) : r;
}
__attribute__((noreturn)) static void cc_exit(void) {
  _exit(0);
}
#endif

// ------------------------------------------------------------------ vector --
// One uniform interface over NEON / SSE2 / AVX2 / AVX-512BW: load CC_VEC bytes,
// return a mask of the lanes equal to any of the given bytes, walk it lowest
// first. NEON has no movemask, so its mask carries four bits per lane and the
// index and clear helpers absorb the difference.
typedef uint64_t cc_mask;

#if defined(__ARM_NEON)
  #include <arm_neon.h>
  #define CC_VEC  16
  #define CC_SIMD "neon"
static inline cc_mask cc_m(uint8x16_t m) {
  return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(m), 4)), 0);
}
static inline cc_mask cc_eq2(const char *p, char a, char b) {
  uint8x16_t v = vld1q_u8((const uint8_t *)p);
  return cc_m(vorrq_u8(vceqq_u8(v, vdupq_n_u8(a)), vceqq_u8(v, vdupq_n_u8(b))));
}
static inline cc_mask cc_eq3(const char *p, char a, char b, char c) {
  uint8x16_t v = vld1q_u8((const uint8_t *)p);
  return cc_m(vorrq_u8(vorrq_u8(vceqq_u8(v, vdupq_n_u8(a)), vceqq_u8(v, vdupq_n_u8(b))), vceqq_u8(v, vdupq_n_u8(c))));
}
  #define CC_IDX(m)    ((unsigned)__builtin_ctzll(m) >> 2)
  #define CC_CLR(m, i) ((m) & ~(0xFULL << ((i) * 4)))

#elif defined(__AVX512BW__)
  #include <immintrin.h>
  #define CC_VEC       64
  #define CC_SIMD      "avx512bw"
static inline cc_mask cc_eq2(const char *p, char a, char b) {
  __m512i v = _mm512_loadu_si512((const void *)p);
  return (cc_mask)(_mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8(a)) | _mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8(b)));
}
static inline cc_mask cc_eq3(const char *p, char a, char b, char c) {
  __m512i v = _mm512_loadu_si512((const void *)p);
  return (cc_mask)(_mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8(a)) | _mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8(b)) |
                   _mm512_cmpeq_epi8_mask(v, _mm512_set1_epi8(c)));
}
  #define CC_IDX(m)    ((unsigned)__builtin_ctzll(m))
  #define CC_CLR(m, i) ((m) & ((m) - 1))

#elif defined(__AVX2__)
  #include <immintrin.h>
  #define CC_VEC       32
  #define CC_SIMD      "avx2"
static inline cc_mask cc_eq2(const char *p, char a, char b) {
  __m256i v = _mm256_loadu_si256((const __m256i *)p);
  return (uint32_t)(_mm256_movemask_epi8(
      _mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8(a)), _mm256_cmpeq_epi8(v, _mm256_set1_epi8(b)))));
}
static inline cc_mask cc_eq3(const char *p, char a, char b, char c) {
  __m256i v = _mm256_loadu_si256((const __m256i *)p);
  __m256i m = _mm256_or_si256(
      _mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8(a)), _mm256_cmpeq_epi8(v, _mm256_set1_epi8(b))),
      _mm256_cmpeq_epi8(v, _mm256_set1_epi8(c)));
  return (uint32_t)_mm256_movemask_epi8(m);
}
  #define CC_IDX(m)    ((unsigned)__builtin_ctzll(m))
  #define CC_CLR(m, i) ((m) & ((m) - 1))

#elif defined(__SSE2__)
  #include <emmintrin.h>
  #define CC_VEC       16
  #define CC_SIMD      "sse2"
static inline cc_mask cc_eq2(const char *p, char a, char b) {
  __m128i v = _mm_loadu_si128((const __m128i *)p);
  return (uint16_t)_mm_movemask_epi8(
      _mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8(a)), _mm_cmpeq_epi8(v, _mm_set1_epi8(b))));
}
static inline cc_mask cc_eq3(const char *p, char a, char b, char c) {
  __m128i v = _mm_loadu_si128((const __m128i *)p);
  __m128i m = _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8(a)), _mm_cmpeq_epi8(v, _mm_set1_epi8(b))),
                           _mm_cmpeq_epi8(v, _mm_set1_epi8(c)));
  return (uint16_t)_mm_movemask_epi8(m);
}
  #define CC_IDX(m)    ((unsigned)__builtin_ctzll(m))
  #define CC_CLR(m, i) ((m) & ((m) - 1))
#else
  #define CC_SIMD "scalar"
#endif

// ------------------------------------------------------------------- bytes --
// Little-endian packing so multi-byte literal compares fold to one load+cmp.
#define PACK4(s)                                                                                      \
  ((uint32_t)(uint8_t)(s)[0] | ((uint32_t)(uint8_t)(s)[1] << 8) | ((uint32_t)(uint8_t)(s)[2] << 16) | \
   ((uint32_t)(uint8_t)(s)[3] << 24))
#define PACK8(s) ((uint64_t)PACK4(s) | ((uint64_t)PACK4((s) + 4) << 32))

static inline uint32_t ld32(const char *p) {
  uint32_t v;
  __builtin_memcpy(&v, p, 4);
  return v;
}
static inline uint64_t ld64(const char *p) {
  uint64_t v;
  __builtin_memcpy(&v, p, 8);
  return v;
}

#define IN_CAP (1u << 20)
// Slack past IN_CAP absorbs the NUL plus the over-reads of ld32/ld64. The
// explicit alignment keeps the vector loop off split cache lines: worth 1-3% of
// a large-payload scan under AVX-512, and free everywhere else.
__attribute__((aligned(64))) static char inbuf[IN_CAP + 64];

static const char OUT_HEAD[] = "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":";
static const char OUT_TAIL[] = "}}";
static const char NOOP[] = "{}";

// Injected shell text. The \" sequences are literal backslash-quote bytes so
// the surrounding JSON string stays valid; Claude decodes them to plain " for
// the shell. MARKER makes re-entry a no-op and must stay in sync with the
// three 8-byte compares in cc_run().
static const char MARKER[] = ": __cc_search_override; ";
static const char FN_GREP[] =
    "grep(){ command \\\"${CLAUDE_CODE_UGREP:-/opt/homebrew/bin/ugrep}\\\" "
    "${CLAUDE_CODE_UGREP_ARGS--G --ignore-files --hidden -I --exclude-dir=.git "
    "--exclude-dir=.svn --exclude-dir=.hg --exclude-dir=.bzr --exclude-dir=.jj "
    "--exclude-dir=.sl} \\\"$@\\\"; }; ";
static const char FN_FIND[] =
    "find(){ command \\\"${CLAUDE_CODE_BFS:-/usr/local/bin/bfs}\\\" "
    "${CLAUDE_CODE_BFS_ARGS--S dfs -regextype findutils-default} \\\"$@\\\"; }; ";
static const char FN_RG[] =
    "rg(){ command \\\"${CLAUDE_CODE_RG:-/opt/homebrew/bin/rg}\\\" "
    "${CLAUDE_CODE_RG_ARGS-} \\\"$@\\\"; }; ";

#define PUSH(p, n)                  \
  do {                              \
    v[nv].base = (p);               \
    v[nv].len = (unsigned long)(n); \
    nv++;                           \
  } while (0)

static void emit(struct cc_iov *v, long n) {
  while (n > 0) {
    long w = cc_writev(v, n);
    if (w <= 0) {
      if (w == -CC_EINTR)
        continue;
      return;
    }
    while (n > 0 && (unsigned long)w >= v->len) {
      w -= (long)v->len;
      v++;
      n--;
    }
    if (n == 0)
      return;
    v->base = (const char *)v->base + w;
    v->len -= (unsigned long)w;
  }
}

__attribute__((noreturn)) static void reply_noop(void) {
  struct cc_iov v = {NOOP, sizeof NOOP - 1};
  emit(&v, 1);
  cc_exit();
}

static const char *skip_ws(const char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  return p;
}

// Find literal lit (n >= 8 bytes, first 8 packed in k0) in [p, end); returns the
// position just past it.
static const char *find_lit(const char *p, const char *end, const char *lit, unsigned n, uint64_t k0) {
  for (; p + n <= end; p++) {
    if (ld64(p) != k0)
      continue;
    unsigned i = 8;
    while (i < n && p[i] == lit[i]) i++;
    if (i == n)
      return p + n;
  }
  return 0;
}

// Closing quote of a JSON string starting at p, honouring backslash escapes.
static const char *str_end(const char *p, const char *end) {
#ifdef CC_VEC
  while (p + CC_VEC <= end) {
    cc_mask m = cc_eq2(p, '"', '\\');
    if (m) {
      p += CC_IDX(m);
      if (*p == '"')
        return p;
      p += 2;
      continue;
    }
    p += CC_VEC;
  }
#endif
  while (p < end) {
    char c = *p;
    if (c == '"')
      return p;
    p += (c == '\\') ? 2 : 1;
  }
  return 0;
}

enum { HIT_GREP = 1, HIT_FIND = 2, HIT_RG = 4, HIT_ALL = 7 };

static inline int ident_ch(unsigned char c) {
  return (unsigned)((c | 32) - 'a') < 26u || (unsigned)(c - '0') < 10u || c == '_' || c == '.' || c == '-';
}

// Does a shell word boundary + needle start at p? s is the value start, end its
// closing quote (always readable, never an identifier byte).
static unsigned tok_at(const char *s, const char *p, const char *end) {
  unsigned bit, n;
  if (*p == 'r') {
    if (end - p < 2 || p[1] != 'g')
      return 0;
    n = 2;
    bit = HIT_RG;
  } else {
    if (end - p < 4)
      return 0;
    uint32_t w = ld32(p);
    if (w == PACK4("grep")) {
      n = 4;
      bit = HIT_GREP;
    } else if (w == PACK4("find")) {
      n = 4;
      bit = HIT_FIND;
    } else {
      return 0;
    }
  }
  // A JSON escape (\n, \t, ...) reads as an identifier byte but is a boundary.
  if (p > s && ident_ch((unsigned char)p[-1]) && !(p - 1 > s && p[-2] == '\\'))
    return 0;
  if (ident_ch((unsigned char)p[n]))
    return 0;
  return bit;
}

static unsigned scan_tokens(const char *s, const char *end) {
  unsigned hit = 0;
  const char *p = s;
#ifdef CC_VEC
  while (p + CC_VEC <= end) {
    cc_mask m = cc_eq3(p, 'g', 'f', 'r');
    while (m) {
      unsigned i = CC_IDX(m);
      hit |= tok_at(s, p + i, end);
      m = CC_CLR(m, i);
    }
    if (hit == HIT_ALL)
      return hit;
    p += CC_VEC;
  }
#endif
  for (; p < end; p++) {
    char c = *p;
    if ((c == 'g' || c == 'f' || c == 'r') && (hit |= tok_at(s, p, end)) == HIT_ALL)
      break;
  }
  return hit;
}

// Walk the tool_input object at p (its '{'); capture the command value between
// its quotes. Returns one past the object's '}', or 0 if malformed.
static const char *walk_object(const char *p, const char *end, const char **cb, const char **ce) {
  int depth = 0;
  for (;;) {
    if (p >= end)
      return 0;
    char c = *p;
    if (c == '"') {
      const char *vs = p + 1;
      const char *q = str_end(vs, end);
      if (!q)
        return 0;
      if (depth == 1 && !*cb && q - vs == 7 && ld32(vs) == PACK4("comm") && ld32(vs + 3) == PACK4("mand")) {
        const char *r = skip_ws(q + 1);
        if (*r == ':') {
          r = skip_ws(r + 1);
          if (*r == '"') {
            *cb = r + 1;
            *ce = str_end(*cb, end);
            if (!*ce)
              return 0;
            p = *ce + 1;
            continue;
          }
        }
        p = r;
        continue;
      }
      p = q + 1;
    } else if (c == '{') {
      depth++;
      p++;
    } else if (c == '}') {
      p++;
      if (--depth == 0)
        return p;
    } else {
      p++;
    }
  }
}

__attribute__((noreturn)) static void cc_run(void) {
  unsigned long len = 0;
  for (;;) {
    long r = cc_read(inbuf + len, IN_CAP - len);
    if (r <= 0) {
      if (r == -CC_EINTR)
        continue;
      break;
    }
    len += (unsigned long)r;
    // Oversized payload: drain so the writer never sees EPIPE, then no-op.
    if (len >= IN_CAP) {
      while ((r = cc_read(inbuf, IN_CAP)) > 0 || r == -CC_EINTR) {
      }
      reply_noop();
    }
  }
  inbuf[len] = '\0';
  const char *end = inbuf + len;

  const char *p = find_lit(inbuf, end, "\"tool_name\"", 11, PACK8("\"tool_na"));
  if (p) {
    p = skip_ws(p);
    if (*p != ':')
      reply_noop();
    p = skip_ws(p + 1);
    if (*p != '"' || ld32(p + 1) != PACK4("Bash") || p[5] != '"')
      reply_noop();
  }

  p = find_lit(inbuf, end, "\"tool_input\"", 12, PACK8("\"tool_in"));
  if (!p)
    reply_noop();
  p = skip_ws(p);
  if (*p != ':')
    reply_noop();
  p = skip_ws(p + 1);
  if (*p != '{')
    reply_noop();

  const char *ti = p, *cb = 0, *ce = 0;
  const char *oe = walk_object(ti, end, &cb, &ce);
  if (!oe || !cb)
    reply_noop();

  // Already rewritten: the marker is injected at the head, so this is O(1).
  if (ce - cb >= (long)(sizeof MARKER - 1) && ld64(cb) == PACK8(": __cc_s") && ld64(cb + 8) == PACK8("earch_ov") &&
      ld64(cb + 16) == PACK8("erride; "))
    reply_noop();

  unsigned hit = scan_tokens(cb, ce);
  if (!hit)
    reply_noop();

  struct cc_iov v[8];
  long nv = 0;
  PUSH(OUT_HEAD, sizeof OUT_HEAD - 1);
  PUSH(ti, cb - ti);
  PUSH(MARKER, sizeof MARKER - 1);
  if (hit & HIT_GREP)
    PUSH(FN_GREP, sizeof FN_GREP - 1);
  if (hit & HIT_FIND)
    PUSH(FN_FIND, sizeof FN_FIND - 1);
  if (hit & HIT_RG)
    PUSH(FN_RG, sizeof FN_RG - 1);
  PUSH(cb, oe - cb);
  PUSH(OUT_TAIL, sizeof OUT_TAIL - 1);
  emit(v, nv);
  cc_exit();
}

// ------------------------------------------------------------------- entry --
#if defined(CC_FREESTANDING)
  #if !defined(__linux__)
    #error "CC_FREESTANDING is Linux-only: macOS refuses to exec non-dyld binaries"
  #endif
// No libc, so no _start from crt1.o: take the raw process entry. The kernel
// hands us a 16-byte aligned stack with no return address, which is one slot
// off what a compiled function expects, so realign before calling in.
__attribute__((used, noreturn)) void cc_entry(void) {
  cc_run();
}
  #if defined(__x86_64__)
__asm__(
    ".globl _start\n"
    ".type _start,@function\n"
    "_start:\n\t"
    "xor %ebp, %ebp\n\t"
    "and $-16, %rsp\n\t"
    "call cc_entry\n\t"
    "hlt\n");
  #elif defined(__aarch64__)
__asm__(
    ".globl _start\n"
    ".type _start,%function\n"
    "_start:\n\t"
    "mov x29, #0\n\t"
    "mov x30, #0\n\t"
    "bl cc_entry\n\t"
    "brk #0\n");
  #else
    #error "CC_FREESTANDING: unsupported architecture"
  #endif
#else
int main(void) {
  cc_run();
}
#endif
