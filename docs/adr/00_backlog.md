# InsuraPro CRM: Architecture Decision Log 

## Decided

**ADR-001: Primary Key Strategy**
UUID v4 as internal surrogate PK. Email as user-facing unique constraint
enforced by the service layer. Decouples internal identity from mutable
attributes. Implemented in utils.cpp (manual mt19937 generator) and
enforced in ClientService.

**ADR-002: Policy as Independent Entity**
Policy is a separate entity with client_uuid as foreign reference.
Models the real one-to-many relationship (one client, many policies).
Implemented: IPolicyRepository and Policy class exist.

**ADR-003: Interaction Types via Inheritance**
Abstract Interaction base class with Appointment and Contract as
derived classes. Virtual display() for polymorphic rendering.
Declared in interaction.hpp, not yet implemented.

**ADR-004: Startup Flow**
New / Load menu at startup. Explicit user choice every time.
Implemented in main.cpp via IIFE pattern.

**ADR-005: TUI Interactive Search**
Stretch goal. Implemented last after core milestones are complete.
No implementation started.

**ADR-006: Auto-Save Configuration**
User-configurable: enabled/disabled toggle and interval in seconds,
stored in a config file. Defaults: enabled, 60 seconds.
No implementation started.

**ADR-007: CSV Schema Design**
Comma as global delimiter. Address stored as three separate columns.
Commas prohibited in all free-text fields (stripped in domain setters).
Field order defined for clients.csv, policies.csv, interactions.csv.
Empty optional fields represented as empty strings between delimiters.

**ADR-008: UUID Generation Strategy**
Manual v4 implementation using mt19937 in utils.cpp. Decided implicitly
by implementation. Not thread-safe (shared static state), must be fixed
before AutoSave thread (Milestone 2).

**ADR-010: Soft Delete vs Hard Delete**
Hard delete. removeClient erases from the vector directly. No deleted_at
or active flag. Cascade delete to policies required when Policy layer
lands.

**ADR-012: Generic Repository Pattern**
Deferred. The three repositories differ enough in serialization logic
(especially Interaction with polymorphic unique_ptr) that a generic
template saves less code than expected. Revisit after project completion
with concrete line counts.

**ADR-014: searchClients Scope**
Current implementation searches first_name and last_name only.
Email, company, city excluded. cmdSearch forces resolution to a
single client via resolveClient(). Decision needed on whether to
expand search fields and whether to display all results without
forcing single selection. Deferred to polish milestone.

**ADR-016: Pre and Post Submission Priorities**
Before submission: CMake deep study, then CI/CD pipeline (GitHub Actions
with build, clang-format, clang-tidy, AddressSanitizer).
After submission: Docker, then SQLite.

**ADR-017: Application Refactoring (Orchestrator + Controller)**
Application becomes thin orchestrator owning controllers via
unordered_map of unique_ptr<IEntityController>. Each controller
(ClientController, PolicyController, InteractionController) implements
the same interface and owns its own service, dispatch table, and
command implementations. Two-level menu: init menu for entity selection,
entity menu for CRUD commands. Save, exit, clear handled at application
level via handleAppCmds(). Implemented.

**ADR-018: File Path Strategy (Directory-Based Multi-Entity Persistence)**
User provides a directory path at startup. Application derives filenames:
<dir>/clients.csv, <dir>/policies.csv, <dir>/interactions.csv.
New session: create directory if missing, repos start empty.
Load session: each repo loads independently, missing file means empty
state (not an error). Throw only on malformed data or permission errors.

**Error Handling Strategy (unnumbered, to be formalized)**
Asserts for programmer errors (active in debug, removed in release).
Exceptions for rare runtime errors (I/O, constraint violations, malformed data).
std::optional for expected absence (find operations returning no result).
Strong exception safety via transactional pattern (build temporary,
commit with noexcept move).

## Open (to be decided during implementation)

**ADR-009: Date/Time Representation**
Plain std::string used today for created_at, updated_at. Interaction
entity needs date field. AutoSave needs timestamp comparison. Current
approach: string format YYYY-MM-DD validated by isValidDate in utils.
Decision on whether to migrate to std::chrono types deferred until
AutoSave implementation.

**ADR-011: Testing Strategy**
No tests exist. Google Test, Catch2, or manual harness not yet chosen.
Must be decided and set up before AutoSave (Milestone 2). ClientService
and CsvClientRepository are highest value first targets.

**ADR-013: Default Filepath**
When user selects "start empty" after a failed load, a default filepath
constant is used. Currently kDefaultFilepath in main.cpp. Will be
replaced by directory-based path strategy (ADR-018) when implemented.

**ADR-015: Performance Validation**
Seed script generates CSV with thousands of fake clients. Load at startup
and measure operations with std::chrono timers around load(), searchClients(),
save(). Compare before and after optimizations (move semantics, reserve).
Use perf for deeper analysis. Address after threading milestone.
