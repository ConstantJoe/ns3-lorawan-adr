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
#include "lorawan-frame-header-downlink.h"
#include "lorawan-mac.h"
#include <ns3/log.h>
#include <ns3/address-utils.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANFrameHeaderDownlink");


/*
The LoRaWAN frame header is slightly different for uplink and downlink.
                    7   6         5   4        3..0
On downlink it is:  ADR RFU       ACK FPending FOptsLen
The NS writes the downlink version and the end device reads it.
*/


LoRaWANFrameHeaderDownlink::LoRaWANFrameHeaderDownlink () : m_devAddr((uint32_t)0), m_frameControl(0), m_frameCounter(0), m_serializeFramePort(true)
{
}

LoRaWANFrameHeaderDownlink::LoRaWANFrameHeaderDownlink (Ipv4Address devAddr, bool adr, bool adrAckReq, bool ack, bool framePending, uint8_t FOptsLen, uint16_t frameCounter, uint16_t framePort) : m_devAddr(devAddr), m_frameCounter(frameCounter), m_framePort(framePort)
{
  m_frameControl = 0;

  setAdr(adr);
  setAck(ack);
  setFramePending(framePending); //DL only

  //if (framePort > 0) //todo: changed, assuming framePort > 0
  m_serializeFramePort = true; // note that the caller should set m_serializeFramePort to true in case of framePort == 0
}

LoRaWANFrameHeaderDownlink::~LoRaWANFrameHeaderDownlink ()
{
}

Ipv4Address
LoRaWANFrameHeaderDownlink::getDevAddr (void) const
{
  return m_devAddr;
}

void
LoRaWANFrameHeaderDownlink::setDevAddr (Ipv4Address addr)
{
  m_devAddr = addr;
}

bool
LoRaWANFrameHeaderDownlink::getAdr () const
{
  return (m_frameControl & LORAWAN_FHDR_ADR_MASK);
}

void
LoRaWANFrameHeaderDownlink::setAdr (bool adr)
{
  if (adr)
    m_frameControl |= LORAWAN_FHDR_ADR_MASK;
  else
    m_frameControl &= ~LORAWAN_FHDR_ADR_MASK;
}

bool
LoRaWANFrameHeaderDownlink::getAck () const
{
  return (m_frameControl & LORAWAN_FHDR_ACK_MASK);
}

void
LoRaWANFrameHeaderDownlink::setAck (bool ack)
{
  if (ack)
    m_frameControl |= LORAWAN_FHDR_ACK_MASK;
  else
    m_frameControl &= ~LORAWAN_FHDR_ACK_MASK;
}

bool
LoRaWANFrameHeaderDownlink::getFramePending () const
{
  return (m_frameControl & LORAWAN_FHDR_FPENDING_MASK);
}

void
LoRaWANFrameHeaderDownlink::setFramePending (bool framePending)
{
  if (framePending)
    m_frameControl |= LORAWAN_FHDR_FPENDING_MASK;
  else
    m_frameControl &= ~LORAWAN_FHDR_FPENDING_MASK;
}

uint16_t
LoRaWANFrameHeaderDownlink::getFrameCounter () const
{
  return m_frameCounter;
}

void
LoRaWANFrameHeaderDownlink::setFrameCounter (uint16_t frameCounter)
{
  m_frameCounter = frameCounter;
}

uint8_t 
LoRaWANFrameHeaderDownlink::getFrameOptionsLength () const
{
  return (m_frameControl & LORAWAN_FHDR_FOPTSLEN_MASK);
}

uint8_t
LoRaWANFrameHeaderDownlink::getFramePort () const
{
  return m_framePort;
}

void
LoRaWANFrameHeaderDownlink::setFramePort (uint8_t framePort)
{
  setSerializeFramePort(true);
  m_framePort = framePort;
}

bool
LoRaWANFrameHeaderDownlink::getSerializeFramePort () const
{
  return m_serializeFramePort;
}

void
LoRaWANFrameHeaderDownlink::setSerializeFramePort (bool serializeFramePort)
{
  m_serializeFramePort = serializeFramePort;
}

TypeId
LoRaWANFrameHeaderDownlink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANFrameHeaderDownlink")
    .SetParent<Header> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANFrameHeaderDownlink> ();
  return tid;
}

TypeId
LoRaWANFrameHeaderDownlink::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
LoRaWANFrameHeaderDownlink::Print (std::ostream &os) const
{
  os << "Device Address = " << std::hex << m_devAddr  << ", frameControl = " << (uint16_t)m_frameControl << ", Frame Counter = " << std::dec << (uint32_t) m_frameCounter;

  if (m_serializeFramePort)
    os << ", Frame Port = " << (uint32_t)m_framePort;
}

uint32_t
LoRaWANFrameHeaderDownlink::GetSerializedSize (void) const
{
  /*
   * Each frame header will consist of 7 bytes plus any frame options (optional) plus a frame port (optional)
   */

  uint8_t frameOptionsLength = m_frameControl & LORAWAN_FHDR_FOPTSLEN_MASK;
  uint8_t framePortLength = m_serializeFramePort == true ? 1 : 0;

  return 7 + frameOptionsLength + framePortLength;
}


void
LoRaWANFrameHeaderDownlink::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU32 (m_devAddr.Get ());
  i.WriteU8 (m_frameControl);
  i.WriteU16 (m_frameCounter);

  // TODO: frame options ...
  if (m_serializeFramePort) { //if framePort is 0, then the MAC layer messages are read directly from the payload. Else they are read from here (from FOpts)
    i.WriteU8 (m_framePort);
    
   
    //TODO: add support for commands inside main payload 
    if(m_macCommandsNS[LinkADRReq].m_isBeingUsed) { 
      i.WriteU8 (LinkADRReq);
      i.WriteU8 (m_dataRateTXPowerByte);
      i.WriteU16 (m_channelMaskBytes);
      i.WriteU8 (m_redundancy);
    } 

     //TODO: support the rest of the commands
  }
    
  
}

uint32_t
LoRaWANFrameHeaderDownlink::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t nBytes = 0;

  // Device address
  nBytes += 4;
  m_devAddr.Set (i.ReadU32());

  // Frame control field
  nBytes += 1;
  uint8_t frameControl = i.ReadU8();
  m_frameControl = 0;
  if (frameControl & LORAWAN_FHDR_ADR_MASK) {
    setAdr(true);
  }
  if (frameControl & LORAWAN_FHDR_ACK_MASK) {
    setAck(true);
  }
  if (frameControl & LORAWAN_FHDR_FPENDING_MASK) { 
    setFramePending(true);
  }

  uint8_t fOptsLen = frameControl & LORAWAN_FHDR_FOPTSLEN_MASK;

  // Frame counter
  nBytes += 2;
  uint16_t frameCounter = i.ReadU16();
  setFrameCounter(frameCounter);

  // The header does not indicate whether Frame Port is present, instead it
  // should be present if there is any Frame Payload. The caller should set
  // m_serializeFramePort to true if there is a frame port.
  if (m_serializeFramePort) {
    nBytes += 1;
    uint8_t framePort = i.ReadU8();
    m_framePort = framePort;
  }

  while(fOptsLen > 0) {

    uint8_t command_id = i.ReadU8();
    fOptsLen--;
    nBytes += 1;

    //TODO: rest of MAC commands
    if(command_id == LinkADRReq) { // LinkADRReq

      NS_LOG_FUNCTION (this << "in ADRReq ");
      m_macCommandsNS[LinkADRReq].m_isBeingUsed = true;
      m_dataRateTXPowerByte = i.ReadU8();
      m_channelMaskBytes = i.ReadU16();
      m_redundancy = i.ReadU8();
      fOptsLen -= 4;
      nBytes += 4;

      m_dataRateIndex = m_dataRateTXPowerByte & 0xF0;
      m_dataRateIndex = m_dataRateIndex >> 4; //TODO: double-check this works
      m_txPowerIndex = m_dataRateTXPowerByte & 0xF;

      // TODO: use this data inside this class (currently done outside the class)

    }
    else {
      //TODO: err. 
    }
  }
  return nBytes;
}

bool
LoRaWANFrameHeaderDownlink::IsAck () const
{
  return getAck();
}

bool
LoRaWANFrameHeaderDownlink::IsFramePending () const
{
  return getFramePending();
}


//true indicates proper variable setup, false indicates failure
//fill in the requisite vars in the format specified in the doc
//set a bool to true that indicates that this command is being used (maybe use an array of booleans for each command i)
//then in Serialise those bytes can be added to m_frameOptions directly if the bool is true
bool LoRaWANFrameHeaderDownlink::AddLoRaADRReq (uint8_t dataRateIndex, uint8_t txPower, uint16_t channelMask, uint8_t chMaskCtrl, uint8_t nbTrans) {

  m_dataRateTXPowerByte = 0;
  m_channelMaskBytes = 0;
  m_redundancy = 0;

  //first, ensure the operation will succeed
  uint8_t curr_len = m_frameControl & LORAWAN_FHDR_FOPTSLEN_MASK;
  if(curr_len + m_macCommandsNS[LinkADRReq].m_size > LORAWAN_FHDR_FOPTSLEN_MAX_SIZE) {
  	//TODO: print err
  	return false;
  }

  if((dataRateIndex > 15) | (txPower > 15)) {
    //TODO: print err
    return false;
  }

  if(chMaskCtrl > 7) {
    //TODO: print err
    return false;
  } 

  if(nbTrans > 7) {
    //TODO: print err
    return false;
  } 

  m_frameControl += m_macCommandsNS[LinkADRReq].m_size; //add size of MAC command to the FOptsLen, which is [3:0] of FCtrl

  dataRateIndex = dataRateIndex << 4; //greatest 4 bits for dataRateIndex

  m_dataRateTXPowerByte |= dataRateIndex;
  m_dataRateTXPowerByte |= txPower; //rest for TxPower

  // each bit in the channelMask represents a channel, a 1 indicates that channel can be used
  m_channelMaskBytes = channelMask; //TODO: not implemented here, a static set of channels is always used (see lorawan.cc)

  // last byte is the Redundancy
  // bits [6:4] are the chMaskCtrl, it controls the block of 16 channels to which chMask applies
  chMaskCtrl = chMaskCtrl << 4;
  m_redundancy |= chMaskCtrl; //TODO: not implemented here, a static set of channels is always used (see lorawan.cc)

  // bits [3:0] are the NbTrans, it is the number of transmissions for each uplink message
  m_redundancy |= nbTrans; //TODO: implement use of NbTrans

  m_macCommandsNS[LinkADRReq].m_isBeingUsed = true;

  return true;
}

} //namespace ns3
