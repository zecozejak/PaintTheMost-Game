//
// Created by Mac2019 on 30/01/2024.
//

#ifndef PAINTTHEMOST_GAME_COMMUNICATION_H
#define PAINTTHEMOST_GAME_COMMUNICATION_H

#include <cstdio>

void sendWithLength(int sock, char *buf, size_t len);
int receive(int sock, char *buf);
#endif //PAINTTHEMOST_GAME_COMMUNICATION_H
