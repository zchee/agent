// fmt-hook: language-independent PostToolUse(Write|Edit) formatter dispatch.
//
// Reads the hook JSON on stdin, extracts the edited file path
// (.tool_response.filePath, else .tool_input.file_path), matches its extension
// against a table, and runs the matching formatter in place. Replaces a shelf
// of per-language `jq | case ... fmt` hooks: no sh -c wrapper (~7ms) and no jq
// (~5ms) -- just the process-startup floor (~1.6ms) plus the formatter.
//
// Register as an EXEC-FORM hook (spawned directly, no shell):
//   { "type":"command", "command":"/path/to/fmt-hook", "args":[] }
// matcher "Write|Edit", event PostToolUse. A missing formatter is a silent
// no-op (execvp fails in the child). CC_FMT_DRYRUN=1 prints the command it
// would run instead of executing it.
//
//   clang -march=native -O3 -ffast-math \
//     -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -o cc-fmt-hook cc-fmt-hook.c
//   strip cc-fmt-hook

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static char inbuf[1 << 20];
static char path[4096];

// extension -> formatter argv (the file path is appended as the last arg).
#define A(...) (const char *const[]){__VA_ARGS__, 0}
struct fmt {
    const char *ext;
    const char *const *argv;
};
static const struct fmt TABLE[] = {
    {"zig", A("zig", "fmt")},
    {"rs", A("rustfmt", "--edition", "2024")},
    {"go", A("gofumpt", "-w")},
    {"py", A("ruff", "format", "-q")},
    {"c", A("clang-format", "-i")},
    {"h", A("clang-format", "-i")},
    {"cc", A("clang-format", "-i")},
    {"cpp", A("clang-format", "-i")},
    {"hpp", A("clang-format", "-i")},
    {"m", A("clang-format", "-i")},
    {"js", A("prettier", "--write", "--log-level", "silent")},
    {"ts", A("prettier", "--write", "--log-level", "silent")},
    {"tsx", A("prettier", "--write", "--log-level", "silent")},
    {"json", A("prettier", "--write", "--log-level", "silent")},
    {"css", A("prettier", "--write", "--log-level", "silent")},
    {"md", A("prettier", "--write", "--log-level", "silent")},
    {"sh", A("shfmt", "-w")},
    {"bash", A("shfmt", "-w")},
    {"lua", A("stylua")},
    {"toml", A("taplo", "format")},
    {0, 0},
};

// Find "key":"value" in JSON j; unescape value into out (cap). 1 on success.
static int json_str(const char *j, const char *key, char *out, size_t cap) {
    size_t klen = strlen(key);
    char pat[64];
    pat[0] = '"';
    memcpy(pat + 1, key, klen);
    pat[klen + 1] = '"';
    pat[klen + 2] = 0;
    for (const char *p = strstr(j, pat); p; p = strstr(p + 1, pat)) {
        const char *q = p + klen + 2;
        while (*q == ' ' || *q == '\t' || *q == '\n') q++;
        if (*q++ != ':')
            continue;
        while (*q == ' ' || *q == '\t' || *q == '\n') q++;
        if (*q++ != '"')
            continue;
        size_t o = 0;
        while (*q && *q != '"' && o < cap - 1) {
            if (*q == '\\') {
                switch (*++q) {
                    case 'n':
                        out[o++] = '\n';
                        break;
                    case 't':
                        out[o++] = '\t';
                        break;
                    case 'u':
                        out[o++] = '?';
                        q += 4;
                        break; /* paths are ~ASCII */
                    default:
                        out[o++] = *q; /* \/ \\ \" -> literal */
                }
                q++;
            } else
                out[o++] = *q++;
        }
        out[o] = 0;
        return o > 0;
    }
    return 0;
}

int main(void) {
    size_t len = 0;
    ssize_t r;
    while (len < sizeof inbuf - 1 && (r = read(0, inbuf + len, sizeof inbuf - 1 - len)) > 0) len += (size_t)r;
    inbuf[len] = 0;

    if (!json_str(inbuf, "filePath", path, sizeof path) && !json_str(inbuf, "file_path", path, sizeof path))
        return 0;

    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;
    const char *dot = strrchr(base, '.');
    if (!dot || !dot[1])
        return 0;
    const char *ext = dot + 1;

    const struct fmt *m = 0;
    for (const struct fmt *t = TABLE; t->ext; t++)
        if (strcmp(t->ext, ext) == 0) {
            m = t;
            break;
        }
    if (!m)
        return 0;

    const char *argv[8];
    int n = 0;
    for (const char *const *a = m->argv; *a && n < 6; a++) argv[n++] = *a;
    argv[n++] = path;
    argv[n] = 0;

    if (getenv("CC_FMT_DRYRUN")) {
        for (int i = 0; i < n; i++) {
            write(1, argv[i], strlen(argv[i]));
            write(1, " ", 1);
        }
        write(1, "\n", 1);
        return 0;
    }

    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    if (pid > 0) {
        int st;
        waitpid(pid, &st, 0);
    }
    return 0;
}
