#pragma once
#include <memory>
#include <vector>

#include "interaction.hpp"

/*
 * IInteractionRepository — abstract interface for interaction data access.
 *
 * Interaction is an abstract base class — it cannot be instantiated or
 * copied. This affects the interface design in two ways:
 *
 * insertInteraction and updateInteraction accept
 * std::unique_ptr<Interaction> by value. The repository takes ownership
 * of the object, so the caller must std::move into the parameter.
 * unique_ptr cannot be copied, making const reference semantically wrong:
 * it implies the callee only observes, not owns.
 *
 * All find and filter methods return
 * std::vector<std::unique_ptr<Interaction>> for the same reason.
 * std::vector<Interaction> is forbidden because Interaction is abstract
 * and cannot be placed in a vector by value.
 *
 * filterByType and filterByDate return an empty vector when no results
 * match — std::optional is not used because an empty result is not a
 * failure, it is a valid answer.
 *
 * findByClientUuid returns all interactions for a given client since
 * a client can have multiple interactions.
 *
 * Virtual destructor is required — without it, deleting a derived
 * repository through this interface pointer causes undefined behavior.
 */

namespace insura::domain {

class IInteractionRepository {
 public:
  virtual void save() const = 0;
  virtual bool isDirty() const = 0;
  virtual void insertInteraction(
      std::unique_ptr<Interaction> interaction) = 0;
  virtual void removeInteraction(std::string_view uuid) = 0;
  virtual void updateInteraction(
      std::unique_ptr<Interaction> updated) = 0;

  /*
   * Linear scan in the current CSV implementation: the repository
   * locks the mutex, walks the full vector, clones each match. Fine
   * at the current scale because interactions are entered one at a
   * time by a human. If the dataset grows past the point where this
   * matters, the right place for an index is the concrete repository,
   * not the interface.
   */
  virtual std::vector<std::unique_ptr<Interaction>> filterByType(
      Interaction::InteractionType type) const = 0;
  virtual std::vector<std::unique_ptr<Interaction>> filterByDate(
      std::string_view start_date, std::string_view end_date) const = 0;
  virtual std::unique_ptr<Interaction> findByUuid(std::string_view uuid) const = 0;
  virtual std::vector<std::unique_ptr<Interaction>> findByClientUuid(
      std::string_view client_uuid) const = 0;
  virtual std::vector<std::unique_ptr<Interaction>> findAll() const = 0;

  virtual ~IInteractionRepository() = default;
};

}  // namespace insura::domain
