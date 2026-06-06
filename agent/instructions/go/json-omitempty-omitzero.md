The most critical difference to keep in mind when choosing between them is that omitempty evaluates whether a container is
"empty" (such as having a length of 0), whereas omitzero strictly evaluates whether the variable remains at its uninitialized initial 
state (the default "zero value" of its type).

## Comparison of omitempty and omitzero Across Data

| TypesType & Value State | omitempty Behavior | omitzero Behavior | Use Case & Recommendation |
| ----------------------- | ------------------ | ----------------- | ------------------------- |
| `time.Time` zero value (0001-01-01...) | Serialized (included) | Omitted | Highly recommend `omitzero`. Use this when you want to completely exclude uninitialized timestamps from your JSON response. | 
| Structs (all fields are zero values) | Serialized (included as `{}`) | Omitted | Highly recommend `omitzero`. Use this to cleanly omit nested structs without resorting to pointer workarounds. |
| Slices / Maps (`nil` state) | Omitted | Omitted | Either works. Use when you only want to omit the field if no memory has been allocated (the value is `nil`). |
| Slices / Maps (allocated empty: `` or `{}`) | Omitted | Serialized (as `` or `{}`) | Choose based on data semantics.• Use `omitzero` if you want to explicitly return `` to represent "0 search results found".• Use `omitempty` if you want to omit the field entirely whenever it contains no elements. |
| Primitives (zero value of `int`, `string`, `bool`, etc.) | Omitted | Omitted | Either works identically. However, for Go 1.24+ codebases, `omitzero` is generally preferred for consistency and clarity. |
| Pointers (`nil` pointer like `*int` or `*struct`) | Omitted | Omitted | Either works. Use pointers alongside these tags when you need to distinguish between an unassigned field and a field explicitly set to its zero value (e.g., `0` or `false`). |


## Concrete Decision Guidelines

### 1. Use omitzero for Datetimes (time.Time) and Structs
In previous versions of Go, even if a struct like time.Time was uninitialized, omitempty would not omit it. Instead, it serialized meaningless default values (such as "0001-01-01T00:00:00Z" or an empty object {}).From Go 1.24 onward, if you want to safely omit uninitialized structs from your JSON output without converting them into pointer types, omitzero is the standard solution. 

### 2. Choose Based on "API Semantics" for Slices and Maps

The most prominent differentiator between the two tags lies in how they handle allocated, empty slices:

- When to choose `omitzero`:
    - When you need to distinguish between "data not calculated/requested" (`nil` -> omitted) and "data calculated but returned zero results" (empty array `` -> serialized).
- When to choose `omitempty`:
    - When you want to completely hide the field from the JSON output as long as it has no elements, regardless of whether the slice is `nil` or initialized as ``.

### 3. Combine Both to Omit "Empty" OR "Zero"

In Go 1.24, you can declare both options together: `json:"hobbies,omitempty,omitzero"`.

Because they are evaluated as a Logical OR, the field will be omitted if it is either unallocated (`nil` / zero value) OR allocated but empty (`len == 0` / empty value).

### 4. Handling Cases Where `0` or `false` Are Legitimate Values

If you want to omit a field when it is "unspecified" but still want to output it if it is explicitly set to `0` or `false`, standard primitive
types will be omitted by both tags (since `0` and `false` are their default zero values).

In such scenarios, you can either:

1. Define the field as a pointer type (`*int` or `*bool`) and rely on its `nil` state for omission.
2. Create a custom wrapper type (e.g., `Undefined`) that implements the `IsZero() bool` method, and pair it with `omitzero` to
   omit the field only when its "present" flag is false.
