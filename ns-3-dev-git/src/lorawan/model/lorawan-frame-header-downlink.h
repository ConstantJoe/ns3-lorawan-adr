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
#ifndef LORAWAN_FRAME_HEADER_DOWNLINK_H
#define LORAWAN_FRAME_HEADER_DOWNLINK_H

#include <ns3/header.h>
#include "ns3/ipv4-address.h"

//common to both uplink and downlink
#define LORAWAN_FHDR_ADR_MASK 0x80
#define LORAWAN_FHDR_ACK_MASK 0x20
#define LORAWAN_FHDR_FOPTSLEN_MASK 0x0F

//only downlink
//0x40 is RFU for downlink
#define LORAWAN_FHDR_FPENDING_MASK 0x10

#define LORAWAN_FHDR_FOPTSLEN_MAX_SIZE 15

//TODO: capitalise all these
//LoRaWAN MAC Commands (NS sends, ED receives)
#define ResetConf           0x01
#define LinkCheckAns        0x02
#define LinkADRReq          0x03
#define DutyCycleReq        0x04
#define RXParamSetupReq     0x05 
#define DevStatusReq        0x06
#define NewChannelReq       0x07
#define RXTimingSetupReq    0x08
#define TxParamSetupReq     0x09
#define DlChannelReq        0x0A
#define RekeyConf           0x0B
#define ADRParamSetupReq    0x0C
#define DeviceTimeAns       0x0D
#define ForceRejoinReq      0x0E
#define RejoinParamSetupReq 0x0F



namespace ns3 {


  typedef struct
  {
    uint8_t m_commandID;
    bool m_isBeingUsed;
    uint8_t m_size;
  } LoRaWANMacCommandDownlink;


/**
 * \ingroup lorawan
 * Represent the Frame Header (FHDR) in LoRaWAN
 */
class LoRaWANFrameHeaderDownlink : public Header
{
public:
  LoRaWANFrameHeaderDownlink (void);

  LoRaWANFrameHeaderDownlink (Ipv4Address devAddr, bool adr, bool adrAckReq, bool ack, bool framePending, uint8_t FOptsLen, uint16_t frameCounter, uint16_t framePort);
  ~LoRaWANFrameHeaderDownlink (void);

  Ipv4Address getDevAddr(void) const;
  void setDevAddr(Ipv4Address);

  bool getAck() const;
  void setAck(bool);

  bool getAdr() const;
  void setAdr(bool);

  bool getFramePending() const;
  void setFramePending(bool);

  uint16_t getFrameCounter() const;
  void setFrameCounter(uint16_t);

  uint8_t getFrameOptionsLength () const;

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

  bool AddLoRaADRReq (uint8_t dataRateIndex, uint8_t txPower, uint16_t channelMask, uint8_t chMaskCtrl, uint8_t nbTrans);

  std::vector<LoRaWANMacCommandDownlink> m_macCommandsNS = { //MAC commands sent by NS
  {0x00,                false, 0}, //empty because no ED command with CID of 0x0E. Not to be used.
  {ResetConf,           false, 2}, //size includes command id
  {LinkCheckAns,        false, 3},
  {LinkADRReq,          false, 5},
  {DutyCycleReq,        false, 2},
  {RXParamSetupReq,     false, 5},
  {DevStatusReq,        false, 1},
  {NewChannelReq,       false, 6},
  {RXTimingSetupReq,    false, 2}, // NOTE: always keep this special high power channel as the last element in the m_supportedChannels vector
  {TxParamSetupReq,     false, 2},
  {DlChannelReq,        false, 5},
  {RekeyConf,           false, 2},
  {ADRParamSetupReq,    false, 2},
  {DeviceTimeAns,       false, 6},
  {ForceRejoinReq,      false, 3}, //empty because no ED command with CID of 0x0E. Not to be used.
  {RejoinParamSetupReq, false, 2},
};


  uint8_t m_dataRateTXPowerByte; // part of LinkADRReq 
  uint16_t m_channelMaskBytes;   // part of LinkADRReq
  uint8_t m_redundancy;          // part of LinkADRReq

  uint8_t m_dataRateIndex; // part of LinkADRReq; only populated in deserialisation
  uint8_t m_txPowerIndex;  // part of txPowerIndex; only populated in deserialisation
  
private:
  Ipv4Address m_devAddr; //!< Short device address of end-device
  uint8_t m_frameControl;
  uint16_t m_frameCounter;
  uint8_t m_framePort; // Not actually part of the frame header, but we include it here for ease of use
  bool m_serializeFramePort;




  
  
}; //LoRaWANFrameHeader

}; // namespace ns-3

#endif /* LORAWAN_FRAME_HEADER_DOWNLINK_H */
