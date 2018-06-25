#include <class_loader/multi_library_class_loader.h>
#include <dccomms_packets/VariableLength2BPacket.h>

namespace dccomms_packets {
VariableLength2BPacket::VariableLength2BPacket() {
  FCS_SIZE = 2; // CRC16
  MAX_PAYLOAD_SIZE = UINT16_MAX;
  _overheadSize = PRE_SIZE + 2 + FCS_SIZE;
  _maxPacketSize = _overheadSize + MAX_PAYLOAD_SIZE;
  _AllocBuffer(_maxPacketSize);
  _Init();
}

void VariableLength2BPacket::_Init() {
  _pre = GetBuffer();
  *_pre = 0x55;
  _payloadSize = (uint16_t *)(_pre + 1);
  _SetPayloadSizeField(0);
  _payload = (uint8_t *)(_payloadSize + 1);
  _fcs = _payload + _GetPayloadSize();
}

void VariableLength2BPacket::CopyFromRawBuffer(void *buffer) {
  uint8_t payloadSize = *((uint8_t *)buffer + PRE_SIZE);
  memcpy(GetBuffer(), buffer, payloadSize + _overheadSize);
}

inline uint8_t *VariableLength2BPacket::GetPayloadBuffer() { return _payload; }

inline uint32_t VariableLength2BPacket::GetPayloadSize() {
  return _GetPayloadSize();
}

inline int VariableLength2BPacket::GetPacketSize() {
  return _overheadSize + _GetPayloadSize();
}

void VariableLength2BPacket::Read(Stream *stream) {
  stream->WaitFor(_pre, PRE_SIZE);
  stream->Read(_payloadSize, 2);
  stream->Read(_payload, _GetPayloadSize() + FCS_SIZE);
}
void VariableLength2BPacket::_SetPayloadSizeField(uint32_t payloadSize) {
  if (_bigEndian)
    *_payloadSize = payloadSize;
  else {
    *(uint8_t *)_payloadSize = (uint8_t)(payloadSize >> 8);
    *(((uint8_t *)_payloadSize) + 1) = (uint8_t)(payloadSize & 0xff);
  }
}

uint32_t VariableLength2BPacket::_GetPayloadSize() {
  uint32_t result;
  if (_bigEndian) {
    result = *_payloadSize;
  } else {
    result = ((*_payloadSize) << 8) | ((*_payloadSize) >> 8);
  }
}

void VariableLength2BPacket::PayloadUpdated(uint32_t payloadSize) {
  _SetPayloadSizeField(payloadSize);
  _fcs = _payload + payloadSize;
  UpdateFCS();
}

void VariableLength2BPacket::GetPayload(void *copy, int size) {
  uint32_t psize = _GetPayloadSize();
  auto copySize = psize < size ? psize : size;
  memcpy(copy, _payload, copySize);
}

uint32_t VariableLength2BPacket::SetPayload(uint8_t *data, uint32_t size) {
  auto copySize = MAX_PAYLOAD_SIZE < size ? MAX_PAYLOAD_SIZE : size;
  _SetPayloadSizeField(copySize);
  memcpy(_payload, data, copySize);
  _fcs = _payload + copySize;
  return copySize;
}

void VariableLength2BPacket::UpdateFCS() {
  uint32_t psize = _GetPayloadSize();
  uint16_t crc = Checksum::crc16(_payload, psize);
  *_fcs = (uint8_t)(crc >> 8);
  *(_fcs + 1) = (uint8_t)(crc & 0xff);
}

bool VariableLength2BPacket::_CheckFCS() {
  uint32_t psize = _GetPayloadSize();
  uint16_t crc = Checksum::crc16(_payload, psize + FCS_SIZE);
  return crc == 0;
}
bool VariableLength2BPacket::PacketIsOk() { return _CheckFCS(); }

CLASS_LOADER_REGISTER_CLASS(VariableLength2BPacketBuilder, IPacketBuilder)
}
