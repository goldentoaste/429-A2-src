#include "cpu/pred/gshare.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"

GShareBP::GShareBP(const GShareBPParams *params)
    : BPredUnit(params),
      gshareBitCount(params->historyBitCount),
      hashOffset(0),
      satCounters(1 << params->historyBitCount, SatCounter(params->satBits)),
      globalHistory(params->numThreads, 0)
{
    // compute bit bit mask to extract history
    historyBitMask = (1 << gshareBitCount) - 1;

    // init pc bit mask, has gshareBitCount number of 1s, then shift by hashOffset
    pcBitMask = ((1 << gshareBitCount) - 1) << hashOffset;

    threshold = (1ULL << (params->satBits - 1)) - 1;
}

inline void GShareBP::updateGlobalHistTaken(ThreadID tid)
{
    globalHistory[tid] = (globalHistory[tid] << 1) | 1;
    globalHistory[tid] = globalHistory[tid] & historyBitMask;
}

inline void GShareBP::updateGlobalHistNotTaken(ThreadID tid)
{
    globalHistory[tid] = (globalHistory[tid] << 1);
    globalHistory[tid] = globalHistory[tid] & historyBitMask;
}

void GShareBP::btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history)
{
    // set the last prediction to false in case of bad btb
    globalHistory[tid] = globalHistory[tid] & (historyBitMask & ~1ULL);
}

bool GShareBP::lookup(ThreadID tid, Addr branch_addr, void *&bp_history)
{
    // offsetted pc bits xor history bits
    unsigned satCounterIndex = ((branch_addr & pcBitMask) >> hashOffset) ^ globalHistory[tid];
    bool taken = satCounters[satCounterIndex] > threshold; // 00 = strong no take, 01 = weak no take, 10 - 2 or higher we take

    GShareHistory *history = new GShareHistory;
    history->historyBackup = globalHistory[tid];
    history->predTaken = taken;
    bp_history = static_cast<void *>(history);

    if (taken)
    {
        updateGlobalHistTaken(tid);
    }
    else
    {
        updateGlobalHistNotTaken(tid);
    }
    return taken;
}

void GShareBP::uncondBranch(ThreadID tid, Addr branch_addr, void *&bp_history)
{
    GShareHistory *history = new GShareHistory;
    history->historyBackup = globalHistory[tid];
    history->predTaken = true;
    bp_history = static_cast<void *>(history);
    updateGlobalHistTaken(tid);
}

void GShareBP::update(ThreadID tid, Addr branch_addr, bool taken,
                      void *bp_history, bool squashed,
                      const StaticInstPtr &inst, Addr corrTarget)
{
    GShareHistory *history = static_cast<GShareHistory *>(bp_history);
    unsigned historySnapshot = history->historyBackup & historyBitMask;
    unsigned satCounterIndex =
        ((branch_addr & pcBitMask) >> hashOffset) ^ historySnapshot;

    if (taken)
    {
        satCounters[satCounterIndex]++;
    }
    else
    {
        satCounters[satCounterIndex]--;
    }

    if (squashed)
    {
        globalHistory[tid] = (history->historyBackup << 1) | taken;
        globalHistory[tid] = globalHistory[tid] & historyBitMask;
    }
    else
    {
        delete history;
    }
}

void GShareBP::squash(ThreadID tid, void *bp_history)
{
    GShareHistory *history = static_cast<GShareHistory *>(bp_history);
    globalHistory[tid] = history->historyBackup;
    delete history;
}

GShareBP *
GShareBPParams::create()
{
    return new GShareBP(this);
}
