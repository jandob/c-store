#pragma once
#ifndef __STORE_H__
#define __STORE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#pragma pack(1)


typedef struct {
    bool connected;
} ConnectionState_t;

typedef struct {
    ConnectionState_t connection;
    uint8_t feedback;
} Store_t;

#pragma pack()

void STORE_subcribe(void* state, void (*callback)(void*, void*));

bool STORE_run();

Store_t* STORE_get(void);

void* STORE_mutate(void* state);

#endif