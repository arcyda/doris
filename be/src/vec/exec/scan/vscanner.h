// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include "common/status.h"
#include "runtime/runtime_state.h"

namespace doris::vectorized {

class Block;
class VScanner {
public:
    Status prepare(RuntimeState* state) { return Status::OK(); }

    Status open(RuntimeState* state) { return Status::OK(); }

    Status get_block(RuntimeState* state, vectorized::Block* block, bool* eos) {
        *eos = true;
        return Status::OK();
    }

    Status close(RuntimeState* state) { return Status::OK(); }

    RuntimeState* runtime_state() { return _runtime_state; }

    int64_t update_wait_worker_timer() { return 0; }

    bool is_open() { return _is_open; }

    int64_t raw_rows_read() { return 0; }
private:
    RuntimeState* _runtime_state;
    bool _is_open = false;
};

} // namespace doris::vectorized
