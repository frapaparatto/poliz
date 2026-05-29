# poliz: Architecture

## Overview

poliz is a terminal application written in C++17 that manages
clients, policies, and interactions (appointments and contracts) for an
insurance company. The codebase is organized into
four layers with dependencies flowing strictly inward. Data is persisted
to CSV files. A background thread saves automatically at a configurable
interval.

This document is the entry point for anyone who wants to read the code,
understand the choices, or follow a feature from CLI prompt down to disk.

## Original Design

The project started as a single-entity CLI application. Application was
a monolithic class that handled all Client CRUD operations directly:
command dispatch, user prompting, service logic, and repository access
were mixed in one 400-line class. Persistence was a single CSV file. No
threading, no configuration, no tests.

## How the Architecture Evolved

When the Policy entity was introduced, the monolithic Application became
unmanageable. Adding six CRUD commands, a service, a repository, and
entity-specific helpers to the same class would have doubled its size
and scattered responsibilities. ADR-017 records the decision to refactor
Application into a thin orchestrator that owns controllers via an
`unordered_map<std::string, std::unique_ptr<IEntityController>>`. Each
controller implements the same interface and manages its own dispatch
table. Application selects the active controller based on user input
and delegates. Adding a new entity means creating one controller class
and one line in the constructor. Application itself never changes.

The original design used a single filepath for client data. With three
entities needing their own CSV files, the startup flow was redesigned
(ADR-018) to accept a directory path and derive all filenames from it.
Missing files start empty rather than throwing, because a CRM with
clients but no policies yet is a valid state.

Threading arrived with the auto-save feature. The background thread
introduced data races on every shared resource: the client and policy
vectors in the repositories, the static `mt19937` in UUID generation,
and the shared buffer in `std::localtime`. Each repository received its
own mutex with `lock_guard` on every data-accessing method. UUID
generation got its own mutex. `std::localtime` was replaced with an
inline `safe_localtime` wrapper dispatching to `localtime_r` on POSIX
and `localtime_s` on Windows via preprocessor macros. ADR-024 documents
the full threading design.

The Interaction entity introduced polymorphism. Appointments and
Contracts share a base class but have different fields. Storing them
by value in a vector would cause object slicing (the derived fields
are lost when copied through the base type). The repository stores
`unique_ptr<Interaction>` instead. This created a new problem for find
operations: you cannot return a `unique_ptr` by value without removing
it from the repository, and returning a raw pointer creates a dangling
reference risk under threading. The solution was a virtual `clone()`
method (ADR-025) that returns an independent copy on the heap. Each
derived class implements `clone()` by delegating to its own copy
constructor. The caller gets a fully owned copy with no connection
to the repository's internal storage.

## Layer Architecture

Dependencies flow inward. No layer knows what is above it. The four
libraries and their direct dependencies are:

- insura::domain depends on nothing else. All other layers depend on it.
- insura::data depends on insura::domain. It implements the repository
  interfaces defined there and has no knowledge of insura::service.
- insura::service depends on insura::domain. It holds repository interface
  references and has no knowledge of insura::data or how persistence works.
- insura::cli depends on insura::domain and insura::service. It has no
  knowledge of insura::data.
- main.cpp (the composition root) is the only translation unit that
  instantiates all four layers and wires them together.

insura::service and insura::data are siblings: both depend on
insura::domain independently. Service calls through the repository
interfaces; data implements them. Neither depends on the other.

CMake enforces the dependency graph at the link level: each layer is a
separate static library and `target_link_libraries` mirrors the allowed
dependencies. A symbol from an unlisted layer causes a linker error rather
than a runtime surprise.

### Domain Layer (insura::domain)

Source: `src/domain/`

The shared language of the system. Every other layer depends on this
module. It contains no I/O, no business logic, no CLI code.

**What lives here:**

- Entities: `Client`, `Policy`, `Interaction` (abstract), `Appointment`,
  `Contract`
- Repository interfaces: `IClientRepository`, `IPolicyRepository`,
  `IInteractionRepository`
- DTOs for the CLI -> service boundary: `ClientData`, `PolicyData`,
  `AppointmentData`, `ContractData`
- Enums and their string converters: `ClientStatus`, `PolicyStatus`,
  `PolicyType`, `InteractionType`, `ContractStatus`
- Pure utility namespaces: `insura::domain::strops` (trim, lower, capitalize,
  contains), `insura::utils` (UUID generation, timestamp, email format
  check, date arithmetic, `safe_localtime`)

**What does not live here:**

- File or stream I/O
- CSV parsing or serialization
- Validation tied to user experience (length limits, prompts)
- Any include of a platform or terminal header

### Data Layer (insura::data)

Source: `src/data/`

The only layer that knows CSV exists. Implements the repository
interfaces from the domain module.

**What lives here:**

- `CsvClientRepository`, `CsvPolicyRepository`, `CsvInteractionRepository`
- `FileHandler`, an RAII wrapper around `std::fstream`
- CSV serialization and deserialization for each entity
- Polymorphic serialization for interactions (dispatch on `getType()`,
  `dynamic_cast` to access derived-class fields)
- The atomic save pattern: write to `<file>.tmp` then `std::rename`
  over the original so a crash mid-write leaves the previous file
  intact
- Per-repository `std::mutex` and `mutable bool dirty_` flag

**What does not live here:**

- Business rules (cascade delete, pricing, uniqueness)
- User prompts or output
- Knowledge of which CLI commands exist

### Service Layer (insura::service)

Source: `src/service/`

Owns business logic and orchestration. Each service receives repository
interfaces by reference in its constructor and never instantiates a
concrete repository itself.

**What lives here:**

- `ClientService`: email uniqueness check, cascade delete (removing a
  client removes all their policies and interactions)
- `PolicyService`: amount and end-date calculation from a constexpr
  4 x 4 pricing table indexed by type and duration; immutability of
  contract-defining fields on edit (ADR-019)
- `InteractionService`: appointment and contract creation, contract
  expiry-date calculation
- `AutoSaveService`: background thread, `std::condition_variable` with
  predicate against the stop flag. On each timeout the service calls the
  bound save callable unconditionally; every repository always writes all
  records. The `isDirty()` flag is used only by the exit flow to decide
  whether to prompt the user, not to gate the auto-save write.

**What does not live here:**

- CSV or file handling
- User input or output (no `std::cin`, no `std::cout`)
- Knowledge of how the CLI groups commands

ClientService is the one cross-entity exception: it holds references
to all three repositories because cascade delete must remove a client's
policies and interactions atomically before the client itself is
erased.

### CLI Layer (insura::cli)

Source: `src/cli/`

Handles user interaction and the command loop.

**What lives here:**

- `Application`: thin orchestrator. Owns
  `std::unordered_map<std::string, std::unique_ptr<IEntityController>>`,
  runs the two-level menu (entity selection, then CRUD command), and
  dispatches application-level commands (`save`, `exit`, `clear`)
- `IEntityController`: pure-virtual interface listing the six CRUD
  commands plus `save`, `isDirty`, and `execute`
- `ClientController`, `PolicyController`, `InteractionController`:
  each owns one service reference and an internal dispatch table
  mapping command strings to `std::function<void()>`
- Views: `ClientView`, `PolicyView`, `InteractionView`. Static classes
  that format records for the terminal
- Shared helpers in `cli_helper`: free functions in `insura::cli` such
  as `resolveClient`, `selectClient`, `resolvePolicy`, `resolveInteraction`,
  `promptRequired`, `promptOptional`. They take service references as
  parameters so no controller depends on another controller

**What does not live here:**

- Business logic. The CLI prepares DTOs and lets the service decide
- CSV knowledge

Note: controllers do access repositories directly for three specific
operations that are intentionally kept out of the service layer: `save()`
and `isDirty()` (called by the application-level command loop), `findAll()`
(for the list command which needs raw iteration), and `findByUuid()` on the
client repository (for building the client-name map used in policy and
interaction display). All business rules go through services.

### Composition Root (main.cpp)

`main.cpp` is the only place in the codebase that knows every layer
exists. It reads `poliz.conf` (ADR-023), prompts for new or load,
constructs the three CSV repositories, wires the three services on top,
and hands everything to `Application`.

```cpp
int main() {
  CrmConfig config = parseConfig();
  CrmRepositories repos;

  /* prompt new / load / exit until the user picks a valid directory */

  insura::service::PolicyService policy_service(*repos.policy_repo);
  insura::service::ClientService client_service(
      *repos.client_repo, *repos.policy_repo, *repos.interactions_repo);
  insura::service::InteractionService interaction_service(
      *repos.interactions_repo);

  insura::cli::Application app(
      config.autosave_enabled, config.autosave_interval_seconds,
      client_service, *repos.client_repo,
      policy_service, *repos.policy_repo,
      interaction_service, *repos.interactions_repo);

  app.run();
  return 0;
}
```

## Orchestrator and Controllers

Application holds controllers behind a uniform interface so the menu
loop does not know which entities exist:

```cpp
class IEntityController {
 public:
  virtual ~IEntityController() = default;
  virtual void save() = 0;
  virtual bool isDirty() const = 0;
  virtual void cmdAdd() = 0;
  virtual void cmdList() = 0;
  virtual void cmdSearch() = 0;
  virtual void cmdView() = 0;
  virtual void cmdEdit() = 0;
  virtual void cmdDelete() = 0;
  virtual void execute(const std::string& cmd) = 0;
};
```

The map is populated in Application's constructor:

```cpp
controllers_["clients"]      = std::make_unique<ClientController>(...);
controllers_["policies"]     = std::make_unique<PolicyController>(...);
controllers_["interactions"] = std::make_unique<InteractionController>(...);
```

Each controller registers its commands in its own constructor, mapping
strings to member functions through `std::function<void()>`:

```cpp
commands_["add"]    = [this]() { cmdAdd();    };
commands_["search"] = [this]() { cmdSearch(); };
commands_["list"]   = [this]() { cmdList();   };
commands_["view"]   = [this]() { cmdView();   };
commands_["edit"]   = [this]() { cmdEdit();   };
commands_["delete"] = [this]() { cmdDelete(); };
```

Adding a new entity is one new controller class plus one line in
Application's constructor. Application itself never changes.

## Polymorphism and Interactions

`Interaction` is an abstract base. `Appointment` and `Contract` derive
from it with their own fields:

```
Interaction (abstract)
  |
  +-- Appointment  (time, duration, report)
  |
  +-- Contract     (value, status, expiry date)
```

The repository stores `std::vector<std::unique_ptr<Interaction>>` to
avoid object slicing. The find methods cannot return the stored pointer
(it would either be removed from the repository or expose a dangling
reference once the lock is released), so the base class declares:

```cpp
virtual std::unique_ptr<Interaction> clone() const = 0;
```

Each derived class implements it by calling its own copy constructor:

```cpp
std::unique_ptr<Interaction> Appointment::clone() const {
  return std::make_unique<Appointment>(*this);
}
```

`findByUuid` and `findAll` build their results by calling `clone()`
under the lock. The caller gets a fully owned, independent copy.

`updated_at_` is `protected` on `Interaction` (not `private`) so derived
setters such as `Appointment::setTime` and `Contract::setStatus` can
update it directly without going through a base-class setter that would
exist only for this internal use.

## Namespaces

| Namespace          | Contents                                                                                |
| ------------------ | --------------------------------------------------------------------------------------- |
| `insura::domain`   | Entities, repository interfaces, DTOs, enums, `strops`                                  |
| `insura::utils`    | UUID generation, timestamp, email and date validation, date arithmetic, `safe_localtime` |
| `insura::data`     | `CsvClientRepository`, `CsvPolicyRepository`, `CsvInteractionRepository`, `FileHandler` |
| `insura::service`  | `ClientService`, `PolicyService`, `InteractionService`, `AutoSaveService`               |
| `insura::cli`      | `Application`, controllers, views, `cli_helper` free functions                          |

`main.cpp` and the anonymous namespace inside it have no namespace.
They are the composition root, not a layer.

## Key Design Decisions

All architecture decision records live in docs/adr/. The index is docs/adr/00_backlog.md.

## C++ Conventions

- **Constructors** take `std::string` by value and move into members.
  New-object constructors (the ones that call `generateUuid()`) assign
  via `std::move` in the constructor body, after validation. Load
  constructors (the ones that accept a pre-existing UUID) use initializer
  lists. In both cases, pass by value lets the caller decide whether to
  copy or move into the parameter, then the constructor moves from the
  parameter into the member. Cost: at most one copy plus one move.
- **Setters** take `const std::string&` because the member already
  exists and may have an allocated buffer. The assignment operator can
  reuse the existing buffer if the incoming string fits within the
  current capacity, avoiding a heap allocation. Pass-by-value-then-move
  in setters is a pessimization: it always allocates a new buffer for
  the parameter, moves it into the member, and discards the old buffer.
- **Getters** return `const std::string&` for strings and
  `const std::optional<std::string>&` for optional strings to avoid
  copies. Getters for primitive types (`double`, `int`, enum) return
  by value since copying a primitive is free.
- **findAll** on Client and Policy repositories returns by value, not
  by reference. Returning a reference would allow the caller to hold a
  pointer into the internal vector after the mutex is released. Any
  write from another thread that triggers reallocation would invalidate
  that pointer. The copy is made while the lock is held, giving the
  caller a consistent snapshot. `findAll` on the Interaction repository
  returns a vector of cloned `unique_ptr`s for the same thread-safety
  reason plus the additional constraint that `unique_ptr` cannot be
  copied.
- **mutable** is used on `dirty_` and `mtx_` in the repositories
  because `save()` is declared `const` in the interface (saving does
  not change the logical state of the data) but needs to modify
  `dirty_` and lock the mutex. `mutable` tells the compiler these
  members are bookkeeping, not part of the object's logical state.
- **std::optional ownership**: Application holds
  `std::optional<AutoSaveService> autosave_` rather than a pointer.
  If `autosave_enabled` is false in the config the optional is left
  empty and no thread exists. If true it is constructed in place. No
  heap allocation, no null check, the type itself says "may or may not
  exist."

## Entity Lifecycle

### Adding a Policy

1. The user selects "policies" at the top-level menu. Application sets
   the active controller to `PolicyController`. The user types "add."
   The dispatch table calls `cmdAdd`.
2. `cmdAdd` prompts for a client. It calls `resolveClient` (a free
   function in `cli_helper`) which asks for a search term, calls
   `ClientService::searchClients`, displays matches, and returns the
   selected `Client`. The controller shows the client's name and email
   and asks for confirmation.
3. The controller then prompts for policy type (enum menu: auto, life,
   home, health), duration (menu: 6, 12, 24, 36 months), and start
   date (validated by `isValidDate`). It calculates the end date and
   amount using `PolicyService::calculateAmount` (a constexpr 4 x 4
   lookup table) and displays them as a preview before the user
   commits.
4. On confirmation, the controller builds a `PolicyData` DTO with the
   user's inputs and calls `PolicyService::addPolicy`. The service
   validates the start date, calculates `end_date` via
   `calculateEndDate` (month arithmetic with day clamping), calculates
   amount from the pricing table, defaults status to `PENDING` if not
   provided, constructs a `Policy` entity, and calls `insertPolicy` on
   the repository.
5. The repository locks its mutex, pushes the `Policy` into its vector,
   sets `dirty_` to true, and unlocks. The auto-save thread, sleeping
   on a condition variable with a 60-second timeout, eventually wakes,
   calls the bound save callable, which iterates all controllers and calls
   `save()` on each repository unconditionally. Each repository locks its
   mutex, serializes all records to a temp file, renames it over the
   original, sets `dirty_` to false, and unlocks. The thread goes back
   to sleep.

### Deleting a Client

1. The user selects "clients" and types "delete." The controller calls
   `resolveClient` to identify the target, displays the client's full
   record, and asks for confirmation.
2. On confirmation, the controller calls `ClientService::deleteClient`
   with the UUID. The service checks that the UUID exists (assert for
   debug, throw for release: belt-and-suspenders pattern, ADR-020).
3. The service then cascades: it fetches all policies for that client
   UUID via `findByClientUuid` on the policy repository and removes
   each one. Then it fetches all interactions for that client UUID and
   removes each one. Finally it removes the client itself. Each removal
   locks the respective repository's mutex, erases the record, and
   sets `dirty_`.
4. When the user eventually exits, Application checks `isDirty` on
   each controller. If any is dirty, it prompts "Save before exiting?"
   If the auto-save thread already saved since the last change
   `dirty_` is false and no prompt appears. The auto-save thread is
   stopped via `AutoSaveService::stop()`, which sets the stop flag,
   notifies the condition variable, releases the lock, and calls join.
   The main thread waits for the auto-save thread to finish before the
   program exits.

## Thread Model

The application has two threads:

- **Main thread**: runs the CLI loop, handles user input, performs
  CRUD operations.
- **Auto-save thread**: sleeps, wakes periodically, saves dirty
  repositories, goes back to sleep.

```
Main thread                          Auto-save thread
---------------------------------    ---------------------------------
runs the CLI loop                    wait_for(N seconds, stop_flag)
user adds a policy:                  on timeout:
  lock policy mutex                    call save_() unconditionally:
  push_back                              for each controller:
  dirty_ = true                            lock repo mutex
  unlock                                   write all records to .tmp
                                           rename .tmp over original
                                           dirty_ = false
                                       go back to wait_for
on exit:
  set stop_flag                      wakes on condv_.notify_one()
  notify condvar                     predicate sees stop_flag
  release control lock               exits loop
  join auto-save thread              thread terminates
  program exits
```

Four mutexes protect four independent shared states:

- The client repository mutex protects `clients_`.
- The policy repository mutex protects `policies_`.
- The interaction repository mutex protects `interactions_`.
- The auto-save control mutex protects the stop flag and works with
  the condition variable.

Each repository manages its own mutex internally. Neither Application
nor AutoSaveService touches repository mutexes directly.

The auto-save thread uses `std::condition_variable::wait_for` with a
predicate checking the stop flag. It either wakes on timeout (time to
save) or on notification (time to shut down). The predicate prevents
spurious and lost wakeups.

Shutdown: main thread sets the stop flag, notifies the condition
variable, releases the lock, calls join. The auto-save thread wakes,
sees the flag, exits its loop. join unblocks and the main thread
continues to exit. The lock must be released before join to avoid
deadlock: main holds the lock, auto-save needs it to check the flag,
neither can proceed.

Full threading design documented in ADR-024.

## Testing

Tests use Catch2 v3.4.0 fetched via FetchContent. Two levels: unit
tests and integration tests. No mocks.

Unit tests cover pure functions in the domain layer: string operations,
date validation, date arithmetic, UUID format, email validation, all
enum converter round-trips, and all 16 pricing table combinations.

Integration tests use real temporary files and real repository
instances. They verify CSV round-trips (serialize, save, load,
deserialize, compare all fields), service business rules (duplicate
email rejection, cascade delete, policy field calculation), and
polymorphic persistence (an Appointment serialized and reloaded is
still an Appointment with all its fields intact).

Repositories are not mocked because a mock tests the mock, not the
code. The CSV repositories are local, fast, and have no external
dependencies. Real files exercise the actual serialization and I/O
logic. Mocking would be justified for external services with rate
limits or network dependencies, which this project does not have.

Full testing strategy documented in ADR-021.
