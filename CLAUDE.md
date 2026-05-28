# CLAUDE.md

## AI Usage Rules

This project is a learning project. AI is a thinking tool, not a
substitute for thinking.

**When AI can write code:**
All four conditions must be true simultaneously:
1. The concept has already been studied and implemented at least once
   manually. This is mechanical repetition, not learning.
2. Scope is limited to one function or equivalent block. If the impulse
   is to expand to "just this class" or "just this layer," stop.
3. Before asking, write comment-specs in the code: what the function
   does, what arguments it accepts, the logical steps. The comments
   are the spec. AI executes, it does not think.
4. Before accepting output: read every line, explain it mentally. If
   any line cannot be explained, do not commit it. Understand it or
   rewrite it.

**When AI cannot write code:**
- New concepts not yet studied or implemented manually
- Anything involving CSAPP labs, codecrafters, or from-scratch
  implementations
- Debugging: always debug manually first
- Architecture decisions: AI gives feedback after a position exists,
  never as the first answer

**When AI can be consulted:**
- After implementation is complete, for feedback and pattern review
- After reading documentation and forming a hypothesis
- After trying 2-3 approaches independently
- If still stuck after a 20-minute timer, AI uses Socratic method
  only: questions, not solutions

## Code Behaviour

- Before writing any code: ask about any doubts, propose a plan
  with the list of files to change and the approach, wait for
  explicit approval. Do not read or edit files until the plan is
  confirmed.
- Changes must be scoped. One concern per edit session.
- If a change involves a concept not yet studied or implemented by
  the user, flag it explicitly and defer to manual implementation.
- Before making changes: grep the codebase for every call site and
  occurrence affected. List them in a numbered checklist. Wait for
  confirmation of completeness, then edit each one and check it off.
- After the plan is executed, build:
  `cmake --build build-debug 2>&1`

## Audit and Review Behaviour

- When asked to audit, review, or report: do NOT modify any files
  unless explicitly instructed. Produce read-only analysis first.
  Wait for confirmation before editing.
- Before writing any documentation output: confirm (1) target file
  path, (2) whether to create new or append, (3) heading and section
  format. Wait for approval, then write.

## Documentation Locations

- Always confirm the target file before writing documentation.
- Personal notes and design docs: `docs/personal/`
- Master TODO: `docs/personal/important_files/master_todo.md`
- Architecture updates: `docs/personal/architecture/`
  Each new update goes there alongside the existing ones.
- ADRs: `docs/adr/` (or wherever the ADR files currently live)
- Daily journal: `docs/daily_logs/`

## Code Style

- Multi-line comment style (`/* ... */`) for explanatory and design
  comments. Single-line `//` only for brief inline notes.
- Write comments only for non-obvious behaviour, constraints, or
  the reasoning behind an implementation. Do not comment what the
  code already says.
- Fully qualified namespace paths (e.g., `insura::domain::strops::`)
  everywhere. No namespace aliases.
- No hyphens, long dashes, or em dashes in markdown files or
  comments. Use commas, colons, or plain text instead.

## User-Facing Messages

- Professional CRM tone. No casual language ("Goodbye," exclamation
  marks, informal phrasing).
- Error messages: lowercase, no "Error:" prefix in the throw message.
  The catch site at the controller boundary adds "Error: " for
  display. No leading spaces or indentation. Example:

```cpp
  /* In the service layer: */
  throw std::invalid_argument("email already in use");

  /* In the controller catch: */
  std::cout << "\nError: " << e.what() << '\n';
```

- Success messages: no leading spaces, clean and direct.
  Example: `"\nClient added successfully.\n"`
- Warning messages (config, non-fatal): prefixed with context,
  no leading spaces.
  Example: `"Config warning [key]: 'value' is not valid. Default used: X\n"`
- No indentation on any user-facing output message. Indentation is
  only for menu option listings.

## Error Handling Strategy (ADR-020)

- assert() for programmer errors (active in debug, removed in release)
- Exceptions for rare runtime errors (I/O failures, constraint
  violations, malformed data)
- std::optional for expected absence (find returning no result)
- Belt-and-suspenders (assert + throw) for destructive operations
  (delete, edit) where data loss is irreversible
- std::invalid_argument for validation errors across all layers
- std::runtime_error for I/O and infrastructure errors only

## Testing Conventions

- Do not break encapsulation or change visibility for testing. If a
  function is hard to test, it is likely poorly designed. Flag it
  and I will redesign it manually before testing proceeds.
- Unit tests: pure logic, no I/O. Catch2 with SECTION and GENERATE.
- Integration tests: real temporary files, real repositories. No mocks.
- Tags on every TEST_CASE: [domain], [strops], [utils], [date],
  [csv], [integration], [service].
- Test names describe behaviour, not function names.
- CHECK for independent assertions in the same section, REQUIRE for
  preconditions and single assertions.
- Catch::Approx for all double comparisons.

## Architecture Rules

- Layered architecture: domain, data, service, cli. Dependencies
  flow inward (cli -> service -> domain, data -> domain).
- Application is a thin orchestrator owning controllers via
  unique_ptr<IEntityController>.
- Controllers own their dispatch tables. Application does not know
  what commands exist.
- Save, exit, clear are application-level commands. CRUD commands
  belong in controllers.
- Shared CLI helpers (resolveClient, selectClient, promptRequired,
  promptOptional) live in cli_helper.hpp/cpp as free functions
  taking service references as parameters.
- Repository interfaces exist for dependency injection, not for
  mocking in tests.
- Setters take const std::string& for buffer reuse. Constructors
  take std::string by value and move.

## Git Conventions

- Conventional Commits format: `<type>(<scope>): <short description>`
- Types: feat, fix, refactor, test, docs, chore, style, perf, build, ci
- Scopes: client, policy, interaction, csv, thread, config, tui, cmake, ci
- Subject line: imperative mood, no capital after colon, no period, max 72 chars
- Body: explain why, not what. Wrap at 72 chars. Blank line between subject and body.
