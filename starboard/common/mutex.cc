// Copyright 2015 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/common/mutex.h"
#include "starboard/common/log.h"

namespace starboard {

Mutex::Mutex() : mutex_() {
#if SB_API_VERSION < 16
  SbMutexCreate(&mutex_);
#else
  pthread_mutex_init(&mutex_, nullptr);
#endif  // SB_API_VERSION < 16
  debugInit();
}

Mutex::~Mutex() {
#if SB_API_VERSION < 16
  SbMutexDestroy(&mutex_);
#else
  pthread_mutex_destroy(&mutex_);
#endif  // SB_API_VERSION < 16
}

void Mutex::Acquire() const {
  debugPreAcquire();
#if SB_API_VERSION < 16
  SbMutexAcquire(&mutex_);
#else
  pthread_mutex_lock(&mutex_);
#endif  // SB_API_VERSION < 16
  debugSetAcquired();
}

bool Mutex::AcquireTry() const {
#if SB_API_VERSION < 16
  bool ok = SbMutexAcquireTry(&mutex_) == kSbMutexAcquired;
#else
  bool ok = pthread_mutex_trylock(&mutex_) == 0;
#endif  // SB_API_VERSION < 16
  if (ok) {
    debugSetAcquired();
  }
  return ok;
}

void Mutex::Release() const {
  debugSetReleased();
#if SB_API_VERSION < 16
  SbMutexRelease(&mutex_);
#else
  pthread_mutex_unlock(&mutex_);
#endif  // SB_API_VERSION < 16
}

void Mutex::DCheckAcquired() const {
#ifdef _DEBUG
  SB_DCHECK(pthread_equal(current_thread_acquired_, pthread_self()));
#endif  // _DEBUG
}

#ifdef _DEBUG
void Mutex::debugInit() {
  current_thread_acquired_ = 0;
}
void Mutex::debugSetReleased() const {
  pthread_t current_thread = pthread_self();
  SB_DCHECK(pthread_equal(current_thread_acquired_, current_thread));
  current_thread_acquired_ = 0;
}
void Mutex::debugPreAcquire() const {
  // Check that the mutex is not held by the current thread.
  pthread_t current_thread = pthread_self();
  SB_DCHECK(!pthread_equal(current_thread_acquired_, current_thread));
}
void Mutex::debugSetAcquired() const {
  // Check that the thread has already not been held.
  SB_DCHECK(current_thread_acquired_ == 0);
  current_thread_acquired_ = pthread_self();
}
#else
void Mutex::debugInit() {}
void Mutex::debugSetReleased() const {}
void Mutex::debugPreAcquire() const {}
void Mutex::debugSetAcquired() const {}
#endif

#if SB_API_VERSION < 16
SbMutex* Mutex::mutex() const {
#else
pthread_mutex_t* Mutex::mutex() const {
#endif
  return &mutex_;
}

ScopedLock::ScopedLock(const Mutex& mutex) : mutex_(mutex) {
  mutex_.Acquire();
}

ScopedLock::~ScopedLock() {
  mutex_.Release();
}

ScopedTryLock::ScopedTryLock(const Mutex& mutex) : mutex_(mutex) {
  is_locked_ = mutex_.AcquireTry();
}

ScopedTryLock::~ScopedTryLock() {
  if (is_locked_) {
    mutex_.Release();
  }
}

bool ScopedTryLock::is_locked() const {
  return is_locked_;
}

}  // namespace starboard
