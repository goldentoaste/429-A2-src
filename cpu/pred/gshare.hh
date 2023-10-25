#ifndef __CPU_PRED_GSHARE_PRED_HH__
#define __CPU_PRED_GSHARE_PRED_HH__

#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "base/sat_counter.hh"
#include "params/GShareBP.hh"

/**
 * Implements a gshare predictor, in which global history is XORed against
 * bits from the PC. It uses the branch history to index into a table of
 * counters. Global history is speculatively updated, and corrected during
 * a squash.
 */
class GShareBP : public BPredUnit
{
public:
  /**
   * Default branch predictor constructor.
   */
  GShareBP(const GShareBPParams *params);
  /**
   * Looks up the given address in the branch predictor and returns
   * a true/false value as to whether it is taken.  Also creates a
   * BPHistory object to store any state it will need on squash/update.
   * @param branch_addr The address of the branch to look up.
   * @param bp_history Pointer that will be set to the BPHistory object.
   * @return Whether or not the branch is taken.
   */
  bool lookup(ThreadID tid, Addr branch_addr, void *&bp_history);

  /**
   * Records that there was an unconditional branch, and modifies
   * the bp history to point to an object that has the previous
   * global history stored in it.
   * @param bp_history Pointer that will be set to the BPHistory object.
   */
  void uncondBranch(ThreadID tid, Addr branch_addr, void *&bp_history);

  /**
   * Updates the branch predictor to Not Taken if a BTB entry is
   * invalid or not found.
   * @param branch_addr The address of the branch to look up.
   * @param bp_history Pointer to any bp history state.
   * @return Whether or not the branch is taken.
   */

  void btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history);

  /**
   * Updates the branch predictor with the actual result of a branch.
   * @param branch_addr The address of the branch to update.
   * @param taken Whether or not the branch was taken.
   * @param bp_history Pointer to the BPHistory object that was created
   * when the branch was predicted.
   * @param squashed is set when this function is called during a squash
   * operation.
   */
  void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
              bool squashed, const StaticInstPtr &inst, Addr corrTarget);

  /**
   * Restores the global branch history on a squash.
   * @param bp_history Pointer to the BPHistory object that has the
   * previous global branch history in it.
   */
  void squash(ThreadID tid, void *bp_history);

private:
  /** Updates global history as taken. */
  inline void updateGlobalHistTaken(ThreadID tid);

  /** Updates global history as not taken. */
  inline void updateGlobalHistNotTaken(ThreadID tid);
  struct GShareHistory
  {
    unsigned historyBackup;
    bool predTaken;
  };

  // total bits in history
  unsigned gshareBitCount; // number of entries is 2**bits

  // hash offset, to offset the point which gshare is hashed with PC
  unsigned hashOffset;

  // bit mask to retrieve bits from the global history
  unsigned historyBitMask;

  // mask to AND the PC address by
  unsigned pcBitMask;

  // how many bits to use in each satCounter
  // unsigned satBits; // XXX - dont implement for now, just use 2 bits sat counters

  // sat counters which is index by history xor pc
 std::vector<SatCounter> satCounters;

  // unsigned int to store state, is vector because need a separate state for each thread.
  std::vector<unsigned> globalHistory;
};

#endif // __CPU_PRED_GSHARE_PRED_HH__
