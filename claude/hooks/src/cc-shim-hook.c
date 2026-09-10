// cc-shim-hook: Claude Code PreToolUse(Bash) hook.
//
// Rewrites the Bash tool's command so chosen command words resolve to chosen
// binaries, by prepending shell function overrides. The overrides win because
// the command is evaluated AFTER Claude Code sources its shell snapshot, so
// they shadow the snapshot's own grep/find functions. Each call execs the
// selected binary DIRECTLY: no cc-applet trampoline, no CLAUDE_CODE_EXECPATH
// indirection.
//
// Which words get shadowed comes from a JSON config when one is present, and
// from the built-in table when it is not. The built-in table is the set this
// hook carried before the config existed, so a machine with no config emits
// byte-identical output to that version.
//
// Config discovery, first hit wins:
//   1. $CC_SHIM_CONFIG                                      explicit override
//   2. ${CLAUDE_CONFIG_DIR:-$HOME/.claude}/hooks/shim-tools.json  personal defaults
//
// There is deliberately no per-project walk. A config here decides which
// binary a bare command word runs, so a repo-supplied config would be code
// execution on every Bash call in that repo. It lives in the user's own
// config dir, at the same trust level as settings.json.
//
// Schema. Unknown members are ignored; every string is copied verbatim as
// JSON string content, so escapes stay escaped:
//
//   { "tools": {
//       "grep": { "bin":      "/opt/homebrew/bin/ugrep",   required
//                 "args":     "-G --hidden",               optional
//                 "bin_env":  "CLAUDE_CODE_UGREP",         optional
//                 "args_env": "CLAUDE_CODE_UGREP_ARGS" }   optional
//   } }
//
// bin_env and args_env keep the value resolvable at run time for free: the
// hook emits ${BIN_ENV:-bin} and ${ARGS_ENV-args}, and the shell, not this
// program, does the lookup. The two forms differ on purpose. ${X:-bin} takes
// the default when X is unset OR empty, because an empty binary path is never
// meaningful. ${Y-args} takes it only when Y is unset, so an explicitly empty
// value passes no arguments at all. Omit bin_env and the path is emitted
// literally; omit args_env and args are emitted literally, or dropped when
// also empty.
//
// A config that cannot be parsed, or that breaks a bound, is rejected whole
// and the built-in table is used instead. There is no partial adoption: a
// half-applied tool table is harder to reason about than either endpoint, and
// the fallback is always a working configuration.
//
// Cost model. A hook invocation is dominated by process startup, so the wins
// are in what it EMITS, and they are large:
//
//   * inject nothing unless the command actually names a configured tool
//   * define only the functions the command actually names
//   * the wrapper is a direct `command "$bin" $args "$@"`: no dispatcher
//     function, no `local`, no arrays, no `eval`
//     (39.2 us -> 5.9 us per search call, 27 us -> 8..17 us of prefix parse)
//   * output is still written by one writev(2) of constant, config and input
//     slices; the config strings are emitted in place, never copied
//
// What the config itself costs, measured on an M3 Max with the page cache
// warm. All of it is noise next to the process:
//
//   envp scan         one pass collecting all four keys, 1.58 us against 619
//                     entries. Per-key lookups would multiply that, hence the
//                     single pass.
//   config read       6.66 us for open+read+close when the file exists, and
//                     0.51 us for a failing open when it does not.
//   config parse      0.08 us for a 364 B config, the same targeted-scan cost
//                     as reading the payload.
//   token scan        a runtime table costs 297 ns against 55 ns for the same
//                     four tools compiled in, on a 212 B command. The 5.4x is
//                     real and it buys the configurability.
//   total             ~8 us against a ~1.5 ms floor, i.e. 0.5%, which is 30x
//                     below the run-to-run spread of spawning the hook at all.
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
//                here (Xeon 8481C). Ship the freestanding build there. Note
//                the config is a larger share of a smaller floor there, and
//                that Linux syscalls are cheaper than the macOS ones measured
//                above; neither number has been taken on Linux.
//
// The program itself carries zero libc symbol imports on every supported
// target (raw syscalls), and scans JSON with the widest vector unit the
// build targets: NEON, SSE2, AVX2 or AVX-512BW. Measured string scan:
// 10.7 GB/s SSE2, 12.8 AVX2, 15.3 AVX-512BW, 40.0 NEON on Apple silicon.
//
// The source lives in hooks/src/ and the binary is deployed to
// /opt/local/bin/, which is the path settings.json registers; both build
// commands below run from the Claude config dir (~/.claude, or
// $CLAUDE_CONFIG_DIR).
//
// Build, macOS arm64:
//   clang -O3 -fno-stack-protector -fno-unwind-tables \
//     -fno-asynchronous-unwind-tables -Wl,-dead_strip -Wl,-x \
//     -o /opt/local/bin/cc-shim-hook hooks/src/cc-shim-hook.c
//   llvm-strip --strip-all /opt/local/bin/cc-shim-hook
//   codesign -f -s - /opt/local/bin/cc-shim-hook
//
// Build, Linux (freestanding: no libc, no dynamic loader). The link flags
// collapse the four PT_LOAD segments into two and drop the section headers,
// worth ~4% of startup and 5256 -> 4056 bytes:
//   clang -O3 -march=native -DCC_FREESTANDING -ffreestanding -nostdlib -static \
//     -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
//     -fno-ident -fuse-ld=lld -Wl,--build-id=none -Wl,--no-rosegment \
//     -Wl,-z,noseparate-code -Wl,-z,norelro -Wl,-z,nosectionheader -Wl,-s \
//     -o /opt/local/bin/cc-shim-hook hooks/src/cc-shim-hook.c
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
// CC_SHIM_DEBUG=1 writes the resolved config source and tool table to stderr
// and changes nothing else; it exists so the test suite and a puzzled reader
// can tell a rejected config from an absent one.

#include <stdint.h>

// ---------------------------------------------------------------- syscalls --
// EINTR is 4 on every target here, so -4 is the retry sentinel everywhere.
#define CC_EINTR 4

#if defined(__APPLE__) && defined(__aarch64__)
  #define CC_RAW        1
  #define CC_SYS_READ   3
  #define CC_SYS_WRITE  4
  #define CC_SYS_OPEN   5
  #define CC_SYS_CLOSE  6
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
static inline long cc_sys_open(const char *path) {
  return cc_syscall(CC_SYS_OPEN, (long)path, 0 /* O_RDONLY */, 0);
}

#elif defined(__linux__) && defined(__x86_64__)
  #define CC_RAW        1
  #define CC_SYS_READ   0
  #define CC_SYS_WRITE  1
  #define CC_SYS_CLOSE  3
  #define CC_SYS_WRITEV 20
  #define CC_SYS_OPENAT 257
  #define CC_SYS_EXIT   231 /* exit_group */
// x86-64 Linux: number in rax, args in rdi/rsi/rdx, -errno in rax.
static inline long cc_syscall(long num, long a0, long a1, long a2) {
  long ret;
  __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(a0), "S"(a1), "d"(a2) : "rcx", "r11", "memory");
  return ret;
}
static inline long cc_sys_open(const char *path) {
  long ret;
  register long r10 __asm__("r10") = 0; /* mode */
  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"((long)CC_SYS_OPENAT), "D"(-100L /* AT_FDCWD */), "S"((long)path), "d"(0L /* O_RDONLY */),
                     "r"(r10)
                   : "rcx", "r11", "memory");
  return ret;
}

#elif defined(__linux__) && defined(__aarch64__)
  #define CC_RAW        1
  #define CC_SYS_OPENAT 56
  #define CC_SYS_CLOSE  57
  #define CC_SYS_READ   63
  #define CC_SYS_WRITE  64
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
static inline long cc_sys_open(const char *path) {
  register long x0 __asm__("x0") = -100; /* AT_FDCWD */
  register long x1 __asm__("x1") = (long)path;
  register long x2 __asm__("x2") = 0; /* O_RDONLY */
  register long x3 __asm__("x3") = 0; /* mode */
  register long x8 __asm__("x8") = CC_SYS_OPENAT;
  __asm__ volatile("svc #0" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x8) : : "cc", "memory");
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
static inline long cc_readfd(int fd, void *buf, unsigned long n) {
  return cc_syscall(CC_SYS_READ, fd, (long)buf, (long)n);
}
static inline void cc_close(int fd) {
  cc_syscall(CC_SYS_CLOSE, fd, 0, 0);
}
static inline int cc_open(const char *path) {
  long fd = cc_sys_open(path);
  return fd < 0 ? -1 : (int)fd;
}
static inline long cc_writev(const struct cc_iov *v, long n) {
  return cc_syscall(CC_SYS_WRITEV, 1, (long)v, n);
}
static inline void cc_warn(const char *s, unsigned long n) {
  cc_syscall(CC_SYS_WRITE, 2, (long)s, (long)n);
}
__attribute__((noreturn)) static void cc_exit(void) {
  cc_syscall(CC_SYS_EXIT, 0, 0, 0);
  __builtin_unreachable();
}
#else
  // Portable fallback. struct cc_iov is layout-compatible with struct iovec.
  #include <errno.h>
  #include <fcntl.h>
  #include <sys/uio.h>
  #include <unistd.h>
static inline long cc_read(void *buf, unsigned long n) {
  long r = (long)read(0, buf, (size_t)n);
  return r < 0 ? (errno == EINTR ? -CC_EINTR : -1) : r;
}
static inline long cc_readfd(int fd, void *buf, unsigned long n) {
  long r = (long)read(fd, buf, (size_t)n);
  return r < 0 ? (errno == EINTR ? -CC_EINTR : -1) : r;
}
static inline void cc_close(int fd) {
  close(fd);
}
static inline int cc_open(const char *path) {
  return open(path, O_RDONLY);
}
static inline long cc_writev(const struct cc_iov *v, long n) {
  long r = (long)writev(1, (const struct iovec *)v, (int)n);
  return r < 0 ? (errno == EINTR ? -CC_EINTR : -1) : r;
}
static inline void cc_warn(const char *s, unsigned long n) {
  ssize_t ignored = write(2, s, (size_t)n);
  (void)ignored;
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
//
// cc_eq2 keeps its compile-time pair for the JSON string walk, whose two
// bytes never change. The token scan needs a set that is only known once the
// config is read, so its candidate bytes are splatted once into cc_cand_init
// and reused for every block: rebuilding them per block cost more than the
// compare itself.
typedef uint64_t cc_mask;

#define CC_MAX_CAND 16

#if defined(__ARM_NEON)
  #include <arm_neon.h>
  #define CC_VEC  16
  #define CC_SIMD "neon"
static inline cc_mask cc_m(uint8x16_t m) {
  return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(m), 4)), 0);
}
static inline cc_mask cc_eq2(const char *p, char a, char b) {
  uint8x16_t v = vld1q_u8((const uint8_t *)p);
  return cc_m(vorrq_u8(vceqq_u8(v, vdupq_n_u8((uint8_t)a)), vceqq_u8(v, vdupq_n_u8((uint8_t)b))));
}
static uint8x16_t cc_candv[CC_MAX_CAND];
static void cc_cand_init(const unsigned char *c, unsigned n) {
  for (unsigned i = 0; i < n; i++) cc_candv[i] = vdupq_n_u8(c[i]);
}
static inline cc_mask cc_eq_set(const char *p, unsigned n) {
  uint8x16_t v = vld1q_u8((const uint8_t *)p);
  uint8x16_t acc = vceqq_u8(v, cc_candv[0]);
  for (unsigned i = 1; i < n; i++) acc = vorrq_u8(acc, vceqq_u8(v, cc_candv[i]));
  return cc_m(acc);
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
static __m512i cc_candv[CC_MAX_CAND];
static void cc_cand_init(const unsigned char *c, unsigned n) {
  for (unsigned i = 0; i < n; i++) cc_candv[i] = _mm512_set1_epi8((char)c[i]);
}
static inline cc_mask cc_eq_set(const char *p, unsigned n) {
  __m512i v = _mm512_loadu_si512((const void *)p);
  cc_mask acc = (cc_mask)_mm512_cmpeq_epi8_mask(v, cc_candv[0]);
  for (unsigned i = 1; i < n; i++) acc |= (cc_mask)_mm512_cmpeq_epi8_mask(v, cc_candv[i]);
  return acc;
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
static __m256i cc_candv[CC_MAX_CAND];
static void cc_cand_init(const unsigned char *c, unsigned n) {
  for (unsigned i = 0; i < n; i++) cc_candv[i] = _mm256_set1_epi8((char)c[i]);
}
static inline cc_mask cc_eq_set(const char *p, unsigned n) {
  __m256i v = _mm256_loadu_si256((const __m256i *)p);
  __m256i acc = _mm256_cmpeq_epi8(v, cc_candv[0]);
  for (unsigned i = 1; i < n; i++) acc = _mm256_or_si256(acc, _mm256_cmpeq_epi8(v, cc_candv[i]));
  return (uint32_t)_mm256_movemask_epi8(acc);
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
static __m128i cc_candv[CC_MAX_CAND];
static void cc_cand_init(const unsigned char *c, unsigned n) {
  for (unsigned i = 0; i < n; i++) cc_candv[i] = _mm_set1_epi8((char)c[i]);
}
static inline cc_mask cc_eq_set(const char *p, unsigned n) {
  __m128i v = _mm_loadu_si128((const __m128i *)p);
  __m128i acc = _mm_cmpeq_epi8(v, cc_candv[0]);
  for (unsigned i = 1; i < n; i++) acc = _mm_or_si128(acc, _mm_cmpeq_epi8(v, cc_candv[i]));
  return (uint16_t)_mm_movemask_epi8(acc);
}
  #define CC_IDX(m)    ((unsigned)__builtin_ctzll(m))
  #define CC_CLR(m, i) ((m) & ((m) - 1))
#else
  #define CC_SIMD "scalar"
static void cc_cand_init(const unsigned char *c, unsigned n) {
  (void)c;
  (void)n;
}
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
static inline int cc_streq(const char *a, const char *b, unsigned n) {
  for (unsigned i = 0; i < n; i++)
    if (a[i] != b[i])
      return 0;
  return 1;
}
static unsigned cc_slen(const char *s) {
  unsigned n = 0;
  while (s[n]) n++;
  return n;
}

#define IN_CAP (1u << 20)
// Slack past IN_CAP absorbs the NUL plus the over-reads of ld32/ld64. The
// explicit alignment keeps the vector loop off split cache lines: worth 1-3% of
// a large-payload scan under AVX-512, and free everywhere else.
__attribute__((aligned(64))) static char inbuf[IN_CAP + 64];

// A tool table this size still fits one writev: 14 slices per tool plus the
// payload's own, well under IOV_MAX.
#define MAX_TOOLS 16
#define MAX_NAME  15
#define CFG_CAP   (1u << 16)
#define PATH_CAP  1024
__attribute__((aligned(64))) static char cfgbuf[CFG_CAP + 64];

static const char OUT_HEAD[] = "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":";
static const char OUT_TAIL[] = "}}";
static const char NOOP[] = "{}";

// Injected shell text, assembled from these constants and slices of the
// config. The \" sequences are literal backslash-quote bytes so the
// surrounding JSON string stays valid; Claude decodes them to plain " for the
// shell. MARKER makes re-entry a no-op and must stay in sync with the three
// 8-byte compares in cc_run(): at 22 bytes it covers offsets 0, 8 and 14, so
// the last load overlaps the second by two bytes.
static const char MARKER[] = ": __cc_shim_override; ";
static const char FN_OPEN[] = "(){ command \\\"";
static const char FN_SUBST[] = "${";
static const char FN_BIN_DEF[] = ":-";
static const char FN_SUBST_END[] = "}";
static const char FN_MID[] = "\\\" ";
static const char FN_ARGS_DEF[] = "-";
static const char FN_ARGS_END[] = "} ";
static const char FN_SPACE[] = " ";
static const char FN_CLOSE[] = "\\\"$@\\\"; }; ";

struct tool {
  const char *name;
  const char *bin;
  const char *args;
  const char *bin_env;
  const char *args_env;
  unsigned nlen, blen, alen, belen, aelen;
  uint32_t w4;  // name packed when nlen == 4, so the common case is one compare
};

static struct tool TOOLS[MAX_TOOLS];
static unsigned NTOOLS;
static unsigned NCAND;
static unsigned char CANDS[CC_MAX_CAND];
static unsigned char IS_CAND[256];
static int DEBUG;

// The set this hook carried before the config existed. Keeping it as the
// fallback is what makes a config-less machine byte-identical to that version.
static const char BI_UGREP_ARGS[] =
    "-G --ignore-files --hidden -I --exclude-dir=.git "
    "--exclude-dir=.svn --exclude-dir=.hg --exclude-dir=.bzr --exclude-dir=.jj "
    "--exclude-dir=.sl";
struct builtin {
  const char *name, *bin, *args, *bin_env, *args_env;
};
static const struct builtin BUILTINS[] = {
    {"grep", "/opt/homebrew/bin/ugrep", BI_UGREP_ARGS,                         "CLAUDE_CODE_UGREP", "CLAUDE_CODE_UGREP_ARGS"},
    {"find", "/usr/local/bin/bfs",      "-S dfs -regextype findutils-default", "CLAUDE_CODE_BFS",   "CLAUDE_CODE_BFS_ARGS"  },
    {"rg",   "/opt/homebrew/bin/rg",    "",                                    "CLAUDE_CODE_RG",    "CLAUDE_CODE_RG_ARGS"   },
};

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

static void warn_str(const char *s) {
  cc_warn(s, cc_slen(s));
}
static void warn_slice(const char *s, unsigned n) {
  cc_warn(s, n);
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

static inline int ident_ch(unsigned char c) {
  return (unsigned)((c | 32) - 'a') < 26u || (unsigned)(c - '0') < 10u || c == '_' || c == '.' || c == '-';
}
// A shell function name we are willing to define, and an environment name we
// are willing to interpolate. Both are stricter than what the shell accepts,
// because the config is the one place a typo becomes injected shell text.
static int name_ok(const char *s, unsigned n) {
  if (n == 0 || n > MAX_NAME)
    return 0;
  if (!((unsigned)((s[0] | 32) - 'a') < 26u || s[0] == '_'))
    return 0;
  for (unsigned i = 1; i < n; i++)
    if (!ident_ch((unsigned char)s[i]))
      return 0;
  return 1;
}
static int env_ok(const char *s, unsigned n) {
  if (n == 0)
    return 0;
  if (!((unsigned)((s[0] | 32) - 'a') < 26u || s[0] == '_'))
    return 0;
  for (unsigned i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if (!((unsigned)((c | 32) - 'a') < 26u || (unsigned)(c - '0') < 10u || c == '_'))
      return 0;
  }
  return 1;
}

// -------------------------------------------------------------- tool table --
static void table_finish(void) {
  unsigned char seen[256] = {0};
  NCAND = 0;
  for (unsigned i = 0; i < 256; i++) IS_CAND[i] = 0;
  for (unsigned i = 0; i < NTOOLS; i++) {
    struct tool *t = &TOOLS[i];
    t->w4 = t->nlen == 4 ? PACK4(t->name) : 0;
    unsigned char c = (unsigned char)t->name[0];
    IS_CAND[c] = 1;
    if (!seen[c] && NCAND < CC_MAX_CAND) {
      seen[c] = 1;
      CANDS[NCAND++] = c;
    }
  }
  cc_cand_init(CANDS, NCAND);
}

static void load_builtins(void) {
  NTOOLS = sizeof BUILTINS / sizeof BUILTINS[0];
  for (unsigned i = 0; i < NTOOLS; i++) {
    struct tool *t = &TOOLS[i];
    t->name = BUILTINS[i].name;
    t->nlen = cc_slen(t->name);
    t->bin = BUILTINS[i].bin;
    t->blen = cc_slen(t->bin);
    t->args = BUILTINS[i].args;
    t->alen = cc_slen(t->args);
    t->bin_env = BUILTINS[i].bin_env;
    t->belen = cc_slen(t->bin_env);
    t->args_env = BUILTINS[i].args_env;
    t->aelen = cc_slen(t->args_env);
  }
  table_finish();
}

// ------------------------------------------------------------ config parse --
// A targeted walk, not a JSON parser: the shape is fixed and anything outside
// it is skipped. Every accepted string is a slice of cfgbuf, so nothing is
// copied and the emitted output can point straight at it.
struct jstr {
  const char *s;
  unsigned n;
};

static const char *j_string(const char *p, const char *end, struct jstr *out) {
  p = skip_ws(p);
  if (p >= end || *p != '"')
    return 0;
  const char *s = p + 1;
  const char *q = str_end(s, end);
  if (!q)
    return 0;
  out->s = s;
  out->n = (unsigned)(q - s);
  return q + 1;
}

// Skip one JSON value, whatever it is.
static const char *j_skip(const char *p, const char *end) {
  p = skip_ws(p);
  if (p >= end)
    return 0;
  if (*p == '"') {
    const char *q = str_end(p + 1, end);
    return q ? q + 1 : 0;
  }
  if (*p == '{' || *p == '[') {
    int depth = 0;
    while (p < end) {
      char c = *p;
      if (c == '"') {
        const char *q = str_end(p + 1, end);
        if (!q)
          return 0;
        p = q + 1;
        continue;
      }
      if (c == '{' || c == '[')
        depth++;
      else if (c == '}' || c == ']') {
        if (--depth == 0)
          return p + 1;
      }
      p++;
    }
    return 0;
  }
  while (p < end && *p != ',' && *p != '}' && *p != ']') p++;
  return p;
}

// One "<name>": { ... } member of the tools object.
static const char *parse_tool(const char *p, const char *end, struct jstr name) {
  p = skip_ws(p);
  if (p >= end || *p != ':')
    return 0;
  p = skip_ws(p + 1);
  if (p >= end || *p != '{')
    return 0;
  p++;

  struct jstr bin = {0, 0}, args = {0, 0}, benv = {0, 0}, aenv = {0, 0};
  for (;;) {
    p = skip_ws(p);
    if (p >= end)
      return 0;
    if (*p == '}') {
      p++;
      break;
    }
    if (*p == ',') {
      p++;
      continue;
    }
    struct jstr key;
    p = j_string(p, end, &key);
    if (!p)
      return 0;
    p = skip_ws(p);
    if (p >= end || *p != ':')
      return 0;
    p++;
    struct jstr *slot = 0;
    if (key.n == 3 && cc_streq(key.s, "bin", 3))
      slot = &bin;
    else if (key.n == 4 && cc_streq(key.s, "args", 4))
      slot = &args;
    else if (key.n == 7 && cc_streq(key.s, "bin_env", 7))
      slot = &benv;
    else if (key.n == 8 && cc_streq(key.s, "args_env", 8))
      slot = &aenv;
    if (slot) {
      p = j_string(p, end, slot);
      if (!p)
        return 0;
    } else {
      p = j_skip(p, end);
      if (!p)
        return 0;
    }
  }

  if (!name_ok(name.s, name.n) || bin.n == 0)
    return 0;
  if (benv.s && !env_ok(benv.s, benv.n))
    return 0;
  if (aenv.s && !env_ok(aenv.s, aenv.n))
    return 0;
  if (NTOOLS >= MAX_TOOLS)
    return 0;

  struct tool *t = &TOOLS[NTOOLS++];
  t->name = name.s;
  t->nlen = name.n;
  t->bin = bin.s;
  t->blen = bin.n;
  t->args = args.s;
  t->alen = args.n;
  t->bin_env = benv.s;
  t->belen = benv.n;
  t->args_env = aenv.s;
  t->aelen = aenv.n;
  return p;
}

// Returns 1 when the whole config was adopted, 0 when the built-ins should
// stand. NTOOLS is only meaningful on success.
static int parse_config(const char *buf, long len) {
  const char *end = buf + len;
  // find_lit packs eight bytes at a time and this literal is seven, so scan
  // for it by hand. A "tools" that turns out not to introduce an object is
  // some other string that happens to read the same -- a comment member, say
  // -- so keep looking rather than rejecting the file over it.
  const char *p = buf, *body = 0;
  for (const char *q = buf; q + 7 <= end; q++) {
    if (q[0] != '"' || !cc_streq(q, "\"tools\"", 7))
      continue;
    const char *r = skip_ws(q + 7);
    if (r >= end || *r != ':')
      continue;
    r = skip_ws(r + 1);
    if (r >= end || *r != '{')
      continue;
    body = r + 1;
    break;
  }
  if (!body)
    return 0;
  p = body;

  NTOOLS = 0;
  for (;;) {
    p = skip_ws(p);
    if (p >= end)
      return 0;
    if (*p == '}')
      break;
    if (*p == ',') {
      p++;
      continue;
    }
    struct jstr name;
    p = j_string(p, end, &name);
    if (!p)
      return 0;
    p = parse_tool(p, end, name);
    if (!p)
      return 0;
  }
  return NTOOLS > 0;
}

// --------------------------------------------------------- config discovery --
static char cfgpath[PATH_CAP];

// One pass over envp for every key we need: a per-key lookup costs a full
// scan each, and this environment carries 619 entries.
static const char *config_path(char **envp) {
  const char *over = 0, *dir = 0, *home = 0;
  if (envp)
    for (char **e = envp; *e; e++) {
      const char *s = *e;
      if (!over && cc_streq(s, "CC_SHIM_CONFIG", 14) && s[14] == '=')
        over = s + 15;
      else if (!dir && cc_streq(s, "CLAUDE_CONFIG_DIR", 17) && s[17] == '=')
        dir = s + 18;
      else if (!home && cc_streq(s, "HOME", 4) && s[4] == '=')
        home = s + 5;
      else if (!DEBUG && cc_streq(s, "CC_SHIM_DEBUG", 13) && s[13] == '=' && s[14] && s[14] != '0')
        DEBUG = 1;
    }
  if (over && *over)
    return over;

  unsigned n = 0;
  const char *base = dir && *dir ? dir : home;
  if (!base || !*base)
    return 0;
  unsigned bl = cc_slen(base);
  const char *tail = dir && *dir ? "/hooks/shim-tools.json" : "/.claude/hooks/shim-tools.json";
  unsigned tl = cc_slen(tail);
  if (bl + tl + 1 > PATH_CAP)
    return 0;
  for (unsigned i = 0; i < bl; i++) cfgpath[n++] = base[i];
  while (n > 1 && cfgpath[n - 1] == '/') n--;
  for (unsigned i = 0; i < tl; i++) cfgpath[n++] = tail[i];
  cfgpath[n] = '\0';
  return cfgpath;
}

static void load_tools(char **envp) {
  const char *path = config_path(envp);
  if (!path) {
    if (DEBUG)
      warn_str("# cc-shim-hook: no config path, using built-ins\n");
    load_builtins();
    return;
  }
  int fd = cc_open(path);
  if (fd < 0) {
    if (DEBUG) {
      warn_str("# cc-shim-hook: no config at ");
      warn_str(path);
      warn_str(", using built-ins\n");
    }
    load_builtins();
    return;
  }
  long len = 0;
  for (;;) {
    long r = cc_readfd(fd, cfgbuf + len, CFG_CAP - (unsigned long)len);
    if (r <= 0) {
      if (r == -CC_EINTR)
        continue;
      break;
    }
    len += r;
    if ((unsigned long)len >= CFG_CAP)
      break;
  }
  cc_close(fd);
  cfgbuf[len] = '\0';

  if (len <= 0 || !parse_config(cfgbuf, len)) {
    if (DEBUG) {
      warn_str("# cc-shim-hook: rejected config ");
      warn_str(path);
      warn_str(", using built-ins\n");
    }
    load_builtins();
    return;
  }
  table_finish();
  if (DEBUG) {
    warn_str("# cc-shim-hook: config ");
    warn_str(path);
    warn_str(" tools:");
    for (unsigned i = 0; i < NTOOLS; i++) {
      warn_str(" ");
      warn_slice(TOOLS[i].name, TOOLS[i].nlen);
    }
    warn_str("\n");
  }
}

// -------------------------------------------------------------- token scan --
// Does a shell word boundary + a configured name start at p? s is the value
// start, end its closing quote (always readable, never an identifier byte).
static unsigned tok_at(const char *s, const char *p, const char *end) {
  long avail = end - p;
  uint32_t w4 = avail >= 4 ? ld32(p) : 0;
  for (unsigned i = 0; i < NTOOLS; i++) {
    const struct tool *t = &TOOLS[i];
    unsigned n = t->nlen;
    if (avail < (long)n)
      continue;
    if (n == 4) {
      if (w4 != t->w4)
        continue;
    } else if (!cc_streq(p, t->name, n))
      continue;
    // A JSON escape (\n, \t, ...) reads as an identifier byte but is a boundary.
    if (p > s && ident_ch((unsigned char)p[-1]) && !(p - 1 > s && p[-2] == '\\'))
      return 0;
    if (ident_ch((unsigned char)p[n]))
      return 0;
    return 1u << i;
  }
  return 0;
}

static unsigned scan_tokens(const char *s, const char *end) {
  unsigned hit = 0, all = (1u << NTOOLS) - 1u;
  const char *p = s;
#ifdef CC_VEC
  while (p + CC_VEC <= end) {
    cc_mask m = cc_eq_set(p, NCAND);
    while (m) {
      unsigned i = CC_IDX(m);
      hit |= tok_at(s, p + i, end);
      m = CC_CLR(m, i);
    }
    if (hit == all)
      return hit;
    p += CC_VEC;
  }
#endif
  for (; p < end; p++)
    if (IS_CAND[(unsigned char)*p] && (hit |= tok_at(s, p, end)) == all)
      break;
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

__attribute__((noreturn)) static void cc_run(char **envp) {
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
  if (ce - cb >= (long)(sizeof MARKER - 1) && ld64(cb) == PACK8(": __cc_s") && ld64(cb + 8) == PACK8("him_over") &&
      ld64(cb + 14) == PACK8("erride; "))
    reply_noop();

  // Only now is the config worth reading: everything above can no-op without
  // ever knowing what the tool table is.
  load_tools(envp);

  unsigned hit = scan_tokens(cb, ce);
  if (!hit)
    reply_noop();

  struct cc_iov v[5 + MAX_TOOLS * 14];
  long nv = 0;
  PUSH(OUT_HEAD, sizeof OUT_HEAD - 1);
  PUSH(ti, cb - ti);
  PUSH(MARKER, sizeof MARKER - 1);
  for (unsigned i = 0; i < NTOOLS; i++) {
    if (!(hit & (1u << i)))
      continue;
    const struct tool *t = &TOOLS[i];
    PUSH(t->name, t->nlen);
    PUSH(FN_OPEN, sizeof FN_OPEN - 1);
    if (t->belen) {
      PUSH(FN_SUBST, sizeof FN_SUBST - 1);
      PUSH(t->bin_env, t->belen);
      PUSH(FN_BIN_DEF, sizeof FN_BIN_DEF - 1);
      PUSH(t->bin, t->blen);
      PUSH(FN_SUBST_END, sizeof FN_SUBST_END - 1);
    } else {
      PUSH(t->bin, t->blen);
    }
    PUSH(FN_MID, sizeof FN_MID - 1);
    if (t->aelen) {
      PUSH(FN_SUBST, sizeof FN_SUBST - 1);
      PUSH(t->args_env, t->aelen);
      PUSH(FN_ARGS_DEF, sizeof FN_ARGS_DEF - 1);
      PUSH(t->args, t->alen);
      PUSH(FN_ARGS_END, sizeof FN_ARGS_END - 1);
    } else if (t->alen) {
      PUSH(t->args, t->alen);
      PUSH(FN_SPACE, sizeof FN_SPACE - 1);
    }
    PUSH(FN_CLOSE, sizeof FN_CLOSE - 1);
  }
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
// off what a compiled function expects, so realign before calling in. The
// original stack pointer is passed through because envp lives on it and there
// is no `environ` to read without libc.
__attribute__((used, noreturn)) void cc_entry(long *sp) {
  long argc = sp[0];
  char **argv = (char **)(sp + 1);
  cc_run(argv + argc + 1);
}
  #if defined(__x86_64__)
__asm__(
    ".globl _start\n"
    ".type _start,@function\n"
    "_start:\n\t"
    "xor %ebp, %ebp\n\t"
    "mov %rsp, %rdi\n\t"
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
    "mov x0, sp\n\t"
    "bl cc_entry\n\t"
    "brk #0\n");
  #else
    #error "CC_FREESTANDING: unsupported architecture"
  #endif
#else
int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  cc_run(envp);
}
#endif
