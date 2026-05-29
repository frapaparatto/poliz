# poliz: Architecture Decision Log

This file is the canonical index of architectural decisions. Entries
that have a full write-up live in their own `ADR-XXX.md` file alongside
this one. Entries without a dedicated file are documented inline below
and are considered complete: the inline text is the record.

## Decided

**ADR-001: Primary Key Strategy**
UUID v4 as internal surrogate primary key. Email as user-facing unique
constraint enforced by the service layer. Decouples internal identity
from mutable attributes. Implemented in `utils.cpp` (manual `mt19937`
generator) and enforced in `ClientService`. Full ADR in ADR-001.md.

**ADR-002: Policy as Independent Entity**
Policy is a separate entity with `client_uuid` as foreign reference.
Models the real one-to-many relationship (one client, many policies).
Implemented: `IPolicyRepository` and `Policy` class exist. The
inheritance hierarchy for interactions (Appointment, Contract derived
from `Interaction`) is treated as part of the domain model and is
documented in `docs/architecture.md` and ADR-025, not as a separate
decision record here.

**ADR-003: Interaction as Polymorphic Entity**
Interaction is an abstract base class. Appointment and Contract derive
from it with their own fields. Storing by value in a vector would cause
object slicing: derived fields are lost when copied through the base
type. The repository stores `std::vector<std::unique_ptr<Interaction>>`
to avoid this. The domain distinction between appointment and contract
(different fields, different lifecycle, different business meaning) maps
naturally to different types sharing a common interface rather than a
single flat struct with optional fields. No dedicated ADR file was
created for this decision. Full rationale, retrieval mechanics, and the
virtual `clone()` pattern are documented in `docs/architecture.md`.
The thread-safe retrieval consequence is recorded in ADR-025.

**ADR-004: Startup Flow**
New / Load menu at startup. Explicit user choice every time.
Implemented in `main.cpp` via named helpers in an anonymous namespace
(`cmdNew`, `cmdLoad`, `makeRepositories`) and a while loop in `main()`
that breaks once a valid `CrmRepositories` is obtained.

**ADR-005: TUI Interactive Search**
Stretch goal. Deliberately not implemented to control scope. No
implementation started.

**ADR-006: Auto-Save Configuration**
User-configurable: enabled / disabled toggle and interval in seconds,
stored in a config file. Defaults: enabled, 60 seconds. Configuration
mechanism documented in ADR-023. AutoSave threading documented in
ADR-024.

**ADR-007: CSV Schema Design**
Comma as global delimiter. Address stored as three separate columns
(`address`, `city`, `postal_code`). Commas prohibited in free-text
fields by convention (not enforced by code). Empty optional fields
represented as empty strings between delimiters. Column layout defined
for `clients.csv` (14 columns), `policies.csv` (10 columns),
`interactions.csv` (variable by type). Full ADR in ADR-007.md.

**ADR-008: UUID Generation Strategy**
Manual v4 implementation using `mt19937` in `utils.cpp`. No external
dependency. Not thread-safe initially; fixed with a static mutex in
`generateUuid` when AutoSave introduced a second thread (see ADR-024).

**ADR-009: Date / Time Representation**
Plain `std::string` for all date fields, YYYY-MM-DD format validated
by `isValidDate` in `utils`. Migration to `std::chrono` types deferred
until a concrete need arises. `safe_localtime` wrapper added to
`utils.hpp` for thread-safe timestamp formatting (see ADR-024).

**ADR-010: Hard Delete**
Hard delete chosen over soft delete. `removeClient` erases the record
and `ClientService::deleteClient` cascades to all related policies and
interactions. No active flag or deleted_at column.

**ADR-011: Testing Strategy (superseded by ADR-021)**
Catch2 chosen, levels defined, no mocking, real temporary files for
integration tests. The full strategy lives in ADR-021.

**ADR-013: Default Filepath (superseded by ADR-018)**
Directory-based path strategy replaced the single-filepath default.
The full strategy lives in ADR-018.

**ADR-015: Performance Validation**
Not implemented. Deliberately deferred. Seed script and
`std::chrono` timing remain the planned approach.

**ADR-017: Application Refactoring (Orchestrator + Controller)**
Application becomes a thin orchestrator owning controllers via
`unordered_map<string, unique_ptr<IEntityController>>`. Each controller
(`ClientController`, `PolicyController`, `InteractionController`)
implements the same interface and owns its own service, dispatch table,
and command implementations. Two-level menu: entity selection, then
CRUD command. Save, exit, clear are application-level commands handled
in `handleAppCmds()`. Full ADR in ADR-017.md.

**ADR-018: File Path Strategy (Directory-Based Multi-Entity Persistence)**
User provides a directory path at startup. Application derives
filenames: `<dir>/clients.csv`, `<dir>/policies.csv`,
`<dir>/interactions.csv`. New session: create directory if missing,
repos start empty. Load session: each repo loads independently, missing
file means empty state (not an error). Throw only on malformed data or
permission errors. Full ADR in ADR-018.md.

**ADR-019: Policy Business Rules and Fixed Pricing**
Amount and end_date are system-calculated, not user input. Only type,
duration, and start_date come from the user. `editPolicy` enforces
immutability on contract-defining fields (type, duration, start_date,
amount, end_date). Pricing table is a `static constexpr double[4][4]`
inside `PolicyService::calculateAmount`. Full ADR in ADR-019.md.

**ADR-020: Error Handling Strategy**
`assert` for programmer errors (active in debug, removed in release).
Exceptions for rare runtime errors (I/O, constraint violations,
malformed data). `std::optional` for expected absence (find returning
no result). Belt-and-suspenders (assert + throw) for destructive
operations (delete, edit). `std::invalid_argument` for validation
errors, `std::runtime_error` for I/O and infrastructure errors. Full
ADR in ADR-020.md.

**ADR-021: Testing Strategy (Catch2)**
Catch2 v3.4.0 via FetchContent. Two levels: unit tests (pure functions,
no I/O) and integration tests (real temporary files, real repositories,
no mocks). Tags on every `TEST_CASE`. Full ADR in ADR-021.md.

**ADR-022: Policy Display with Client Name Resolution**
`resolvePolicy` returns `std::optional<std::pair<Policy, Client>>` so
the view can render the client name alongside the policy without
controllers calling each other. List displays use a UUID-to-name map
built once per command. No "Unknown" fallback: an unresolved client
asserts (programmer error). Full ADR in ADR-022.md.

**ADR-023: Configuration System Design**
Flat `key=value` configuration file `poliz.conf` read at startup.
`CrmConfig` struct with in-class defaults lives in the anonymous
namespace of `main.cpp`. `parseConfig()` plus `applyConfigLine()`
handle parsing and validation. Missing file means defaults, silent.
Unknown keys warn and are ignored. Full ADR in ADR-023.md.

**ADR-024: Auto-Save Threading**
One background thread owned by `AutoSaveService`, sleeping on a
`std::condition_variable` with `wait_for` and a stop-flag predicate.
Wakes on timeout (save dirty repositories) or notification (shutdown).
Each repository owns its own mutex; `AutoSaveService` never touches
repository mutexes directly. UUID generation and `safe_localtime` get
their own thread-safety mechanisms. Application holds the service as
`std::optional` so it is constructed only when `autosave_enabled` is
true. Full ADR in ADR-024.md.

**ADR-025: Polymorphic Interaction Retrieval (clone pattern)**
`Interaction` is an abstract base; `Appointment` and `Contract` derive.
Repository stores `std::vector<std::unique_ptr<Interaction>>` to avoid
object slicing. `findByUuid` returns `std::unique_ptr<Interaction>` and
`findAll` returns a vector of `unique_ptr`. Both build their results by
calling a virtual `clone()` on each stored entity under the lock so the
caller gets an independent copy. Each derived class implements
`clone()` by delegating to its own copy constructor. Full ADR in
ADR-025.md.

## Undocumented decisions (brief)

These are deliberate decisions that did not warrant a full ADR but are
recorded here so they are not rediscovered as accidents.

**Atomic save via tmp + rename**
Repositories write to a temporary file and rename it over the target
on success. A crash mid-write leaves the old file intact rather than a
half-written one. Deliberate crash-safety pattern.

**Policy deduplication not enforced**
The service layer does not reject a policy that is identical to an
existing one for the same client. Duplicate policies are assumed not
to be created by the user. Deliberate deferral; to be added in a
future validation pass.

**Time format validation deferred**
Appointment and contract times are stored as plain strings with no
format constraint. The user can enter `22:10`, `10:00 PM`, or
`fourteen hundred` and the system accepts it. Validating against a
canonical format (`HH:MM` 24-hour, or AM / PM) was considered and
deliberately skipped: the CRM does not schedule reminders or compute
durations against this field, so a free-form string is sufficient for
the current use case. To be revisited if reminders or calendar
integration are added.

**Phone number validation deferred**
`setPhone` accepts any digit string. Country-code prefix normalization
(e.g. `+39` for the Italian market) and length checks are not enforced.
Phone numbers are used only for display, not for outbound calls or
SMS, so a stricter format does not improve correctness at this stage.

**Domain constructors accept whitespace-only strings for first_name and last_name**
The `Client` new-object constructor checks `first_name.empty()` and
`last_name.empty()` but not whitespace-only strings. `promptRequired`
in `cli_helper.cpp` trims input before calling constructors, closing
the gap in normal CLI use. A non-CLI caller would need to trim
independently.

**Policy amount and end_date are system-derived, not user input**
The user only chooses type, duration, and start_date when creating or
editing a policy. Amount comes from the constexpr pricing table and
end_date from month arithmetic on start_date plus duration. This is
treated as a business rule: the price list and contract length are
company-controlled. Allowing manual entry would let the user produce
contracts that do not match the published price, which is a business
problem before a code problem. ADR-019 records the full rationale.

**No service interfaces (`IClientService` etc.)**
`IClientRepository`, `IPolicyRepository`, and `IInteractionRepository`
exist for dependency injection (constructor injection into services,
enabling future storage swaps). No `IClientService`, `IPolicyService`,
or `IInteractionService` interfaces exist because the CRM has no
alternate service implementations and no mock-based test layer that
would benefit from them.

**`std::optional` for conditional AutoSaveService ownership**
`Application` holds `AutoSaveService` as `std::optional<AutoSaveService>`
rather than a raw instance or a pointer. If `autosave_enabled` is false
in the config, the optional is left empty and no thread is created. If
enabled, the optional is constructed in place. `std::optional` avoids
heap allocation and makes the "may or may not exist" semantics explicit
in the type.

**`updated_at_` protected in `Interaction`**
The `updated_at_` field is declared `protected` in the `Interaction`
base class rather than `private`. Derived class setters (`setTime`,
`setDuration`, `setReport` on `Appointment`; `setValue`, `setStatus`
on `Contract`) need to update `updated_at_` when a field changes.
Making it `protected` avoids a base-class setter that would exist only
for this internal use.

**Cascade delete on `deleteClient`**
`ClientService::deleteClient` does not just erase the client record.
It first fetches all policies for the client uuid via
`policies_.findByClientUuid` and removes each one, then fetches all
interactions for that uuid and removes each one, then finally removes
the client. The cascade is a business rule: a client has no meaning
without their policies and interactions, and orphaned policies cannot
be displayed because policy views resolve the client name. The
ordering (children first, parent last) keeps the policy and
interaction lookups valid for as long as the cascade is running. Hard
delete was chosen over soft delete in ADR-010; the cascade is the
consequence of that choice.

**`ClientService` depends on all three repositories**
`ClientService` is constructed with `IClientRepository&`,
`IPolicyRepository&`, and `IInteractionRepository&`. This is the only
service that holds references to repositories other than its own.
A reader would expect each service to own exactly one repository, but
the cascade delete above forces the cross-entity dependency: the
client service is the one place that knows the "delete a client also
deletes their policies and interactions" rule, so it needs write
access to all three stores. The alternative (a coordinator at a higher
layer) was rejected because no other operation needs the same
cross-entity write capability.

**`mutable dirty_` and `mutable mtx_` on logically const `save`**
Each CSV repository declares `mutable bool dirty_ = false` and
`mutable std::mutex mtx_`. The `save()` method is `const` at the
interface level (and `findAll`, `findByUuid`, `findByEmail` are also
`const`) but acquires the mutex and reads or writes `dirty_` through
`mutable`. The `mutable` on the mutex is idiomatic C++11: a mutex is
synchronization machinery, not part of the object's logical state.
The `mutable` on `dirty_` is the deliberate part: dirty tracking is
bookkeeping for the auto-save thread, not data the caller can observe
or change. `save()` being `const` says "saving does not modify the
records I store"; the auto-save flag is below that surface.

**`searchClients` searches first and last name only**
`ClientService::searchClients` matches against `getFirstName()` and
`getLastName()` using case-insensitive substring search. Email, phone,
company, and city are not searched. This is intentional: the resolution
pipeline is a name-based lookup and email is a separate uniqueness
constraint handled by `findByEmail`. Extending search to other fields
is deferred.

**`isValidDate` lower year bound is 2026**
`utils::date::isValidDate` rejects any year before 2026 (the project
start year). Historical policy or appointment dates cannot be entered.
This is a deliberate simplification; the bound should be revisited if
backfilling older records becomes a requirement.

**Auto-save writes all records unconditionally**
`AutoSaveService` calls the bound save callable on every timeout
regardless of whether data has changed. Each repository's `save()`
always writes all records to disk. The `dirty_` flag is used only by
`Application::cmdExit` to decide whether to prompt the user before
closing, not to gate the write. This means the disk is written to every
interval even on idle sessions. The cost is acceptable at CLI scale.

**Input normalization in the CLI layer**
The CLI layer normalizes user input before passing it to the service:
first name, last name, job title, company, address, city, and notes are
passed through `strops::capitalize`; email is passed through
`strops::lower`. Phone and postal code are validated as digits-only but
not otherwise transformed. The service receives already-normalized data
and performs no additional normalization itself. Normalization belongs
to the CLI because it is a presentation concern: the service assumes
clean data and should not know that a human typed the input. A future
non-CLI caller (batch import, REST handler) is responsible for
normalizing its own input before calling the service.

**Dual validation in CLI and domain layers**
Email and phone format are validated twice: once in the CLI
(`cmdAdd`, `cmdEdit`) for UX (re-prompt immediately, do not reach the
service with bad data), and once in the domain (`Client` constructor
and setters) as an invariant guard. The duplication is intentional.
The domain copy is the last line of defence and must not trust that
any caller has pre-validated. The CLI copy is a presentation
responsibility: exceptions from the domain are for unexpected
failures, not for guiding the user through a form.

**`strops` module extracted for cross-layer sharing**
`lower`, `capitalize`, `trim`, and `ci_contains` started in the
anonymous namespace of `application.cpp`. `ClientService::searchClients`
also needs case-insensitive comparison but cannot reach into
`application.cpp` (anonymous namespace has internal linkage). The
functions were moved to `src/domain/strops.hpp` and `strops.cpp` in
`insura::domain::strops`. Domain is the correct home because it is the
only layer that all other layers can depend on without violating the
dependency rule.

**Case-insensitive search uses `std::search` with BinaryPredicate**
`strops::ci_contains` uses `std::search` from `<algorithm>` with a
lambda comparator (`tolower(a) == tolower(b)`) instead of lowercasing
copies of the haystack strings before comparing. The copy approach
costs two heap allocations per client per search call; `std::search`
with a predicate does zero allocations and compares characters in
place. For name-length strings the asymptotic cost is the same; the
allocation pressure is eliminated. Storing names pre-lowercased was
rejected because it destroys user-typed casing permanently (names
like `iPhone` or `van der Berg` cannot be recovered once lowercased).

**Client entity resolution pipeline (`resolveClient`, `selectClient`)**
`selectClient` and `resolveClient` are free functions in `insura::cli`
in `cli_helper.hpp` / `cli_helper.cpp`, shared across all three
controllers. `selectClient` is a pure UI primitive: given a non-empty
list, print a numbered menu, return the chosen item. `resolveClient`
owns the full search interaction: prompt for a term, call
`searchClients`, branch on 0 / 1 / N results, call `selectClient`
only when disambiguation is needed. Controllers never call each other
to resolve entities; they call `resolveClient` or `resolvePolicy`
with service references injected at construction.

**`cmdSearch` produces a list; `cmdView` resolves to one record**
`cmdSearch` calls `searchClients` and passes the results to
`ClientView::displayAll`. It does not resolve to a single client.
`cmdView`, `cmdEdit`, and `cmdDelete` call `resolveClient`, which
returns `std::optional<Client>`. The distinction is intentional:
search is a browsing operation that ends with display; view/edit/delete
are action operations that require a single unambiguous target.

**`filterByType` and `filterByDate` are linear scans**
`CsvInteractionRepository::filterByType` and `filterByDate` lock the
repository mutex and walk the full `interactions_` vector, cloning
matches into the result. There is no index, no sorted structure, no
short-circuit. This is fine at the current scale (interactions are
created one at a time by a human, so the vector stays small) and the
filter methods exist primarily because the interaction controller
search flow needs them. The right place for an index, if one is ever
needed, is the repository implementation, not the interface; the
interface contract stays the same.
