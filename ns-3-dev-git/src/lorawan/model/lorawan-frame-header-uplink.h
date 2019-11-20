/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 IDLab-imec
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Floris Van den Abeele <floris.vandenabeele@ugent.be>
 */
#ifndef LORAWAN_FRAME_HEADER_UPLINK_H
#define LORAWAN_FRAME_HEADER_UPLINK_H

#include <ns3/header.h>
#include "ns3/ipv4-address.h"

//common to both
#define LORAWAN_FHDR_ADR_MASK 0x80
#define LORAWAN_FHDR_ACK_MASK 0x20
#define LORAWAN_FHDR_FOPTSLEN_MASK 0x0F

//only uplink
#define LORAWAN_FHDR_ADRACKREQ_MASK 0x40
#define LORAWAN_FHDR_CLASSB_MASK 0x10

#define LORAWAN_FHDR_FOPTSLEN_MAX_SIZE 15

//TODO: capitalise all these
//LoRaWAN MAC Commands (ED sends, NS receives)
#define ResetInd            0x01
#define LinkCheckReq        0x02
#define LinkADRAns          0x03
#define DutyCycleAns        0x04
#define RXParamSetupAns     0x05 
#define DevStatusAns        0x06
#define NewChannelAns       0x07
#define RXTimingSetupAns    0x08
#define TxParamSetupAns     0x09
#define DlChannelAns        0x0A
#define RekeyInd            0x0B
#define ADRParamSetupAns    0x0C
#define DeviceTimeReq       0x0D
#define RejoinParamSetupAns 0x0F


namespace ns3 {

typedef struct
{
  uint8_t m_commandID;
  bool m_isBeingUsed;
  uint8_t m_size;
} LoRaWANMacCommandUplink;


/**
 * \ingroup lorawan
 * Represent the Frame Header (FHDR) in LoRaWAN
 */
class LoRaWANFrameHeaderUplink : public Header
{
public:
  LoRaWANFrameHeaderUplink (void);

  LoRaWANFrameHeaderUplink (Ipv4Address devAddr, bool adr, bool adrAckReq, bool ack, bool framePending, uint8_t FOptsLen, uint16_t frameCounter, uint16_t framePort);
  ~LoRaWANFrameHeaderUplink (void);

  Ipv4Address getDevAddr(void) const;
  void setDevAddr(Ipv4Address);

  bool getAck() const;
  void setAck(bool);

  bool getAdr() const;
  void setAdr(bool);

  bool getAdrAckReq() const;
  void setAdrAckReq(bool);

  bool getClassB() const; //note: not currently used.
  void setClassB(bool); 

  uint16_t getFrameCounter() const;
  void setFrameCounter(uint16_t);

  uint8_t getFramePort() const;
  void setFramePort(uint8_t);

  bool getSerializeFramePort() const;
  void setSerializeFramePort(bool);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  bool IsAck() const;
  bool IsFramePending() const;

  bool AddLoRaADRAns (bool powerAck, bool drAck, bool channelMaskAck);

  std::vector<LoRaWANMacCommandUplink> m_macCommandsED = { //MAC commands sent by ED
  {0x00,                false, 0}, //empty because no ED command with CID of 0x00. Not to be used.
  {ResetInd,            false, 2}, // OTA devices MUST NOT implement this command
  {LinkCheckReq,        false, 1},
  {LinkADRAns,          false, 2}, //size includes command id
  {DutyCycleAns,        false, 1},
  {RXParamSetupAns,     false, 2},
  {DevStatusAns,        false, 3},
  {NewChannelAns,       false, 2},
  {RXTimingSetupAns,    false, 1}, // NOTE: always keep this special high power channel as the last element in the m_supportedChannels vector
  {TxParamSetupAns,     false, 1},
  {DlChannelAns,        false, 2},
  {RekeyInd,            false, 2},
  {ADRParamSetupAns,    false, 1},
  {DeviceTimeReq,       false, 1},
  {0x0E,                false, 0}, //empty because no ED command with CID of 0x0E. Not to be used.
  {RejoinParamSetupAns, false, 2},
};

  uint8_t m_status; //used in LinkADRAns

private:
  Ipv4Address m_devAddr; //!< Short device address of end-device
  uint8_t m_frameControl;
  uint16_t m_frameCounter;
  uint8_t m_framePort; // Not actually part of the frame header, but we include it here for ease of use TODO: isn't it?
  bool m_serializeFramePort;
  
}; //LoRaWANFrameHeader

}; // namespace ns-3

#endif /* LORAWAN_FRAME_HEADER_UPLINK_H */
