// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/condition-variable.h"

#include <errno.h>
#include <time.h>

#include "src/base/platform/time.h"

#if V8_OS_STARBOARD
#include "starboard/common/time.h"
#endif

namespace v8 {
namespace base {

#if V8_OS_POSIX

ConditionVariable::ConditionVariable() {
#if (V8_OS_FREEBSD || V8_OS_NETBSD || V8_OS_OPENBSD || \
     (V8_OS_LINUX && V8_LIBC_GLIBC))
  // On Free/Net/OpenBSD and Linux with glibc we can change the time
  // source for pthread_cond_timedwait() to use the monotonic clock.
  pthread_condattr_t attr;
  int result = pthread_condattr_init(&attr);
  DCHECK_EQ(0, result);
  result = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  DCHECK_EQ(0, result);
  result = pthread_cond_init(&native_handle_, &attr);
  DCHECK_EQ(0, result);
  result = pthread_condattr_destroy(&attr);
#else
  int result = pthread_cond_init(&native_handle_, nullptr);
#endif
  DCHECK_EQ(0, result);
  USE(result);
}


ConditionVariable::~ConditionVariable() {
#if defined(V8_OS_MACOSX)
  // This hack is necessary to avoid a fatal pthreads subsystem bug in the
  // Darwin kernel. http://crbug.com/517681.
  {
    Mutex lock;
    MutexGuard l(&lock);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1;
    pthread_cond_timedwait_relative_np(&native_handle_, &lock.native_handle(),
                                       &ts);
  }
#endif
  int result = pthread_cond_destroy(&native_handle_);
  DCHECK_EQ(0, result);
  USE(result);
}


void ConditionVariable::NotifyOne() {
  int result = pthread_cond_signal(&native_handle_);
  DCHECK_EQ(0, result);
  USE(result);
}


void ConditionVariable::NotifyAll() {
  int result = pthread_cond_broadcast(&native_handle_);
  DCHECK_EQ(0, result);
  USE(result);
}


void ConditionVariable::Wait(Mutex* mutex) {
  mutex->AssertHeldAndUnmark();
  int result = pthread_cond_wait(&native_handle_, &mutex->native_handle());
  DCHECK_EQ(0, result);
  USE(result);
  mutex->AssertUnheldAndMark();
}


bool ConditionVariable::WaitFor(Mutex* mutex, const TimeDelta& rel_time) {
  struct timespec ts;
  int result;
  mutex->AssertHeldAndUnmark();
#if V8_OS_MACOSX
  // Mac OS X provides pthread_cond_timedwait_relative_np(), which does
  // not depend on the real time clock, which is what you really WANT here!
  ts = rel_time.ToTimespec();
  DCHECK_GE(ts.tv_sec, 0);
  DCHECK_GE(ts.tv_nsec, 0);
  result = pthread_cond_timedwait_relative_np(
      &native_handle_, &mutex->native_handle(), &ts);
#else
#if (V8_OS_FREEBSD || V8_OS_NETBSD || V8_OS_OPENBSD || \
     (V8_OS_LINUX && V8_LIBC_GLIBC))
  // On Free/Net/OpenBSD and Linux with glibc we can change the time
  // source for pthread_cond_timedwait() to use the monotonic clock.
  result = clock_gettime(CLOCK_MONOTONIC, &ts);
  DCHECK_EQ(0, result);
  Time now = Time::FromTimespec(ts);
#else
  // The timeout argument to pthread_cond_timedwait() is in absolute time.
  Time now = Time::NowFromSystemTime();
#endif
  Time end_time = now + rel_time;
  DCHECK_GE(end_time, now);
  ts = end_time.ToTimespec();
  result = pthread_cond_timedwait(
      &native_handle_, &mutex->native_handle(), &ts);
#endif  // V8_OS_MACOSX
  mutex->AssertUnheldAndMark();
  if (result == ETIMEDOUT) {
    return false;
  }
  DCHECK_EQ(0, result);
  return true;
}

#elif V8_OS_WIN

ConditionVariable::ConditionVariable() {
  InitializeConditionVariable(&native_handle_);
}


ConditionVariable::~ConditionVariable() {}

void ConditionVariable::NotifyOne() { WakeConditionVariable(&native_handle_); }

void ConditionVariable::NotifyAll() {
  WakeAllConditionVariable(&native_handle_);
}


void ConditionVariable::Wait(Mutex* mutex) {
  mutex->AssertHeldAndUnmark();
  SleepConditionVariableSRW(&native_handle_, &mutex->native_handle(), INFINITE,
                            0);
  mutex->AssertUnheldAndMark();
}


bool ConditionVariable::WaitFor(Mutex* mutex, const TimeDelta& rel_time) {
  int64_t msec = rel_time.InMilliseconds();
  mutex->AssertHeldAndUnmark();
  BOOL result = SleepConditionVariableSRW(
      &native_handle_, &mutex->native_handle(), static_cast<DWORD>(msec), 0);
#ifdef DEBUG
  if (!result) {
    // On failure, we only expect the CV to timeout. Any other error value means
    // that we've unexpectedly woken up.
    // Note that WAIT_TIMEOUT != ERROR_TIMEOUT. WAIT_TIMEOUT is used with the
    // WaitFor* family of functions as a direct return value. ERROR_TIMEOUT is
    // used with GetLastError().
    DCHECK_EQ(static_cast<DWORD>(ERROR_TIMEOUT), GetLastError());
  }
#endif
  mutex->AssertUnheldAndMark();
  return result != 0;
}

#elif V8_OS_STARBOARD

ConditionVariable::ConditionVariable() {
#if SB_API_VERSION < 16
  SbConditionVariableCreate(&native_handle_, nullptr);
#else
#if !SB_HAS_QUIRK(NO_CONDATTR_SETCLOCK_SUPPORT)
  pthread_condattr_t attribute;
  pthread_condattr_init(&attribute);
  pthread_condattr_setclock(&attribute, CLOCK_MONOTONIC);

  int result = pthread_cond_init(&native_handle_, &attribute);
  DCHECK(result == 0);

  pthread_condattr_destroy(&attribute);
#else
  int result = pthread_cond_init(&native_handle_, nullptr);
  DCHECK(result == 0);
#endif  // !SB_HAS_QUIRK(NO_CONDATTR_SETCLOCK_SUPPORT)
#endif  // SB_API_VERSION < 16
}

ConditionVariable::~ConditionVariable() {
#if SB_API_VERSION < 16
  SbConditionVariableDestroy(&native_handle_);
#else
  pthread_cond_destroy(&native_handle_);
#endif  // SB_API_VERSION < 16
}

void ConditionVariable::NotifyOne() {
#if SB_API_VERSION < 16
  SbConditionVariableSignal(&native_handle_);
#else
  pthread_cond_signal(&native_handle_);
#endif  // SB_API_VERSION < 16
}

void ConditionVariable::NotifyAll() {
#if SB_API_VERSION < 16
  SbConditionVariableBroadcast(&native_handle_);
#else
  pthread_cond_broadcast(&native_handle_);
#endif  // SB_API_VERSION < 16
}

void ConditionVariable::Wait(Mutex* mutex) {
#if SB_API_VERSION < 16
  SbConditionVariableWait(&native_handle_, &mutex->native_handle());
#else
  pthread_cond_wait(&native_handle_, &mutex->native_handle());
#endif  // SB_API_VERSION < 16
}

bool ConditionVariable::WaitFor(Mutex* mutex, const TimeDelta& rel_time) {
#if SB_API_VERSION < 16
  int64_t microseconds = static_cast<int64_t>(rel_time.InMicroseconds());
  SbConditionVariableResult result = SbConditionVariableWaitTimed(
      &native_handle_, &mutex->native_handle(), microseconds);
  DCHECK(result != kSbConditionVariableFailed);
  return result == kSbConditionVariableSignaled;
#else
#if !SB_HAS_QUIRK(NO_CONDATTR_SETCLOCK_SUPPORT)
  int64_t timeout_time_usec = starboard::CurrentMonotonicTime();
#else
  int64_t timeout_time_usec = starboard::CurrentPosixTime();
#endif  // !SB_HAS_QUIRK(NO_CONDATTR_SETCLOCK_SUPPORT)
  timeout_time_usec += static_cast<int64_t>(rel_time.InMicroseconds());

  struct timespec delay_timestamp;
  delay_timestamp.tv_sec = timeout_time_usec / 1000'000;
  delay_timestamp.tv_nsec = (timeout_time_usec % 1000'000) * 1000;

  int result = pthread_cond_timedwait(&native_handle_, &mutex->native_handle(), &delay_timestamp);
  return result == 0;
#endif  // SB_API_VERSION < 16
}

#endif  // V8_OS_STARBOARD

}  // namespace base
}  // namespace v8
