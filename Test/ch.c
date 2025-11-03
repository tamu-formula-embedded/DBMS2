#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum
{
    STATE_ESTABLISH_COMMS,
    STATE_CHARGING,
    STATE_BALANCING_ODDS,
    STATE_BALANCING_EVENS,
    STATE_DONE,
    __N_CHARGING_STATES
} charging_state_t;

char* charging_state_str[__N_CHARGING_STATES] = {"NO_COMMS", "CHARGING", "BAL_ODDS", "BAL_EVENS", "DONE"};

typedef struct
{
    bool connected;
    int32_t min_v;
    int32_t max_v;
    int32_t threshold;

    charging_state_t state;
    charging_state_t prev_state;
    time_t state_start_time; // for elapsed time checks
} charging_t;

#define MAX_V_CONSTANT   4200 // example value, adjust as needed
#define BALANCE_TIME_SEC 10   // 10s per balancing cycle

// Forward declarations
void charging_enter_state(charging_t* c, charging_state_t new_state);
void charging_update(charging_t* c);
bool load_config(charging_t* charging, const char* filename);

// helper: seconds since state entered
double seconds_in_state(charging_t* c)
{
    return difftime(time(NULL), c->state_start_time);
}

void charging_enter_state(charging_t* c, charging_state_t new_state)
{
    if (c->state != new_state)
    {
        c->prev_state = c->state;
        c->state = new_state;
        c->state_start_time = time(NULL);

        switch (new_state)
        {
        case STATE_ESTABLISH_COMMS:
            printf("entered ESTABLISH_COMMS\n");
            break;
        case STATE_CHARGING:
            printf("entered CHARGING\n");
            printf(" -> Enable charger\n");
            printf(" -> Start balancing timer\n");
            break;
        case STATE_BALANCING_ODDS:
            printf("entered BALANCING_ODDS\n");
            break;
        case STATE_BALANCING_EVENS:
            printf("entered BALANCING_EVENS\n");
            break;
        case STATE_DONE:
            printf("entered DONE\n");
            break;
        }
    }
}

void charging_update(charging_t* c)
{
    switch (c->state)
    {

    case STATE_ESTABLISH_COMMS:
        printf("update ESTABLISH_COMMS\n");
        // Transition conditions
        if (!c->connected)
        {
            // stay disconnected; remain idle
            // do nothing
        }
        else
        {
            charging_enter_state(c, STATE_CHARGING);
        }
        break;

    case STATE_CHARGING:
        printf("update CHARGING\n");
        // condition: disconnected → back to ESTABLISH_COMMS
        if (!c->connected)
        {
            charging_enter_state(c, STATE_ESTABLISH_COMMS);
        }
        // condition: fully charged
        else if (c->max_v > MAX_V_CONSTANT)
        {
            charging_enter_state(c, STATE_DONE);
        }
        // condition: imbalance detected
        else if ((c->max_v - c->min_v) > c->threshold)
        {
            charging_enter_state(c, STATE_BALANCING_ODDS);
        }
        break;

    case STATE_BALANCING_ODDS:
        printf("update BALANCING_ODDS\n");
        if (!c->connected)
        {
            charging_enter_state(c, STATE_ESTABLISH_COMMS);
        }
        else if (seconds_in_state(c) >= BALANCE_TIME_SEC)
        {
            charging_enter_state(c, STATE_BALANCING_EVENS);
        }
        break;

    case STATE_BALANCING_EVENS:
        printf("update BALANCING_EVENS\n");
        if (!c->connected)
        {
            charging_enter_state(c, STATE_ESTABLISH_COMMS);
        }
        else if (seconds_in_state(c) >= BALANCE_TIME_SEC)
        {
            charging_enter_state(c, STATE_CHARGING);
        }
        break;

    case STATE_DONE:
        printf("update DONE\n");
        if (!c->connected)
        {
            charging_enter_state(c, STATE_ESTABLISH_COMMS);
        }
        break;
    }

    // diagnostic output
    printf("conn=%d min_v=%d max_v=%d thld=%d state=%s\n", c->connected, c->min_v, c->max_v, c->threshold,
           charging_state_str[c->state]);
}

bool load_config(charging_t* charging, const char* filename)
{
    FILE* f = fopen(filename, "r");
    if (!f)
    {
        perror("fopen config file");
        return false;
    }

    char line[128], key[64];
    int32_t value;

    while (fgets(line, sizeof(line), f))
    {
        // ignore empty lines and comments
        if (line[0] == '#' || line[0] == '\n') continue;

        if (sscanf(line, "%63[^=]=%d", key, &value) == 2)
        {
            if (strcmp(key, "connected") == 0) charging->connected = (value != 0);
            else if (strcmp(key, "min_v") == 0) charging->min_v = value;
            else if (strcmp(key, "max_v") == 0) charging->max_v = value;
            else if (strcmp(key, "threshold") == 0) charging->threshold = value;
            else fprintf(stderr, "Unknown config key: %s\n", key);
        }
    }

    fclose(f);
    return true;
}

int main(void)
{
#define CFG_FILE "ch.txt"

    charging_t charging = {.connected = false,
                           .min_v = 0,
                           .max_v = 0,
                           .threshold = 0,
                           .state = STATE_ESTABLISH_COMMS,
                           .prev_state = STATE_ESTABLISH_COMMS,
                           .state_start_time = 0};

    charging.state_start_time = time(NULL);
    load_config(&charging, CFG_FILE);

    while (1)
    {
        load_config(&charging, CFG_FILE);
        charging_update(&charging);
        usleep(500 * 1000); // 0.5s loop
    }

    return 0;
}
