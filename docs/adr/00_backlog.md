# InsuraPro CRM: Architecture Decision Log 

## Decided

**ADR-001: Primary Key Strategy**
UUID v4 as internal surrogate PK. Email as user-facing unique constraint
enforced by the service layer. Decouples internal identity from mutable
attributes. Implemented in utils.cpp (manual mt19937 generator) and
enforced in ClientService. Full ADR in ADR-001.md.

**ADR-002: Policy as Independent Entity**
Policy is a separate entity with client_uuid as foreign reference.
Models the real one-to-many relationship (one client, many policies).
Implemented: IPolicyRepository and Policy class exist.

**ADR-003: Interaction Types via Inheritance**
Abstract Interaction base class with Appointment and Contract as
derived classes. Virtual clone() for safe polymorphic retrieval.
Type-specific fields on each derived class; updated_at_ protected
so derived setters can update it directly. Full ADR in ADR-003.md.

**ADR-004: Startup Flow**
New / Load menu at startup. Explicit user choice every time.
Implemented in main.cpp via named helpers in an anonymous namespace
(cmdNew, cmdLoad, makeRepositories) and a while loop in main() that
breaks once a valid CrmRepositories is obtained.

**ADR-005: TUI Interactive Search**
Stretch goal. Deliberately not implemented to control scope.
No implementation started.

**ADR-006: Auto-Save Configuration**
User-configurable: enabled/disabled toggle and interval in seconds,
stored in a config file. Defaults: enabled, 60 seconds. Configuration
mechanism documented in ADR-023.md. AutoSave threading documented in
ADR-024.md.

**ADR-007: CSV Schema Design**
Comma as global delimiter. Address stored as three separate columns
(address, city, postal_code). Commas prohibited in free-text fields
by convention (not enforced by code). Empty optional fields represented
as empty strings between delimiters. Column layout defined for
clients.csv (14 cols), policies.csv (10 cols), interactions.csv
(variable by type). Full ADR in ADR-007.md.

**ADR-008: UUID Generation Strategy**
Manual v4 implementation using mt19937 in utils.cpp. No external
dependency. Not thread-safe initially; fixed with a static mutex
in generateUuid when AutoSave introduced a second thread (see ADR-024).

**ADR-009: Date/Time Representation**
Plain std::string used for all date fields (YYYY-MM-DD format validated
by isValidDate in utils). Migration to std::chrono types deferred until
a concrete need arises. safe_localtime wrapper added to utils.hpp for
thread-safe timestamp formatting (see ADR-024).

**ADR-010: Soft Delete vs Hard Delete**
Hard delete. removeClient erases the record and cascades to all related
policies and interactions. No active flag or deleted_at column. Cascade
enforced in ClientService::deleteClient, not in the repository layer.
Full ADR in ADR-010.md.

**ADR-011: Testing Strategy**
Superseded by ADR-021.md. Catch2 chosen, four testing levels defined,
no mocking, real temporary files for integration tests.

**ADR-013: Default Filepath**
Superseded by ADR-018.md. Directory-based path strategy replaced
the single-filepath default.

**ADR-015: Performance Validation**
Not implemented. Deliberately deferred post-submission. Seed script
and std::chrono timing approach remain the planned approach.

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

## Undocumented decisions (brief)

**Atomic save via tmp+rename**
Repositories write to a temporary file and rename it over the target
on success. A crash mid-write leaves the old file intact rather than
a half-written one. Deliberate crash-safety pattern; no full ADR written.

**Policy deduplication not enforced**
The service layer does not reject a policy that is identical to an
existing one for the same client. Duplicate policies are assumed not
to be created by the user. Deliberate deferral; to be added in a
future validation pass.

**No service interfaces (IClientService etc.)**
IClientRepository and IPolicyRepository exist for dependency injection
(constructor injection into services, enabling future storage swaps).
No IClientService, IPolicyService, or IInteractionService interfaces
exist. 

**std::optional for conditional AutoSaveService ownership**
Application holds AutoSaveService as std::optional<AutoSaveService>
rather than a raw instance or a pointer. If autosave_enabled is false
in the config, the optional is left empty and no thread is created.
If enabled, the optional is constructed in-place. 

**updated_at_ protected in Interaction**
The updated_at_ field is declared protected in the Interaction base
class rather than private. Derived class setters (setTime, setDuration,
setReport on Appointment; setValue, setStatus on Contract) need to
update updated_at_ when a field changes. Making it protected avoids
a base-class setter that would exist only for this internal use.

Missing documentations
- the fact that I decided deliberately to not validate time 22:10 for example or using a specific format like am or pm
- the fact that I've decided to not validate phone numbers for now
- the fact that I explicitly decided to avoid user to add in policy the amount and the end date and create a constraints, like a business rule to be pre-calcualted (I could have done also for policies but I don't care for now)  

Things to remove
- adr about interactions: is not necessary since I've not discussed the creation of a flat class, a base class with inherited was the main decision and is also documented in the architecture. I eliminated it so please update the document removing all reference to ADR-003
- ADR-010 could be documented briefly just saying 'soft deleve vs hard delete and I chose the hard one'. Is not necessary to explore each case further


