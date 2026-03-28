# Modern Go Version Matrix

## Contents

- Go 1.0+
- Go 1.8+
- Go 1.13+
- Go 1.18+
- Go 1.19+
- Go 1.20+
- Go 1.21+
- Go 1.22+
- Go 1.23+
- Go 1.24+
- Go 1.25+
- Go 1.26+

## Features by Go Version

### Go 1.0+

- `time.Since`: `time.Since(start)` instead of `time.Now().Sub(start)`

### Go 1.8+

- `time.Until`: `time.Until(deadline)` instead of `deadline.Sub(time.Now())`

### Go 1.13+

- `errors.Is`: `errors.Is(err, target)` instead of `err == target` (works with wrapped errors)

### Go 1.18+

- Prefer `any` over `interface{}`.
- Prefer `bytes.Cut` over `bytes.Index` plus manual slicing when splitting once.
- Prefer `strings.Cut` over `strings.Index` plus manual slicing when splitting once.

### Go 1.19+

- `any`: Use `any` instead of `interface{}`
- `bytes.Cut`: `before, after, found := bytes.Cut(b, sep)` instead of Index+slice
- `strings.Cut`: `before, after, found := strings.Cut(s, sep)`

```go
var flag atomic.Bool
flag.Store(true)
if flag.Load() { ... }

var ptr atomic.Pointer[Config]
ptr.Store(cfg)
```

### Go 1.20+

- `strings.Clone`: `strings.Clone(s)` to copy string without sharing memory
- `bytes.Clone`: `bytes.Clone(b)` to copy byte slice
- `strings.CutPrefix/CutSuffix`: `if rest, ok := strings.CutPrefix(s, "pre:"); ok { ... }`
- `errors.Join`: `errors.Join(err1, err2)` to combine multiple errors
- `context.WithCancelCause`: `ctx, cancel := context.WithCancelCause(parent)` then `cancel(err)`
- `context.Cause`: `context.Cause(ctx)` to get the error that caused cancellation

### Go 1.21+

**Built-ins:**
- `min`/`max`: `max(a, b)` instead of if/else comparisons
- `clear`: `clear(m)` to delete all map entries, `clear(s)` to zero slice elements

**slices package:**
- `slices.Contains`: `slices.Contains(items, x)` instead of manual loops
- `slices.Index`: `slices.Index(items, x)` returns index (-1 if not found)
- `slices.IndexFunc`: `slices.IndexFunc(items, func(item T) bool { return item.ID == id })`
- `slices.SortFunc`: `slices.SortFunc(items, func(a, b T) int { return cmp.Compare(a.X, b.X) })`
- `slices.Sort`: `slices.Sort(items)` for ordered types
- `slices.Max`/`slices.Min`: `slices.Max(items)` instead of manual loop
- `slices.Reverse`: `slices.Reverse(items)` instead of manual swap loop
- `slices.Compact`: `slices.Compact(items)` removes consecutive duplicates in-place
- `slices.Clip`: `slices.Clip(s)` removes unused capacity
- `slices.Clone`: `slices.Clone(s)` creates a copy

**maps package:**
- `maps.Clone`: `maps.Clone(m)` instead of manual map iteration
- `maps.Copy`: `maps.Copy(dst, src)` copies entries from src to dst
- `maps.DeleteFunc`: `maps.DeleteFunc(m, func(k K, v V) bool { return condition })`

**sync package:**
- `sync.OnceFunc`: `f := sync.OnceFunc(func() { ... })` instead of `sync.Once` + wrapper
- `sync.OnceValue`: `getter := sync.OnceValue(func() T { return computeValue() })`

**context package:**
- `context.AfterFunc`: `stop := context.AfterFunc(ctx, cleanup)` runs cleanup on cancellation
- `context.WithTimeoutCause`: `ctx, cancel := context.WithTimeoutCause(parent, d, err)`
- `context.WithDeadlineCause`: Similar with deadline instead of duration

### Go 1.22+

**Loops:**
- `for i := range n`: `for i := range len(items)` instead of `for i := 0; i < len(items); i++`
- Loop variables are now safe to capture in goroutines (each iteration has its own copy)

**cmp package:**
- `cmp.Or`: `cmp.Or(flag, env, config, "default")` returns first non-zero value

```go
// Instead of:
name := os.Getenv("NAME")
if name == "" {
    name = "default"
}
// Use:
name := cmp.Or(os.Getenv("NAME"), "default")
```

**reflect package:**
- `reflect.TypeFor`: `reflect.TypeFor[T]()` instead of `reflect.TypeOf((*T)(nil)).Elem()`

**net/http:**
- Enhanced `http.ServeMux` patterns: `mux.HandleFunc("GET /api/{id}", handler)` with method and path params
- `r.PathValue("id")` to get path parameters

### Go 1.23+

- `maps.Keys(m)` / `maps.Values(m)` return iterators
- `slices.Collect(iter)` not manual loop to build slice from iterator
- `slices.Sorted(iter)` to collect and sort in one step

```go
keys := slices.Collect(maps.Keys(m))       // not: for k := range m { keys = append(keys, k) }
sortedKeys := slices.Sorted(maps.Keys(m))  // collect + sort
for k := range maps.Keys(m) { process(k) } // iterate directly
```

**time package**

- `time.Tick`: Use `time.Tick` freely — as of Go 1.23, the garbage collector can recover unreferenced tickers, even if they haven't been stopped. The Stop method is no longer necessary to help the garbage collector. There is no longer any reason to prefer NewTicker when Tick will do.

### Go 1.24+

- `t.Context()` not `context.WithCancel(context.Background())` in tests.
  ALWAYS use t.Context() when a test function needs a context.

Before:
```go
func TestFoo(t *testing.T) {
    ctx, cancel := context.WithCancel(context.Background())
    defer cancel()
    result := doSomething(ctx)
}
```
After:
```go
func TestFoo(t *testing.T) {
    ctx := t.Context()
    result := doSomething(ctx)
}
```

- `omitzero` not `omitempty` in JSON struct tags.
  ALWAYS use omitzero for time.Duration, time.Time, structs, slices, maps.

Before:
```go
type Config struct {
    Timeout time.Duration `json:"timeout,omitempty"` // doesn't work for Duration!
}
```
After:
```go
type Config struct {
    Timeout time.Duration `json:"timeout,omitzero"`
}
```

- `b.Loop()` not `for i := 0; i < b.N; i++` in benchmarks.
  ALWAYS use b.Loop() for the main loop in benchmark functions.

Before:
```go
func BenchmarkFoo(b *testing.B) {
    for i := 0; i < b.N; i++ {
        doWork()
    }
}
```
After:
```go
func BenchmarkFoo(b *testing.B) {
    for b.Loop() {
        doWork()
    }
}
```

- `strings.SplitSeq` not `strings.Split` when iterating.
  ALWAYS use SplitSeq/FieldsSeq when iterating over split results in a for-range loop.

Before:
```go
for _, part := range strings.Split(s, ",") {
    process(part)
}
```
After:
```go
for part := range strings.SplitSeq(s, ",") {
    process(part)
}
```
Also: `strings.FieldsSeq`, `bytes.SplitSeq`, `bytes.FieldsSeq`.

### Go 1.25+

- `wg.Go(fn)` not `wg.Add(1)` + `go func() { defer wg.Done(); ... }()`.
  ALWAYS use wg.Go() when spawning goroutines with sync.WaitGroup.

Before:
```go
var wg sync.WaitGroup
for _, item := range items {
    wg.Add(1)
    go func() {
        defer wg.Done()
        process(item)
    }()
}
wg.Wait()
```
After:
```go
var wg sync.WaitGroup
for _, item := range items {
    wg.Go(func() {
        process(item)
    })
}
wg.Wait()
```

**testing/synctest package**

- Use `testing/synctest` for deterministic testing of concurrent code with goroutines and time.
- `synctest.Run(func())` executes code in a bubble where time and goroutines are controlled.
- Fast, deterministic tests for timeouts, tickers, and race conditions.

#### Time

Within a bubble, the time package uses a fake clock.
Each bubble has its own clock.
The initial time is midnight UTC 2000-01-01.
Time in a bubble only advances when every goroutine in the bubble is durably blocked.
See below for the exact definition of "durably blocked".

For example, this test runs immediately rather than taking two seconds:
```go
 func TestTime(t *testing.T) {
 	synctest.Test(t, func(t *testing.T) {
 		start := time.Now() // always midnight UTC 2000-01-01
 		go func() {
 			time.Sleep(1 * time.Second)
 			t.Log(time.Since(start)) // always logs "1s"
 		}()
 		time.Sleep(2 * time.Second) // the goroutine above will run before this Sleep returns
 		t.Log(time.Since(start))    // always logs "2s"
 	})
 }
```

Time stops advancing when the root goroutine of the bubble exits.

#### Blocking

A goroutine in a bubble is "durably blocked" when it is blocked and can only be unblocked by another goroutine in the same bubble.
A goroutine which can be unblocked by an event from outside its bubble is not durably blocked.
The Wait function blocks until all other goroutines in the bubble are durably blocked.

For example:
```go
func TestWait(t *testing.T) {
	synctest.Test(t, func(t *testing.T) {
		done := false
		go func() {
			done = true
		}()
		// Wait will block until the goroutine above has finished.
		synctest.Wait()
		t.Log(done) // always logs "true"
	})
}
```

When every goroutine in a bubble is durably blocked:

- Wait returns, if it has been called.
- Otherwise, time advances to the next time that will
  unblock at least one goroutine, if there is such a time
  and the root goroutine of the bubble has not exited.
- Otherwise, there is a deadlock and Test panics.

The following operations durably block a goroutine:

- a blocking send or receive on a channel created within the bubble
- a blocking select statement where every case is a channel created
  within the bubble
- `sync.Cond.Wait`
- `sync.WaitGroup.Wait`, when `sync.WaitGroup.Add` was called within the bubble
- `time.Sleep`

Operations not in the above list are not durably blocking.
In particular, the following operations may block a goroutine,
but are not durably blocking because the goroutine can be unblocked
by an event occurring outside its bubble:
- locking a sync.Mutex or sync.RWMutex
- blocking on I/O, such as reading from a network socket
- system calls

#### Isolation

A channel, `time.Timer`, or `time.Ticker` created within a bubble
is associated with it. Operating on a bubbled channel, timer, or
ticker from outside the bubble panics.

A `sync.WaitGroup` becomes associated with a bubble on the first call to Add or Go.
Once a `WaitGroup` is associated with a bubble, calling Add or Go from outside that bubble is a fatal error.
(As a technical limitation, a WaitGroup defined as a package variable, such as "var wg sync.WaitGroup", cannot be associated
with a bubble and operations on it may not be durably blocking. This limitation does not apply to a *WaitGroup stored in a
package variable, such as "var wg = new(sync.WaitGroup)".)

`sync.Cond.Wait` is durably blocking. Waking a goroutine in a bubble blocked on `Cond.Wait` from outside the bubble is a fatal error.

Cleanup functions and finalizers registered with `runtime.AddCleanup` and `runtime.SetFinalizer` run outside of any bubble.

### Go 1.26+

- `new(val)` not `x := val; &x` — returns pointer to any value.
  Go 1.26 extends new() to accept expressions, not just types.
  Type is inferred: new(0) → *int, new("s") → *string, new(T{}) → *T.
  DO NOT use `x := val; &x` pattern — always use new(val) directly.
  DO NOT use redundant casts like new(int(0)) — just write new(0).
  Common use case: struct fields with pointer types.

Before:
```go
timeout := 30
debug := true
cfg := Config{
    Timeout: &timeout,
    Debug:   &debug,
}
```
After:
```go
cfg := Config{
    Timeout: new(30),   // *int
    Debug:   new(true), // *bool
}
```

- `errors.AsType[T](err)` not `errors.As(err, &target)`.
  ALWAYS use errors.AsType when checking if error matches a specific type.

Before:
```go
var pathErr *os.PathError
if errors.As(err, &pathErr) {
    handle(pathErr)
}
```
After:
```go
if pathErr, ok := errors.AsType[*os.PathError](err); ok {
    handle(pathErr)
}
```
