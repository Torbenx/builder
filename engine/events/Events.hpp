#pragma once

class Scheduler;

class Task {
public:
    void yield();
    virtual void run() = 0;
    
    void* stack_pointer = nullptr;
    void** stack = nullptr;
    friend Scheduler;
};

class Scheduler {
public:
    void execute(Task* task);
};
