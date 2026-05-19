#include "auto_save_service.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace insura::service {

AutoSaveService::~AutoSaveService() {
  if (autosave_.joinable()) stop();
}

AutoSaveService::AutoSaveService(std::function<void()> save, int interval)
    : save_(save), interval_(interval) {
  start();
}

void AutoSaveService::start() {
  autosave_ = std::thread([this]() {
    while (!stop_flag_) {
      std::unique_lock<std::mutex> lock(mtx_);
      condv_.wait_for(lock, std::chrono::seconds(interval_),
                      [this] { return stop_flag_.load(); });

      if (stop_flag_) break;

      /* calling unlock because here the mutex prevents only
       * data races on the stop flag, each save function
       * has its own mutex */
      lock.unlock();

      /* do the actual job */
      save_();
    }
  });
}

void AutoSaveService::stop() {
  std::unique_lock<std::mutex> lock(mtx_);
  stop_flag_ = true;

  lock.unlock();
  condv_.notify_one();
  if (autosave_.joinable()) autosave_.join();
}

}  // namespace insura::service
