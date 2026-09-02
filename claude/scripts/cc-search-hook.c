// cc-search-hook: Claude Code PreToolUse(Bash) hook.
//
// Rewrites the Bash tool's command so grep/find/rg resolve to configurable
// full-featured binaries, by prepending shell function overrides. The overrides
// win because the command is evaluated AFTER Claude Code sources its shell
// snapshot, so they shadow the snapshot's own grep/find functions. Each search
// execs the selected binary DIRECTLY: no cc-applet trampoline, no
// CLAUDE_CODE_EXECPATH indirection.
//
// Cost model (Apple silicon, macOS 27). A hook invocation costs ~1.5 ms of
// fork/exec/dyld that no userspace code can remove: macOS 27/arm64 SIGKILLs
// non-dyld static executables and ld refuses dynamic ones that do not link
// libSystem, so the dyld+libSystem floor is mandatory. Measured, this program
// is indistinguishable from `int main(void){return 0;}`. The wins that remain
// are therefore in what it EMITS, and they are large:
//
//   * inject nothing unless the command actually names grep/find/rg
//   * define only the functions the command actually names
//   * the wrapper is a direct `command "$bin" $args "$@"`: no dispatcher
//     function, no `local`, no arrays, no `eval`
//     (39.2 us -> 5.9 us per search call, 27 us -> 8..17 us of prefix parse)
//
// The binary itself sits on the floor: zero libSystem symbol imports (raw arm64
// svc syscalls), no output buffer at all (one writev(2) of constant and input
// slices), and a NEON JSON scan. Build:
//
//   clang -O3 -fno-stack-protector -fno-unwind-tables \
//     -fno-asynchronous-unwind-tables -Wl,-dead_strip -Wl,-x \
//     -o cc-search-hook cc-search-hook.c
//   strip -x cc-search-hook && codesign -f -s - cc-search-hook
//
// -march=native buys nothing (NEON is baseline on arm64) and would risk an
// illegal instruction on another Mac. strip invalidates the linker's ad-hoc
// signature, so the codesign step is mandatory, not cosmetic: arm64 SIGKILLs
// unsigned binaries.
//
// CLAUDE_CODE_{UGREP,BFS,RG} select executables; unset or empty values use the
// paths below. The matching *_ARGS variables hold trusted shell words. They are
// word-split by the shell, not re-parsed by eval, so quotes inside them stay
// literal and glob characters inside them are expanded against the cwd. Unset
// *_ARGS uses the defaults below; an explicitly empty value passes none.

#include <stdint.h>

#if defined(__APPLE__) && defined(__aarch64__)
#define CC_RAW 1
#endif
#if defined(__ARM_NEON)
#include <arm_neon.h>
#define CC_NEON 1
#endif

#define IN_CAP (1u << 20)
#define EINTR_ 4

struct cc_iov {
    const void *base;
    unsigned long len;
};

#ifdef CC_RAW
// arm64 macOS: syscall number in x16, BSD errors reported through the carry
// flag. Returns -errno on failure, mirroring the Linux convention.
static inline long cc_syscall(long num, long a0, long a1, long a2) {
    register long x0 __asm__("x0") = a0;
    register long x1 __asm__("x1") = a1;
    register long x2 __asm__("x2") = a2;
    register long x16 __asm__("x16") = num;
    __asm__ volatile("svc #0x80\n\tb.cc 1f\n\tneg x0, x0\n1:"
                     : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x16)
                     :
                     : "cc", "memory", "x3", "x4", "x5", "x6", "x7", "x8", "x9",
                       "x10", "x11", "x12", "x13", "x14", "x15", "x17");
    return x0;
}
static inline long cc_read(void *buf, unsigned long n) { return cc_syscall(3, 0, (long)buf, (long)n); }
static inline long cc_writev(const struct cc_iov *v, long n) { return cc_syscall(121, 1, (long)v, n); }
__attribute__((noreturn)) static void cc_exit(void) {
    cc_syscall(1, 0, 0, 0);
    __builtin_unreachable();
}
#else
// Portable fallback. struct cc_iov is layout-compatible with struct iovec.
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
static inline long cc_read(void *buf, unsigned long n) {
    long r = (long)read(0, buf, (size_t)n);
    return r < 0 ? (errno == EINTR ? -EINTR_ : -1) : r;
}
static inline long cc_writev(const struct cc_iov *v, long n) {
    long r = (long)writev(1, (const struct iovec *)v, (int)n);
    return r < 0 ? (errno == EINTR ? -EINTR_ : -1) : r;
}
__attribute__((noreturn)) static void cc_exit(void) { _exit(0); }
#endif

// Little-endian packing so multi-byte literal compares fold to one load+cmp.
#define PACK4(s)                                                                                   \
    ((uint32_t)(uint8_t)(s)[0] | ((uint32_t)(uint8_t)(s)[1] << 8) |                                \
     ((uint32_t)(uint8_t)(s)[2] << 16) | ((uint32_t)(uint8_t)(s)[3] << 24))
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

// Slack past IN_CAP absorbs the NUL plus the 8-byte over-reads of ld32/ld64.
static char inbuf[IN_CAP + 32];

static const char OUT_HEAD[] =
    "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":";
static const char OUT_TAIL[] = "}}";
static const char NOOP[] = "{}";

// Injected shell text. The \" sequences are literal backslash-quote bytes so
// the surrounding JSON string stays valid; Claude decodes them to plain " for
// the shell. MARKER makes re-entry a no-op and must stay in sync with the
// three 8-byte compares in main().
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

#define PUSH(p, n)                                                                                 \
    do {                                                                                           \
        v[nv].base = (p);                                                                          \
        v[nv].len = (unsigned long)(n);                                                            \
        nv++;                                                                                      \
    } while (0)

static void emit(struct cc_iov *v, long n) {
    while (n > 0) {
        long w = cc_writev(v, n);
        if (w <= 0) {
            if (w == -EINTR_) continue;
            return;
        }
        while (n > 0 && (unsigned long)w >= v->len) {
            w -= (long)v->len;
            v++;
            n--;
        }
        if (n == 0) return;
        v->base = (const char *)v->base + w;
        v->len -= (unsigned long)w;
    }
}

__attribute__((noreturn)) static void reply_noop(void) {
    struct cc_iov v = {NOOP, sizeof NOOP - 1};
    emit(&v, 1);
    cc_exit();
}

#ifdef CC_NEON
// One nibble per lane; ctz(mask)>>2 is the matching byte index.
static inline uint64_t movemask(uint8x16_t v) {
    return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(v), 4)), 0);
}
#endif

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

// Find literal lit (n >= 8 bytes, first 8 packed in k0) in [p, end); returns the
// position just past it.
static const char *find_lit(const char *p, const char *end, const char *lit, unsigned n, uint64_t k0) {
    for (; p + n <= end; p++) {
        if (ld64(p) != k0) continue;
        unsigned i = 8;
        while (i < n && p[i] == lit[i]) i++;
        if (i == n) return p + n;
    }
    return 0;
}

// Closing quote of a JSON string starting at p, honouring backslash escapes.
static const char *str_end(const char *p, const char *end) {
#ifdef CC_NEON
    const uint8x16_t vq = vdupq_n_u8('"'), vb = vdupq_n_u8('\\');
    while (p + 16 <= end) {
        uint8x16_t v = vld1q_u8((const uint8_t *)p);
        uint64_t m = movemask(vorrq_u8(vceqq_u8(v, vq), vceqq_u8(v, vb)));
        if (m) {
            p += (unsigned)__builtin_ctzll(m) >> 2;
            if (*p == '"') return p;
            p += 2;
            continue;
        }
        p += 16;
    }
#endif
    while (p < end) {
        char c = *p;
        if (c == '"') return p;
        p += (c == '\\') ? 2 : 1;
    }
    return 0;
}

enum { HIT_GREP = 1, HIT_FIND = 2, HIT_RG = 4, HIT_ALL = 7 };

static inline int ident_ch(unsigned char c) {
    return (unsigned)((c | 32) - 'a') < 26u || (unsigned)(c - '0') < 10u || c == '_' || c == '.' ||
           c == '-';
}

// Does a shell word boundary + needle start at p? s is the value start, end its
// closing quote (always readable, never an identifier byte).
static unsigned tok_at(const char *s, const char *p, const char *end) {
    unsigned bit, n;
    if (*p == 'r') {
        if (end - p < 2 || p[1] != 'g') return 0;
        n = 2;
        bit = HIT_RG;
    } else {
        if (end - p < 4) return 0;
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
    if (p > s && ident_ch((unsigned char)p[-1]) && !(p - 1 > s && p[-2] == '\\')) return 0;
    if (ident_ch((unsigned char)p[n])) return 0;
    return bit;
}

static unsigned scan_tokens(const char *s, const char *end) {
    unsigned hit = 0;
    const char *p = s;
#ifdef CC_NEON
    const uint8x16_t vg = vdupq_n_u8('g'), vf = vdupq_n_u8('f'), vr = vdupq_n_u8('r');
    while (p + 16 <= end) {
        uint8x16_t v = vld1q_u8((const uint8_t *)p);
        uint64_t m = movemask(vorrq_u8(vorrq_u8(vceqq_u8(v, vg), vceqq_u8(v, vf)), vceqq_u8(v, vr)));
        while (m) {
            unsigned i = (unsigned)__builtin_ctzll(m) >> 2;
            hit |= tok_at(s, p + i, end);
            m &= ~(0xFULL << (i * 4));
        }
        if (hit == HIT_ALL) return hit;
        p += 16;
    }
#endif
    for (; p < end; p++) {
        char c = *p;
        if ((c == 'g' || c == 'f' || c == 'r') && (hit |= tok_at(s, p, end)) == HIT_ALL) break;
    }
    return hit;
}

// Walk the tool_input object at p (its '{'); capture the command value between
// its quotes. Returns one past the object's '}', or 0 if malformed.
static const char *walk_object(const char *p, const char *end, const char **cb, const char **ce) {
    int depth = 0;
    for (;;) {
        if (p >= end) return 0;
        char c = *p;
        if (c == '"') {
            const char *vs = p + 1;
            const char *q = str_end(vs, end);
            if (!q) return 0;
            if (depth == 1 && !*cb && q - vs == 7 && ld32(vs) == PACK4("comm") &&
                ld32(vs + 3) == PACK4("mand")) {
                const char *r = skip_ws(q + 1);
                if (*r == ':') {
                    r = skip_ws(r + 1);
                    if (*r == '"') {
                        *cb = r + 1;
                        *ce = str_end(*cb, end);
                        if (!*ce) return 0;
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
            if (--depth == 0) return p;
        } else {
            p++;
        }
    }
}

int main(void) {
    unsigned long len = 0;
    for (;;) {
        long r = cc_read(inbuf + len, IN_CAP - len);
        if (r <= 0) {
            if (r == -EINTR_) continue;
            break;
        }
        len += (unsigned long)r;
        // Oversized payload: drain so the writer never sees EPIPE, then no-op.
        if (len >= IN_CAP) {
            while ((r = cc_read(inbuf, IN_CAP)) > 0 || r == -EINTR_) {}
            reply_noop();
        }
    }
    inbuf[len] = '\0';
    const char *end = inbuf + len;

    const char *p = find_lit(inbuf, end, "\"tool_name\"", 11, PACK8("\"tool_na"));
    if (p) {
        p = skip_ws(p);
        if (*p != ':') reply_noop();
        p = skip_ws(p + 1);
        if (*p != '"' || ld32(p + 1) != PACK4("Bash") || p[5] != '"') reply_noop();
    }

    p = find_lit(inbuf, end, "\"tool_input\"", 12, PACK8("\"tool_in"));
    if (!p) reply_noop();
    p = skip_ws(p);
    if (*p != ':') reply_noop();
    p = skip_ws(p + 1);
    if (*p != '{') reply_noop();

    const char *ti = p, *cb = 0, *ce = 0;
    const char *oe = walk_object(ti, end, &cb, &ce);
    if (!oe || !cb) reply_noop();

    // Already rewritten: the marker is injected at the head, so this is O(1).
    if (ce - cb >= (long)(sizeof MARKER - 1) && ld64(cb) == PACK8(": __cc_s") &&
        ld64(cb + 8) == PACK8("earch_ov") && ld64(cb + 16) == PACK8("erride; "))
        reply_noop();

    unsigned hit = scan_tokens(cb, ce);
    if (!hit) reply_noop();

    struct cc_iov v[8];
    long nv = 0;
    PUSH(OUT_HEAD, sizeof OUT_HEAD - 1);
    PUSH(ti, cb - ti);
    PUSH(MARKER, sizeof MARKER - 1);
    if (hit & HIT_GREP) PUSH(FN_GREP, sizeof FN_GREP - 1);
    if (hit & HIT_FIND) PUSH(FN_FIND, sizeof FN_FIND - 1);
    if (hit & HIT_RG) PUSH(FN_RG, sizeof FN_RG - 1);
    PUSH(cb, oe - cb);
    PUSH(OUT_TAIL, sizeof OUT_TAIL - 1);
    emit(v, nv);
    cc_exit();
}
