#pragma once
#include <cstdint>

#pragma pack(1)
struct Colour {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

#pragma pack(1)
struct PacketHeader {
  uint8_t magic[2]; // Should be ASCII "TS"
  uint8_t version_type;

  uint8_t getversion() const;
  uint8_t gettype() const;
};

#pragma pack(1)
struct ColourPacket {
  Colour colour[8][8]; // 8x8 grid of pixels
};

struct Channel {
  uint8_t name[32] = {0};
  uint8_t mac[6] = {0};
};

typedef void (*handleEndSession)();
typedef void (*handlePulse)();
typedef void (*handleColour)(ColourPacket packet);

void setColourCallback(handleColour);

void setPulseCallback(handlePulse);
void setEndSessionCallback(handleEndSession);

Colour AverageColour(Colour, Colour);
Colour AverageColour(Colour, Colour, Colour, Colour);

void ParsePacket(const uint8_t *mac, const uint8_t *data, int len);
