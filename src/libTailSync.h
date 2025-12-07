#pragma once

#ifdef LIBTAILSYNC_USE_FASTLED
#include <FastLED.h>
#endif

// https://github.com/clangd/clangd/issues/1167
static_assert(true, "I am here to end the preamble before the pragma!");

#pragma pack(push, 1)
struct Colour {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
#ifdef LIBTAILSYNC_USE_FASTLED
  operator CRGB() const { return CRGB{red, green, blue}; }
#endif
};

struct PacketHeader {
  uint8_t magic[2]; // Should be ASCII "TS"
  uint8_t version_type;
  uint8_t nonce; // not a real nonce, just used for de-dupe

  uint8_t getversion() const;
  uint8_t gettype() const;
};

struct ColourPacket {
  Colour head[6] = {};
  Colour body[6] = {};
  Colour arms[6] = {};
  Colour legs[6] = {};
  Colour tail[6] = {};
};

struct MetaPacket {
  uint8_t channelName[32] = {};
  uint8_t djName[32] = {};
  uint8_t songName[32] = {};
  uint8_t colourRate = 25; // assume 25 colour packets per second
};
#pragma pack(pop)

struct Channel {
  uint8_t name[32] = {0};
  uint8_t mac[6] = {0};
};

extern uint8_t lastNonce;

typedef void (*handleEndSession)();
typedef void (*handlePulse)();
typedef void (*handleColour)(const ColourPacket &packet);
typedef void (*handleMetaChange)(const MetaPacket &packet);

void setColourCallback(handleColour);
void setMetaChangeCallback(handleMetaChange);
void setPulseCallback(handlePulse);
void setEndSessionCallback(handleEndSession);

Colour AverageColour(Colour, Colour);
Colour AverageColour(Colour, Colour, Colour, Colour);

void ParsePacket(const uint8_t *mac, const uint8_t *data, int len);
