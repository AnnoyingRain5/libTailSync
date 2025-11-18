#include <Arduino.h>
#include <cstdint>
#include <libTailSync.h>

// user callbacks
handleColour handleColour_ = nullptr;
handlePulse handlePulse_ = nullptr;
handleEndSession handleEndSession_ = nullptr;

Channel currentChannel;

static uint8_t zero_mac[6] = {0, 0, 0, 0, 0, 0};

uint8_t PacketHeader::getversion() const {
  return (this->version_type & 0xf0) >> 4;
}

uint8_t PacketHeader::gettype() const { return this->version_type & 0x0f; }

// returns true if packet is valid. Also sets currentChannel for the first
// packet
bool checkPacket(PacketHeader header, const uint8_t *mac, int len) {
  // 54 = T, 53 = S
  if (header.magic[0] != 0x54 || header.magic[1] != 0x53) {
    return false; // skip anything not matching the magic
  }
  if (header.getversion() != 0) {
    Serial.printf("[ERROR]: Recieved seemingly valid packet with unsupported "
                  "version \"%d\"",
                  header.getversion());
    return false;
  }
  // if this is on a different "channel", and is not a meta packet
  if ((memcmp(mac, currentChannel.mac, 6) != 0 && header.gettype() != 2)) {
    // if there is no channel selected
    if (memcmp(currentChannel.mac, zero_mac, 6) == 0) {
      memcpy(currentChannel.mac, mac, sizeof(currentChannel.mac));
      return true;
    }
    return false;
  }
  if (header.gettype() == 1) {
    if (sizeof(PacketHeader) + sizeof(ColourPacket) != len) {
      return false;
    }
  }
  return true;
}

void setColourCallback(handleColour cb) { handleColour_ = cb; }

void setPulseCallback(handlePulse cb) { handlePulse_ = cb; }
void setEndSessionCallback(handleEndSession cb) { handleEndSession_ = cb; }

Colour AverageColour(Colour c1, Colour c2) {
  Colour out;
  out.red = (c1.red + c2.red) / 2;
  out.green = (c1.green + c2.green) / 2;
  out.blue = (c1.blue + c2.blue) / 2;
  return out;
}

Colour AverageColour(Colour c1, Colour c2, Colour c3, Colour c4) {
  Colour out;
  out.red = (c1.red + c2.red + c3.red + c4.red) / 4;
  out.green = (c1.green + c2.green + c3.green + c4.green) / 4;
  out.blue = (c1.blue + c2.blue + c3.blue + c4.blue) / 4;
  return out;
}

// ensures packet is valid, then calls the appropriate callback
void ParsePacket(const uint8_t *mac, const uint8_t *data, int len) {
  Serial.println("got packet!");
  PacketHeader header;
  // ensure packet is at least the length of the header
  if (len < sizeof(PacketHeader)) {
    return;
  }
  memcpy(&header, data, sizeof(header));

  // exit if packet is not intended for us
  if (!checkPacket(header, mac, len)) {
    return;
  }

  switch (header.gettype()) {
  // pulse
  case 0x0: {
    if (handlePulse_ != nullptr) {
      handlePulse_();
    }
    break;
  }
  // colour
  case 0x1: {
    ColourPacket colourPacket;
    const uint8_t *payload_start = data + sizeof(PacketHeader);
    memcpy(&colourPacket, payload_start, sizeof(ColourPacket));
    if (handleColour_ != nullptr) {
      handleColour_(colourPacket);
    }
    break;
  }
  // endSession
  case 0xf: {
    if (handleEndSession_ != nullptr) {
      handleEndSession_();
    }
    break;
  }
  default: {
    Serial.printf("[ERROR]: Unknown PacketType %d", header.gettype());
    break;
  }
  }
}
