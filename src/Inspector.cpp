/**
 * Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "Inspector.hpp"

namespace ecl {

void
Inspector::printActionTypeDuration(ActionType action, size_t d)
{
  switch (action) {
    case ActionType::init:
      cout << " init: " << d << " ms.\n";
      break;
    case ActionType::useDiscovery:
      cout << " useDiscovery: " << d << " ms.\n";
      break;
    case ActionType::initDiscovery:
      cout << " initDiscovery: " << d << " ms.\n";
      break;
    case ActionType::initContext:
      cout << " initContext: " << d << " ms.\n";
      break;
    case ActionType::initQueue:
      cout << " initQueue: " << d << " ms.\n";
      break;
    case ActionType::initBuffers:
      cout << " initBuffers: " << d << " ms.\n";
      break;
    case ActionType::initKernel:
      cout << " initKernel: " << d << " ms.\n";
      break;
    case ActionType::writeBuffersDummy:
      cout << " writeBuffersDummy: " << d << " ms.\n";
      break;
    case ActionType::writeBuffers:
      cout << " writeBuffers: " << d << " ms.\n";
      break;
    case ActionType::deviceStart:
      cout << " deviceStart: " << d << " ms.\n";
      break;
    case ActionType::schedulerStart:
      cout << " schedulerStart: " << d << " ms.\n";
      break;
    case ActionType::deviceReady:
      cout << " deviceReady: " << d << " ms.\n";
      break;
    case ActionType::deviceRun:
      cout << " deviceRun: " << d << " ms.\n";
      break;
    case ActionType::completeWork:
      cout << " completeWork: " << d << " ms.\n";
      break;
    case ActionType::deviceEnd:
      cout << " deviceEnd: " << d << " ms.\n";
      break;
    case ActionType::schedulerEnd:
      cout << " schedulerEnd: " << d << " ms.\n";
      break;
  }
}

} // namespace ecl
