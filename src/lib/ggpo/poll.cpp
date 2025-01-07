/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "types.h"
#include "poll.h"
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <algorithm>
#include "platform_linux.h" // Assuming this contains GetCurrentTimeMS

#define MAX_POLLABLE_HANDLES 64

Poll::Poll() : _handle_count(0), _start_time(0) {
    // Create a dummy eventfd to simplify things.
    int efd = eventfd(0, EFD_SEMAPHORE);
    ASSERT(efd != -1);
    _handles[_handle_count++] = efd;
}

Poll::~Poll() {
    for (int i = 0; i < _handle_count; ++i) {
        close(_handles[i]);
    }
}

void Poll::RegisterHandle(IPollSink *sink, int h, void *cookie) {
    ASSERT(_handle_count < MAX_POLLABLE_HANDLES - 1);
    _handles[_handle_count] = h;
    _handle_sinks[_handle_count] = PollSinkCb(sink, cookie);
    _handle_count++;
}

void Poll::RegisterMsgLoop(IPollSink *sink, void *cookie) {
    _msg_sinks.emplace_back(sink, cookie);
}

void Poll::RegisterLoop(IPollSink *sink, void *cookie) {
    _loop_sinks.emplace_back(sink, cookie);
}

void Poll::RegisterPeriodic(IPollSink *sink, int interval, void *cookie) {
    _periodic_sinks.emplace_back(sink, cookie, interval);
}

void Poll::Run() {
    while (Pump(100)) {
        continue;
    }
}

bool Poll::Pump(int timeout) {
    int elapsed = Platform::GetCurrentTimeMS() - _start_time;
    if (_start_time == 0) {
        _start_time = Platform::GetCurrentTimeMS();
    }

    int maxwait = ComputeWaitTime(elapsed);
    if (maxwait != INFINITE) {
        timeout = MIN(timeout, maxwait);
    }

    struct pollfd fds[MAX_POLLABLE_HANDLES];
    for (int i = 0; i < _handle_count; ++i) {
        fds[i].fd = _handles[i];
        fds[i].events = POLLIN;
    }

    int res = poll(fds, _handle_count, timeout);
    bool finished = false;

    if (res > 0) {
        for (int i = 0; i < _handle_count; ++i) {
            if (fds[i].revents & POLLIN) {
                finished = !_handle_sinks[i].sink->OnHandlePoll(_handle_sinks[i].cookie) || finished;
            }
        }
    }

    for (PollSinkCb &cb : _msg_sinks) {
        finished = !cb.sink->OnMsgPoll(cb.cookie) || finished;
    }

    for (PollPeriodicSinkCb &cb : _periodic_sinks) {
        if (cb.interval + cb.last_fired <= elapsed) {
            cb.last_fired = (elapsed / cb.interval) * cb.interval;
            finished = !cb.sink->OnPeriodicPoll(cb.cookie, cb.last_fired) || finished;
        }
    }

    for (PollSinkCb &cb : _loop_sinks) {
        finished = !cb.sink->OnLoopPoll(cb.cookie) || finished;
    }

    return finished;
}

int Poll::ComputeWaitTime(int elapsed) {
    int waitTime = INFINITE;
    for (PollPeriodicSinkCb &cb : _periodic_sinks) {
        int timeout = (cb.interval + cb.last_fired) - elapsed;
        if (waitTime == INFINITE || (timeout < waitTime)) {
            waitTime = MAX(timeout, 0);
        }
    }
    return waitTime;
}
