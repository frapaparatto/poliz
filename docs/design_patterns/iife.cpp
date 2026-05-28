/*
 * IIFE — Immediately Invoked Lambda / Complex Initialization
 * Source: jason-turner-best-practices/ch3_complex_initialization.md
 *
 *
 * The problem
 * -----------
 * When a variable's value depends on a condition, the naive approach is to
 * declare it empty and assign inside a branch:
 *
 *   std::string s;
 *   if (x == 0) s = "zero";
 *   else        s = "other";
 *
 * This forces two problems:
 *  1. s cannot be const — it is mutated after declaration.
 *  2. s is default-constructed first (empty string), then assigned — two
 *     operations instead of one.
 *
 *
 * Three solutions (from ch3_complex_initialization.md)
 * -----------------------------------------------------
 *
 * 1. Ternary — for simple two-way choices:
 *
 *      const std::string s = (x == 0) ? "zero" : "other";
 *
 *    Becomes unreadable when nested or extended beyond two branches.
 *
 * 2. Named function — when the logic is reusable or complex enough to
 *    justify a name:
 *
 *      std::string describe(int x) {
 *          switch (x) { case 0: return "zero"; default: return "other"; }
 *      }
 *      const std::string s = describe(x);
 *
 * 3. IIFE (Immediately Invoked Lambda) — when the logic is a one-time
 *    initialization too complex for a ternary, but too local for a named
 *    function. Define a lambda and call it immediately with ():
 *
 *      const std::string s = [&]() {
 *          switch (x) { case 0: return "zero"; default: return "other"; }
 *      }();
 *       ^^
 *       this () invokes the lambda right away; the result initializes s.
 *
 *    s is now const, initialized in a single operation, and the branching
 *    logic stays local — it doesn't pollute the function with a named helper
 *    that will never be called a second time.
 *
 *
 * When a branch cannot produce a valid value
 * ------------------------------------------
 * If a branch cannot produce a valid value (e.g. the user chose Exit),
 * calling std::exit(0) is the simplest option. No return value is needed
 * and the variable after the IIFE is guaranteed valid if that line is
 * reached.
 *
 * Note: std::exit skips stack unwinding — local destructors do not run.
 * This is safe when no resources need flushing at the exit point, but
 * revisit if cleanup logic is added later. If proper unwinding is required,
 * throw a dedicated exception instead and catch it outside the IIFE.
 *
 *
 * Always spell out the return type when it is not obvious
 * -------------------------------------------------------
 * Without an explicit trailing return type the compiler may fail to deduce
 * it when different branches return values of different (but compatible)
 * types, e.g. two make_unique calls with different arguments.
 *
 *   const auto repo = [&]() -> std::unique_ptr<CSVClientRepository> {
 *       ...
 *   }();
 *
 *
 * =========================================================================
 * Codebase examples (refactoring reference)
 * =========================================================================
 *
 * 1. src/main.cpp — APPLIED
 * -------------------------
 * repo was declared uninitialized and then assigned inside if/else branches,
 * preventing it from being const and leaving a path (invalid option) that
 * fell off the end of a non-void lambda (UB). Converted to IIFE with a
 * while(true) re-prompt loop and UserExit thrown for option 3.
 *
 *   // Before
 *   std::unique_ptr<CSVClientRepository> repo;   // mutable, uninitialized
 *   if (option == 1) { repo = ...; }
 *   else if (option == 2) { while (true) { repo = ...; break; } }
 *   else { std::cerr << "Invalid\n"; return nullptr; }  // UB path
 *
 *   // After
 *   const auto repo = [&]() -> std::unique_ptr<CSVClientRepository> {
 *       while (true) {
 *           // prompt, parse, reprompt on bad input
 *           if (option == 1) { ... return make_unique<...>(filepath); }
 *           if (option == 2) { ... }   // inner loop, std::exit(0) on Exit
 *           if (option == 3) std::exit(0);  // skips local destructors
 *       }
 *   }();
 *   // repo is const and always valid if this line is reached.
 *
 *
 * 2. src/data/csv_client_repository.cpp — CANDIDATE
 *    CSVClientRepository::load(), around line 83
 * --------------------------------------------------
 * tmp_clients is a temporary vector built inside a scope then moved into
 * clients_. The IIFE eliminates the temporary and makes the assignment
 * read as "clients_ = the result of loading from disk".
 *
 *   // Before
 *   std::vector<Client> tmp_clients;
 *   if (std::filesystem::exists(filepath_)) {
 *       FileHandler in(...);
 *       std::string line;
 *       while (std::getline(in.getStream(), line))
 *           tmp_clients.push_back(deserialize(line));
 *   } else { throw std::runtime_error("File doesn't exist"); }
 *   clients_ = std::move(tmp_clients);
 *
 *   // After
 *   clients_ = [&]() {
 *       if (!std::filesystem::exists(filepath_))
 *           throw std::runtime_error("Error: File doesn't exist");
 *       std::vector<Client> result;
 *       FileHandler in(filepath_, std::ios::in);
 *       std::string line;
 *       while (std::getline(in.getStream(), line))
 *           result.push_back(deserialize(line));
 *       return result;
 *   }();
 *
 *
 * 3. src/cli/application.cpp — CANDIDATE
 *    Application::promptEditData(), repeated prompt-string blocks
 * ---------------------------------------------------------------
 * Several prompt strings are built from a ternary and stored as non-const
 * locals inside bare {} scopes. The IIFE makes them const and scales
 * better when the condition grows beyond two branches.
 *
 *   // Before
 *   std::string prompt = current.getPhone()
 *       ? "Phone [" + current.getPhone().value() + "]: "
 *       : "Phone (optional): ";
 *
 *   // After
 *   const std::string prompt = [&]() {
 *       if (current.getPhone())
 *           return "Phone [" + current.getPhone().value() + "]: ";
 *       return std::string{"Phone (optional): "};
 *   }();
 *
 *   // Note: for a simple two-branch case like this, the ternary is already
 *   // fine. Prefer the IIFE when the condition has three or more branches.
 *
 */
