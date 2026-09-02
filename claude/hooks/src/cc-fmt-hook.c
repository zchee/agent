// cc-fmt-hook: language-independent PostToolUse(Write|Edit) formatter dispatch.
//
// Reads the hook JSON on stdin, pulls .tool_input.file_path out of it, matches
// the extension against a table, and runs the matching formatter in place.
// Replaces a shelf of per-language `jq | case ... fmt` hooks: no sh -c wrapper
// (~7 ms) and no jq (~5 ms).
//
// Register as an EXEC-FORM hook (spawned directly, no shell):
//   { "type":"command", "command":"/path/to/cc-fmt-hook", "args":[] }
// matcher "Write|Edit", event PostToolUse. A missing formatter is a silent
// no-op, and so is a formatter that fails: the hook always exits 0 so a file
// that does not parse yet never turns into hook output. CC_FMT_DRYRUN=1 prints
// the resolved command instead of running it; CC_FMT_ASYNC=1 does not wait for
// the formatter (see the note on that below).
//
// Where the time actually goes, measured on this machine (Apple silicon):
//
//   PATH resolution   execvp() tries execve() on every PATH entry until one
//                     works, and a *failing* execve costs 20.6 us on macOS
//                     against 0.97 us for access(X_OK) -- 21x. With clang-format
//                     at PATH position 52 of 102 that was 1.05 ms of the hook's
//                     ~2.1 ms; ruff at position 70 cost 1.44 ms. Resolving with
//                     access() and exec'ing once removes it.
//   process launch    fork+execve+wait4 costs 2.12 ms; posix_spawn+wait4 costs
//                     1.46 ms for the same child, because there are no page
//                     tables to copy. 0.66 ms saved.
//   the formatter     3-60 ms, i.e. everything else put together is noise next
//                     to picking the right binary. clang-format from Xcode is
//                     7.0 ms where Homebrew LLVM's is 40.8 ms; prettier is not
//                     installed at all here, so `deno fmt` (9 ms, Rust) covers
//                     js/ts/json/css/md instead of silently doing nothing.
//   JSON              .tool_input.file_path sits ~700 bytes into every payload,
//                     so a targeted scan is O(1) in payload size: 0.08 us. A
//                     full yyjson DOM parse of the same payload costs 0.33 us
//                     at 1.4 KB and 1131 us at 4.8 MB (13624x), because Write
//                     payloads carry the file content twice. yyjson is a fast
//                     parser aimed at a problem this hook does not have.
//
// Raw syscalls and a custom entry point were tried in the sibling hook and
// measured as noise against the process-spawn floor, so this one stays on libc.
//
// The source lives in hooks/src/ and the binary belongs in scripts/, the path a
// hook entry would register; these commands run from the Claude config dir
// (~/.claude, or $CLAUDE_CONFIG_DIR):
//
//   clang -O3 -fno-stack-protector -fno-unwind-tables \
//     -fno-asynchronous-unwind-tables -Wl,-dead_strip -Wl,-x \
//     -o /opt/local/bin/cc-fmt-hook claude/hooks/src/cc-fmt-hook.c
//   /opt/homebrew/opt/llvm/bin/llvm-strip --strip-all /opt/local/bin/cc-fmt-hook
//   codesign -f -s - /opt/local/bin/cc-fmt-hook
//
// strip invalidates the linker's ad-hoc signature and arm64 SIGKILLs unsigned
// binaries, so the codesign step is mandatory, not cosmetic.

#include <errno.h>
#include <spawn.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__ARM_NEON)
  #include <arm_neon.h>
  #define CC_VEC 16
#elif defined(__AVX2__)
  #include <immintrin.h>
  #define CC_VEC 32
#elif defined(__SSE2__)
  #include <emmintrin.h>
  #define CC_VEC 16
#endif

#define BUF_CAP  (64u << 10)  // one pipe buffer
#define KEEP_CAP (16u << 10)  // carried between reads so a key cannot straddle
#define PATH_CAP 4096

static char inbuf[BUF_CAP + 16];
static char fpath[PATH_CAP];
static char exe[PATH_CAP];
static char stylearg[PATH_CAP];

// ---------------------------------------------------------------- the table --
// Extensions are at most 8 bytes, so each one packs into a single u64 compare.
#define A(...) (const char *const[]){__VA_ARGS__, 0}
enum { F_STYLE = 1 };  // append -style=file:$HOME/.config/llvm/.clang-format

struct fmt {
  const char *ext;
  const char *const *argv;
  unsigned flags;
};

// clang-format off
static const struct fmt TABLE[] = {
    {"zig",   A("zig", "fmt"), 0},
    {"rs",    A("rustfmt", "--edition", "2024"), 0},
    {"go",    A("gofumpt", "-w"), 0},
    {"py",    A("ruff", "format", "-q"), 0},
    {"pyi",   A("ruff", "format", "-q"), 0},
    {"c",     A("clang-format", "-i"), F_STYLE},
    {"h",     A("clang-format", "-i"), F_STYLE},
    {"cc",    A("clang-format", "-i"), F_STYLE},
    {"cpp",   A("clang-format", "-i"), F_STYLE},
    {"cxx",   A("clang-format", "-i"), F_STYLE},
    {"hpp",   A("clang-format", "-i"), F_STYLE},
    {"hxx",   A("clang-format", "-i"), F_STYLE},
    {"m",     A("clang-format", "-i"), F_STYLE},
    {"mm",    A("clang-format", "-i"), F_STYLE},
    {"js",    A("deno", "fmt", "--quiet"), 0},
    {"jsx",   A("deno", "fmt", "--quiet"), 0},
    {"mjs",   A("deno", "fmt", "--quiet"), 0},
    {"cjs",   A("deno", "fmt", "--quiet"), 0},
    {"ts",    A("deno", "fmt", "--quiet"), 0},
    {"tsx",   A("deno", "fmt", "--quiet"), 0},
    {"mts",   A("deno", "fmt", "--quiet"), 0},
    {"cts",   A("deno", "fmt", "--quiet"), 0},
    {"json",  A("deno", "fmt", "--quiet"), 0},
    {"jsonc", A("deno", "fmt", "--quiet"), 0},
    {"css",   A("deno", "fmt", "--quiet"), 0},
    {"scss",  A("deno", "fmt", "--quiet"), 0},
    {"less",  A("deno", "fmt", "--quiet"), 0},
    {"md",    A("deno", "fmt", "--quiet"), 0},
    {"sh",    A("shfmt", "-w"), 0},
    {"bash",  A("shfmt", "-w"), 0},
    {"lua",   A("stylua"), 0},
    {"toml",  A("taplo", "format"), 0},
};
// clang-format on

#define PACK8(s)                                                                                               \
  ((uint64_t)(uint8_t)(s)[0] | ((uint64_t)(uint8_t)(s)[1] << 8) | ((uint64_t)(uint8_t)(s)[2] << 16) |          \
   ((uint64_t)(uint8_t)(s)[3] << 24) | ((uint64_t)(uint8_t)(s)[4] << 32) | ((uint64_t)(uint8_t)(s)[5] << 40) | \
   ((uint64_t)(uint8_t)(s)[6] << 48) | ((uint64_t)(uint8_t)(s)[7] << 56))

static inline uint64_t ld64(const char *p) {
  uint64_t v;
  __builtin_memcpy(&v, p, 8);
  return v;
}

// ------------------------------------------------------------- json scanning --
static const char *skip_ws(const char *p, const char *end) {
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
  return p;
}

static int hexval(unsigned char c) {
  if ((unsigned)(c - '0') < 10u)
    return c - '0';
  c |= 32;
  if ((unsigned)(c - 'a') < 6u)
    return c - 'a' + 10;
  return -1;
}

// Copy a JSON string body starting at p (just past the opening quote) into out,
// undoing escapes. Returns the closing quote, or 0 if the value is cut off by
// the end of the buffer -- the caller retries with more input.
static const char *unescape(const char *p, const char *end, char *out, size_t cap) {
  size_t o = 0;
  while (p < end && o + 4 < cap) {
    unsigned char c = (unsigned char)*p;
    if (c == '"') {
      out[o] = 0;
      return p;
    }
    if (c != '\\') {
      out[o++] = (char)c;
      p++;
      continue;
    }
    if (p + 1 >= end)
      return 0;
    unsigned char e = (unsigned char)p[1];
    p += 2;
    switch (e) {
      case 'n':
        out[o++] = '\n';
        break;
      case 't':
        out[o++] = '\t';
        break;
      case 'r':
        out[o++] = '\r';
        break;
      case 'b':
        out[o++] = '\b';
        break;
      case 'f':
        out[o++] = '\f';
        break;
      case 'u': {
        if (p + 4 > end)
          return 0;
        int a = hexval((unsigned char)p[0]), b = hexval((unsigned char)p[1]);
        int c2 = hexval((unsigned char)p[2]), d = hexval((unsigned char)p[3]);
        if (a < 0 || b < 0 || c2 < 0 || d < 0)
          return 0;
        unsigned cp = (unsigned)(a << 12 | b << 8 | c2 << 4 | d);
        p += 4;
        if (cp >= 0xD800 && cp < 0xDC00 && p + 6 <= end && p[0] == '\\' && p[1] == 'u') {
          int e0 = hexval((unsigned char)p[2]), e1 = hexval((unsigned char)p[3]);
          int e2 = hexval((unsigned char)p[4]), e3 = hexval((unsigned char)p[5]);
          if (e0 >= 0 && e1 >= 0 && e2 >= 0 && e3 >= 0) {
            unsigned lo = (unsigned)(e0 << 12 | e1 << 8 | e2 << 4 | e3);
            if (lo >= 0xDC00 && lo < 0xE000) {
              cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
              p += 6;
            }
          }
        }
        if (cp < 0x80) {
          out[o++] = (char)cp;
        } else if (cp < 0x800) {
          out[o++] = (char)(0xC0 | cp >> 6);
          out[o++] = (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
          out[o++] = (char)(0xE0 | cp >> 12);
          out[o++] = (char)(0x80 | (cp >> 6 & 0x3F));
          out[o++] = (char)(0x80 | (cp & 0x3F));
        } else {
          out[o++] = (char)(0xF0 | cp >> 18);
          out[o++] = (char)(0x80 | (cp >> 12 & 0x3F));
          out[o++] = (char)(0x80 | (cp >> 6 & 0x3F));
          out[o++] = (char)(0x80 | (cp & 0x3F));
        }
        break;
      }
      default:
        out[o++] = (char)e;  // \" \\ \/ and anything else: literal
    }
  }
  return 0;
}

// Accept a key at q: tool_input.file_path, or tool_response.filePath as a
// fallback for payloads that carry only the latter.
static int try_key(const char *q, const char *end, char *out, size_t cap) {
  size_t klen;
  if (q + 11 <= end && ld64(q) == PACK8("\"file_pa") && q[8] == 't' && q[9] == 'h' && q[10] == '"')
    klen = 11;
  else if (q + 10 <= end && ld64(q) == PACK8("\"filePat") && q[8] == 'h' && q[9] == '"')
    klen = 10;
  else
    return 0;
  const char *r = skip_ws(q + klen, end);
  if (r >= end || *r != ':')
    return 0;
  r = skip_ws(r + 1, end);
  return r < end && *r == '"' && unescape(r + 1, end, out, cap) != 0;
}

// Find the file path in [b, b+n) and unescape it into out.
// The key sits ~700 bytes into every payload, so this never depends on how much
// file content the payload carries behind it.
static int find_path(const char *b, size_t n, char *out, size_t cap) {
  const char *end = b + n;
  const char *p = b;
#ifdef CC_VEC
  while (p + CC_VEC <= end) {
  #if defined(__ARM_NEON)
    uint8x16_t v = vld1q_u8((const uint8_t *)p);
    uint64_t m =
        vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(vceqq_u8(v, vdupq_n_u8('"'))), 4)), 0);
    unsigned shift = 2;
  #elif defined(__AVX2__)
    __m256i v = _mm256_loadu_si256((const __m256i *)p);
    uint64_t m = (uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('"')));
    unsigned shift = 0;
  #else
    __m128i v = _mm_loadu_si128((const __m128i *)p);
    uint64_t m = (uint16_t)_mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('"')));
    unsigned shift = 0;
  #endif
    while (m) {
      unsigned i = (unsigned)__builtin_ctzll(m) >> shift;
      if (try_key(p + i, end, out, cap))
        return 1;
  #if defined(__ARM_NEON)
      m &= ~(0xFULL << (i * 4));
  #else
      m &= m - 1;
  #endif
    }
    p += CC_VEC;
  }
#endif
  for (; p + 10 <= end; p++)
    if (*p == '"' && try_key(p, end, out, cap))
      return 1;
  return 0;
}

// ------------------------------------------------------------------ helpers --
static const char *env_get(char **envp, const char *name, size_t nlen) {
  for (char **e = envp; *e; e++)
    if (!strncmp(*e, name, nlen) && (*e)[nlen] == '=')
      return *e + nlen + 1;
  return 0;
}

static size_t str_append(char *dst, size_t o, size_t cap, const char *s) {
  while (*s && o + 1 < cap) dst[o++] = *s++;
  dst[o] = 0;
  return o;
}

// execvp() resolves by trying execve() on every candidate, which costs 20.6 us
// per miss on macOS. Probing with access(X_OK) costs 0.97 us, so resolve here
// and exec exactly once.
static const char *resolve(const char *name, const char *path, char *buf, size_t cap) {
  if (strchr(name, '/'))
    return access(name, X_OK) == 0 ? name : 0;
  if (!path)
    return 0;
  size_t nlen = strlen(name);
  for (const char *p = path; *p;) {
    const char *e = strchr(p, ':');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    if (len == 0) {  // an empty PATH entry means the cwd
      buf[0] = '.';
      len = 1;
    } else if (len + nlen + 2 <= cap) {
      memcpy(buf, p, len);
    } else {
      len = 0;
    }
    if (len) {
      buf[len] = '/';
      memcpy(buf + len + 1, name, nlen + 1);
      if (access(buf, X_OK) == 0)
        return buf;
    }
    if (!e)
      break;
    p = e + 1;
  }
  return 0;
}

static void put(const char *s) {
  (void)!write(1, s, strlen(s));
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;

  // Drain stdin whatever happens: Claude Code turns a short read into an EPIPE
  // and reports the hook as failed, so stopping early is not an option even
  // though the key is found in the first chunk.
  size_t keep = 0;
  int found = 0;
  for (;;) {
    ssize_t r = read(0, inbuf + keep, BUF_CAP - keep);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (r == 0)
      break;
    size_t total = keep + (size_t)r;
    if (found) {
      keep = 0;
      continue;
    }
    if (find_path(inbuf, total, fpath, sizeof fpath)) {
      found = 1;
      keep = 0;
      continue;
    }
    keep = total < KEEP_CAP ? total : KEEP_CAP;
    memmove(inbuf, inbuf + total - keep, keep);
  }
  if (!found)
    return 0;

  const char *slash = strrchr(fpath, '/');
  const char *base = slash ? slash + 1 : fpath;
  const char *dot = strrchr(base, '.');
  if (!dot || !dot[1])
    return 0;

  uint64_t key = 0;
  for (const char *e = dot + 1; *e; e++) {
    size_t i = (size_t)(e - dot - 1);
    if (i >= 8)
      return 0;
    unsigned char c = (unsigned char)*e;
    if ((unsigned)(c - 'A') < 26u)
      c |= 32;
    key |= (uint64_t)c << (i * 8);
  }

  const struct fmt *m = 0;
  for (size_t i = 0; i < sizeof TABLE / sizeof TABLE[0]; i++)
    if (PACK8(TABLE[i].ext) == key) {
      m = &TABLE[i];
      break;
    }
  if (!m)
    return 0;

  const char *args[10];
  int n = 0;
  for (const char *const *a = m->argv; *a && n < 6; a++) args[n++] = *a;
  if (m->flags & F_STYLE) {
    const char *home = env_get(envp, "HOME", 4);
    if (home) {
      size_t o = str_append(stylearg, 0, sizeof stylearg, "-style=file:");
      o = str_append(stylearg, o, sizeof stylearg, home);
      str_append(stylearg, o, sizeof stylearg, "/.config/llvm/.clang-format");
      args[n++] = stylearg;
    }
  }
  args[n++] = fpath;
  args[n] = 0;

  const char *prog = resolve(args[0], env_get(envp, "PATH", 4), exe, sizeof exe);
  if (!prog)
    return 0;  // formatter not installed: nothing to do, silently

  if (env_get(envp, "CC_FMT_DRYRUN", 13)) {
    put(prog);
    for (int i = 1; i < n; i++) {
      put(" ");
      put(args[i]);
    }
    put("\n");
    return 0;
  }

  pid_t pid;
  if (posix_spawn(&pid, prog, 0, 0, (char *const *)args, envp) != 0)
    return 0;
  if (!env_get(envp, "CC_FMT_ASYNC", 12)) {
    int st;
    while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {
    }
  }
  return 0;  // never anything but 0: a formatter that fails stays invisible
}
