# InsuraPro CRM: Architecture

## 1. OVERVIEW

InsuraPro CRM is a console application for a fictitious Italian insurance company,
written in C++17. It manages three core entities: clients, policies, and
interactions. Policies belong to clients. Interactions are either appointments or
contracts, and each belongs to a client. The application is organized into four
layers: domain, data, service, and cli. Dependencies flow strictly inward. The cli
layer knows about service; service knows about domain; data knows about domain;
nothing knows about a layer above it. Data is persisted to CSV files in a
a directory specified by the user. A background thread handles periodic saves so that data
is not lost between manual saves.

---

## 2. ORIGINAL DESIGN

The initial architecture was written in March 2026 before any code existed.
Application was a monolithic class that owned the main loop, handled startup and
shutdown, and contained all client CRUD operations directly. There was a single
filepath for client data, which the user provided at startup. No threading existed;
all saves were manual. There were no controllers and no IEntityController
interface. The service and repository layers were already planned, but the cli
layer was essentially Application plus a small view class. The full original
specification is in docs/personal/architecture/architecture.md.

---

## 3. HOW THE ARCHITECTURE EVOLVED

### Monolithic Application to orchestrator plus controller (ADR-017)

The trigger was the addition of the Policy entity. With client CRUD already in
Application, adding policy commands to the same class would have pushed it past a
thousand lines, with three sets of CRUD handlers, three dispatch tables, and three
sets of helpers all in one place. The previous design had no mechanism to contain
logic for each entity; everything lived directly on Application.

The decision was to turn Application into a thin orchestrator that owns controllers
through a map of unique_ptr<IEntityController>. Each controller (ClientController,
PolicyController, InteractionController) implements the same interface and owns its
own service reference, dispatch table, and command implementations. Application
selects the active controller based on user input and delegates. A new entity
requires one new controller class and one map entry in Application's constructor.
Application itself does not change when entities are added.

The menu became a structure with two levels: the outer loop selects the active controller
(clients, policies, interactions), the inner loop dispatches commands (add, list,
search, view, edit, delete, back). Save and exit remain commands at the application
level, handled directly by Application.

### Single filepath to directory based persistence (ADR-018)

The original design implied one filepath per entity. With three entities each
needing its own CSV file, requiring three separate paths from the user was
unnecessary friction. The decision was to prompt for a directory once and derive
all three filenames from it: clients.csv, policies.csv, interactions.csv. Adding a
fourth entity means one additional filename derivation with no change to the
startup flow.

The startup behavior also changed: if a file does not exist when loading, the
repository starts empty rather than throwing. A CRM with clients but no policies is
a valid state, not an error. Throws are reserved for malformed data and I/O
failures.

### Adding threading: repository mutexes, AutoSaveService, safe_localtime (ADR-024)

The auto save feature required a background thread. Two threading problems that did
not exist in the original design (which ran on a single thread) had to be solved.

First, findAll in the client and policy repositories previously returned const
references to internal vectors. With a background thread that could reallocate
those vectors during insert or remove, any caller holding such a reference would
have undefined behavior. The fix was to return by value: findAll copies the vector
while the mutex is held and gives the caller a snapshot independent of subsequent
modifications.

Second, std::localtime relies on a shared static buffer that is a data race when
two threads call it concurrently. Rather than a mutex, which would force the
threads to serialize on every timestamp, the fix was an inline wrapper
(safe_localtime in utils.hpp) that calls localtime_r on POSIX and localtime_s on
Windows. Both write into a stack buffer passed by the caller, with no shared state.

AutoSaveService encapsulates the background thread, a condition variable, a stop
flag, and the interval. It receives a std::function<void()> save callable from
Application and knows nothing about controllers, repositories, or entity types. The
save callable is Application::cmdSave, which saves all controllers. Each repository
owns its own mutex and manages its own thread safety internally; AutoSaveService
never touches a repository mutex directly.

UUID generation also had a latent thread safety problem: generateUuid uses a shared
static mt19937 engine. A static mutex was added around the engine, mirroring the
approach used for other shared mutable state.

### Interaction hierarchy: abstract base with clone (ADR-003, ADR-025)

Appointments and contracts share a common set of base fields (uuid, client_uuid,
type, date, optional notes, created_at, updated_at) but carry entirely different
type specific fields. An appointment has time, duration, and an optional meeting report. A contract has a monetary value, a product name, a signed date, an optional
expiry date, and a status enum.

The flat struct option (one class with all fields from both types, std::optional for
fields that do not apply) does not scale: every code path that reads type specific
fields must check a discriminator first, and the compiler cannot prevent an
a setter that belongs to Appointment from being called on a Contract object. Adding a third
interaction type means adding more optional fields to the same class and more
branches in every handler.

The decision was an abstract base class Interaction with concrete derived classes
Appointment and Contract. Each derived class contains exactly its own fields.
The repository stores objects as std::vector<std::unique_ptr<Interaction>>. Virtual
dispatch handles type specific behavior without branching on the discriminator.

Polymorphic storage introduces a retrieval problem: returning through a base type by
value causes object slicing (an Appointment copied as an Interaction loses its
type specific fields). The solution is a virtual clone() method declared pure in
Interaction and implemented in each derived class by delegating to its copy
constructor. findByUuid and findAll return owned clones, not raw pointers into the
internal vector. This is safe under threading because the clone is made while the
mutex is held, and after the method returns the caller's copy has no connection to
the repository's internal data.

---

## 4. CURRENT ARCHITECTURE

### Domain layer (insura::domain)

The domain layer is the shared language of the system. Every other layer depends on
it; it depends on nothing above it. It contains the three entity classes (Client,
Policy, Interaction and its derived classes Appointment and Contract), the three
repository interfaces (IClientRepository, IPolicyRepository,
IInteractionRepository), data transfer objects (ClientData, PolicyData,
AppointmentData, ContractData, PolicyFilter), and pure utility namespaces
(insura::domain::strops for string operations, insura::utils for UUID generation,
validation, timestamps, and date arithmetic).

The domain layer has no I/O. Validation logic lives here in entity constructors and
setters via exceptions rather than in the service layer as the original design
specified. This was a deliberate revision: a Client with an invalid email cannot be
constructed, which eliminates an entire class of bugs where invalid data enters the
system through a service layer that might be bypassed.

The repository interfaces live in the domain layer rather than the data layer so
that both the service layer (which uses them) and the data layer (which implements
them) can include them without creating an upward dependency. The service layer
depends only on the interfaces, never on the concrete CSV implementations.

### Data layer (insura::data)

The data layer is the only part of the system that knows CSV exists. It contains
CsvClientRepository, CsvPolicyRepository, and CsvInteractionRepository, each
implementing the corresponding repository interface from the domain layer. It also
contains FileHandler, a RAII wrapper that opens a file in its constructor and closes
it in its destructor.

Each repository owns a std::vector of its entity type, a filepath, a dirty flag,
and a std::mutex. Every method that touches the vector locks the mutex using
std::lock_guard. The dirty flag is marked mutable so that save() (declared const)
can reset it after writing. Serialization converts an entity to a string of comma separated
fields; deserialization splits a CSV line and constructs an entity from the fields.
Optional fields are serialized as empty strings and deserialized back to std::nullopt
when empty, using the stringToOptional utility.

The interaction repository uses std::vector<std::unique_ptr<Interaction>> rather than
a vector of values because Interaction is abstract. Serialization dispatches through
dynamic_cast to separate serializeAppointment and serializeContract methods.
Deserialization reads the type column first and branches to the correct field
sequence.

### Service layer (insura::service)

The service layer owns all business logic. It receives repository interfaces by
reference at construction time and never instantiates a concrete repository.
ClientService, PolicyService, and InteractionService handle the domain rules for
their respective entities. AutoSaveService manages the background save thread.

ClientService enforces email uniqueness across add and edit operations by calling
findByEmail before committing. It also coordinates cascade deletion: when a client
is deleted, ClientService finds all policies for that client and removes them via
the policy repository, then finds all interactions and removes them via the
interaction repository, then removes the client. The cascade runs in service code
because only the service layer holds references to all three repositories
simultaneously.

PolicyService enforces the fixed pricing model from ADR-019. When adding a policy,
the user provides type, duration, and start date. PolicyService calls
calculateAmount to look up the price from a fixed table and calculateEndDate to
compute the end date. The controller never performs these calculations; it collects
user input into a PolicyData DTO and passes it to the service.

AutoSaveService wraps a std::thread, a std::condition_variable, a stop flag, and an
interval. It is constructed with a save callable and an interval in seconds. The
thread waits on the condition variable; when the interval expires it calls the save
callable if any repository is dirty. stop() sets the flag and notifies the condition
variable, waking the thread immediately. The destructor calls stop() as a safety
net. Application binds its cmdSave method as the save callable, so AutoSaveService
saves everything without knowing what "everything" is.

### CLI layer (insura::cli)

The cli layer translates user input into service calls and formats service output
for the terminal. It contains Application, three controllers (ClientController,
PolicyController, InteractionController), view functions organized by entity
(client_view, policy_view, interaction_view), a menu class for display, and shared
resolution helpers in cli_helper.

Application is a thin orchestrator. Its constructor receives all three service
references and all three repository references, constructs the three controllers,
and stores them in an unordered_map<string, unique_ptr<IEntityController>>. If auto
save is enabled, it also constructs an AutoSaveService. run() drives the outer
loop: it reads a command, checks if it is a command at the application level (save, exit,
clear), and otherwise looks up the active controller by name and delegates to it.

Each controller implements IEntityController and owns its own service reference,
repository reference, and dispatch table (unordered_map<string, function<void()>>).
execute() is a single map lookup and call. The dispatch table is populated in the
constructor. Adding a command is one map entry.

The shared resolution pipeline lives in insura::cli as free functions: resolveClient,
resolvePolicy, resolveInteraction, selectClient, selectPolicy, selectInteraction,
promptRequired, promptOptional. All controllers call these free functions rather than
duplicating the logic. resolveClient prompts for a search term, calls the service,
and if multiple results come back delegates to selectClient to let the user pick from
a numbered list. resolvePolicy runs resolveClient first, fetches that client's
policies, then delegates to selectPolicy. These helpers are for view, edit, and
delete flows only; cmdSearch shows the full results list without forcing resolution
to one entry.

View functions are stateless and receive data as parameters. PolicyView receives a
client name map (unordered_map<string, string>) for list and search display, built
in the controller per call from only the UUIDs present in the current result set.

---

## 5. C++ CONVENTIONS AND RATIONALE

### Constructors take std::string by value and move

Entity and repository constructors take std::string parameters by value and move
them into members in the initializer list. The member does not exist yet at
initialization time, so there is no existing buffer to reuse. Taking by value lets
the caller decide whether to copy or move: if the caller passes a temporary, the value is
move constructed at the call site; if the caller passes a named variable they want
to keep, it is copy constructed. In both cases the constructor receives an
owned string and moves it into the member with zero additional copies.

The alternative (const std::string& in constructors) always copies, even when the
caller is passing a temporary that could have been moved.

### Setters take const std::string&

Setters for string members take const std::string& rather than std::string by value.
The member already exists and has a potentially allocated buffer. The assignment
operator can reuse that buffer if its capacity is sufficient, avoiding a heap
allocation when the new value fits in the existing space.

Taking by value and then moving in setters is a pessimization: it always allocates a
new buffer for the parameter (discarding the old member buffer) and then moves that
new allocation into the member, which discards a potentially reusable buffer.

### Getters return by reference for strings and by value for primitives

Getters for std::string members return const std::string&. Returning by value would
copy the string on every call. A const reference gives the caller access for reading only,
with no copy. Getters for std::optional<std::string> members return
const std::optional<std::string>& for the same reason. Getters for primitive types
(double, int, enum) return by value, since copying a primitive is the same cost as a
reference access and avoids dangling reference concerns.

### findAll returns by value

findAll on the client and policy repositories returns std::vector<T> by value.
Returning a reference to the internal vector would allow the caller to hold a pointer
into that vector after the mutex releases. If the background save thread or the main
thread inserts or removes an element that reallocates the vector, any outstanding
reference becomes a dangling reference, which is undefined behavior. Returning by
value copies the vector while the lock is held. The caller receives a consistent
snapshot with no connection to the repository's internal state.

findAll on the interaction repository returns std::vector<std::unique_ptr<Interaction>>
by value for the same thread safety reason, with the additional constraint that
unique_ptr cannot be copied. The vector elements are clones produced by clone(), made
while the lock is held, so the caller owns independent copies.

### Deviations

One documentation inconsistency was found in src/domain/i_policy_repository.hpp at
line 16. The comment reads "insertPolicy and updatePolicy accept const Policy& to
avoid an unnecessary copy." The actual declarations on lines 35 and 37 are
`virtual void insertPolicy(Policy policy) = 0;` and
`virtual void updatePolicy(Policy updated) = 0;`, which take by value. The code
follows the correct convention. The comment is stale and does not reflect the actual
interface.

No other violations of the four conventions were found across all .hpp and .cpp
files in src/.

---

## 6. ENTITY LIFECYCLE WALKTHROUGH

### Adding a policy

The user is in the policy menu and types "add". PolicyController::cmdAdd is called
from the dispatch table.

cmdAdd calls the shared helper resolveClient, which prompts for a search term, calls
ClientService::searchClients, and if multiple clients match, calls selectClient to
display a numbered list and return the selected entry. If the user enters an empty
term or no clients match, resolveClient returns nullopt and cmdAdd returns without
creating anything.

Once a client is resolved, cmdAdd prompts for the policy type from a numbered list
of the four available types (AUTO, LIFE, HOME, HEALTH), then prompts for a duration
from a numbered list (6, 12, 24, 36 months), then prompts for a start date string in
YYYY-MM-DD format and validates it with isValidDate.

cmdAdd then calls PolicyService::calculateAmount with the type and duration to show
the user the price from the fixed table, and calls insura::utils::date::calculateEndDate
to show the computed end date. The user sees both values before confirming.

On confirmation, cmdAdd constructs a PolicyData DTO with the client UUID, type,
start date, and duration, and calls PolicyService::addPolicy. addPolicy calls
calculateAmount and calculateEndDate again internally, constructs a Policy with a
newly generated UUID, and calls IPolicyRepository::insertPolicy. The repository
locks its mutex, appends the policy to its internal vector, sets dirty to true, and
releases the lock.

At some point after that, the auto save thread wakes from its wait, sees that at
least one repository is dirty, and calls the save callable. The callable calls each
controller's save(), which calls the repository's save(). CsvPolicyRepository::save
locks the mutex, opens the file via FileHandler, iterates the policies vector,
serializes each policy to a CSV line, and writes it. After writing, dirty is reset
to false and the lock releases.

### Deleting a client

The user is in the client menu and types "delete". ClientController::cmdDelete is
called.

cmdDelete calls resolveClient to identify the client. Once resolved, it calls
confirmClient to display the client's name and email and ask the user to confirm the
deletion.

On confirmation, cmdDelete calls ClientService::deleteClient with the client's UUID.

deleteClient begins with belt and suspenders (ADR-020): it calls findByUuid and
asserts that the result has a value, then checks the result with a guard and
throws std::invalid_argument if the client was not found. The assert fires in debug
builds to surface any bug immediately; the throw survives in release as a safety net
for callers that bypass the resolution pipeline.

deleteClient then calls findByClientUuid on the policy repository to get all policies
for this client and removes each policy by UUID via removePolicy. It then calls
findByClientUuid on the interaction repository and removes each interaction by UUID
via removeInteraction. Finally it calls removeClient on the client repository. Each
remove operation locks the relevant mutex, erases the element from the internal
vector, sets dirty to true, and releases the lock.

After deleteClient returns, the dirty flag is true on any repository that had records
removed. At the next auto save interval, the save thread writes the updated state to
disk, and the deleted client along with all their policies and interactions no longer
appears in the CSV files.

---

## 7. THREAD MODEL

The application runs two threads. The main thread runs the CLI loop: it reads user
input, calls controller methods, and updates repository state. The auto save thread
runs inside AutoSaveService and wakes periodically to write dirty state to disk.

Three mutexes exist, one per repository (CsvClientRepository::mtx_,
CsvPolicyRepository::mtx_, CsvInteractionRepository::mtx_). Every repository method
that reads or writes the internal vector acquires the mutex using std::lock_guard at
the start of the method and releases it when the guard goes out of scope. The two
threads can interleave at the method boundary but not within a method. A separate
mutex exists inside AutoSaveService for its condition variable wait; this mutex is
internal to AutoSaveService and is not shared with any repository.

The three repository mutexes are independent. The main thread can insert a client
while the auto save thread writes policies; they do not block each other because they
acquire different mutexes. A single global mutex covering all repositories would
prevent this concurrency unnecessarily.

The shutdown sequence is as follows. The user types "exit". Application::cmdExit
checks whether any controller reports isDirty. If dirty, Application prompts the
user to save; if the user agrees, cmdSave is called on the main thread, which calls
save() on all controllers, resetting dirty to false on all repositories. Application
then calls stop() on AutoSaveService. stop() sets the stop flag to true and calls
notify_one on the condition variable, waking the auto save thread immediately
regardless of the remaining interval. The auto save thread wakes, checks the stop
flag, and exits its loop without saving again (the main thread save already handled
it). stop() then calls join() and waits for the thread to finish. Once join returns,
the thread is gone and Application's destructor completes.

If auto save is disabled, no AutoSaveService is constructed and no background thread
exists. The repository mutexes still exist but are only ever acquired by the single
main thread, which is correct and adds negligible overhead.

Full design details and rationale for each threading decision are in ADR-024.

---

## 8. TESTING APPROACH

The test suite has two active levels. Both use Catch2 v3 and real objects with no
mocking. The repository interfaces exist for dependency injection so that the
concrete CSV implementations can be swapped, not for creating test doubles. Mocking
a repository would test the mock's return values, not the actual serialization, file
I/O, or parsing logic. Full rationale is in ADR-021.

Level 1 covers pure functions in the domain layer with no I/O: string operations
(strops), date arithmetic and validation (utils::date), general utilities (UUID
generation, email and phone validation, stringToOptional), status enum converters
for all three entity types, and the policy pricing table across all 16 type and
duration combinations. These tests have no files and no setup beyond constructing
inputs.

Level 2 uses real temporary files and real repository instances. It covers serialize
and deserialize round trips for all three entities, save and reload cycles (insert,
save, construct a new repository, load, compare), and service layer rules (duplicate
email rejection, cascade delete, policy field calculation via addPolicy, interactions
referencing valid clients, client delete cascading to interactions). The temporary
directory is created before each test case and removed after.

Level 3 (performance at realistic data volume) and Level 4 (CLI output golden file
comparison) are defined in ADR-021 but deferred.
