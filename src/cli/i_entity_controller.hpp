#pragma once

#include <string>
namespace insura::cli {

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

}  // namespace insura::cli
