// cc-search-hook (standalone, fastest): Claude Code PreToolUse(Bash) hook.
//
// Redirects grep/find/rg to configurable full binaries by injecting shell
// function overrides at the head of the command. No cc-applet trampoline and
// no CLAUDE_CODE_EXECPATH indirection: each search execs the selected binary
// DIRECTLY (one exec instead of the shadow->cc-applet->binary two execs).
//
// The overrides win because the command runs (eval) AFTER Claude Code sources
// its shell snapshot, so these definitions shadow the snapshot's own grep/find
// functions. Claude's curated search flags are replicated so results match the
// shadow's behavior (drop them for plain grep semantics).
//
// The hook binary sits at the process-startup floor (~1.6 ms): no stdio, no
// malloc, no getenv, static buffers, a compile-time-constant prefix, single
// write(2). Build:
//
//   clang -march=native -O3 -ffast-math \
//     -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -o cc-search-hook cc-search-hook.c
//   strip cc-search-hook
//
// CLAUDE_CODE_{UGREP,BFS,RG} select executables; unset or empty values use the
// paths below. The matching *_ARGS variables contain trusted shell words.
// Unset *_ARGS values use the current flags; explicitly empty values pass none.

#include <string.h>
#include <unistd.h>

static char inbuf[1 << 20];

// Shell text injected at the command head. The \" sequences are literal
// backslash-quote bytes so the surrounding JSON string stays valid; after
// Claude JSON-decodes the command they become plain " for the shell.
// *_ARGS is parsed in an array-assignment eval; caller argv is saved first and
// is never interpolated into that eval.
#define PREFIX                                                                    \
    ": __cc_search_override; "                                                    \
    "__cc_search_run(){ "                                                         \
    "local __cc_search_bin=\\\"$1\\\" __cc_search_args=\\\"$2\\\"; shift 2; "     \
    "local -a __cc_search_head __cc_search_tail; __cc_search_tail=(\\\"$@\\\"); " \
    "eval \\\"__cc_search_head=( $__cc_search_args )\\\" || return 2; "           \
    "command \\\"$__cc_search_bin\\\" \\\"${__cc_search_head[@]}\\\" "            \
    "\\\"${__cc_search_tail[@]}\\\"; }; "                                         \
    "grep(){ __cc_search_run "                                                    \
    "\\\"${CLAUDE_CODE_UGREP:-/opt/homebrew/bin/ugrep}\\\" "                      \
    "\\\"${CLAUDE_CODE_UGREP_ARGS--G --ignore-files --hidden -I "                 \
    "--exclude-dir=.git --exclude-dir=.svn --exclude-dir=.hg --exclude-dir=.bzr " \
    "--exclude-dir=.jj --exclude-dir=.sl}\\\" \\\"$@\\\"; }; "                    \
    "find(){ __cc_search_run \\\"${CLAUDE_CODE_BFS:-/usr/local/bin/bfs}\\\" "     \
    "\\\"${CLAUDE_CODE_BFS_ARGS--S dfs -regextype findutils-default}\\\" "        \
    "\\\"$@\\\"; }; "                                                             \
    "rg(){ __cc_search_run \\\"${CLAUDE_CODE_RG:-/opt/homebrew/bin/rg}\\\" "      \
    "\\\"${CLAUDE_CODE_RG_ARGS-}\\\" \\\"$@\\\"; }; "

static const char OUTPUT_HEADER[] = "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":";
static const char PREFIX_TEXT[] = PREFIX;
static char outbuf[sizeof inbuf + sizeof OUTPUT_HEADER + sizeof PREFIX_TEXT + 2];

static const char *find_cmd_open(const char *s) {
    const char *p = strstr(s, "\"tool_input\"");
    if (!p)
        return 0;
    const char *k = strstr(p, "\"command\"");
    if (!k)
        return 0;
    k += 9;
    while (*k == ' ' || *k == '\t' || *k == '\n') k++;
    if (*k != ':')
        return 0;
    k++;
    while (*k == ' ' || *k == '\t' || *k == '\n') k++;
    if (*k != '"')
        return 0;
    return k + 1;
}

int main(void) {
    static const char NOOP[] = "{}";
    size_t len = 0;
    ssize_t r;
    while (len < sizeof inbuf - 1 && (r = read(0, inbuf + len, sizeof inbuf - 1 - len)) > 0) len += (size_t)r;
    inbuf[len] = '\0';

    if (len == 0 || strstr(inbuf, "__cc_search_override")) {
        (void)!write(1, NOOP, 2);
        return 0;
    }
    const char *co = find_cmd_open(inbuf);
    if (!co) {
        (void)!write(1, NOOP, 2);
        return 0;
    }

    const char *ti = strchr(strstr(inbuf, "\"tool_input\""), '{');
    int depth = 0, instr = 0, esc = 0;
    const char *e = ti;
    for (; *e; e++) {
        char c = *e;
        if (instr) {
            if (esc)
                esc = 0;
            else if (c == '\\')
                esc = 1;
            else if (c == '"')
                instr = 0;
        } else {
            if (c == '"')
                instr = 1;
            else if (c == '{')
                depth++;
            else if (c == '}' && --depth == 0) {
                e++;
                break;
            }
        }
    }

    char *o = outbuf;
    memcpy(o, OUTPUT_HEADER, sizeof OUTPUT_HEADER - 1);
    o += sizeof OUTPUT_HEADER - 1;
    size_t pre = (size_t)(co - ti);
    memcpy(o, ti, pre);
    o += pre;
    memcpy(o, PREFIX_TEXT, sizeof PREFIX_TEXT - 1);
    o += sizeof PREFIX_TEXT - 1;
    size_t rest = (size_t)(e - co);
    memcpy(o, co, rest);
    o += rest;
    *o++ = '}';
    *o++ = '}';
    (void)!write(1, outbuf, (size_t)(o - outbuf));
    return 0;
}
