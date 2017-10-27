#pragma once

#include <stdint.h>
void cardWriteCommand(const uint8_t *command);
void cardPolledTransfer(uint32_t flags, uint32_t *destination, uint32_t length, const uint8_t *command);
void cardStartTransfer(const uint8_t *command, uint32_t *destination, int channel, uint32_t flags);
uint32_t cardWriteAndRead(const uint8_t *command, uint32_t flags);
void cardParamCommand (uint8_t command, uint32_t parameter, uint32_t flags, uint32_t *destination, uint32_t length);
void cardReadHeader(uint8_t *header);
uint32_t cardReadID(uint32_t flags);
void cardReset();
