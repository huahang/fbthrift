/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thrift/lib/cpp/async/TEventBaseManager.h"
#include "thrift/lib/cpp/async/TEventBase.h"

using namespace apache::thrift::async;

// Interface for D code.  Note that we use C-style functions instead of C++
// members, to avoid mismatched virtual tables.
extern "C" {

TEventBase* teventbasemanager_getEventBase(TEventBaseManager* manager) {
  return manager->getEventBase();
}

}// extern "C"
