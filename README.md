# poliz

A working C++ console CRM application with clean layered architecture, modern C++ idioms, threading (auto-save), and file I/O.

## Overview

poliz manages three core entities for an insurance
company: clients, policies, and interactions (appointments and contracts).
The application runs entirely in the terminal. Data is persisted to CSV files in
a user-specified directory. A background thread saves automatically at a
configurable interval so that data is never lost between manual saves.

### Features

- **Clients**: add, list, search, view detail, edit all fields, delete (with
cascade removal of all associated policies and interactions).
- **Policies**: add (type, duration, and start date are the only inputs; the
system calculates the end date and amount from a fixed pricing table), list,
search, view detail, edit status and notes, delete.
- **Interactions**: add appointments (date, time, duration, optional report) and
contracts (date, value, product name, signed date, optional expiry, status),
list, search, view detail, edit, delete.
- **Application level**: manual save at any time, configurable auto-save in the
background, clean shutdown with a save prompt when there is unsaved data.

## Architecture

The codebase is organized into four layers. Dependencies flow strictly inward:
no layer knows what is above it. main.cpp is the composition root; it constructs
all repositories, services, and controllers and passes them into Application.
Application belongs to insura::cli and calls into insura::service. The service
layer calls into insura::domain through repository interfaces. insura::data
implements those interfaces and also depends on insura::domain. Nothing in a
lower layer knows that a higher layer exists.

- **insura::domain**: entities, repository interfaces, enums, DTOs, and pure
  utilities (string ops, date arithmetic, validation, UUID). No I/O.
- **insura::data**: CSV repositories implementing the domain interfaces. The
  only layer that knows CSV exists. Atomic write pattern, mutex-guarded access.
- **insura::service**: business logic (email uniqueness, cascade delete, policy
  pricing). Receives repository interfaces by reference, no knowledge of CSV or
  the CLI.
- **insura::cli**: controllers with dispatch tables, view classes, shared
  resolution helpers. `Application` is a thin orchestrator owning one controller
  per entity.

insura::service and insura::data are independent siblings: both depend on
insura::domain, neither depends on the other. Service calls through repository
interfaces defined in the domain; data implements them. main.cpp is the only
place that instantiates all four layers together.

CMake enforces the dependency graph at the link level: each layer is a separate
static library and `target_link_libraries` mirrors the allowed dependencies.
A symbol from an unlisted layer causes a linker error.


## Building

### Prerequisites

* CMake 3.14 or later
* A C++17-compatible compiler (GCC 8+, Clang 7+, MSVC 2017+)
* An internet connection for the first build (Catch2 is fetched via
  `FetchContent` from GitHub at configure time)

No other dependencies. The project has no third-party runtime libraries.

### Configure and build

```bash
# Debug build (assertions active, no optimizations)
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Release build (assertions removed via NDEBUG, -O2)
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

### Run

```bash
./build-debug/src/poliz
```

The startup menu offers `new` (create an empty CRM in a directory) or `load`
(load existing CSV files from a directory). The default directory is read from
`poliz.conf` in the project root.

### Run the test suite

```bash
cd build-debug && ctest --output-on-failure
```

Or run the test binary directly for verbose Catch2 output:

```bash
./build-debug/tests/crm_tests
./build-debug/tests/crm_tests --reporter compact   # compact output
```

### CMake target layout

```
poliz
  src/
    insura_domain   (static library)
    insura_data     (static library, links insura_domain)
    insura_service  (static library, links insura_domain)
    insura_cli      (static library, links insura_domain, insura_service)
    poliz           (executable, links all four libraries)
  tests/
    crm_tests       (executable, links Catch2, insura_data, insura_service, insura_cli)
```

Compiler flags active on all targets: `-Wall -Wextra -Wpedantic`.

### Why C++17

C++17 was chosen as the learning target for several reasons. It introduced
several language features that were directly useful in this project and that are
now considered standard practice: `std::optional` for representing absent values
without null pointers, structured bindings for destructuring pairs and tuples,
`if constexpr` for compile-time branching, `std::filesystem` for portable path
manipulation, and `std::string_view` for non-owning string references.

C++17 is also widely supported across all major compilers without flags or
experimental headers, which kept the build setup straightforward.

### What C++20 would change

Several implementation choices in this project exist because C++17 does not
provide a better tool. These are documented here not as deficiencies but as
deliberate trade-offs with a known upgrade path.

- **Date arithmetic**: dates are stored as `std::string` in `YYYY-MM-DD` format.
Custom helpers (`isValidDate`, `calculateEndDate`, `isDateAfter`) handle
validation and arithmetic manually. In C++20, `std::chrono::year_month_day`
provides type-safe date arithmetic. `calculateEndDate` and `isValidDate` would
become one-liners using the calendar library, and date parsing errors would be
type errors caught at compile time rather than runtime throws.
- **Thread-safe timestamp formatting**: `utils.hpp` contains an inline
`safe_localtime` wrapper that dispatches to `localtime_r` on POSIX and
`localtime_s` on Windows via preprocessor macros. This is necessary because
`std::localtime` relies on a shared static buffer that is a data race under
threading. In C++20, `std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::...)` is
thread safe by design and eliminates the platform-specific wrapper entirely.
- **String formatting**: output formatting uses `std::ostringstream` and manual
concatenation throughout (for example, formatting a policy amount as a EUR
string or building a timestamped log line). In C++20, `std::format` provides
the same capability with a cleaner syntax, better performance, and compile-time
format string validation.
- **Range-based filtering**: search operations (filtering clients by name,
filtering policies by client UUID) use explicit for loops with
`push_back`. In C++20, `std::ranges::views::filter` and
`std::ranges::views::transform` compose these into declarative pipelines
without intermediate storage.
- **Module system**: the codebase uses the traditional header and source file
pair for every translation unit. C++20 modules would eliminate `#pragma once`
guards, reduce redundant parsing of headers across translation units, and
make the dependency graph explicit in the language rather than inferred from
include paths.

## Design Patterns

The patterns below appear in the production code. Each is noted with where it
lives and why it was chosen over the alternatives.

- **Repository pattern**: `IClientRepository`, `IPolicyRepository`, and
`IInteractionRepository` are abstract interfaces in the domain layer. Concrete
CSV implementations live in the data layer. Services receive interfaces by
reference at construction. This means the persistence mechanism could be
replaced (for example, swapped for a SQLite implementation) without touching
the service layer. The interfaces also make the dependency graph explicit:
nothing above the data layer knows that CSV exists.
- **Orchestrator and controller**: `Application` is a thin orchestrator that
owns one `IEntityController` per entity. It selects the active controller
based on user input and delegates all CRUD operations to it. Controllers are
registered in the constructor by name; `Application` never mentions client,
policy, or interaction types directly. This was the result of ADR-017, which
replaced a monolithic `Application` class of roughly 400 lines with an
orchestrator of roughly 80 lines.
- **Command dispatch table**: each controller maintains an
`unordered_map<string, function<void()>>` populated in its constructor.
`run()` is a single lookup and call with no switch statements or if-else
chains. Adding a command is one map entry; removing one is one erasure.
- **Prototype (clone)**: the `Interaction` abstract class declares a pure virtual
`clone()` returning `std::unique_ptr<Interaction>`. Each derived class
implements it by delegating to its own copy constructor. The interaction
repository uses this to hand out owned, independent copies without exposing
raw pointers into its internal vector, which would be unsafe under the
background auto-save thread.
- **RAII resource management**: `FileHandler` wraps `std::fstream`. The file
opens in the constructor and closes in the destructor. No caller can forget
to close. The auto-save thread lifetime is managed the same way: the
`AutoSaveService` constructor starts the thread; `stop()` sets the flag and
joins. The destructor calls `stop()` as a safety net.
- **Dependency injection**: every service receives its repository interface by
reference in its constructor. No service instantiates a repository. This keeps
the service layer testable in isolation: integration tests construct real
repositories with temporary files and pass them to the service under test, with
no mocking needed.
- **Data Transfer Object**: `ClientData`, `PolicyData`, and `InteractionData`
are plain structs with optional fields that carry data from the CLI layer to the
service layer. The controller collects user input into the DTO and passes it to
the service. The service validates, enriches (calculates end date and amount for
policies), and constructs the entity. No partially constructed entity crosses a
layer boundary.

## Testing

Tests are written with Catch2 v3.4.0, fetched automatically by CMake at
configure time. There are two levels of tests.

**Unit tests** cover pure functions in the domain layer with no I/O.

| File | Coverage |
|---|---|
| `test_strops.cpp` | trim, lower, capitalize, contains |
| `test_utils.cpp` | isValidDate, isLeapYear, isDateAfter, calculateEndDate, isValidEmail, isDigitsOnly, generateUuid, stringToOptional |
| `test_status_converter.cpp` | all ClientStatus enum-to-string and string-to-enum round trips |
| `test_interaction_status_converter.cpp` | all InteractionType and ContractStatus converters |
| `test_calculate_amount.cpp` | all 16 type/duration combinations in the policy pricing table |
| `test_domain.cpp` | Client and Policy construction and invariant enforcement |

**Integration tests** (`integration_test.cpp`) use real temporary files and
real repository instances. No mocks. The repository interfaces exist for
dependency injection and a future storage swap, not for creating test doubles.
A mock would test the mock, not the serialization or file I/O. Integration tests
cover serialize and deserialize round trips for all three entities, save and
reload cycles, service-level rules (duplicate email rejection, cascade delete,
policy field calculation), and cross-entity integrity (interactions reference
valid clients, client delete cascades to interactions).

Catch2 conventions used throughout: test names describe behavior, not function
names. `SECTION` groups related assertions. `GENERATE` with tables for
parameterized data (all enum-to-string mappings). `CHECK` for independent
assertions in the same section, `REQUIRE` for preconditions and single
assertions. `REQUIRE_THROWS_AS` for exception paths. `Catch::Approx` for all
double comparisons.

## Configuration

The file `poliz.conf` in the project root is read at startup. If it is
missing, defaults apply silently. The user edits the file directly; there is no
config command in the menu.

```ini
# poliz Configuration
autosave_enabled=true
autosave_interval_seconds=60
default_directory=poliz_data
clients_filename=clients.csv
policies_filename=policies.csv
interactions_filename=interactions.csv
```

Invalid values print a warning to stderr and keep the default. Unknown keys are
ignored with a warning. The parser splits on the first `=` only, so values may
contain `=` characters.

The design decision between a Git-style layered config system with typed
callbacks and this single flat file is documented in ADR-023. The flat file was
chosen because the CRM has five configuration keys. A section-based dispatcher
with typed callbacks for five fields is disproportionate to the problem.

## Architecture Decision Records

All architecture decision records live in docs/adr/. The index is docs/adr/00_backlog.md.

## AI Usage

This project was built as a learning exercise. AI was used as a thinking tool
under a strict protocol defined in `CLAUDE.md`.

- **Socratic reasoning**: when stuck on a design or implementation problem after
at least 20 minutes of independent work, AI was consulted using the Socratic
method only: it asked questions and challenged assumptions rather than providing
solutions. No code was generated in this mode.
- **Architecture review**: after forming an architectural position independently
(usually after studying relevant documentation and implementing a first version),
AI was consulted for feedback on that position. It never proposed an
architecture first. ADR-017 (orchestrator and controller), ADR-020 (error
handling strategy), and ADR-024 (auto-save threading design) each went through
this review after the decision was already made.
- **Repetitive pattern code**: once a concept had been studied and implemented
manually at least once, AI generated mechanical repetitions of that pattern
within strictly bounded scope (one function at a time). In every case,
comment-specs describing the function's purpose, parameters, and logical steps
were written first. Every line of the generated output was read, understood, and
manually verified before being committed. Functions generated this way include:

    * Status converter pairs (`statusToString` and `statusFromString`) for
  `PolicyStatus`, `PolicyType`, `InteractionType`, and `ContractStatus`, after
  the `ClientStatus` converters were written manually.
    * The `serialize` and `deserialize` methods in `CsvPolicyRepository` and
  `CsvInteractionRepository`, after `CsvClientRepository` serialization was
  implemented manually.
    * The `safe_localtime` inline wrapper in `utils.hpp`, after the threading
  problem was identified, the two platform-specific APIs were read in their
  documentation, and the preprocessor macro structure was understood.
    * The `GENERATE` table in `test_calculate_amount.cpp`, after the Catch2
  parameterized test pattern was learned and applied manually in the status
  converter tests.

**What AI did not do**: architecture decisions were never delegated. Debugging
was always manual first. New language concepts were always studied and
implemented by hand before AI was involved. The overall structure, layer
boundaries, interface designs, and data model reflect independent design choices
made before any AI consultation.

The study notes and documentation files in docs/ were written by AI after a
specific workflow. Each note started with a discussion where I explained my
understanding of the topic in my own words. AI asked questions, corrected
misconceptions, and challenged assumptions until the understanding was verified.
Only after that verification step did AI write the final note, using my
reasoning and explanations as the content source but producing cleaner English
prose. The ideas and reasoning in every note are mine. The writing quality is
AI-assisted.

## Future Improvements

- Replace CSV persistence with a relational database for transactional writes and indexed queries.
- Interactive search with real-time filtering as the user types.
- Stricter input validation: time format enforcement (HH:MM 24-hour), phone number country-code normalization, policy deduplication on type + client + start date.
- Address autocomplete via geocoding API.
- Performance test suite with generated datasets to measure load, save, and search under realistic volume.
