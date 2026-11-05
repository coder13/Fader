// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"
#include <fader/Playback.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <pthread/qos.h>
#endif

namespace fader::qualification {
// Diagnostic host only. Never call from Engine, Playback or a device callback.
// Snapshots are sampled before/after host loops, not per callback or per paint.
struct ThreadPolicySnapshot {
    HostTime before{}, after{};
    bool supported = false, defaultPolicy = false;
    int qosStatus = -1, requestedQos = 0, relativePriority = 0;
    int machStatus = -1, releaseStatus = -1, timebaseStatus = -1;
    unsigned policyWords = 0, period = 0, computation = 0, constraint = 0;
    unsigned timebaseNumerator = 0, timebaseDenominator = 0;
    bool preemptible = false;
};

inline ThreadPolicySnapshot readThreadPolicy() noexcept {
    ThreadPolicySnapshot result;
    result.before = monotonicTime();
#if defined(__APPLE__)
    result.supported = true;
    qos_class_t requested{};
    result.qosStatus = pthread_get_qos_class_np(pthread_self(), &requested, &result.relativePriority);
    result.requestedQos = int(requested);
    const auto thread = mach_thread_self();
    thread_time_constraint_policy_data_t policy{};
    mach_msg_type_number_t count = THREAD_TIME_CONSTRAINT_POLICY_COUNT;
    boolean_t defaultPolicy = false;
    result.machStatus = thread_policy_get(thread, THREAD_TIME_CONSTRAINT_POLICY,
                                          reinterpret_cast<thread_policy_t>(&policy), &count, &defaultPolicy);
    result.releaseStatus = mach_port_deallocate(mach_task_self(), thread);
    result.policyWords = count;
    result.defaultPolicy = defaultPolicy;
    if (result.machStatus == KERN_SUCCESS && count == THREAD_TIME_CONSTRAINT_POLICY_COUNT) {
        result.period = policy.period;
        result.computation = policy.computation;
        result.constraint = policy.constraint;
        result.preemptible = policy.preemptible;
    }
    mach_timebase_info_data_t timebase{};
    result.timebaseStatus = mach_timebase_info(&timebase);
    if (result.timebaseStatus == KERN_SUCCESS) {
        result.timebaseNumerator = timebase.numer;
        result.timebaseDenominator = timebase.denom;
    }
#endif
    result.after = monotonicTime();
    return result;
}

inline juce::var threadPolicyJson(const ThreadPolicySnapshot &sample, HostTime origin) {
    auto *out = new juce::DynamicObject();
    out->setProperty("supported", sample.supported);
    const auto relative = [origin](HostTime time) {
        return time >= origin ? juce::int64(time - origin) : -juce::int64(origin - time);
    };
    out->setProperty("beforeNanosecondsFromStart", relative(sample.before));
    out->setProperty("afterNanosecondsFromStart", relative(sample.after));
    out->setProperty("queryWallNanoseconds", juce::int64(sample.after - sample.before));
    if (!sample.supported)
        return out;
    out->setProperty("pthreadQosStatus", sample.qosStatus);
    out->setProperty("pthreadRequestedQosClass", sample.requestedQos);
    out->setProperty("pthreadRelativePriority", sample.relativePriority);
    out->setProperty("machTimeConstraintStatus", sample.machStatus);
    out->setProperty("machPortReleaseStatus", sample.releaseStatus);
    out->setProperty("machReturnedPolicyWords", int(sample.policyWords));
    out->setProperty("machReturnedDefaultPolicy", sample.defaultPolicy);
    out->setProperty("periodAbsoluteTicks", juce::int64(sample.period));
    out->setProperty("computationAbsoluteTicks", juce::int64(sample.computation));
    out->setProperty("constraintAbsoluteTicks", juce::int64(sample.constraint));
    out->setProperty("preemptibleField", sample.preemptible);
    out->setProperty("machTimebaseStatus", sample.timebaseStatus);
    out->setProperty("machTimebaseNumerator", int(sample.timebaseNumerator));
    out->setProperty("machTimebaseDenominator", int(sample.timebaseDenominator));
    return out;
}
} // namespace fader::qualification
