/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu May 28 20:30:32 2015 EDT
 * BASED ON THE FILE: cons.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the cons worker in C++
 */

#include "cons_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Cons_ccWorkerTypes;

class Cons_ccWorker : public Cons_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    // Test worker
    return RCC_ADVANCE;
  }
};

CONS_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
CONS_CC_END_INFO
