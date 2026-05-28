#pragma once
#include <optional>
#include <string>
#include <vector>

#include "client.hpp"

/*
 *
 * TODO: add those details about implementation in a note (decisions log or
 * journal or something like that)
 *
 * IClientRepository — abstract interface for client data access.
 *
 * The repository pattern treats the data source as a collection you
 * can query and modify without knowing how data is stored underneath.
 * The repository owns the data. Callers receive copies or operate
 * through controlled methods — they never hold ownership of internal
 * objects.
 *
 * This interface lives in the domain layer because both the service
 * layer (which uses it) and the data layer (which implements it) need
 * to see it. Placing it here avoids any upward dependency.
 *
 * The concrete implementation (CsvClientRepository) is injected at
 * runtime via dependency injection. The service never knows whether
 * storage is CSV, SQLite, or in-memory.
 *
 * Method design notes:
 *
 * insertClient, updateClient: accept Client by value. The caller passes a
 *   temporary or std::move-casts a local — the parameter is move-constructed,
 *   not copied. Inside the repository, std::move transfers ownership into the
 *   internal vector with zero additional copies.
 *
 * removeClient: accepts only the uuid because that is the internal
 *   primary key. The service resolves user-facing identifiers (email,
 *   name) to uuid before calling this.
 *
 * updateClient: original draft passed only uuid, which is insufficient
 *   — the repository also needs the updated data to persist. Corrected
 *   to accept both uuid and the updated Client object.
 *
 * findByUuid, findByEmail: return std::optional<Client> because the
 *   client may or may not exist. std::optional cannot hold a reference
 *   in C++17 — returning by value gives the caller a copy, which is
 *   correct since the caller should not hold a reference into the
 *   repository's internal state.
 *
 * findAll: original draft used std::optional<std::vector<unique_ptr<Client>>>.
 *   Three corrections applied:
 *   - std::optional removed: an empty vector already represents
 *     "no clients found". Optional implies the operation itself may
 *     fail, not that results may be empty.
 *   - unique_ptr removed: returning unique_ptr transfers ownership out
 *     of the repository, which contradicts the repository being the
 *     source of truth. Returning Client by value gives the caller a
 *     snapshot they can display without affecting repository state.
 *   - weak_ptr considered and rejected: weak_ptr breaks cycles between
 *     shared_ptrs. It does not apply here.
 *
 * virtual destructor: required on any abstract base class. Without it,
 *   deleting a derived object through a base pointer calls only the
 *   base destructor, causing resource leaks and undefined behavior.
 *   = default defines it inline in the header, no .cpp needed.
 *
 * keyword order: virtual always precedes the return type.
 */

namespace insura::domain {

class IClientRepository {
 public:
  /*
   * save and the read methods are const at the interface level, but
   * concrete implementations need to write to a dirty flag and lock
   * an internal mutex. Both members are declared mutable in the CSV
   * implementations: dirty tracking is bookkeeping for the auto-save
   * thread, not data the caller can observe; the mutex is
   * synchronization machinery, not part of the object's logical
   * state.
   */
  virtual void save() const = 0;
  virtual bool isDirty() const = 0;
  virtual void insertClient(Client client) = 0;
  virtual void removeClient(std::string_view uuid) = 0;
  virtual void updateClient(Client updated) = 0;
  virtual std::optional<Client> findByUuid(std::string_view uuid) const = 0;
  virtual std::optional<Client> findByEmail(std::string_view email) const = 0;
  virtual std::vector<Client> findAll() const = 0;
  virtual ~IClientRepository() = default;
};

}  // namespace insura::domain
