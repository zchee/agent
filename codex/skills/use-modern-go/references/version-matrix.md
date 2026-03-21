# Modern Go Version Matrix

Converted from JetBrains `go-modern-guidelines` `use-modern-go` and rewritten for Codex skill consumption.

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

## Go 1.0+

- Prefer `time.Since(start)` over `time.Now().Sub(start)`.

## Go 1.8+

- Prefer `time.Until(deadline)` over `deadline.Sub(time.Now())`.

## Go 1.13+

- Prefer `errors.Is(err, target)` over direct equality when wrapped errors are possible.

## Go 1.18+

- Prefer `any` over `interface{}`.
- Prefer `bytes.Cut` over `bytes.Index` plus manual slicing when splitting once.
- Prefer `strings.Cut` over `strings.Index` plus manual slicing when splitting once.

## Go 1.19+

- Prefer `fmt.Appendf` when appending formatted data to an existing byte slice.
- Prefer typed atomics such as `atomic.Bool`, `atomic.Int64`, and `atomic.Pointer[T]` over manually coordinating narrower atomic primitives.

```go
var ready atomic.Bool
ready.Store(true)

var current atomic.Pointer[Config]
current.Store(cfg)
```

## Go 1.20+

- Prefer `strings.Clone` to copy a string without sharing backing storage.
- Prefer `bytes.Clone` to copy a byte slice.
- Prefer `strings.CutPrefix` and `strings.CutSuffix` over manual prefix or suffix trimming.
- Prefer `errors.Join` to combine multiple errors.
- Prefer `context.WithCancelCause` and `context.Cause` when callers need the cancellation reason.

## Go 1.21+

### Built-ins

- Prefer `min` and `max` over ad hoc comparison branches.
- Prefer `clear(m)` to empty a map and `clear(s)` to zero a slice.

### `slices`

- Prefer `slices.Contains` over manual membership loops.
- Prefer `slices.Index` or `slices.IndexFunc` over open-coded search loops.
- Prefer `slices.Sort` or `slices.SortFunc` over custom sort boilerplate.
- Prefer `slices.Max` and `slices.Min` over manual extrema loops.
- Prefer `slices.Reverse` over hand-written swap loops.
- Prefer `slices.Compact` to remove consecutive duplicates in place.
- Prefer `slices.Clip` to drop unused capacity.
- Prefer `slices.Clone` to copy slices.

### `maps`

- Prefer `maps.Clone` to copy maps.
- Prefer `maps.Copy` to merge or copy entries between maps.
- Prefer `maps.DeleteFunc` when deleting conditionally.

### `sync`

- Prefer `sync.OnceFunc` for fire-once actions.
- Prefer `sync.OnceValue` for memoized initialization that returns a value.

### `context`

- Prefer `context.AfterFunc` for cleanup on cancellation.
- Prefer `context.WithTimeoutCause` and `context.WithDeadlineCause` when timeout or deadline reasons matter to callers.

## Go 1.22+

### Loops

- Prefer `for i := range n` over `for i := 0; i < n; i++` when iterating over a count.
- Remember that range loop variables now have per-iteration capture semantics, so goroutines and closures no longer need the old capture workaround.

### `cmp`

- Prefer `cmp.Or` to choose the first non-zero value from a short list of candidates.

```go
name := cmp.Or(os.Getenv("NAME"), "default")
```

### `reflect`

- Prefer `reflect.TypeFor[T]()` over `reflect.TypeOf((*T)(nil)).Elem()`.

### `net/http`

- Prefer method-aware ServeMux patterns such as `mux.HandleFunc("GET /api/{id}", handler)`.
- Prefer `r.PathValue("id")` for extracted path parameters.

## Go 1.23+

- Prefer ranging directly over `maps.Keys(m)` or `maps.Values(m)` instead of building a temporary slice first.
- Prefer `slices.Collect(iter)` when you need a slice from an iterator.
- Prefer `slices.Sorted(iter)` when you need collection plus sorting in one step.

```go
keys := slices.Collect(maps.Keys(m))
sortedKeys := slices.Sorted(maps.Keys(m))
for k := range maps.Keys(m) {
	process(k)
}
```

### `time`

- `time.Tick` is acceptable again when it matches the use case, because the runtime can recover unreferenced tickers. Use `time.NewTicker` only when you need explicit stop control or additional ticker methods.

## Go 1.24+

### Testing

- Prefer `t.Context()` over creating a fresh background context in tests.

```go
func TestFoo(t *testing.T) {
	ctx := t.Context()
	result := doSomething(ctx)
	_ = result
}
```

### JSON tags

- Prefer `omitzero` over `omitempty` for fields where zero values should be omitted reliably, including `time.Duration`, `time.Time`, structs, slices, and maps.

```go
type Config struct {
	Timeout time.Duration `json:"timeout,omitzero"`
}
```

### Benchmarks

- Prefer `b.Loop()` over `for i := 0; i < b.N; i++` for the main benchmark loop.

```go
func BenchmarkFoo(b *testing.B) {
	for b.Loop() {
		doWork()
	}
}
```

### Split iteration

- Prefer `strings.SplitSeq` or `strings.FieldsSeq` when you only need to iterate the results.
- Prefer `bytes.SplitSeq` or `bytes.FieldsSeq` for byte slices in the same situation.

```go
for part := range strings.SplitSeq(s, ",") {
	process(part)
}
```

## Go 1.25+

- Prefer `wg.Go(fn)` over `wg.Add(1)` plus a goroutine wrapper when using `sync.WaitGroup`.

```go
var wg sync.WaitGroup
for _, item := range items {
	wg.Go(func() {
		process(item)
	})
}
wg.Wait()
```

## Go 1.26+

- Prefer `new(value)` over introducing a temporary local just to take its address.
- Prefer `errors.AsType[T](err)` over `errors.As(err, &target)` when matching a concrete error type.

```go
cfg := Config{
	Timeout: new(30),
	Debug:   new(true),
}

if pathErr, ok := errors.AsType[*os.PathError](err); ok {
	handle(pathErr)
}
```
