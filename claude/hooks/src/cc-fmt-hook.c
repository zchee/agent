// cc-fmt-hook: config-driven PostToolUse(Write|Edit) formatter dispatch.
//
// Reads the hook JSON on stdin, pulls .tool_input.file_path out of it, matches
// the path against rules loaded from fmt-hooks.json, and runs the matching
// formatter in place. Replaces a shelf of per-language `jq | case ... fmt`
// hooks: no sh -c wrapper (~7 ms) and no jq (~5 ms).
//
// There are no built-in formatters. Every rule comes from a config file, so a
// machine with no fmt-hooks.json anywhere formats nothing at all. That makes a
// missing config indistinguishable from "nothing to do" at run time, which is
// what CC_FMT_DRYRUN=1 exists for: it names every config that was loaded or
// rejected and says which rule matched, or that none did.
// hooks/src/tests/fmt-hooks.example.json is a starting point covering the usual
// languages.
//
// Register as an EXEC-FORM hook (spawned directly, no shell):
//   { "type":"command", "command":"/path/to/cc-fmt-hook", "args":[] }
// matcher "Write|Edit", event PostToolUse. A missing formatter is a silent
// no-op, and so is a formatter that fails: the hook always exits 0, and the
// formatter's own stdin, stdout and stderr are /dev/null, so a file that does
// not parse yet never turns into hook output. CC_FMT_CONFIG adds a config that
// overrides every other, and CC_FMT_TRUST fences off where project configs are
// honoured -- see the config section below for both.
//
// CC_FMT_ASYNC=1 returns without waiting for the formatter. Two things have to
// be true for that to mean anything, and both are the reason the child gets
// /dev/null and a process group of its own:
//
//   the descriptors  Claude Code collects a hook's stdout and stderr as
//                    strings, so it waits for those pipes to reach EOF, not for
//                    the hook to exit. A child that inherits them holds the
//                    write ends open after the hook is gone and the wait
//                    happens anyway: measured here, a 2 s formatter let the
//                    hook exit in 4 ms and the pipes close at 2042 ms, which is
//                    exactly when the synchronous path finished. Handing the
//                    child /dev/null instead closes both at 4 ms. The same
//                    inheritance deadlocks a caller that pipes stdio without
//                    draining it -- 200 KB of formatter stderr fills the pipe,
//                    the formatter blocks in write(), the hook blocks in
//                    waitpid(), and neither ever returns.
//   the process group  an unwaited formatter outlives the hook, so a timeout
//                    that signals the hook's process group would reach it in
//                    the middle of rewriting a file. POSIX_SPAWN_SETPGROUP puts
//                    the async child in its own group; the synchronous child
//                    stays in the hook's group on purpose, because there
//                    abandoning the hook should abandon the formatter too.
//
// A rule that chains several formatters (see the config section) needs
// something to run them one after another once the hook has returned, so
// under CC_FMT_ASYNC the hook forks a runner that detaches the same way --
// /dev/null on its stdio, a process group of its own -- and walks the chain
// before exiting. That fork is the one place the hook pays for page tables; a
// single command, async or not, still goes through posix_spawn.
//
// ------------------------------------------------------------------ config --
// Config files are applied lowest precedence first. A later file overrides
// earlier ones rule by rule and leaves every rule it does not mention alone:
//
//   1. ${CLAUDE_CONFIG_DIR:-$HOME/.claude}/fmt-hooks.json   personal defaults
//   2. <dir>/.claude/fmt-hooks.json and <dir>/.fmt-hooks.json, for every
//      directory from / down to the edited file's own directory, outermost
//      first -- so a project overrides the personal defaults, and a subproject
//      in a monorepo overrides its parent
//   3. $CC_FMT_CONFIG                                       explicit override
//
// Anchoring the walk on the edited file rather than on the process cwd is what
// makes it correct across worktrees and monorepos: the hook runs from whatever
// directory the session happens to sit in, which is not necessarily anywhere
// near the file being written. For a file under $HOME the walk passes through
// $HOME/.claude/fmt-hooks.json again, which is harmless: it is the same content
// already applied as step 1, at the same relative precedence.
//
// A project config names programs to execute, and step 2 finds it by walking
// the tree rather than by asking anyone -- so a cloned repository that ships
// .claude/fmt-hooks.json gets to run a command of its choosing on every write,
// without the trust prompt Claude Code puts in front of a repository's
// .claude/settings.json. $CC_FMT_TRUST fences that off: set it to a
// colon-separated list of path prefixes, and step 2 runs only for files under
// one of them (matched at a '/' boundary, so /a/b does not admit /a/bc).
// Leaving it unset trusts every directory, which is the right default only on
// a machine where you also trust every checkout:
//
//   CC_FMT_TRUST=$HOME/src:$HOME/rust
//
// Steps 1 and 3 are yours by construction and are never fenced.
//
// Each file is a JSON object mapping a rule -- or a comma-separated list of
// rules -- to the argv that formats a file, with the path appended as the last
// argument, or to a list of such argvs that run one after another:
//
//   {
//     "rs":              ["rustfmt", "--edition", "2024"],
//     "c,h,cpp":         ["clang-format", "-i", "-style=file:${HOME}/x/.clang-format"],
//     "go":              [["gofmt", "-s", "-w"], ["gofumpt", "-w", "-extra"]],
//     "py":              [["ruff", "check", "--fix", "-q"], ["ruff", "format", "-q"]],
//     "Makefile":        ["some-make-formatter", "-w"],
//     "*_pb2.py":        [],
//     "*/vendor/*":      null
//   }
//
// A chain runs in order, each command on the file as the previous one left
// it, and every command runs regardless of what the earlier ones returned: an
// exit status is not a signal here, because `ruff check --fix` reports the
// violations it could not fix with status 1 after fixing the rest, and `ruff
// format` should still follow. A command that is not installed is skipped and
// the rest of the chain still runs. A chain is one value, so a project config
// that overrides "go" replaces the whole chain, not one link of it. An empty
// array or null turns a rule off; an empty array inside a chain is an error,
// since it names nothing to run. ${VAR} inside an argument is substituted from
// the environment and an argument naming an unset variable is dropped, which
// is what keeps machine specific paths out of a shared config.
//
// A rule holds at most CFG_ARGV tokens across its whole chain, counting one
// separator per command. A rule past that is registered as turned off rather
// than truncated -- a command missing its trailing "-w" would print to
// /dev/null and format nothing, which is worse than not running, and so would
// the lower-precedence rule this one meant to replace -- and CC_FMT_DRYRUN
// names it. The same rule applies to ${VAR} expansion: a command whose
// expanded arguments do not fit the buffer leaves the chain whole, never
// shortened.
//
// A formatter races the next edit of the same file whether or not it is
// chained; a chain only makes the window as long as its links put together,
// and two quick writes of one file can have two chains interleaving on it.
// That is inherent to formatting on PostToolUse, not something this hook can
// serialise, so prefer fast formatters in a chain and keep CC_FMT_ASYNC for
// the slow ones.
//
// A syntactically invalid file is discarded whole rather than half-applied:
// each file is parsed against a snapshot of the rules accumulated so far and
// rolled back on failure, so one broken project config cannot corrupt the
// personal defaults. It is discarded silently, too -- a hook that writes to
// stderr on every edit is worse than one that quietly does less.
//
// ----------------------------------------------------------------- matching --
// Three passes, in this order, first hit wins:
//
//   1. exact file name, case sensitive:  "Makefile", "Cargo.toml", ".clang-format"
//   2. glob, against the base name, or against the whole path when the rule
//      contains '/':  "*_test.go", "*/vendor/*", "*/testdata/*.json"
//      '*' crosses '/' (fnmatch without FNM_PATHNAME), so "*/vendor/*" matches
//      at any depth.
//   3. extension, case insensitive, written with or without the dot: "rs", ".rs"
//
// The order is what makes exceptions expressible:
//
//   {"Cargo.toml": [], "toml": ["taplo", "format"]}
//
// formats every .toml except Cargo.toml, because the name pass runs before the
// extension pass. A file whose base name starts with a dot has no extension
// (".gitignore" is a name, not an extension), so dotfiles only ever match
// passes 1 and 2.
//
// -------------------------------------------------------------------- costs --
// Where the time actually goes, measured on this machine (Apple silicon):
//
//   PATH resolution   execvp() tries execve() on every PATH entry until one
//                     works, and a *failing* execve costs 20.6 us on macOS
//                     against 0.97 us for access(X_OK) -- 21x. With rustfmt at
//                     PATH position 60 of 102 that is over 1 ms of a ~2 ms
//                     hook. Resolving with access() and exec'ing once removes
//                     it.
//   process launch    fork+execve+wait4 costs 2.12 ms; posix_spawn+wait4 costs
//                     1.46 ms for the same child, because there are no page
//                     tables to copy. 0.66 ms saved. The three /dev/null opens
//                     ride along inside the spawn as file actions, which the
//                     kernel performs in the child: no extra syscall here.
//   the formatter     3-60 ms, i.e. everything else put together is noise next
//                     to picking the right binary. clang-format from Xcode is
//                     7.0 ms where Homebrew LLVM's is 40.8 ms.
//   the config walk   two open() attempts per directory level, and a failing
//                     open() measures 0.45 us here, so a path 8 levels deep
//                     costs ~7 us whether or not any config exists. End to end
//                     that is unmeasurable: 1 level against 33 differ by less
//                     than the 0.5 ms run-to-run spread of spawning the hook
//                     at all, and ~7 us is 0.02% of a rustfmt run.
//   JSON              .tool_input.file_path sits ~700 bytes into every payload,
//                     so a targeted scan is O(1) in payload size: 0.08 us. A
//                     full yyjson DOM parse of the same payload costs 0.33 us
//                     at 1.4 KB and 1131 us at 4.8 MB (13624x), because Write
//                     payloads carry the file content twice. yyjson is a fast
//                     parser aimed at a problem this hook does not have.
//   rule lookup       three linear passes of strcmp/fnmatch over a few dozen
//                     rules. The u64-packed keys this file used while the table
//                     was compiled in are gone: they capped a rule at 8 bytes,
//                     which no longer covers "CMakeLists.txt", and they were
//                     saving nanoseconds against a millisecond spawn.
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
#include <fcntl.h>
#include <fnmatch.h>
#include <spawn.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__AVX2__)
  #include <immintrin.h>
  #define CC_VEC 32
#elif defined(__SSE2__)
  #include <emmintrin.h>
  #define CC_VEC 16
#elif defined(__ARM_NEON)
  #include <arm_neon.h>
  #define CC_VEC 16
#endif

#define BUF_CAP  (64u << 10)  // one pipe buffer
#define KEEP_CAP (16u << 10)  // carried between reads so a key cannot straddle
#define PATH_CAP 4096

#define CFG_CAP     (64u << 10)  // holds every config file read, concatenated
#define CFG_ENTRIES 192
#define CFG_ARGV    32              // tokens per rule, chain separators included
#define CMD_MAX     (CFG_ARGV / 2)  // a command is at least a program and its separator
#define ARENA_CAP   8192            // holds arguments after ${VAR} substitution
#define WALK_MAX    64              // directory levels searched for a project config

static char inbuf[BUF_CAP + 16];
static char fpath[PATH_CAP];
static char exe[CMD_MAX][PATH_CAP];
static char cfgbuf[CFG_CAP + 1];
static size_t cfgbuf_used;
static char arena[ARENA_CAP];
static size_t arena_used;
static int dryrun;

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

// Encode one code point; callers guarantee 4 bytes of room.
static size_t utf8(char *o, unsigned cp) {
  if (cp < 0x80) {
    o[0] = (char)cp;
    return 1;
  }
  if (cp < 0x800) {
    o[0] = (char)(0xC0 | cp >> 6);
    o[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    o[0] = (char)(0xE0 | cp >> 12);
    o[1] = (char)(0x80 | (cp >> 6 & 0x3F));
    o[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  }
  o[0] = (char)(0xF0 | cp >> 18);
  o[1] = (char)(0x80 | (cp >> 12 & 0x3F));
  o[2] = (char)(0x80 | (cp >> 6 & 0x3F));
  o[3] = (char)(0x80 | (cp & 0x3F));
  return 4;
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
        o += utf8(out + o, cp);
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

static int ci_eq(const char *a, const char *b) {
  for (;; a++, b++) {
    unsigned char x = (unsigned char)*a, y = (unsigned char)*b;
    if ((unsigned)(x - 'A') < 26u)
      x |= 32;
    if ((unsigned)(y - 'A') < 26u)
      y |= 32;
    if (x != y)
      return 0;
    if (!x)
      return 1;
  }
}

// access(X_OK) says yes to a directory too, and an empty program name turns
// every PATH entry into one; the stat runs only on a hit, so it costs nothing
// on the misses that dominate.
static int runnable(const char *p) {
  struct stat st;
  return access(p, X_OK) == 0 && stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

// execvp() resolves by trying execve() on every candidate, which costs 20.6 us
// per miss on macOS. Probing with access(X_OK) costs 0.97 us, so resolve here
// and exec exactly once.
static const char *resolve(const char *name, const char *path, char *buf, size_t cap) {
  if (!*name)
    return 0;
  if (strchr(name, '/'))
    return runnable(name) ? name : 0;
  if (!path)
    return 0;
  size_t nlen = strlen(name);
  for (const char *p = path; *p;) {
    const char *e = strchr(p, ':');
    const char *dir = p;
    size_t len = e ? (size_t)(e - p) : strlen(p);
    if (len == 0) {  // an empty PATH entry means the cwd
      dir = ".";
      len = 1;
    }
    if (len + nlen + 2 <= cap) {
      memcpy(buf, dir, len);
      buf[len] = '/';
      memcpy(buf + len + 1, name, nlen + 1);
      if (runnable(buf))
        return buf;
    }
    if (!e)
      break;
    p = e + 1;
  }
  return 0;
}

// Claude Code turns a short read on the hook's stdin into an EPIPE, and reports
// that as "Hook command closed stdin before hook input was fully written",
// status 1 -- a warning on the user's screen. So the payload must be consumed
// to the end even though the file path sits in the first chunk. What it does not
// have to be is consumed *first*: a Write payload carries the file content
// twice, and draining 8.6 MB of it costs 674 us here, which is 40% of the
// hook's whole cost. Spawning the formatter before the drain hides all of it
// behind a formatter that runs for tens of milliseconds.
static void drain_stdin(void) {
  for (;;) {
    ssize_t r = read(0, inbuf, BUF_CAP);
    if (r > 0)
      continue;
    if (r < 0 && errno == EINTR)
      continue;
    return;
  }
}

static void put(const char *s) {
  (void)!write(1, s, strlen(s));
}

static void put_line(const char *a, const char *b) {
  put(a);
  put(b);
  put("\n");
}

// ------------------------------------------------------------------- rules --
// argv holds the rule's whole chain flat: each command's tokens, a null after
// each, and a second null after the last. One command is the degenerate chain
// {"rustfmt", 0, 0}; a disabled rule is {0}.
struct cfg_entry {
  const char *key;
  uint8_t glob;  // needs fnmatch rather than a plain compare
  uint8_t path;  // match against the whole path, not the base name
  const char *argv[CFG_ARGV + 1];
};

static struct cfg_entry cfg[CFG_ENTRIES];
static struct cfg_entry cfg_bak[CFG_ENTRIES];
static int cfg_n;
static int cfg_skipped;

static char *cfg_ws(char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  return p;
}

// Unescape a JSON string in place; *pp points just past the opening quote and
// is advanced past the closing one. Output never outgrows input, so this writes
// over the source. Returns 0 on a malformed string.
static char *cfg_str(char **pp) {
  char *r = *pp, *w = r, *start = r;
  for (;;) {
    unsigned char c = (unsigned char)*r;
    if (c == 0)
      return 0;
    if (c == '"') {
      *w = 0;
      *pp = r + 1;
      return start;
    }
    if (c != '\\') {
      *w++ = (char)c;
      r++;
      continue;
    }
    r++;
    switch (*r) {
      case 'n':
        *w++ = '\n';
        r++;
        break;
      case 't':
        *w++ = '\t';
        r++;
        break;
      case 'r':
        *w++ = '\r';
        r++;
        break;
      case 'b':
        *w++ = '\b';
        r++;
        break;
      case 'f':
        *w++ = '\f';
        r++;
        break;
      case 0:
        return 0;
      case 'u': {
        r++;
        int a = hexval((unsigned char)r[0]), b = r[0] ? hexval((unsigned char)r[1]) : -1;
        int c2 = (r[0] && r[1]) ? hexval((unsigned char)r[2]) : -1;
        int d = (r[0] && r[1] && r[2]) ? hexval((unsigned char)r[3]) : -1;
        if (a < 0 || b < 0 || c2 < 0 || d < 0)
          return 0;
        unsigned cp = (unsigned)(a << 12 | b << 8 | c2 << 4 | d);
        r += 4;
        if (cp >= 0xD800 && cp < 0xDC00 && r[0] == '\\' && r[1] == 'u') {
          int e0 = hexval((unsigned char)r[2]), e1 = hexval((unsigned char)r[3]);
          int e2 = hexval((unsigned char)r[4]), e3 = hexval((unsigned char)r[5]);
          if (e0 >= 0 && e1 >= 0 && e2 >= 0 && e3 >= 0) {
            unsigned lo = (unsigned)(e0 << 12 | e1 << 8 | e2 << 4 | e3);
            if (lo >= 0xDC00 && lo < 0xE000) {
              cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
              r += 6;
            }
          }
        }
        w += utf8(w, cp);
        break;
      }
      default:
        *w++ = *r++;
    }
  }
}

// Register a flat chain of n tokens (n <= CFG_ARGV, checked by the parser)
// under one rule. A rule already present is replaced, which is how a project
// config overrides the personal one.
static void cfg_put(const char *key, const char *const *argv, int nargv) {
  struct cfg_entry *slot = 0;
  for (int i = 0; i < cfg_n; i++)
    if (!strcmp(cfg[i].key, key))
      slot = &cfg[i];
  if (!slot) {
    if (cfg_n >= CFG_ENTRIES) {
      cfg_skipped++;
      return;
    }
    slot = &cfg[cfg_n++];
  }
  slot->key = key;
  slot->path = strchr(key, '/') != 0;
  slot->glob = slot->path || strpbrk(key, "*?[") != 0;
  memcpy(slot->argv, argv, (size_t)nargv * sizeof argv[0]);
  slot->argv[nargv] = 0;
}

// Split a key on commas ("c, h, cpp") and register each rule. Splitting is
// destructive, which is fine: keys live in cfgbuf and nothing else reads them.
// Only surrounding whitespace is trimmed, so a rule may contain spaces --
// "CMakeLists.txt" and "My Notes.md" are both expressible.
static void cfg_add(char *keys, const char *const *argv, int nargv) {
  for (char *k = keys; *k;) {
    while (*k == ',' || *k == ' ' || *k == '\t') k++;
    if (!*k)
      break;
    char *tok = k, *comma = strchr(k, ','), *end;
    if (comma) {
      end = comma;
      k = comma + 1;
    } else {
      end = k + strlen(k);
      k = end;
    }
    while (end > tok && (end[-1] == ' ' || end[-1] == '\t')) end--;
    *end = 0;
    if (*tok)
      cfg_put(tok, argv, nargv);
    else
      cfg_skipped++;
  }
}

// The strings of an array whose '[' is already consumed, through its ']':
// zero or more, comma separated. Every string is counted in *n but only the
// first cap are stored, so the caller can tell a rule that is merely too long
// (dropped) from one that is malformed (the file is rejected). *pp is left
// past the ']'. Returns 0 on a malformed array.
static int cfg_strings(char **pp, const char **out, int *n, int cap) {
  char *p = cfg_ws(*pp);
  if (*p == ']') {
    *pp = p + 1;
    return 1;
  }
  for (;;) {
    if (*p++ != '"')
      return 0;
    char *v = cfg_str(&p);
    if (!v)
      return 0;
    if (*n < cap)
      out[*n] = v;
    (*n)++;
    p = cfg_ws(p);
    if (*p == ',') {
      p = cfg_ws(p + 1);
      continue;
    }
    if (*p != ']')
      return 0;
    *pp = p + 1;
    return 1;
  }
}

// One rule's value, laid out flat as cfg_entry.argv describes:
//
//   null | []          disabled              -> n = 0
//   ["a", "b"]         one command           -> a b 0
//   [["a"], ["b","c"]] a chain               -> a 0 b c 0
//
// Anything else -- a scalar, a string next to an array, an empty command --
// is malformed. n counts every token including the separators, and may come
// back above cap for a rule that is too long. Returns 0 on malformed input.
static int cfg_value(char **pp, const char **out, int *n, int cap) {
  char *p = *pp;
  if (!strncmp(p, "null", 4)) {
    *pp = p + 4;
    return 1;
  }
  if (*p++ != '[')
    return 0;
  p = cfg_ws(p);
  if (*p != '[') {  // [] or a single command
    if (!cfg_strings(&p, out, n, cap))
      return 0;
    if (*n) {
      if (*n < cap)
        out[*n] = 0;
      (*n)++;
    }
    *pp = p;
    return 1;
  }
  for (;;) {  // a chain: every element is a non-empty command
    p++;
    int before = *n;
    if (!cfg_strings(&p, out, n, cap) || *n == before)
      return 0;
    if (*n < cap)
      out[*n] = 0;
    (*n)++;
    p = cfg_ws(p);
    if (*p == ',') {
      p = cfg_ws(p + 1);
      if (*p != '[')
        return 0;
      continue;
    }
    if (*p != ']')
      return 0;
    *pp = p + 1;
    return 1;
  }
}

// Object of string -> value. Anything else invalidates the file.
static int cfg_parse(char *p) {
  p = cfg_ws(p);
  if (*p++ != '{')
    return 0;
  p = cfg_ws(p);
  if (*p == '}')
    return 1;
  for (;;) {
    p = cfg_ws(p);
    if (*p++ != '"')
      return 0;
    char *keys = cfg_str(&p);
    if (!keys)
      return 0;
    p = cfg_ws(p);
    if (*p++ != ':')
      return 0;
    p = cfg_ws(p);
    const char *argv[CFG_ARGV];
    int n = 0;
    if (!cfg_value(&p, argv, &n, CFG_ARGV))
      return 0;
    if (n > CFG_ARGV) {  // too long to hold: turned off whole, never truncated
      if (dryrun)
        put_line("# rule too long, turned off: ", keys);
      n = 0;
    }
    cfg_add(keys, argv, n);
    p = cfg_ws(p);
    if (*p == ',') {
      p++;
      continue;
    }
    if (*p == '}')
      return 1;
    return 0;
  }
}

// Read one config file and merge it over what is already loaded. The rules are
// snapshotted first so a malformed file rolls back to exactly the state before
// it -- half a config is worse than none, and one bad project file must not
// take the personal defaults with it.
static void cfg_read(const char *path) {
  if (cfgbuf_used >= CFG_CAP)
    return;
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return;
  size_t start = cfgbuf_used, len = 0;
  for (;;) {
    ssize_t r = read(fd, cfgbuf + start + len, CFG_CAP - start - len);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (r == 0)
      break;
    len += (size_t)r;
    if (start + len >= CFG_CAP)
      break;
  }
  close(fd);
  cfgbuf[start + len] = 0;
  cfgbuf_used = start + len + 1;

  int saved_n = cfg_n;
  memcpy(cfg_bak, cfg, (size_t)cfg_n * sizeof cfg[0]);
  if (!cfg_parse(cfgbuf + start)) {
    memcpy(cfg, cfg_bak, (size_t)saved_n * sizeof cfg[0]);
    cfg_n = saved_n;
    if (dryrun)
      put_line("# rejected (not an object of rule -> argv or [argv, ...]): ", path);
    return;
  }
  if (dryrun)
    put_line("# config: ", path);
}

// Is file under one of the ':'-separated prefixes in trust? A prefix must end
// at a '/' boundary, so /a/b admits /a/b/c but not /a/bc.
static int trusted(const char *file, const char *trust) {
  for (const char *p = trust; *p;) {
    const char *e = strchr(p, ':');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    while (len > 1 && p[len - 1] == '/') len--;  // a trailing slash is noise
    if (len && !strncmp(file, p, len) && (file[len] == '/' || file[len] == 0))
      return 1;
    if (!e)
      break;
    p = e + 1;
  }
  return 0;
}

// Every directory from / down to the file's own, outermost first, so the
// closest config is applied last and wins.
static void cfg_walk(const char *file) {
  const char *sl[WALK_MAX];
  int total = 0, nd = 0, seen = 0;
  for (const char *p = file; *p; p++)
    if (*p == '/')
      total++;
  int skip = total > WALK_MAX ? total - WALK_MAX : 0;
  for (const char *p = file; *p; p++)
    if (*p == '/' && seen++ >= skip)
      sl[nd++] = p;

  char buf[PATH_CAP];
  for (int i = 0; i < nd; i++) {
    size_t dl = (size_t)(sl[i] - file);  // 0 for the root directory itself
    if (dl + sizeof "/.claude/fmt-hooks.json" >= sizeof buf)
      continue;
    memcpy(buf, file, dl);
    memcpy(buf + dl, "/.claude/fmt-hooks.json", sizeof "/.claude/fmt-hooks.json");
    cfg_read(buf);
    memcpy(buf + dl, "/.fmt-hooks.json", sizeof "/.fmt-hooks.json");
    cfg_read(buf);
  }
}

static void cfg_load(const char *file, char **envp) {
  char buf[PATH_CAP];
  const char *dir = env_get(envp, "CLAUDE_CONFIG_DIR", 17);
  size_t o = 0;
  if (dir) {
    o = str_append(buf, 0, sizeof buf, dir);
  } else {
    const char *home = env_get(envp, "HOME", 4);
    if (home)
      o = str_append(buf, str_append(buf, 0, sizeof buf, home), sizeof buf, "/.claude");
  }
  if (o) {
    str_append(buf, o, sizeof buf, "/fmt-hooks.json");
    cfg_read(buf);
  }
  const char *trust = env_get(envp, "CC_FMT_TRUST", 12);
  if (!trust || trusted(file, trust))
    cfg_walk(file);
  else if (dryrun)
    put("# project configs skipped: the path is outside $CC_FMT_TRUST\n");
  const char *over = env_get(envp, "CC_FMT_CONFIG", 13);
  if (over)
    cfg_read(over);
}

// Name, then glob, then extension. The order is the contract: it is what lets
// a name rule carve an exception out of an extension rule.
static const char *const *cfg_lookup(const char *path, const char *base, const char *ext) {
  for (int i = 0; i < cfg_n; i++)
    if (!cfg[i].glob && !strcmp(cfg[i].key, base))
      return cfg[i].argv;
  for (int i = 0; i < cfg_n; i++)
    if (cfg[i].glob && fnmatch(cfg[i].key, cfg[i].path ? path : base, 0) == 0)
      return cfg[i].argv;
  if (ext)
    for (int i = 0; i < cfg_n; i++) {
      const char *k = cfg[i].key;
      if (!cfg[i].glob && ci_eq(k[0] == '.' ? k + 1 : k, ext))
        return cfg[i].argv;
    }
  return 0;
}

// ----------------------------------------------------------------- running --
// Substitute ${VAR} from the environment. Returns 0 when a referenced variable
// is unset, which drops the argument, and expand_full when the result does not
// fit what is left of the arena -- a different answer, because a shortened
// argument is a different command and the caller must drop the whole one.
static const char expand_full[1] = "";

static const char *expand(const char *s, char **envp) {
  if (!strchr(s, '$'))
    return s;
  if (arena_used >= ARENA_CAP)
    return expand_full;
  char *o = arena + arena_used;
  size_t cap = ARENA_CAP - arena_used, w = 0;  // w + 1 < cap before every store keeps o[w] = 0 inside
  for (const char *p = s; *p;) {
    if (p[0] == '$' && p[1] == '{') {
      const char *e = strchr(p + 2, '}');
      if (!e)
        return 0;
      size_t nlen = (size_t)(e - (p + 2));
      char name[64];
      if (nlen == 0 || nlen >= sizeof name)
        return 0;
      memcpy(name, p + 2, nlen);
      name[nlen] = 0;
      const char *v = env_get(envp, name, nlen);
      if (!v)
        return 0;
      for (; *v; v++) {
        if (w + 1 >= cap)
          return expand_full;
        o[w++] = *v;
      }
      p = e + 1;
    } else {
      if (w + 1 >= cap)
        return expand_full;
      o[w++] = *p++;
    }
  }
  o[w] = 0;
  arena_used += w + 1;
  return o;
}

// One command of a chain, ready to exec: the program resolved on PATH, the
// argv with ${VAR} substituted and the file path appended. A command has at
// most CFG_ARGV - 1 tokens (its separator takes the last slot), so the path
// and the null fit in CFG_ARGV + 1.
struct run {
  const char *prog;
  const char *args[CFG_ARGV + 1];
};

// CMD_MAX is CFG_ARGV / 2 only because cfg_value refuses an empty command:
// every command then costs at least a program and a separator.
static struct run runs[CMD_MAX];

// Walk the flat chain one command at a time into runs[]: cmd stops on each
// command's separator and the loop increment steps over it. A command whose
// program cannot be named, does not fit after expansion or is not installed is
// left out; the rest of the chain still runs. Returns how many are runnable.
static int build_runs(const char *const *chain, char **envp) {
  const char *path = env_get(envp, "PATH", 4);
  int nrun = 0;
  for (const char *const *cmd = chain; *cmd; cmd++) {
    struct run *r = &runs[nrun];
    const char *skip = 0;
    int n = 0;
    for (; *cmd; cmd++) {
      const char *v = expand(*cmd, envp);
      if (v == expand_full)
        skip = "# too long after ${VAR} expansion, command skipped: ";
      else if (!v && n == 0)
        skip = "# unset ${VAR} in the program name: ";
      else if (v)
        r->args[n++] = v;
      if (skip) {
        if (dryrun)
          put_line(skip, *cmd);
        while (*cmd) cmd++;  // the rest of this command is moot: stop on its separator
        break;
      }
    }
    if (skip)
      continue;
    r->args[n++] = fpath;
    r->args[n] = 0;
    r->prog = resolve(r->args[0], path, exe[nrun], sizeof exe[nrun]);
    if (!r->prog) {  // formatter not installed: nothing to do, silently
      if (dryrun)
        put_line("# not on PATH: ", r->args[0]);
      continue;
    }
    nrun++;
  }
  return nrun;
}

static pid_t spawn(const struct run *r, posix_spawn_file_actions_t *fap, posix_spawnattr_t *atp, char **envp) {
  pid_t pid;
  return posix_spawn(&pid, r->prog, fap, atp, (char *const *)r->args, envp) == 0 ? pid : -1;
}

static void reap(pid_t pid) {
  int st;
  while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {
  }
}

// Run links [from, nrun) one after another, each waited for before the next
// starts, all in the caller's process group.
static void run_links(int from, int nrun, posix_spawn_file_actions_t *fap, char **envp) {
  for (int i = from; i < nrun; i++) {
    pid_t pid = spawn(&runs[i], fap, 0, envp);
    if (pid > 0)
      reap(pid);
  }
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;

  // Read only as far as the file path. Everything after it is drained later,
  // once the formatter is running -- see drain_stdin(). KEEP_CAP bytes are
  // carried across reads so the key cannot be split by a chunk boundary.
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
    if (find_path(inbuf, total, fpath, sizeof fpath)) {
      found = 1;
      break;
    }
    keep = total < KEEP_CAP ? total : KEEP_CAP;
    memmove(inbuf, inbuf + total - keep, keep);
  }
  dryrun = env_get(envp, "CC_FMT_DRYRUN", 13) != 0;
  if (!found) {  // the loop only leaves without a path at end of input
    if (dryrun)
      put("# no file path in the payload\n");
    return 0;
  }

  const char *slash = strrchr(fpath, '/');
  const char *base = slash ? slash + 1 : fpath;
  const char *dot = strrchr(base, '.');
  // A leading dot makes a dotfile, not an extension: ".gitignore" is a name.
  const char *ext = (dot && dot != base && dot[1]) ? dot + 1 : 0;

  cfg_load(fpath, envp);
  if (dryrun && cfg_skipped)
    put("# some rules were skipped: empty, too long, or past the entry limit\n");

  const char *const *chain = cfg_lookup(fpath, base, ext);
  if (!chain || !chain[0]) {
    if (dryrun)
      put_line(chain ? "# rule matched but disabled: " : "# no rule matches: ", base);
    drain_stdin();
    return 0;
  }

  int nrun = build_runs(chain, envp);
  if (!nrun) {
    drain_stdin();
    return 0;
  }

  if (dryrun) {
    for (int i = 0; i < nrun; i++) {
      put(runs[i].prog);
      for (const char **a = runs[i].args + 1; *a; a++) {
        put(" ");
        put(*a);
      }
      put("\n");
    }
    drain_stdin();
    return 0;
  }

  // The formatter talks to nobody: stdin is EOF so one that reads it cannot
  // hang the hook, and stdout and stderr are discarded rather than inherited.
  // Inheriting them is what makes CC_FMT_ASYNC a lie and what deadlocks a
  // caller that does not drain the pipe -- see the async note in the header.
  posix_spawn_file_actions_t fa;
  posix_spawn_file_actions_t *fap = 0;
  if (posix_spawn_file_actions_init(&fa) == 0 &&
      posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0) == 0 &&
      posix_spawn_file_actions_addopen(&fa, 1, "/dev/null", O_WRONLY, 0) == 0 &&
      posix_spawn_file_actions_addopen(&fa, 2, "/dev/null", O_WRONLY, 0) == 0)
    fap = &fa;

  int async = env_get(envp, "CC_FMT_ASYNC", 12) != 0;

  // An async chain has to be sequenced by someone after the hook is gone, so a
  // forked runner does it: detached exactly like an async formatter -- its own
  // process group, /dev/null on the hook's stdio so the pipes to Claude Code
  // close with the hook and the payload pipe is the hook's alone to drain --
  // then one posix_spawn + waitpid per link. Both sides call setpgid, the way
  // a job-control shell does, so the runner is out of the hook's group no
  // matter which of them runs first. If /dev/null cannot be opened the
  // descriptors are closed outright: the links reopen theirs through the file
  // actions, and a closed pipe is still a closed pipe to Claude Code. A failed
  // fork degrades to the synchronous path below rather than to nothing.
  if (async && nrun > 1) {
    pid_t runner = fork();
    if (runner == 0) {
      setpgid(0, 0);
      int nul = open("/dev/null", O_RDWR);
      for (int fd = 0; fd < 3; fd++)
        if (nul < 0 || dup2(nul, fd) < 0)
          close(fd);
      if (nul > 2)
        close(nul);
      run_links(0, nrun, fap, envp);
      _exit(0);
    }
    if (runner > 0) {
      setpgid(runner, runner);
      drain_stdin();
      return 0;
    }
    async = 0;
  }

  // An async formatter outlives the hook, so it must not share the hook's
  // process group: a timeout signalling that group would land on a formatter
  // halfway through rewriting the file. A synchronous one stays in the group.
  posix_spawnattr_t at;
  posix_spawnattr_t *atp = 0;
  if (async && posix_spawnattr_init(&at) == 0 && posix_spawnattr_setflags(&at, (short)POSIX_SPAWN_SETPGROUP) == 0 &&
      posix_spawnattr_setpgroup(&at, 0) == 0)
    atp = &at;

  pid_t pid = spawn(&runs[0], fap, atp, envp);

  drain_stdin();  // concurrent with the first formatter, which is the point

  if (async)
    return 0;
  if (pid > 0)
    reap(pid);
  run_links(1, nrun, fap, envp);  // each link waits for the one before it
  return 0;                       // never anything but 0: a formatter that fails stays invisible
}
