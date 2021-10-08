/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main file of the project
 *
 *        Created:  03/24/2016 19:40:56
 *
 *         Author:  Gustavo Pantuza
 *   Organization:  Software Community
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>

#include "store.h"

static void on_connection_update(ConnectionState_t* new_connection, ConnectionState_t* old_connection) {
    printf("on_connection_update: %d\n", new_connection->connected);
    if (new_connection->connected == false) {
        ConnectionState_t* old_conn = STORE_mutate(&STORE_get()->connection);
        old_conn->connected = true;
    }
    uint8_t* feedback = STORE_mutate(&STORE_get()->feedback);
    *feedback = 50;
}

static void on_feedback_update(uint8_t* new_feedback, uint8_t* old_feedback) {
    printf("on_feedback_update\n");
}

int main (void) {
    
    STORE_subcribe(&STORE_get()->connection, on_connection_update);
    STORE_subcribe(&STORE_get()->feedback, on_feedback_update);

    ConnectionState_t* old_conn = STORE_mutate(&STORE_get()->connection);
    old_conn->connected = false;

    printf("---> run\n");

    while (STORE_run()) {;}

    return EXIT_SUCCESS;
}

