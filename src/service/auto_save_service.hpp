#pragma once

#include <atomic>
#include <condition_variable>
#include <thread>
namespace insura::service {

class AutoSaveService {
 public:
  AutoSaveService(std::function<void()> save, int interval);
  ~AutoSaveService();
  void stop();

 private:
  void start();
  std::function<void()> save_;
  int interval_;

  std::atomic<bool> stop_flag_{false};
  std::mutex mtx_;
  std::condition_variable condv_;
  std::thread autosave_;
};

}  // namespace insura::service
