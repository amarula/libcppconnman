#pragma once

#include <glib.h>

#include <condition_variable>
#include <thread>

struct QtThreadBundle {
    std::thread::id main_tid;
    std::thread::id loop_tid;

    QtThreadBundle()
        : main_tid(std::this_thread::get_id()),
          loop_(g_main_loop_new(nullptr, FALSE)),
          qt_simulator_([this]() {
              this->loop_tid = std::this_thread::get_id();
              g_idle_add(&QtThreadBundle::on_loop_started, this);
              g_main_loop_run(this->loop_);
              g_main_loop_unref(this->loop_);
          }) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return running_; });
        }
    }

    ~QtThreadBundle() {
        if (qt_simulator_.joinable()) {
            g_main_loop_quit(loop_);
            qt_simulator_.join();
        }
    }

   private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool running_{false};
    GMainLoop* loop_;
    std::thread qt_simulator_;

    static auto on_loop_started(gpointer user_data) -> gboolean {
        auto* self = static_cast<QtThreadBundle*>(user_data);
        {
            std::lock_guard<std::mutex> const lock(self->mtx_);
            self->running_ = true;
        }
        self->cv_.notify_all();
        return G_SOURCE_REMOVE;
    }
};
