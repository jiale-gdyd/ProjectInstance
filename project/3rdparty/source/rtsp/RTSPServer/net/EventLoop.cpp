#include "EventLoop.h"

namespace xop {
EventLoop::EventLoop(uint32_t num_threads) : index_(1)
{
    num_threads_ = 1;
    if (num_threads > 0) {
        num_threads_ = num_threads;
    }

    this->Loop();
}

EventLoop::~EventLoop()
{
    this->Quit();
}

std::shared_ptr<TaskScheduler> EventLoop::GetTaskScheduler()
{
    std::lock_guard<std::mutex> locker(mutex_);
    if (task_schedulers_.size() == 1) {
        return task_schedulers_.at(0);
    } else {
        auto task_scheduler = task_schedulers_.at(index_);
        index_++;
        if (index_ >= task_schedulers_.size()) {
            index_ = 1;
        }

        return task_scheduler;
    }

    return nullptr;
}

void EventLoop::Loop()
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (!task_schedulers_.empty()) {
        return;
    }

    for (uint32_t n = 0; n < num_threads_; n++)  {
        std::shared_ptr<TaskScheduler> task_scheduler_ptr(new EpollTaskScheduler(n));

        task_schedulers_.push_back(task_scheduler_ptr);
        std::shared_ptr<std::thread> thread(new std::thread(&TaskScheduler::Start, task_scheduler_ptr.get()));
        thread->native_handle();
        threads_.push_back(thread);
    }
}

void EventLoop::Quit()
{
    std::lock_guard<std::mutex> locker(mutex_);

    for (auto iter : task_schedulers_) {
        iter->Stop();
    }

    for (auto iter : threads_) {
        iter->join();
    }

    task_schedulers_.clear();
    threads_.clear();
}

void EventLoop::UpdateChannel(ChannelPtr channel)
{
    std::lock_guard<std::mutex> locker(mutex_);
    if (task_schedulers_.size() > 0) {
        task_schedulers_[0]->UpdateChannel(channel);
    }
}

void EventLoop::RemoveChannel(ChannelPtr &channel)
{
    std::lock_guard<std::mutex> locker(mutex_);
    if (task_schedulers_.size() > 0) {
        task_schedulers_[0]->RemoveChannel(channel);
    }
}

TimerId EventLoop::AddTimer(TimerEvent timerEvent, uint32_t msec)
{
    std::lock_guard<std::mutex> locker(mutex_);
    if (task_schedulers_.size() > 0) {
        return task_schedulers_[0]->AddTimer(timerEvent, msec);
    }

    return 0;
}

void EventLoop::RemoveTimer(TimerId timerId)
{
    std::lock_guard<std::mutex> locker(mutex_);
    if (task_schedulers_.size() > 0) {
        task_schedulers_[0]->RemoveTimer(timerId);
    }
}

bool EventLoop::AddTriggerEvent(TriggerEvent callback)
{   
    std::lock_guard<std::mutex> locker(mutex_);
    if (task_schedulers_.size() > 0) {
        return task_schedulers_[0]->AddTriggerEvent(callback);
    }

    return false;
}
}
