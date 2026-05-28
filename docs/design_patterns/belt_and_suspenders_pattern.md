# Belt and Suspenders Pattern

Assert and throw on the same condition for destructive operations.

Assert catches the bug in debug builds. Throw prevents data
corruption in release builds where asserts are removed.

```cpp
auto item = repo_.findByUuid(uuid);
assert(item.has_value() && "item must exist at this point");
if (!item) throw std::invalid_argument("item not found");
```

Apply when: the operation has irreversible side effects (delete,
edit, write). A wrong input reaching this code silently corrupts
or loses data.

Do not apply when: the operation is read-only (find, search, list).
A wrong input returns nothing via std::optional. No damage occurs.
Assert-only is sufficient for non-destructive consistency checks.

Why both: the service layer is caller-agnostic. Today one caller
(CLI) guarantees the precondition. A future caller (API, batch
import) might not. The assert documents the contract. The throw
enforces it regardless of who calls.
