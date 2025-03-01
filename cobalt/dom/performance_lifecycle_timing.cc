// Copyright 2021 The Cobalt Authors. All Rights Reserved.
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


#include "cobalt/dom/performance_lifecycle_timing.h"

#include "cobalt/dom/performance.h"

namespace cobalt {
namespace dom {

namespace {

std::string TranslateApplicationStateToString(base::ApplicationState state) {
  switch (state) {
    case base::kApplicationStateBlurred:
      return "ApplicationStateBlurred";
    case base::kApplicationStateConcealed:
      return "ApplicationStateConcealed";
    case base::kApplicationStateFrozen:
      return "ApplicationStateFrozen";
    case base::kApplicationStateStarted:
      return "ApplicationStateStarted";
    case base::kApplicationStateStopped:
      return "ApplicationStateStopped";
  }

  NOTREACHED() << "state = " << state;
  return "INVALID_APPLICATION_STATE";
}

DOMHighResTimeStamp ConvertMonotonicTimestampToDOMHiResTimeStamp(
    const DOMHighResTimeStamp time_origin, int64_t monotonic_time) {
  // Current delta from Windows epoch.
  int64_t time_delta =
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds() -
      (base::TimeTicks::Now() - base::TimeTicks()).InMicroseconds();
  base::Time base_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::TimeDelta::FromMicroseconds(time_delta + monotonic_time));
  return ClampTimeStampMinimumResolution(
      base_time.ToJsTime() - time_origin,
      Performance::kPerformanceTimerMinResolutionInMicroseconds);
}

}  // namespace

PerformanceLifecycleTiming::PerformanceLifecycleTiming(
    const std::string& name, const DOMHighResTimeStamp time_origin)
    : PerformanceEntry(name, 0.0, 0.0), time_origin_(time_origin) {}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_preload() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_preload);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_start() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_start);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_blur() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_blur);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_focus() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_focus);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_conceal() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_conceal);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_reveal() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_reveal);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_freeze() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_freeze);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_unfreeze() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_unfreeze);
}

DOMHighResTimeStamp PerformanceLifecycleTiming::app_deeplink() const {
  return ReportDOMHighResTimeStamp(lifecycle_timing_info_.app_deeplink);
}

std::string PerformanceLifecycleTiming::current_state() const {
  return TranslateApplicationStateToString(
      lifecycle_timing_info_.current_state);
}

std::string PerformanceLifecycleTiming::last_state() const {
  return TranslateApplicationStateToString(lifecycle_timing_info_.last_state);
}

void PerformanceLifecycleTiming::LogInvalidStateTransition(
    base::ApplicationState state) {
  DLOG(INFO) << "Current State: "
             << TranslateApplicationStateToString(GetCurrentState());
  DLOG(INFO) << "Next State: " << TranslateApplicationStateToString(state);
  NOTREACHED() << "Invalid application state transition.";
}

void PerformanceLifecycleTiming::SetApplicationState(
    base::ApplicationState state, int64_t timestamp) {
  switch (state) {
    case base::kApplicationStateBlurred:
      if (GetCurrentState() == base::kApplicationStateStarted ||
          // TODO: Figure out why the current state is not set
          // by SetApplicationStartOrPreloadTimestamp.
          GetCurrentState() == base::kApplicationStateStopped) {
        lifecycle_timing_info_.app_blur = timestamp;
      } else if (GetCurrentState() == base::kApplicationStateConcealed) {
        lifecycle_timing_info_.app_reveal = timestamp;
      } else {
        LogInvalidStateTransition(state);
      }
      break;
    case base::kApplicationStateConcealed:
      if (GetCurrentState() == base::kApplicationStateBlurred ||
          GetCurrentState() == base::kApplicationStateStopped) {
        lifecycle_timing_info_.app_conceal = timestamp;
      } else if (GetCurrentState() == base::kApplicationStateFrozen) {
        lifecycle_timing_info_.app_unfreeze = timestamp;
      } else {
        LogInvalidStateTransition(state);
      }
      break;
    case base::kApplicationStateFrozen:
      lifecycle_timing_info_.app_freeze = timestamp;
      break;
    case base::kApplicationStateStarted:
      if (GetCurrentState() == base::kApplicationStateBlurred) {
        if (lifecycle_timing_info_.app_start == 0) {
          lifecycle_timing_info_.app_start = timestamp;
        }
        lifecycle_timing_info_.app_focus = timestamp;
      }
      break;
    case base::kApplicationStateStopped:
      NOTREACHED() << "Not support the application stopped state.";
      break;
    default:
      NOTREACHED() << "Invalid application state = " << state;
      break;
  }
  SetLifecycleTimingInfoState(state);
}

void PerformanceLifecycleTiming::SetApplicationStartOrPreloadTimestamp(
    bool is_preload, int64_t timestamp) {
  if (is_preload) {
    lifecycle_timing_info_.app_preload = timestamp;
    SetLifecycleTimingInfoState(base::kApplicationStateConcealed);
  } else {
    lifecycle_timing_info_.app_start = timestamp;
    SetLifecycleTimingInfoState(base::kApplicationStateStarted);
  }
}

void PerformanceLifecycleTiming::SetDeepLinkTimestamp(int64_t timestamp) {
  lifecycle_timing_info_.app_deeplink = timestamp;
}

base::ApplicationState PerformanceLifecycleTiming::GetCurrentState() const {
  return lifecycle_timing_info_.current_state;
}

void PerformanceLifecycleTiming::SetLifecycleTimingInfoState(
    base::ApplicationState state) {
  lifecycle_timing_info_.last_state = lifecycle_timing_info_.current_state;
  lifecycle_timing_info_.current_state = state;
}

DOMHighResTimeStamp PerformanceLifecycleTiming::ReportDOMHighResTimeStamp(
    int64_t timestamp) const {
  if (timestamp != 0) {
    return ConvertMonotonicTimestampToDOMHiResTimeStamp(time_origin_,
                                                        timestamp);
  }
  return PerformanceEntry::start_time();
}

}  // namespace dom
}  // namespace cobalt
