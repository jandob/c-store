#include "store.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#pragma pack(1)

static size_t STORE_state_sizes[] = {
    sizeof(ConnectionState_t),
    sizeof(uint8_t),
};

typedef struct {
    void* state;
    void (*callback)(void*, void*);
} StateSubscriber_t;


Store_t _store = {
    .connection = {.connected = false},
};
Store_t _last_store;

typedef struct {
    size_t offset;
    size_t size;
} StateDiff_t;

typedef struct {
    size_t in;
    size_t out;
    uint8_t storage[1000];
} FIFO_t;

#pragma pack()

static StateSubscriber_t subscribers[10];

static FIFO_t _state_diff_fifo = {0};

static bool _get_state_offset(void* state_ptr, size_t* offset, size_t* size) {
    *offset = 0;
    for (size_t i = 0; i < sizeof(STORE_state_sizes) / sizeof(STORE_state_sizes[0]); i++) {
       if (((uint8_t*)&_store) + *offset == state_ptr) {
          *size = STORE_state_sizes[i];
          return true;
       }
       *offset += STORE_state_sizes[i];
    }
    return false;
}

StateDiff_t* _peek_diff(void) {
    if (_state_diff_fifo.in == _state_diff_fifo.out) {
        return NULL;
    }
    StateDiff_t* diff = (StateDiff_t*)(_state_diff_fifo.storage + _state_diff_fifo.out);
    return diff;
}


StateDiff_t* _pop_diff(void) {
    if (_state_diff_fifo.in == _state_diff_fifo.out) {
        return NULL;
    }
    StateDiff_t* diff = (StateDiff_t*)(_state_diff_fifo.storage + _state_diff_fifo.out);
    _state_diff_fifo.out += sizeof(StateDiff_t) + diff->size;
    return diff;
}

void _apply_diff(StateDiff_t* diff) {
    uint8_t* data = (uint8_t*) diff + sizeof(StateDiff_t);
    memcpy(((uint8_t*) &_store) + diff->offset, data, diff->size);
}


StateDiff_t* _push_diff(void* state_ptr) {
    // find state in store
    size_t offset = 0;
    size_t size = 0;
    bool found = _get_state_offset(state_ptr, &offset, &size);
    if (!found) {
        //error no such state
        return NULL;
    }

    // add to diff fifo
    uint8_t* buffer = _state_diff_fifo.storage + _state_diff_fifo.in;

    StateDiff_t* diff = (StateDiff_t*) buffer;
    diff->offset = offset;
    diff->size = size;
    memcpy(((uint8_t*) diff) + sizeof(StateDiff_t), state_ptr, size);
    
    _state_diff_fifo.in += sizeof(StateDiff_t) + size;

    return diff;
}

void STORE_subcribe(void* state, void (*callback)(void*, void*)) {
    for (size_t i = 0; i < sizeof(subscribers)/sizeof(subscribers[0]); i++) {
        if (subscribers[i].state == NULL) {
            subscribers[i].state = state;
            subscribers[i].callback = callback;
            return;
        }
    }
}

bool STORE_run() {
    StateDiff_t* diff = _peek_diff();
    if (diff == NULL) {
        // no diffs pending;
        return false;
    }
    // call subscriber
    void* state = ((uint8_t*)&_store) + diff->offset;
    for (size_t i = 0; i < sizeof(subscribers) / sizeof(subscribers[0]); i++) {
       if (subscribers[i].state == state) {
           void* new_state = ((uint8_t*) diff) + sizeof(StateDiff_t);
           subscribers[i].callback(new_state, state);
       }
    }
    // apply diff
    _apply_diff(diff);
    if (_pop_diff() == NULL) {
        //error
    };
    return true;
}

Store_t* STORE_get(void) {
    return &_store;
}
void* STORE_mutate(void* state_ptr) {
    StateDiff_t* diff = _push_diff(state_ptr);
    if (diff == NULL) {
        return NULL;
    }
    return (uint8_t*) diff + sizeof(StateDiff_t);
}