/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _POLL_H
#define _POLL_H

#include <vector>
struct pollfd;
#include <cassert>

#define MAX_POLLABLE_HANDLES     64

#ifndef INFINITE
#define INFINITE -1 // Represents infinite timeout
#endif

class IPollSink {
public:
   virtual ~IPollSink() { }
   virtual bool OnHandlePoll(void *) { return true; }
   virtual bool OnMsgPoll(void *) { return true; }
   virtual bool OnPeriodicPoll(void *, int ) { return true; }
   virtual bool OnLoopPoll(void *) { return true; }
};

class Poll {
public:
   Poll(void);
   ~Poll();

   void RegisterHandle(IPollSink *sink, int h, void *cookie = NULL);
   void RegisterMsgLoop(IPollSink *sink, void *cookie = NULL);
   void RegisterPeriodic(IPollSink *sink, int interval, void *cookie = NULL);
   void RegisterLoop(IPollSink *sink, void *cookie = NULL);

   void Run();
   bool Pump(int timeout);

protected:
   int ComputeWaitTime(int elapsed);

   struct PollSinkCb {
      IPollSink   *sink;
      void        *cookie;
      PollSinkCb(IPollSink *s = nullptr, void *c = nullptr) : sink(s), cookie(c) {}
   };

   struct PollPeriodicSinkCb : public PollSinkCb {
      int         interval;
      int         last_fired;
      PollPeriodicSinkCb(IPollSink *s, void *c, int i) :
         PollSinkCb(s, c), interval(i), last_fired(0) {}
   };

   int               _start_time;
   int               _handle_count;
   int               _handles[MAX_POLLABLE_HANDLES];
   PollSinkCb        _handle_sinks[MAX_POLLABLE_HANDLES];

   std::vector<PollSinkCb>          _msg_sinks;
   std::vector<PollSinkCb>          _loop_sinks;
   std::vector<PollPeriodicSinkCb>  _periodic_sinks;
};

#endif
