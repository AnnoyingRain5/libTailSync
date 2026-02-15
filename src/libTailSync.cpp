#include <Arduino.h>
#include <Logging/TailSyncLogging.h>
#include <libTailSync.h>
#ifdef TAILSYNC_ENABLE_WEBCONFIG
#include <WebConfig/TailSyncWebConfig.h>
#endif
#define currentChannel (knownChannels[channelIndex])

// user callbacks
handleColour handleColour_ = nullptr;
handlePulse handlePulse_ = nullptr;
handleEndSession handleEndSession_ = nullptr;
handleMetaChange handleMetaChange_ = nullptr;
handleUserModeTick handleUserModeTick_ = nullptr;

uint8_t channelIndex = 0;
Channel knownChannels[64] = {};
uint8_t lastNonce = 0;
uint16_t buttonHeldCounter = 0;
bool buttonHeld = false;
Mode controllerMode = MODE_TAILSYNC; // TODO change this to MODE_USER

#ifdef LIBTAILSYNC_MODE_BUTTON
uint8_t modeButtonPin = LIBTAILSYNC_MODE_BUTTON;
#else
uint8_t modeButtonPin = 255;
#endif

void initTailSync() { pinMode(modeButtonPin, INPUT_PULLUP); }

static Logger logger = Logger("LibTailSync");
static uint8_t zero_mac[6] = {0, 0, 0, 0, 0, 0};

uint8_t PacketHeader::getversion() const {
  return (this->version_type & 0xf0) >> 4;
}

void changeChannel() {
  channelIndex++;
  // if the index is larger than the array size
  if (channelIndex >= sizeof(knownChannels) / sizeof(Channel)) {
    channelIndex = 0;
  }
  // if we have run out of data
  if (memcmp(knownChannels[channelIndex].mac, zero_mac, 6) == 0) {
    channelIndex = 0;
  }
}

uint8_t PacketHeader::gettype() const { return this->version_type & 0x0f; }

// returns true if packet is valid. Also sets currentChannel for the first
// packet
bool checkPacket(PacketHeader header, const uint8_t *mac, int len) {
  // 54 = T, 53 = S
  if (header.magic[0] != 0x54 || header.magic[1] != 0x53) {
    logger.log(DEBUG, "Packet doesn't match magic!");
    return false; // skip anything not matching the magic
  }

  if (header.getversion() != 0) {
    logger.log(ERROR,
               "Received seemingly valid packet with unsupported version %d",
               header.getversion());
    return false;
  }

  // if this is on a different "channel", and is not a meta packet
  // this will fail if the sender has a null MAC, however that is kinda their
  // fault tbh
  if (memcmp(mac, currentChannel.mac, 6) != 0) {
    // if there is no channel selected
    if (memcmp(currentChannel.mac, zero_mac, 6) == 0) {
      memcpy(currentChannel.mac, mac, 6);
    } else {
      if (millis() - currentChannel.lastHeard >= 30000) {
        memcpy(currentChannel.mac, zero_mac, 6);
      }
      for (Channel &channel : knownChannels) {
        if (memcmp(mac, channel.mac, 6) == 0) {
          channel.lastHeard = millis();
          break;
        }
        if (memcmp(channel.mac, zero_mac, 6) == 0) {
          memcpy(channel.mac, mac, 6);
          channel.lastHeard = millis();
          break;
        }
        // might as well clean up stale entries while we are here
        if (millis() - channel.lastHeard >= 30000) {
          memcpy(channel.mac, zero_mac, 6);
        }
      }
    }
    // if it's not a metapacket
    if (header.gettype() != 2) {
      return false;
    }
  }
  currentChannel.lastHeard = millis();

  if (header.gettype() == 1) {
    if (sizeof(PacketHeader) + sizeof(ColourPacket) != len) {
      logger.log(WARNING, "Colour Packet is the wrong size!");
      return false;
    }
  }
  if (header.nonce == lastNonce) {
    return false; // we have already seen this packet
  }
  lastNonce = header.nonce;

  return true;
}

void setColourCallback(handleColour cb) { handleColour_ = cb; }

void setPulseCallback(handlePulse cb) { handlePulse_ = cb; }
void setEndSessionCallback(handleEndSession cb) { handleEndSession_ = cb; }
void setMetaChangeCallback(handleMetaChange cb) { handleMetaChange_ = cb; }
void setUserModeTickCallback(handleUserModeTick cb) {
  handleUserModeTick_ = cb;
}

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
  if (controllerMode != MODE_TAILSYNC) {
    if (controllerMode == MODE_CONFIG) {
      logger.log(DEBUG, "Got packet, but in config mode, ignoring.");
      return;
    }
    controllerMode = MODE_TAILSYNC;
    return;
  }
  logger.log(DEBUG, "got packet!");
  // ensure packet is at least the length of the header
  if (len < sizeof(PacketHeader)) {
    return;
  }
  PacketHeader header{};
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
  // meta
  case 0x2: {
    MetaPacket metaPacket;
    const uint8_t *payload_start = data + sizeof(PacketHeader);
    memcpy(&metaPacket, payload_start, sizeof(MetaPacket));
    if (handleMetaChange_ != nullptr) {
      handleMetaChange_(metaPacket);
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
    logger.log(ERROR, "Unknown PacketType %d", header.gettype());
    break;
  }
  }
}

void tick() {
  // 255 is the sentinel value, if it's 255, it's not set
  if (modeButtonPin != 255) {
    if (digitalRead(modeButtonPin) == LOW) {
      if (buttonHeldCounter >= 5000) {
        controllerMode = MODE_CONFIG;
        if (handleColour_ != nullptr) {
          // send a fake empty colour packet to blank out the LEDs
          handleColour_(ColourPacket());
        }
      } else {
        buttonHeldCounter++;
        delay(1);
      }
    } else {
      buttonHeldCounter = 0;
    }
  }
  switch (controllerMode) {
  case MODE_TAILSYNC:
    if (digitalRead(modeButtonPin) == LOW) {
      if (!buttonHeld) {
        buttonHeld = true;
        changeChannel();
      }
    } else {
      buttonHeld = false;
    }
    break;
  case MODE_USER:
    if (handleUserModeTick_ != nullptr) {
      handleUserModeTick_();
    }
    break;

  case MODE_CONFIG:
#ifdef TAILSYNC_ENABLE_WEBCONFIG
    WebConfig_tick();
#else
    logger.log(ERROR, "Mode set to MODE_CONFIG, when WebConfig "
                      "was disabled at compile time!?!?!? "
                      "Resetting mode to MODE_TAILSYNC.");
    controllerMode = MODE_TAILSYNC;
#endif
    break;
  }
}