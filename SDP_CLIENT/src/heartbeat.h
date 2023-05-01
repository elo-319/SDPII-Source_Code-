#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_
#include "inttypes.h"
#include "stdbool.h"

void heartbeat_main(uint8_t *hbrate,uint8_t* contactsts);
bool heartbeat_config();

#endif
