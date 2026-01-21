#pragma once

#include <cstdint>

namespace middleware::concurrency
{

extern void suspendAllInterrupts();

struct ScopedCoreLock
{
    ScopedCoreLock();
    ~ScopedCoreLock();

    ScopedCoreLock(const ScopedCoreLock&) = delete;
    ScopedCoreLock& operator=(const ScopedCoreLock&) = delete;
};

struct ScopedECULock
{
    ScopedECULock(volatile uint8_t*);
    ~ScopedECULock();

    ScopedECULock(const ScopedECULock&) = delete;
    ScopedECULock& operator=(const ScopedECULock&) = delete;
};

}  // namespace middleware::concurrency

#define MW_SINGLE_CORE_LOCK middleware::concurrency::ScopedCoreLock();
#define MW_ECU_LOCK middleware::concurrency::ScopedECULock();
