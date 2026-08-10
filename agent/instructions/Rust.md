# Agent Guidelines for Rust Code Quality

This document provides guidelines for maintaining high-quality Rust code. These rules MUST be followed by all AI coding agents and contributors.

## Your Core Principles

All code you write MUST be fully optimized.

"Fully optimized" includes:

- maximizing algorithmic big-O efficiency for memory and runtime
- using parallelization and SIMD where appropriate
- following proper style conventions for Rust (e.g. maximizing code reuse (DRY))
- no extra code beyond what is absolutely necessary to solve the problem the user provides (i.e. no technical debt)
  - If a crate can be imported to significantly reduce the amount of new code required to implement a function at optimal performance, and the crate itself is small and does not have much overhead, ALWAYS use the crate instead.
  - Before adding a crate, check whether the functionality has since been stabilized in `std` (e.g. `LazyLock`, `OnceLock`, async fn in traits). Prefer `std` over third-party crates when equivalent.

## Edition and Toolchain

- **MUST** use Rust Edition 2024 (`edition = "2024"`) for all new projects.
- **MUST** pin the toolchain with `rust-toolchain.toml` for reproducible builds.
- **MUST** declare `rust-version` (MSRV) in `Cargo.toml`; cargo's MSRV-aware resolver will respect it.
- When migrating older code, run `cargo fix --edition` and review the diff manually; do not blindly accept mechanical rewrites.
- Note Edition 2024 semantic changes when writing code:
  - `std::env::set_var` / `remove_var` are `unsafe`; avoid them entirely — configure via process environment or config structs instead.
  - `extern` blocks require `unsafe extern`; attributes like `no_mangle` are written `#[unsafe(no_mangle)]`.
  - Return-position `impl Trait` captures all lifetimes by default; use `use<'a, T>` capture syntax when narrower capture is required.

## Preferred Tools

- Use `cargo` for project management, building, and dependency management.
- Use `indicatif` to track long-running operations with progress bars. The message should be contextually sensitive.
- Use `serde` with `serde_json` for JSON serialization/deserialization.
- Use `ratatui` and `crossterm` for terminal applications/TUIs.
  - Include logical and intuitive mouse controls for all TUIs.
  - **ALWAYS** account for interface scrolling offsets when calculating click locations
- Use `axum` for creating any web servers or HTTP APIs.
  - Keep request handlers async, returning `Result<Response, AppError>` to centralize error handling.
  - Use layered extractors and shared state structs instead of global mutable data.
  - Add `tower` middleware (timeouts, tracing, compression) for observability and resilience.
  - Offload CPU-bound work to `tokio::task::spawn_blocking` or background services to avoid blocking the reactor.
- When reporting errors to the console, use `tracing::error!` or `log::error!` instead of `println!`.
- If the project involves the creation of images (e.g. PNG/WEBP), you have permission to use the Read tool to verify the rendered images fit the user and application requirements.
- If designing applications with a web-based front end interface, e.g. compiling to WASM or using `dioxus`:
  - All deep computation **MUST** occur within Rust processes (i.e. the WASM binary or the `dioxus` app Rust process). **NEVER** use JavaScript for deep computation.
  - The front-end **MUST** use Pico CSS and vanilla JavaScript. **NEVER** use jQuery or any component-based frameworks such as React.
  - The front-end should prioritize speed and common HID guidelines.
  - The app should use adaptive light/dark themes by default, with a toggle to switch the themes.
  - The typography/theming of the application **MUST** be modern and unique, similar to that of popular single-page web/mobile. **ALWAYS** add an appropriate font for headers and body text. You may reference fonts from Google Fonts.
  - **NEVER** use the Pico CSS defaults as-is: a separate CSS/SCSS file is encouraged. The design **MUST** logically complement the semantics of the application use case.
  - **ALWAYS** rebuild the WASM binary if any underlying Rust code that affects it is touched.
- If using Python to implement Rust code using PyO3/`maturin`:
  - Rebuild the Python package with `maturin` after finishing all Rust code changes.
  - **ALWAYS** use `uv` for Python package management and to create a `.venv` if it is not present. **NEVER** use the base system Python installation.
  - Ensure `.venv` is added to `.gitignore`.
  - Ensure `ipykernel` and `ipywidgets` are installed in `.venv` for Jupyter Notebook compatibility. This should not be in package requirements.
  - Python code follows the same design rules as Rust code (single responsibility, ≤5 parameters, early returns).
  - **NEVER** use mutable objects (lists, dicts) as default argument values
  - **MUST** use type hints for all function signatures (parameters and return values); use `T | None` for nullable types (not `Optional[T]`)
  - **NEVER** use `Any` type unless absolutely necessary
  - **MUST** run `ty` and resolve all type errors

## Code Style and Formatting

- **MUST** use meaningful, descriptive variable and function names
- **MUST** follow Rust API Guidelines and idiomatic Rust conventions
- **MUST** use 4 spaces for indentation (never tabs)
- **NEVER** use emoji, or unicode that emulates emoji (e.g. ✓, ✗). The only exception is when writing tests and testing the impact of multibyte characters.
- Use snake_case for functions/variables/modules, PascalCase for types/traits, SCREAMING_SNAKE_CASE for constants
- Limit line length to 100 characters (rustfmt default); use rustfmt `style_edition = "2024"`
- Assume the user is a Python expert, but a Rust novice. Include additional code comments around Rust-specific nuances that a Python developer may not recognize.
- **MUST** avoid including redundant comments which are tautological or self-demonstrating (e.g. cases where it is easily parsable what the code does at a glance or its function name giving sufficient information as to what the code does, so the comment does nothing other than waste user time)
- **MUST** avoid including comments which leak what this file contains, or leak the original user prompt, ESPECIALLY if it's irrelevant to the output code.

## Documentation

- **MUST** include doc comments for all public functions, structs, enums, and methods
- **MUST** document function parameters, return values, and errors
- Keep comments up-to-date with code changes
- Include examples in doc comments for complex functions; doc examples are compiled and run as doctests, so keep them valid

Example doc comment:

```rust
/// Calculate the total cost of items including tax.
///
/// # Arguments
///
/// * `items` - Slice of item structs with price fields
/// * `tax_rate` - Tax rate as decimal (e.g., 0.08 for 8%)
///
/// # Errors
///
/// Returns [`CalculationError::EmptyItems`] if `items` is empty.
/// Returns [`CalculationError::InvalidTaxRate`] if `tax_rate` is negative.
///
/// # Examples
///
/// ```
/// let items = vec![Item { price: 10.0 }, Item { price: 20.0 }];
/// let total = calculate_total(&items, 0.08)?;
/// assert_eq!(total, 32.40);
/// # Ok::<(), CalculationError>(())
/// ```
pub fn calculate_total(items: &[Item], tax_rate: f64) -> Result<f64, CalculationError> {
```

## Type System

- **MUST** leverage Rust's type system to prevent bugs at compile time
- **NEVER** use `.unwrap()` in library code; use `.expect()` only for invariant violations with a descriptive message
- **MUST** use meaningful custom error types with `thiserror` (2.x)
- Use newtypes to distinguish semantically different values of the same underlying type
- Prefer `Option<T>` over sentinel values; use combinators like `is_some_and`, `is_none_or`, and `inspect` over manual matches when clearer
- Use native `async fn` in traits and return-position `impl Trait` in traits (RPITIT); reach for the `async-trait` crate **only** when `dyn`-compatibility is required
- Use `#[diagnostic::on_unimplemented]` on public traits where a custom compiler hint meaningfully improves downstream error messages
- Trait upcasting (`dyn Sub` → `dyn Super`) is stable; use it instead of manual `as_super()` shims

## Error Handling

- **NEVER** use `.unwrap()` in production code paths
- **MUST** use `Result<T, E>` for fallible operations
- **MUST** use `thiserror` for defining error types and `anyhow` for application-level errors
- **MUST** propagate errors with `?` operator where appropriate
- Provide meaningful error messages with context using `.context()` from `anyhow`
- Use `let-else` for extracting values with early returns on failure:

```rust
let Some(config) = load_config(path) else {
    return Err(AppError::MissingConfig);
};
```

## Function Design

- **MUST** keep functions focused on a single responsibility
- **MUST** prefer borrowing (`&T`, `&mut T`) over ownership when possible
- Limit function parameters to 5 or fewer; use a config struct for more
- Return early to reduce nesting; prefer `let-else` and let chains (`if let Some(x) = a && x > 0`) over nested `if let` pyramids
- Use iterators and combinators over explicit loops where clearer
- Use async closures (`async |x| { ... }`) instead of `|x| async move { ... }` when the closure needs to borrow from its environment

## Struct and Enum Design

- **MUST** keep types focused on a single responsibility
- **MUST** derive common traits: `Debug`, `Clone`, `PartialEq` where appropriate
- Use `#[derive(Default)]` when a sensible default exists; use `#[default]` on enum variants
- Prefer composition over inheritance-like patterns
- Use builder pattern for complex struct construction
- Make fields private by default; provide accessor methods when needed
- Use `#[non_exhaustive]` on public enums/structs that may grow, to preserve semver compatibility

## Testing

- **MUST** write unit tests for all new functions and types
- **MUST** mock external dependencies (APIs, databases, file systems)
- **MUST** use the built-in `#[test]` attribute; run with `cargo nextest run` if available, otherwise `cargo test`
- Follow the Arrange-Act-Assert pattern
- Do not commit commented-out tests
- Use `#[cfg(test)]` modules for test code

## Imports and Dependencies

- **MUST** avoid wildcard imports (`use module::*`) except for preludes, test modules (`use super::*`), and prelude re-exports
- **MUST** document dependencies in `Cargo.toml` with version constraints
- Use `[workspace.dependencies]` to unify versions across workspace members
- Organize imports: standard library, external crates, local modules; let `rustfmt` handle ordering
- **NEVER** add `lazy_static` or `once_cell` as dependencies; use `std::sync::LazyLock` and `std::sync::OnceLock`

## Rust Best Practices

- **NEVER** use `unsafe` unless absolutely necessary; document safety invariants with `// SAFETY:` comments when used
- Use `&raw const` / `&raw mut` instead of the `addr_of!` macros when raw pointers are unavoidable
- **MUST** call `.clone()` explicitly on non-`Copy` types; avoid hidden clones in closures and iterators
- **MUST** use pattern matching exhaustively; avoid catch-all `_` patterns when possible
- **MUST** use `format!` with inline captured identifiers (`format!("{name}")`, not `format!("{}", name)`)
- Use `#[expect(lint, reason = "...")]` instead of `#[allow(lint)]` when suppressing lints, so stale suppressions become compile warnings
- Use iterators and iterator adapters over manual loops
- Use `enumerate()` instead of manual counter variables
- Prefer `if let` / `while let` for single-pattern matching, and `matches!` for boolean pattern checks

## Memory and Performance

- **MUST** avoid unnecessary allocations; prefer `&str` over `String` when possible
- **MUST** use `Cow<'_, str>` when ownership is conditionally needed
- Use `Vec::with_capacity()` when the size is known
- Prefer stack allocation over heap when appropriate
- Use `Arc` and `Rc` judiciously; prefer borrowing
- Use `LazyLock` for lazily-initialized statics instead of runtime `Option<T>` + lock dances

## Benchmarking and Optimization

- **NEVER** run benchmarks in parallel, as the benchmarks will compete for resources and the results will be invalid
- **NEVER** game the benchmarks. Do not manipulate the benchmarks themselves to satisfy any required performance constraints
- **NEVER** run benchmarks with `target-cpu=native` or any other `RUSTFLAGS`
- Use `criterion` or `divan` for microbenchmarks; do not hand-roll timing loops
- If benchmarking against another crate or library, ensure the benchmarks are apples-to-apples comparisons
- Ensure benchmark tests are independent. If the tests are dependent due to a feature (e.g. caching), ensure the feature is disabled

## Concurrency

- **MUST** use `Send` and `Sync` bounds appropriately
- **MUST** prefer `tokio` for async runtime in async applications
- **MUST** use `rayon` for CPU-bound parallelism
- Avoid `Mutex` when `RwLock` or lock-free alternatives are appropriate
- Use channels for message passing: `tokio::sync::mpsc` in async code, `std::sync::mpsc` or `crossbeam` in sync code
- Use `std::thread::scope` for scoped threads instead of `crossbeam::scope`

## Security

- **NEVER** store secrets, API keys, or passwords in code. Only store them in `.env`
  - Ensure `.env` is declared in `.gitignore`
- **MUST** read sensitive configuration from environment variables via `dotenvy` or `std::env::var`; **NEVER** mutate the environment at runtime (`set_var` is `unsafe` in Edition 2024)
- **NEVER** log sensitive information (passwords, tokens, PII)
- Use `secrecy` crate for sensitive data types
- Run `cargo audit` (or `cargo deny check`) when adding new dependencies

## Version Control

- **MUST** write clear, descriptive commit messages
- **NEVER** commit commented-out code; delete it
- **NEVER** commit debug `println!` statements or `dbg!` macros
- **NEVER** commit credentials or sensitive data

## Tools

- **MUST** use `rustfmt` for code formatting
- **MUST** use `clippy` for linting: `cargo clippy --all-targets --all-features -- -D warnings`
- **MUST** ensure code compiles with no warnings (use `-D warnings` flag in CI, not `#![deny(warnings)]` in source)
- Use `cargo` for building, testing, and dependency management
- Use `cargo nextest run` (fall back to `cargo test`) for running tests
- Use `cargo doc` for generating documentation
- For projects which build a Python package, **NEVER** build with `cargo build --features python`: this will always fail. Instead, **ALWAYS** use `maturin`.
- **NEVER** use the `Explore` tool for `Cargo.lock`: it is large and irrelevant. Read `Cargo.lock` **ONLY** if it's extremely relevant.

## Before Committing

- [ ] All tests pass (`cargo nextest run` if available, otherwise `cargo test`)
- [ ] No compiler warnings (`cargo build`)
- [ ] Clippy passes (`cargo clippy --all-targets -- -D warnings`)
- [ ] Code is formatted (`cargo fmt --check`)
- [ ] Doctests pass (`cargo test --doc`) if doc examples were added or changed
- [ ] If the project creates a Python package and Rust code is touched, rebuild the Python package (`source .venv/bin/activate && maturin develop --release --features python`)
- [ ] If the project creates a WASM package and Rust code is touched, rebuild the WASM package (`wasm-pack build --target web --out-dir web/pkg`)
- [ ] All public items have doc comments
- [ ] No commented-out code or debug statements
- [ ] No hardcoded credentials

---

**Remember:** Prioritize clarity and maintainability over cleverness. This is your core directive.
