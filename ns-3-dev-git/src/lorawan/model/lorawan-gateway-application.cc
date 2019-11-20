/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/node-list.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "lorawan.h"
#include "lorawan-net-device.h"
#include "lorawan-gateway-application.h"
#include "lorawan-frame-header-uplink.h"
#include "lorawan-frame-header-downlink.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANGatewayApplication");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANGatewayApplication);


const std::vector<LoRaWANAdrSnrDrRequirement> LoRaWANNetworkServer::m_adrSnrRequirementsSemtech = {
  {0, -20.0},
  {1, -17.5},
  {2, -15.0},
  {3, -12.5},
  {4, -10.0},
  {5, -7.5}
};

const std::vector<LoRaWANAdrSnrDrRequirement> LoRaWANNetworkServer::m_adrSnrRequirementsVDA = {
{0, -25.6243},
{1, -22.7568},
{2, -20.0254},
{3, -17.3749},
{4, -14.8485},
{5, -12.2833}
};

Ptr<LoRaWANNetworkServer> LoRaWANNetworkServer::m_ptr = NULL;

LoRaWANNetworkServer::LoRaWANNetworkServer () : m_endDevices(), m_pktSize(0), m_generateDataDown(false), m_confirmedData(false), m_endDevicesPopulated(false), m_downstreamIATRandomVariable(nullptr), m_nrRW1Sent(0), m_nrRW2Sent(0), m_nrRW1Missed(0), m_nrRW2Missed(0) {}

TypeId
LoRaWANNetworkServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANNetworkServer")
    .SetParent<Application> ()
    .SetGroupName("LoRaWAN")
    .AddConstructor<LoRaWANNetworkServer> ()
    .AddAttribute ("PacketSize", "The size of DS packets sent to end devices",
                   UintegerValue (21),
                   MakeUintegerAccessor (&LoRaWANNetworkServer::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("GenerateDataDown",
                   "Generate DS packets for sending to end devices. Note that DS Acks will be send regardless of this boolean.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LoRaWANNetworkServer::m_generateDataDown),
                   MakeBooleanChecker ())
    .AddAttribute ("ConfirmedDataDown",
                   "Send Downstream data as Confirmed Data DOWN MAC packets."
                   "False means Unconfirmed data down packets are sent.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LoRaWANNetworkServer::GetConfirmedDataDown,
                                        &LoRaWANNetworkServer::SetConfirmedDataDown),
                   MakeBooleanChecker ())
    .AddAttribute ("DownstreamIAT", "A RandomVariableStream used to pick the time between subsequent DS transmissions to an end device.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=10]"),
                   MakePointerAccessor (&LoRaWANNetworkServer::m_downstreamIATRandomVariable),
                   MakePointerChecker <RandomVariableStream>())
    .AddTraceSource ("nrRW1Sent",
                     "The number of times that a DS packet was sent in RW1 by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_nrRW1Sent),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("nrRW2Sent",
                     "The number of times that a DS packet was sent in RW2 by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_nrRW2Sent),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("nrRW1Missed",
                     "The number of times RW1 was missed for all end devics served by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_nrRW1Missed),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("nrRW2Missed",
                     "The number of times RW2 was missed for all end devics served by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_nrRW2Missed),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("DSMsgGenerated",
                     "A DS msg for an end device has been generated by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_dsMsgGeneratedTrace),
                     "ns3::TracedValueCallback::LoRaWANDSMessageTracedCallback")
    .AddTraceSource ("DSMsgTransmitted",
                     "A DS msg has been transmitted by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_dsMsgTransmittedTrace),
                     "ns3::TracedValueCallback::LoRaWANDSMessageTransmittedTracedCallback")
    .AddTraceSource ("DSMsgAckd",
                     "DS msg has been acknowledged by end device.",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_dsMsgAckdTrace),
                     "ns3::TracedValueCallback::LoRaWANDSMessageTracedCallback")
    .AddTraceSource ("DSMsgDropped",
                     "DS msg has been dropped by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_dsMsgDroppedTrace),
                     "ns3::TracedValueCallback::LoRaWANDSMessageTracedCallback")
    .AddTraceSource ("USMsgReceived",
                     "An US msg has been received by this network server",
                     MakeTraceSourceAccessor (&LoRaWANNetworkServer::m_usMsgReceivedTrace),
                     "ns3::TracedValueCallback::LoRaWANDSMessageTracedCallback")
    .AddAttribute ("SnrCutoffValuesSource",
                   "The source for the SNR cutoff values used in the ADR algorithm. True for the Semtech document, false for Van den Abeele original simulated values. True is more accurate to the real ADR algorithm, false is based on the values used to create the original phy layer simulator.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LoRaWANNetworkServer::m_snrCutoffValuesSource),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
LoRaWANNetworkServer::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  Object::DoInitialize ();
}

void
LoRaWANNetworkServer::PopulateEndDevices (void)
{
  // only populate end devices once:
  if (m_endDevicesPopulated)
    return;

  // Populate m_endDevices based on ns3::NodeList
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
  {
    Ptr<Node> nodePtr(*it);
    Address devAddr = nodePtr->GetDevice (0)->GetAddress();
    if (Ipv4Address::IsMatchingType (devAddr)) {
      Ipv4Address ipv4DevAddr = Ipv4Address::ConvertFrom (devAddr);
      if (ipv4DevAddr.IsEqual (Ipv4Address(0xffffffff))) { // gateway?
        continue;
      }

      // Construct LoRaWANEndDeviceInfoNS object
      LoRaWANEndDeviceInfoNS info = InitEndDeviceInfo (ipv4DevAddr);
      uint32_t key = ipv4DevAddr.Get ();
      m_endDevices[key] = info; // store object
    } else {
      NS_LOG_ERROR (this << " Unable to allocate device address");
      continue;
    }
  }
  m_endDevicesPopulated = true;
}

void
LoRaWANNetworkServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  PrintFinalDetails();

  Object::DoDispose ();
}

LoRaWANEndDeviceInfoNS
LoRaWANNetworkServer::InitEndDeviceInfo (Ipv4Address ipv4DevAddr)
{
  uint32_t key = ipv4DevAddr.Get ();

  // Construct LoRaWANEndDeviceInfoNS object
  LoRaWANEndDeviceInfoNS info;
  info.m_deviceAddress = ipv4DevAddr;
  info.m_rx1DROffset = 0; // default
  info.m_setAck = false;

  if (m_generateDataDown) {
    Time t = Seconds (this->m_downstreamIATRandomVariable->GetValue ());
    info.m_downstreamTimer = Simulator::Schedule (t, &LoRaWANNetworkServer::DSTimerExpired, this, key);
    NS_LOG_DEBUG (this << " DS Traffic Timer for node " << ipv4DevAddr << " scheduled at " << t);
  }

  return info;
}

Ptr<LoRaWANNetworkServer>
LoRaWANNetworkServer::getLoRaWANNetworkServerPointer ()
{
  if (!LoRaWANNetworkServer::m_ptr) {
    LoRaWANNetworkServer::m_ptr = CreateObject<LoRaWANNetworkServer> ();
    LoRaWANNetworkServer::m_ptr->Initialize ();
  }

  return LoRaWANNetworkServer::m_ptr;
}

void
LoRaWANNetworkServer::HandleUSPacket (Ptr<LoRaWANGatewayApplication> lastGW, Address from, Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO("In HandleUSPacket!");

  // PacketSocketAddress fromAddress = PacketSocketAddress::ConvertFrom (from);

  // Decode Frame header
  //LoRaWANFrameHeader frmHdr;
  LoRaWANFrameHeaderUplink frmHdr;
  frmHdr.setSerializeFramePort (true); // Assume that frame Header contains Frame Port so set this to true so that RemoveHeader will deserialize the FPort
  packet->RemoveHeader (frmHdr);

  // Find end device meta data:
  Ipv4Address deviceAddr = frmHdr.getDevAddr ();

  //NS_LOG_INFO(this << "Received packet from device addr = " << deviceAddr);
  uint32_t key = deviceAddr.Get ();
  auto it = m_endDevices.find (key);
  if (it == m_endDevices.end ()) { // not found, so create a new struct and insert it (note this should have already happened in DoInitialize()):
    NS_LOG_WARN (this << " end device with address = " << deviceAddr << " not found in m_endDevices, allocating");

    LoRaWANEndDeviceInfoNS info = InitEndDeviceInfo (deviceAddr);
    m_endDevices[key] = info;
    it = m_endDevices.find (key);
  }

  // Always update number of received upstream packets:
  it->second.m_nUSPackets += 1;                      

  // Always update last seen GWs:
  if ((Simulator::Now () - it->second.m_lastSeen) > Seconds(1.0)) { // assume a new upstream transmission, so clear the vector of seenGWs
    it->second.m_lastGWs.clear ();
  }
  it->second.m_lastGWs.push_back (lastGW);

  // if a packet is new, add it to the m_frameSNRHistory. If it's a duplicate, modify the original's SNR and GtwDiversity as needed.

  // Check for duplicate.
  // Depending on the frame counter and received time, we can classify the US Packet as:
  // i) The first time the NS sees the US Packet: i.e. new frame counter up value
  // ii) Retransmission of a previously transmitted US Packet (then the NS has to reply with an Ack): i.e. frame counter up already seen, seen longer than 1 second ago
  // iii) The same transmission received by a second Gateway (in this case we can drop the packet): i.e. frame counter up already seen, seen shorter than 1 second ago
  bool firstRX = it->second.m_nUSPackets == 0;
  bool processMACAck = true;
  NS_LOG_INFO("check if retransmission or duplicate " << frmHdr.getFrameCounter () << " " << it->second.m_fCntUp << " " << firstRX);
  if (frmHdr.getFrameCounter () <= it->second.m_fCntUp && !firstRX) {
    NS_LOG_INFO("either retransmission or duplicate");

    Time t = Simulator::Now () - it->second.m_lastSeen;
    if (t <= Seconds (1.0)) { // assume US packet is really a duplicate received by a second gateway
      // Duplicate, drop packet
      it->second.m_nUSDuplicates += 1;
      NS_LOG_INFO (this << " Duplicate detected: " << frmHdr.getFrameCounter () << " <= " << it->second.m_fCntUp << " &&  t = " << t << " < 1 second => dropping packet");
      // TODO: add trace for dropping duplicate packets?

      //this is a duplicate (arrived through multiple gateways), modify the original's SNR and GtwDiversity
      //loop through vector (newest are at the start, so search should be quick)
      //TODO: add a bool to define if ADR algorithm is in use
      for (auto & row : it->second.m_frameSNRHistory) {
        if (row.frameCounter == frmHdr.getFrameCounter ()) {
          row.gtwDiversity++;

          LoRaWANPhyParamsTag phyParamsTag;
          if (packet->PeekPacketTag (phyParamsTag)) {
            if (row.snrMax < phyParamsTag.GetSinrAvg ()) {
              row.snrMax = phyParamsTag.GetSinrAvg ();
              NS_LOG_INFO("Modifying current row, the sinr was:" << phyParamsTag.GetSinrAvg ());
            }
          } else {
            NS_LOG_INFO("LoRaWANPhyParamsTag not found on packet");
            NS_LOG_WARN (this << " LoRaWANPhyParamsTag not found on packet.");
          }
          break;
        }
      }
      //TODO: what happens in the case of a retransmission?

      return;
    } else { // assume US packet is a retransmission
      NS_LOG_INFO("it's a retransmission " << frmHdr.getFrameCounter () << " " << it->second.m_fCntUp << " " << firstRX);

      it->second.m_nUSRetransmission += 1;
      processMACAck = false; // as we have already receive this US packet is a retransmission, we should not process the Ack flag set in the MAC header (but we should still open a RW or reply with an Ack if necessary)
    }
  } else { // new US frame counter value -> update number of unique packets received and US frame counter
    NS_LOG_INFO("its a new packet");
    it->second.m_nUniqueUSPackets += 1;
    it->second.m_fCntUp = frmHdr.getFrameCounter (); // update US frame counter

    //TODO: add a bool to define if ADR algorithm is in use
    if(it->second.m_nUniqueUSPackets % ADR_FREQUENCY == 0) {
        it->second.m_setAdr = true; //send a dl packet with the result of running the ADR algorithm
    }

        //TODO: add a bool to define if ADR algorithm is in use
    //this is a new packet, add it to the m_frameSNRHistory.
    if(it->second.m_frameSNRHistory.size() == 20) { //keep a max of 20
        it->second.m_frameSNRHistory.pop_back();
    }
     LoRaWANPhyParamsTag phyParamsTag;
    if (packet->PeekPacketTag (phyParamsTag)) {
      LoRaWANAdrSnrRow newRow = {frmHdr.getFrameCounter(), phyParamsTag.GetSinrAvg (), 1}; //new packet, create a new row 
      NS_LOG_INFO("Creating a new row, the sinr was:" << phyParamsTag.GetSinrAvg ());
      it->second.m_frameSNRHistory.insert(it->second.m_frameSNRHistory.begin(), newRow);
    } else {
      NS_LOG_INFO("LoRaWANPhyParamsTag not found on packet");
      NS_LOG_WARN (this << " LoRaWANPhyParamsTag not found on packet.");
    }
  }

  // Update fields in LoRaWANEndDeviceInfoNS:
  it->second.m_lastSeen = Simulator::Now ();

  // Parse PhyRx Packet Tag
  LoRaWANPhyParamsTag phyParamsTag;
  if (packet->RemovePacketTag (phyParamsTag)) {
    it->second.m_lastChannelIndex = phyParamsTag.GetChannelIndex ();
    it->second.m_lastDataRateIndex = phyParamsTag.GetDataRateIndex ();
    it->second.m_lastCodeRate = phyParamsTag.GetCodeRate ();
  } else {
    NS_LOG_WARN (this << " LoRaWANPhyParamsTag not found on packet.");
  }

  // Parse MAC Message Type Packet Tag
  LoRaWANMsgTypeTag msgTypeTag;
  if (packet->RemovePacketTag (msgTypeTag)) {
    LoRaWANMsgType msgType = msgTypeTag.GetMsgType();

    if (msgType == LORAWAN_CONFIRMED_DATA_UP) {
      it->second.m_setAck = true; // Set ack bit in next DS msg
      NS_LOG_DEBUG (this << " Received Confirmed Data UP. Next DS Packet will have Ack bit set"); //TODO: does this actually add a DS packet to the list?
    }
  } else {
    NS_LOG_WARN (this << " LoRaWANMsgTypeTag not found on packet.");
  }

  // Log that NS received an US packet:
  m_usMsgReceivedTrace (key, msgTypeTag.GetMsgType(), packet);

  // Parse Ack flag:
  if (processMACAck && frmHdr.getAck ()) {
    it->second.m_nUSAcks += 1;

    if (!it->second.m_downstreamQueue.empty ()) { // there is a DS message in the queue
      if (it->second.m_downstreamQueue.front()->m_downstreamMsgType == LORAWAN_CONFIRMED_DATA_DOWN) { // End device confirmed reception of DS packet, so we can remove it:
        LoRaWANNSDSQueueElement* ptr = it->second.m_downstreamQueue.front();

        // LOG that network server received an Acknowledgment for a DS packet
        m_dsMsgAckdTrace (key, ptr->m_downstreamTransmissionsRemaining, ptr->m_downstreamMsgType, ptr->m_downstreamPacket);

        this->DeleteFirstDSQueueElement (key);

        NS_LOG_DEBUG (this << " Received Ack for Confirmed DS packet, removing packet from DS queue for end device " << deviceAddr);
      } else {
        NS_LOG_ERROR (this << " Upstream frame has Ack bit set, but downstream frame msg type is not Confirmed (msgType = " << it->second.m_downstreamQueue.front()->m_downstreamMsgType << ")");
      }
    } else {
      // One occurence of this condition is when the NS receives a retransmission that re-acknowledges a previously send DS confirmed packet
      // This condition is fulfilled when the DS Ack for the previously transmitted US frame was sent by the NS but not received by the end device
      // Note that an end device retransmitting a frame will not change the Ack bit between retransmissions (ns-3 implementation limitation, to be fixed)
      NS_LOG_WARN (this << " Upstream frame has Ack bit set, but there is no downstream frame queued.");
    }
  }

  //TODO: parse AdrAckReq flag, Class B flag, ADR flag, m_macCommandsED data structure for included MAC commands.
  if (frmHdr.getAdrAckReq ()) {
    //device is asking for a dl packet to be sent.
    //this is functionally the same as having a confirmed packet, so we can handle it the same way
    //TODO: double-check this
    it->second.m_setAck = true;
  }


  //parse MAC commands
  for(std::vector<LoRaWANMacCommandUplink>::iterator it = frmHdr.m_macCommandsED.begin(); it != frmHdr.m_macCommandsED.end(); ++it) {
    if(it->m_isBeingUsed) {
      //TODO: write functions inside the frame-header to extract the MAC command properly
      // for now, since the ADR command is the only one implemented, we will just check for that one.
      if(it->m_commandID == LinkADRAns) {
          uint8_t status = frmHdr.m_status;
          //TODO: what to do with this? Indicates if tx, dr, and channel mask were set correctly.
          //If not, resend?
          //Just report failures for now
          //TODO: report failures.
      } else {
        //TODO: report unexpected behaviour
      }
    }
  }  

  NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds ()
                       << " the Network Server received " << packet->GetSize () << " bytes from "
                       << (uint32_t) key );

  // We should always schedule a timer, even when m_downstreamPacket is NULL as a new DS packet might be generated between now and RW1
  if (it->second.m_rw1Timer.IsRunning()) {
    NS_LOG_ERROR (this << " Scheduling RW1 timer while RW1 timer was already scheduled for " << it->second.m_rw1Timer.GetTs ());
  }
  Time receiveDelay = MicroSeconds (RECEIVE_DELAY1);
  it->second.m_rw1Timer = Simulator::Schedule (receiveDelay, &LoRaWANNetworkServer::RW1TimerExpired, this, key);
}

bool
LoRaWANNetworkServer::HaveSomethingToSendToEndDevice (uint32_t deviceAddr)
{
  uint32_t key = deviceAddr;
  auto it_ed = m_endDevices.find (key);

  return it_ed->second.m_downstreamQueue.size() > 0 || it_ed->second.m_setAck;
}

void
LoRaWANNetworkServer::RW1TimerExpired (uint32_t deviceAddr)
{
  NS_LOG_FUNCTION (this << deviceAddr);

  uint32_t key = deviceAddr;
  auto it_ed = m_endDevices.find (key);

  // Check whether any GW in lastGWs can send a downstream transmission immediately (i.e. right now) in RW1
  bool foundGW = false;
  // The RW1 LoRa channel and data rate are the same as used in the last US transmission
  const uint8_t dsChannelIndex = it_ed->second.m_lastChannelIndex;
  const uint8_t dsDataRateIndex = it_ed->second.m_lastDataRateIndex;;
  for (auto it_gw = it_ed->second.m_lastGWs.cbegin(); it_gw != it_ed->second.m_lastGWs.cend(); it_gw++) {
    if ((*it_gw)->CanSendImmediatelyOnChannel (dsChannelIndex, dsDataRateIndex)) {
      foundGW = true;
      this->SendDSPacket (deviceAddr, *it_gw, true, false);
      break;
    }
  }

  if (!foundGW) {
    NS_LOG_DEBUG (this << " No gateway available for transmission in RW1, scheduling timer for DS transmission in RW2");

    // Increment m_nrRW1Missed only if there is something to send:
    if (HaveSomethingToSendToEndDevice (deviceAddr)) {
      m_nrRW1Missed++;
    }

    if (it_ed->second.m_rw2Timer.IsRunning()) {
      NS_LOG_ERROR (this << " Scheduling RW2 timer while RW2 timer was already scheduled for " << it_ed->second.m_rw2Timer.GetTs ());
    }

    // Time receiveDelay = MicroSeconds (RECEIVE_DELAY2);
    Time receiveDelay = (it_ed->second.m_lastSeen + MicroSeconds (RECEIVE_DELAY2)) - Simulator::Now ();
    NS_ASSERT (receiveDelay > 0);
    it_ed->second.m_rw2Timer = Simulator::Schedule (receiveDelay, &LoRaWANNetworkServer::RW2TimerExpired, this, key);
  }
}

void
LoRaWANNetworkServer::RW2TimerExpired (uint32_t deviceAddr)
{
  NS_LOG_FUNCTION (this << deviceAddr);

  uint32_t key = deviceAddr;
  auto it_ed = m_endDevices.find (key);

  // Check whether any GW in lastGWs can send a downstream transmission immediately (i.e. right now) in RW2
  // The RW2 LoRa channel is a fixed channel depending on the region, for EU this is the high power 869.525 MHz channel
  const uint8_t dsChannelIndex = LoRaWAN::m_RW2ChannelIndex;
  const uint8_t dsDataRateIndex = LoRaWAN::m_RW2DataRateIndex;
  bool foundGW = false;
  for (auto it_gw = it_ed->second.m_lastGWs.cbegin(); it_gw != it_ed->second.m_lastGWs.cend(); it_gw++) {
    if ((*it_gw)->CanSendImmediatelyOnChannel (dsChannelIndex, dsDataRateIndex)) {
      foundGW = true;
      this->SendDSPacket (deviceAddr, *it_gw, false, true);
      break;
    }
  }

  if (!foundGW) {
    // Increment m_nrRW2Missed only if there is something to send:
    if (HaveSomethingToSendToEndDevice (deviceAddr)) {
      m_nrRW2Missed++;
      NS_LOG_INFO (this << " Unable to send DS transmission to device addr " << deviceAddr << " in RW1 and RW2, no gateway was available.");
    }
  }
}

void
LoRaWANNetworkServer::SendDSPacket (uint32_t deviceAddr, Ptr<LoRaWANGatewayApplication> gatewayPtr, bool RW1, bool RW2)
{

  //TODO: add check for device's m_adr bit. If received, send a packet of correct length.
  
  // If a Frame contains MAC commands, FOptsLen in the FCtrl must be set to the number of bytes used.
  // If FOptsLen != 0, then port 0 cannot be used (FPort must either not be present or not be 0).
  // If present, an FPort value of 0 indicates that the FRMPayload contains MAC commands only.
  // A single data frame can contain any sequence of MAC commands, either piggybacked in the FOpts field, or when sent as a separate data frame, in the FRMPayload
  // (With FPort being set to 0)
  // A MAC command consists of a command identifier (CID) of 1 octet followed by a possibly empty command-specific sequence of octets
  // The LinkADRReq MAC command is 4 bytes long, and contains: DataRate_TxPower (1byte), ChMask (2 bytes), Redundancy (1 byte)
  // LinkAdrReq CID is 0x03. So 5 bytes total. Responded to by LinkAdrAns (CID = 0x04), which is 1 byte long (two bytes total)

  //TODO: add check for device's m_adrAck bit from last received packet. If received and not otherwise sending a packet, send an empty packet. 


  // Search device in m_endDevices:
  auto it = m_endDevices.find (deviceAddr);
  if (it == m_endDevices.end ()) { // end device not found
    NS_LOG_ERROR (this << " Could not find device info struct in m_endDevices for dev addr " << deviceAddr << ". Aborting DS Transmission");
    return;
  }

  // Check if we have a last known GW for the device:
  // bool haveGW = it->second.m_lastGWs.size () > 0;
  // if (!haveGW) {
  //   NS_LOG_ERROR (this << " lastGW is not set for dev addr " << deviceAddr << ". Aborting DS Transmission");
  //   return;
  // }
  // Ptr <LoRaWANGatewayApplication> lastGW = *it->second.m_lastGWs.begin ();

  // Figure out which DS packet to send
  LoRaWANNSDSQueueElement elementToSend;
  bool deleteQueueElement = false;
  if (it->second.m_downstreamQueue.size() > 0) {
    LoRaWANNSDSQueueElement* element = it->second.m_downstreamQueue.front ();

    // Bookkeeping for Confirmed packets:
    if (element->m_downstreamMsgType == LORAWAN_CONFIRMED_DATA_DOWN) {
      // Count number of retransmissions:
      if (element->m_isRetransmission) {
        it->second.m_nDSRetransmission++;
      }

      // Update for next transmission:
      element->m_downstreamTransmissionsRemaining--; // decrement
      element->m_isRetransmission = true;
    }

    // Should we delete pending packet after transmission?
    if (element->m_downstreamMsgType != LORAWAN_CONFIRMED_DATA_DOWN) { // delete the queueelement object after the send operation
      deleteQueueElement = true;
    } else {
      if (element->m_downstreamTransmissionsRemaining == 0) {// in case of CONFIRMED_DATA_DOWN, delete the pending transmission when the number of remaining transmissions has reached 1
        deleteQueueElement = true;

        // LOG that network server will delete DS packet from queue
        m_dsMsgDroppedTrace (deviceAddr, 0, element->m_downstreamMsgType, element->m_downstreamPacket);
      }
    }

    elementToSend.m_downstreamPacket = element->m_downstreamPacket;
    elementToSend.m_downstreamMsgType = element->m_downstreamMsgType;
    elementToSend.m_downstreamFramePort = element->m_downstreamFramePort;
    elementToSend.m_downstreamTransmissionsRemaining = element->m_downstreamTransmissionsRemaining;
  } else {
      /*if(it->second.m_setAdr) { //If there is no data to be sent down, but we need to send ADR data. TODO: make this a more general MAC command bool
        ///////////
        //TODO: double-check this. The DSTimerExpired method implies that the create<Packet>(num) used there INCLUDES the size of the LoRaWAN header (13 bytes)
        //but here the "empty" packets are of size 0, shouldn't they be 13 bytes? Maybe ask in GitHub group.
        //BUT the Frame Header gets added here, and the MAC layer header in the mac, so there must be something wrong? Is the check in DSTimerExpired incorrect?
        //for now I'm going to assume they were incorrect.
        NS_LOG_DEBUG (this << " Generating empty downstream packet to send ADR for dev addr " << deviceAddr);
        elementToSend.m_downstreamPacket = Create<Packet> (0); // TODO: think about this. The message gets added to the payload instead of FOpts when theres no other payload
        // this makes a difference in the encryption. But in our case does it make any difference?
        elementToSend.m_downstreamMsgType = LORAWAN_UNCONFIRMED_DATA_DOWN; // should also set msg type
        elementToSend.m_downstreamFramePort = 0; // empty packet, so don't send frame port
        elementToSend.m_downstreamTransmissionsRemaining = 0;
        ///////////
      } else*/ if (it->second.m_setAck) {
        NS_LOG_DEBUG (this << " Generating empty downstream packet to send Ack for dev addr " << deviceAddr);
        elementToSend.m_downstreamPacket = Create<Packet> (0); // create empty packet so that we can send the Ack
        elementToSend.m_downstreamMsgType = LORAWAN_UNCONFIRMED_DATA_DOWN; // should also set msg type
        elementToSend.m_downstreamFramePort = 0; // empty packet, so don't send frame port
        elementToSend.m_downstreamTransmissionsRemaining = 0;  
      } else { //i.e. !it->second.m_setAck && !it->second.m_setAdr
        // Not really a warning as there is just no need to send a DS packet (i.e. no data and no Ack)
        NS_LOG_INFO (this << " No downstream packet found nor is ack bit set for dev addr " << deviceAddr << ". Aborting DS transmission");
        return;  
      }
  }

  // LOG DS msg transmission
  uint8_t rwNumber = RW1 ? 1 : 2;
  m_dsMsgTransmittedTrace (deviceAddr, elementToSend.m_downstreamTransmissionsRemaining, elementToSend.m_downstreamMsgType, elementToSend.m_downstreamPacket, rwNumber);

  // Make a copy here, this is u
  Ptr<Packet> p;
  if (deleteQueueElement)
    p = elementToSend.m_downstreamPacket;
  else
    p = elementToSend.m_downstreamPacket->Copy (); // make a copy, so that we don't alter elementToSend.m_downstreamPacket as we might re-use this packet later (e.g. retransmission)

  // Construct Frame Header:
  //LoRaWANFrameHeader fhdr;
  LoRaWANFrameHeaderDownlink fhdr;
  fhdr.setDevAddr (Ipv4Address (deviceAddr));
  fhdr.setAck (it->second.m_setAck);
  fhdr.setFramePending (it->second.m_framePending);
  fhdr.setFrameCounter (it->second.m_fCntDown++);
  if (elementToSend.m_downstreamFramePort > 0)
    fhdr.setFramePort (elementToSend.m_downstreamFramePort);

  if(it->second.m_setAdr) { //run ADR?
    /*
      TODO: 
    if(m_downstreamFramePort == 0) {
        //the mac message is sent inside the frame payload
        //the packet itself (of the correct size) has already been created above
        //just add the information that should be in those 5 bytes
        //but how to indicate to the device that this is a MAC layer message? Through if fPort==0?
    } else {
        //add in FOpts to the FrameHeader (5 bytes, generated by a call to AdaptiveDataRate)
        //modify frame header initial bits to indicate size of mac layer messages
    }
    for now just doing use of FOpts
    */
    LoRaWANADRAlgoritmResult adrRes = AdaptiveDataRate(deviceAddr); //generates the dataRate, txPower, channelMask, chMaskCtrl, and nbTrans that the device should use. 
    if(adrRes.status) {

      if(adrRes.dr == it->second.m_lastDataRateIndex && adrRes.txPower == it->second.m_lastTxPowerIndex) 
      {
          NS_LOG_INFO ("No change: ADR algorithm (NS side) for device " << deviceAddr << " ran successfully at time " << Simulator::Now ().GetSeconds () << " but no change was required" ); 
      }
      else 
      {
          NS_LOG_INFO ("ADR algorithm (NS side) for device " << deviceAddr << " ran successfully at time " << Simulator::Now ().GetSeconds () <<
        ", old dr= " << it->second.m_lastDataRateIndex <<   
        ", new dr= " << adrRes.dr << " new txPow= " << adrRes.txPower << " channelMask=" << adrRes.channelMask << " chMaskCtrl=" << adrRes.chMaskCtrl << " nbTrans=" << adrRes.nbTrans);
        fhdr.AddLoRaADRReq(adrRes.dr, adrRes.txPower, adrRes.channelMask, adrRes.chMaskCtrl, adrRes.nbTrans); 
      }       
      it->second.m_setAdr = false;
    } else {
      //TODO: ADR didn't run properly, report err. But packet can still be sent.
      NS_LOG_INFO ("ADR algorithm (NS side) failed for device " << deviceAddr);
    }
    

    
  }

  p->AddHeader (fhdr);

  // Add Phy Packet tag to specify channel, data rate and code rate:
  uint8_t dsChannelIndex;
  uint8_t dsDataRateIndex;
  if (RW1) {
    dsChannelIndex = it->second.m_lastChannelIndex;
    dsDataRateIndex = LoRaWAN::GetRX1DataRateIndex (it->second.m_lastDataRateIndex, it->second.m_rx1DROffset);
  } else if (RW2) {
    dsChannelIndex = LoRaWAN::m_RW2ChannelIndex;
    dsDataRateIndex = LoRaWAN::m_RW2DataRateIndex;
  } else {
    NS_FATAL_ERROR (this << " Either RW1 or RW2 should be true");
    return;
  }

  LoRaWANPhyParamsTag phyParamsTag;
  phyParamsTag.SetChannelIndex (dsChannelIndex);
  phyParamsTag.SetDataRateIndex (dsDataRateIndex);
  phyParamsTag.SetCodeRate (it->second.m_lastCodeRate);
  phyParamsTag.SetSinrAvg (0); //not actually used here
  phyParamsTag.SetTxPowerIndex (0); //use max tx power for dl TODO: enable tx power index use for DL side too.

  p->AddPacketTag (phyParamsTag);

  // Set Msg type
  LoRaWANMsgTypeTag msgTypeTag;
  msgTypeTag.SetMsgType (elementToSend.m_downstreamMsgType);
  p->AddPacketTag (msgTypeTag);

  // Update DS Packet counters:
  it->second.m_nDSPacketsSent += 1;
  if (RW1) {
    m_nrRW1Sent++;
    it->second.m_nDSPacketsSentRW1 += 1;
  } else if (RW2) {
    it->second.m_nDSPacketsSentRW2 += 1;
    m_nrRW2Sent++;
  }
  if (it->second.m_setAck)
    it->second.m_nDSAcks += 1;

  // Store gatewayPtr as last DS GW:
  it->second.m_lastDSGW = gatewayPtr;

  // Ask gateway application on lastseenGW to send the DS packet:
  gatewayPtr->SendDSPacket (p);
  NS_LOG_INFO (this << " Sent DS Packet to device addr " << deviceAddr << " via GW #" << gatewayPtr->GetNode()->GetId() << " in RW" << (RW1 ? "1" : "2"));

  // Reset data structures
  it->second.m_setAck = false; // we only sent an Ack once, see Note on page 75 of LoRaWAN std

  // For some cases (see deleteQueueElement bool), remove the pending DS packet here
  if (deleteQueueElement) {
    this->DeleteFirstDSQueueElement (deviceAddr);
  }
}

void
LoRaWANNetworkServer::DSTimerExpired (uint32_t deviceAddr)
{
  NS_LOG_FUNCTION (this << deviceAddr);

  auto it = m_endDevices.find (deviceAddr);
  if (it == m_endDevices.end ()) { // end device not found
    NS_LOG_ERROR (this << " Could not find device info struct in m_endDevices for dev addr " << deviceAddr);
    return;
  }

  // Generate a Downstream packet
  if (it->second.m_downstreamQueue.size () > 0)
    NS_LOG_INFO(this << " DS queue for end device " << Ipv4Address(deviceAddr) << " is not empty");

  NS_ASSERT (m_pktSize >= 8 + 1 + 4); // should be able to send at least frame header, MAC header and MAC MIC
  bool generatePacket = true;
  if (m_pktSize < 8 + 1 + 4) {
    NS_LOG_ERROR (this << " m_pktSize = " << m_pktSize << " is too small, not generating DS packets");
    generatePacket = false;
  }

  if (generatePacket) {
    uint8_t frmPayloadSize = m_pktSize - (8 + 1 + 4);

    Ptr<Packet> packet;
    if (frmPayloadSize >= sizeof(uint64_t)) { // check whether payload size is large enough to hold 64 bit integer
      // send decrementing counter as payload (note: globally shared counter)
      uint8_t* payload = new uint8_t[frmPayloadSize](); // the parenthesis initialize the allocated memory to zero
      if (payload) {
        const uint64_t counter = LoRaWANCounterSingleton::GetCounter ();
        ((uint64_t*)payload)[0] = counter; // copy counter to beginning of payload
        packet = Create<Packet> (payload, frmPayloadSize);
        delete[] payload;
      } else {
        packet = Create<Packet> (frmPayloadSize);
      }
    } else {
      packet = Create<Packet> (frmPayloadSize);
    }

    LoRaWANNSDSQueueElement* element = new LoRaWANNSDSQueueElement ();
    element->m_downstreamPacket = packet;
    element->m_downstreamFramePort = 1;
    if (m_confirmedData) {
      element->m_downstreamMsgType = LORAWAN_CONFIRMED_DATA_DOWN;
      element->m_downstreamTransmissionsRemaining = DEFAULT_NUMBER_DS_TRANSMISSIONS;
    } else {
      element->m_downstreamMsgType = LORAWAN_UNCONFIRMED_DATA_DOWN;
      element->m_downstreamTransmissionsRemaining = 1;
    }
    element->m_isRetransmission = false;
    it->second.m_downstreamQueue.push_back (element);
    it->second.m_nDSPacketsGenerated += 1;

    m_dsMsgGeneratedTrace (deviceAddr, element->m_downstreamTransmissionsRemaining, element->m_downstreamMsgType, element->m_downstreamPacket);
    NS_LOG_DEBUG (this << " Added downstream packet with size " << m_pktSize  << " to DS queue for end device " << Ipv4Address(deviceAddr) << ". queue size = " << it->second.m_downstreamQueue.size());
  }

  // Reschedule timer:
  Time t = Seconds (this->m_downstreamIATRandomVariable->GetValue ());
  it->second.m_downstreamTimer = Simulator::Schedule (t, &LoRaWANNetworkServer::DSTimerExpired, this, deviceAddr);
  NS_LOG_DEBUG (this << " DS Traffic Timer for end device " << it->second.m_deviceAddress << " scheduled at " << t);
}

void
LoRaWANNetworkServer::DeleteFirstDSQueueElement (uint32_t deviceAddr)
{
  auto it = m_endDevices.find (deviceAddr);
  if (it == m_endDevices.end ()) { // end device not found
    NS_LOG_ERROR (this << " Could not find device info struct in m_endDevices for dev addr " << deviceAddr << ". Unable to delete DS queue element.");
    return;
  }

  LoRaWANNSDSQueueElement* ptr = it->second.m_downstreamQueue.front ();
  delete ptr;
  it->second.m_downstreamQueue.pop_front ();
}

int64_t
LoRaWANNetworkServer::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_downstreamIATRandomVariable->SetStream (stream);
  return 1;
}

void
LoRaWANNetworkServer::SetConfirmedDataDown (bool confirmedData)
{
  NS_LOG_FUNCTION (this << confirmedData);
  m_confirmedData = confirmedData;
}

bool
LoRaWANNetworkServer::GetConfirmedDataDown (void) const
{
  return m_confirmedData;
}



LoRaWANADRAlgoritmResult
LoRaWANNetworkServer::AdaptiveDataRate (uint32_t deviceAddr)
{
 
  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " running ADR algorithm (NS side) for device " << deviceAddr);

  //the algorithm from Semtech
  /*
  Each time a node performs an uplink transmission, the frame may be received by several gateways. Inside each of the gateways, the frame is tagged with its signal 
  strength (RSSI) and Signal to Noise ratio in dB (SNR). The gateways then forward the tagged frame to the network server.

So for a given frame the server will receive several copies, each coming from a different gateway and tagged with different RSSI and SNR values.

For a given frame, the notation {gatewayID , frame_nb, SNR} represents the SNR measured by gateway ID.

Each frame features a 16 bits frame counter (F) field which is incremented with every transmission.

So for each transmission of a given node (DevAddr) we define the notation SNRmax(devAddr,F) where:

* F is the value of the frame counter of the frame

* DevAddr is the network address of the node as defined in the LoRaWAN spec

* SNRmax is the maximum of the various SNRs reported by the different gateways who received this given frame.

For each node taken into account by the algorithm, the following inputs must be available in memory.

1. The frame counter values of the last 20 received frames Fn, Fn-1 .. , Fn-19

2. The list of the 20 associated SNRmax(DevAddr, Fn to n-19)

3. The number of gateway which received each of the frame (the receive diversity) GtwDiversity(devAddr,Fn to n-19) [not yet used by this algorithm , reserved for future use] 

Each time a new frame with frame counter N+x reaches the server, the SNRmax(Nif,Fn-x) value is computed by analyzing the multiple instances of the frame reported by the different gateways.

The triplet {N+x , SNRmax(devAddr,Fn-x) , GtwDiversity(devAddr,Fn+x)} is inserted at the end table. Each time a new line is inserted at the end of the table, the lines 2 to 19 are 
shifted up and the first line is discarded so as to keep the length of the table constantly equal to 20. If a frame is retransmitted several time by the end-device (same frame counter) 
only the best transmission is kept. That means that if a frame with the same frame counter value than the newly received frame already exists in the table (it can only be the last one) 
then the SNRmax value is updated if the new frame SNRmax value is greater than the one already stored in the table. 

Each time the server decides to send an ADR command to a node, it uses the previously described data structure to perform the following computations.

First the Max (not average) SNR (SNRm) is computed over the middle column of the table. Then we compute:

SNRmargin = SNRm – SNR(DR) - margin_db

* Where margin_db is the installation margin of the network (typically 10dB in most networks), this is a device specific static parameter. This may be part of a device profile.

* Where SNR(DR) is the required SNR to successfully demodulate as a function of Data Rate given in the following table. DR is the data rate of the end-device’s last received frame. 
  */

  //the ADR algorithm is called on a particular device.
 // Search device in m_endDevices:
  auto it = m_endDevices.find (deviceAddr);
  if (it == m_endDevices.end ()) { // end device not found
    NS_LOG_ERROR (this << " Could not find device info struct in m_endDevices for dev addr " << deviceAddr << ". Aborting ADR algorithm");
    LoRaWANADRAlgoritmResult adrResFailure = {false, 0, 0, 0, 0, 0};
    return adrResFailure;
  }
  //this function is called when the server decides to send an ADR command to a node.

  if(it->second.m_lastDataRateIndex == 5 && it->second.m_lastTxPowerIndex == 7) 
  {
      NS_LOG_INFO("Running ADR, for device " << deviceAddr << " but device already uses the fastest DR and lowest TX power");
      LoRaWANADRAlgoritmResult adrRes = {true, it->second.m_lastDataRateIndex, it->second.m_lastTxPowerIndex, 0, 0, 0}; //TODO: ChannelMask, chMaskCtrl, and NbTrans are not currently implemented.
      return adrRes;   
  }

  //calculate SNRm - the max SNR over the table
  double snrM = -128.0;
  for (auto & row : it->second.m_frameSNRHistory) {
    if(row.snrMax > snrM) {
      snrM = row.snrMax;
    }

    if(row.snrMax == 0.0) {
      NS_LOG_ERROR(this << "snrMax was zero exactly.");
    }
  }
  
  //compute SNRmargin = SNRm - SNR(DR) - margin_db
  //margin_db is the installation margin of the network, and is a device specific static parameter (typically 10dB but tunable. 5dB is default - lower margin_db uses higher data rates generally, so less reliability but more energy efficiency)
  //SNR(DR) is as follows:
  /*
  Data Rate   | Required SNR
  DR0         |   -20
  DR1         |   -17.5
  DR2         |   -15
  DR3         |   -12.5
  DR4         |   -10
  DR5         |   -7.5
  */ 
  double snrDr;
  if(m_snrCutoffValuesSource) {
      snrDr = m_adrSnrRequirementsSemtech[it->second.m_lastDataRateIndex].snr;
  } else {
      snrDr = m_adrSnrRequirementsVDA[it->second.m_lastDataRateIndex].snr;
  }

  double SNRmargin = snrM - snrDr - it->second.m_marginDb;
  
  int nStep = int(SNRmargin/3);
  

  uint8_t dr = it->second.m_lastDataRateIndex;
  uint8_t tx = it->second.m_lastTxPowerIndex; //The device is to presume the power level is 0 (max) unless it changes it. (This will go out of sync sometimes between ns and ed). 

  NS_LOG_INFO ("ADR Info: snrM: " << snrM << " snrDr: " << snrDr << " SNRmargin " << SNRmargin << " nStep " << nStep << " dr " << dr << " tx " << tx);

  //then run this algorithm:
  //this algorithm is based on Things Network implementation. only EU868 band for now.
  while (nStep!=0) {
    if (nStep > 0) {
      if(dr < 5) {
        dr += 1; //dr index
      } else {
        if (tx == 7) { //i.e if tx power is the min already
          break;
        }
        tx += 1; //i.e. drop tx power by 2dB
      }
      nStep -= 1;

    } else {
      if (tx > 0) { //i.e. tx power is less than max
        tx -= 1; //i.e. increase tx power by 2dB
        nStep += 1;
      } else {
        //tx power is already the max
        break;
      }
    }
  }

  LoRaWANADRAlgoritmResult adrRes;
  if(dr == it->second.m_lastDataRateIndex && tx == it->second.m_lastTxPowerIndex) {
    //no change required
    NS_LOG_INFO("ADR ran, no change required");
    adrRes = {true, dr, tx, 0, 0, 0}; //TODO: ChannelMask, chMaskCtrl, and NbTrans are not currently implemented.    
  }
  else
  {
    it->second.m_lastTxPowerIndex = tx; //the tx power index is maintained on this side, the new dr will be saved after the next uplink is received.
    NS_LOG_INFO ("ADR Info: new dr " << dr << " new tx " << tx);
    adrRes = {true, dr, tx, 0, 0, 0}; //TODO: ChannelMask, chMaskCtrl, and NbTrans are not currently implemented.  
  }
   
  return adrRes;
}



void
LoRaWANNetworkServer::PrintFinalDetails ()
{
  for (auto d = m_endDevices.cbegin(); d != m_endDevices.cend(); d++) {
    std::cout << d->second.m_deviceAddress.Get() - 1 << "\t" <<  d->second.m_nDSPacketsGenerated <<  
    "\t" << d->second.m_nDSPacketsSent << "\t" << d->second.m_nDSPacketsSentRW1 << "\t" << d->second.m_nDSPacketsSentRW2 << 
    "\t" << d->second.m_nDSRetransmission << "\t" << d->second.m_nDSAcks << "\t" << d->second.m_nUSPackets << std::endl;
  }
  
}


LoRaWANGatewayApplication::LoRaWANGatewayApplication ()
  : m_socket (0),
    m_connected (false),
    m_totalRx (0)
{
  NS_LOG_FUNCTION (this);
}

TypeId
LoRaWANGatewayApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANGatewayApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<LoRaWANGatewayApplication> ()
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&LoRaWANGatewayApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

LoRaWANGatewayApplication::~LoRaWANGatewayApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
LoRaWANGatewayApplication::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  this->m_lorawanNSPtr = LoRaWANNetworkServer::getLoRaWANNetworkServerPointer ();

  // chain up
  Application::DoInitialize ();
}

void
LoRaWANGatewayApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  this->m_lorawanNSPtr = nullptr;
  // clear ref count in static member, as to destroy the LoRaWANNetworkServer object.
  // Note we should only destroy the NS object when the simulation is stopped and all gateway applications are destroyed.
  // So we assume that a gateway is not destroyed before the end of the simulation

  if (LoRaWANNetworkServer::haveLoRaWANNetworkServerObject ())
    LoRaWANNetworkServer::clearLoRaWANNetworkServerPointer ();

  // chain up
  Application::DoDispose ();
}

void
LoRaWANGatewayApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
LoRaWANGatewayApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t
LoRaWANGatewayApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  return LoRaWANNetworkServer::getLoRaWANNetworkServerPointer ()->AssignStreams (stream);
}

bool
LoRaWANGatewayApplication::CanSendImmediatelyOnChannel (uint8_t channelIndex, uint8_t dataRateIndex)
{
  NS_LOG_FUNCTION (this << (unsigned)channelIndex << (unsigned)dataRateIndex);

  Ptr<LoRaWANNetDevice> device = DynamicCast<LoRaWANNetDevice> (GetNode ()->GetDevice (0));

  if (!device) {
    NS_LOG_ERROR (this << " Cannot get LoRaWANNetDevice pointer belonging to this gateway");
    return false;
  } else {
    return device->CanSendImmediatelyOnChannel (channelIndex, dataRateIndex);
  }
}

void LoRaWANGatewayApplication::SendDSPacket (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  // p represents MACPayload

  // Get the requested data rate from the packet tag
  uint8_t dataRateIndex = 12; // SF12 as default value

  LoRaWANPhyParamsTag phyParamsTag;
  if (p->PeekPacketTag (phyParamsTag)) {
	  dataRateIndex = phyParamsTag.GetDataRateIndex();
  }

  // Set NetDevice MTU Data rate before calling socket::Send
  Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (GetNode ()->GetDevice (0));
  netDevice->SetMTUSpreadingFactor(LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor);

  m_txTrace (p);
  m_socket->Send (p);

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                << "s LoRaWANGatewayApplication application on node #"
                << GetNode()->GetId()
                << " sent a downstream packet of size "
                <<  p->GetSize ());
}

// Application Methods
void LoRaWANGatewayApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::PacketSocketFactory"));
      m_socket->Bind ();

      PacketSocketAddress socketAddress;
      socketAddress.SetSingleDevice (GetNode ()->GetDevice (0)->GetIfIndex ()); // Set the address to match only a specified NetDevice...
      m_socket->Connect (Address (socketAddress)); // packet-socket documentation mentions: "Send: send the input packet to the underlying NetDevices with the default destination address. The socket must be bound and connected."

      m_socket->Listen ();
      //m_socket->SetAllowBroadcast (true); // TODO: does not work on packet socket?
      m_socket->SetRecvCallback (MakeCallback (&LoRaWANGatewayApplication::HandleRead, this));

    }

  // instruct Network Server to populate end devices data structure:
  // NOTE that we call PopulateEndDevices in StartApplication and not in DoInitialize as the attributes for the NetworkServer object have not yet been set at the of DoInitialize()
  this->m_lorawanNSPtr->PopulateEndDevices ();
}

void LoRaWANGatewayApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("LoRaWANGatewayApplication found null socket to close in StopApplication");
    }
}

void LoRaWANGatewayApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();

      if (PacketSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s gateway on node #"
                       << GetNode()->GetId()
                       <<  " received " << packet->GetSize () << " bytes from "
                       << PacketSocketAddress::ConvertFrom(from).GetPhysicalAddress () 
                       << ", total Rx " << m_totalRx << " bytes");

          this->m_lorawanNSPtr->HandleUSPacket (this, from, packet);
        }
      else
        {
          NS_LOG_WARN (this << " Unexpected address type");
        }
    }
}

void LoRaWANGatewayApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void LoRaWANGatewayApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

} // Namespace ns3
