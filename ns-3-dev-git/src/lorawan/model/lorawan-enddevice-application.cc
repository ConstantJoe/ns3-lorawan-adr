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








/*


The proposed LoRaWAN modifications:

1. Targetting semi-lossy links on ED-side (which may not be fixed by the algorithm as semi-lossy and non-lossy links look the same): 

the NS maintains the packet delivery rate of every ED since join / DR change / tx pow change. Can tell if frames have been missed using frame counter in the uplink.

when the NS sees that an ED has set the ADRAckReq bit in an uplink frame, it uses this number to decide whether to push the DR or TX power of a device down 1 or not (can use the predefined AddLoRaADRReq command for this).
but what are the variables that come into play here? number of frames will always be at least 32, that's a good judge of PDR. What is an acceptable PDR for LoRaWAN? Could take into account the DR spread across nodes. Or could just use a defined threshold i.e. if PDR < 80, push down.
Can implement both, why not.

so todo:
each ns_ed object has new vars - #recentRecved, and #recentTotal
every time an uplink frame is received - recentRecved++, and recentTotal += newFrmCnt - m_fCntUp
every time an ED with ADRAckReq bit is set - run new algorithm (to be defined), result of which is of type LoRaWANADRAlgoritmResult, send it down.

also TODO: should ADR frame counter be reset every time the DR from uplink frames changes? Otherwise there'll be a big mix of SNR values. Think about this, and check what other implementations do.

2. Targetting too-slow longs on the NS-side (which may occur due to the join procedure, for example)

the NS algorithm is ran every N frames (usually 20). However, since a max of max approach is taken the NS can effectively know much earlier than this if a node will be taking a big jump in DR.
So, maintain a confidence value based on the maxSNR of frames, and if this confidence value reaches some threshold, make an earlier call to ADR which specifically requests a downlink frame to be sent containing the MAC command. 

Base on: avg, std dev of frames.

The issue is: if there is otherwise no downlink requirement for the NS (pretty common for LoRaWAN), currently the ADR command will only be sent once every 20 uplink frames / once every 32 uplink frames (depends on approach)  

To check: in the other papers and in the ThingsNetwork and LoRaServer, does a call to the ADR algorithm actually SCHEDULE a new downlink frame?

Can do some maths to find the point (DR jumps) where the device will ultimately save energy by making the jump earlier. 

so todo:
so, on the ns side every end device has a new parameter: adr_confidence
also on the ns side, every end device has new parameters: avg_snr, (maybe more?)
every time a unique uplink frame is received, a function is called: adr_confidence_update, which adds to adr_confidence
every time m_adr is checked, adr_confidence is also checked. If true, adr is ran.
every time m_adr is reset, adr_confidence is also reset.

3. Specifically define in the ADR algorithm whether to send an empty ADR or not.

Work out which does what, list them out.

Do some maths on the difference.


*/


// Based on:
// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "lorawan.h"
#include "lorawan-net-device.h"
#include "lorawan-enddevice-application.h"
#include "lorawan-frame-header-uplink.h"
#include "lorawan-frame-header-downlink.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANEndDeviceApplication");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANEndDeviceApplication);

TypeId
LoRaWANEndDeviceApplication::GetTypeId (void)
{
  // Construct default value for the channel random variable:
  std::stringstream channelRandomVariableSS;
  const uint32_t channelRandomVariableDefaultMin = 0;
  const uint32_t channelRandomVariableDefaultMax = (LoRaWAN::m_supportedChannels.size () - 1) - 1; // additional -1 as not to use the 10% RDC channel as an upstream channel
  channelRandomVariableSS << "ns3::UniformRandomVariable[Min=" << channelRandomVariableDefaultMin << "|Max=" << channelRandomVariableDefaultMax << "]";
  //std::cout << "LoRaWANEndDeviceApplication::GetTypeId: " << channelRandomVariableSS.str() << std::endl;

  static TypeId tid = TypeId ("ns3::LoRaWANEndDeviceApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<LoRaWANEndDeviceApplication> ()
    .AddAttribute ("DataRateIndex",
                   "DataRate index used for US transmissions of this end device.",
                   UintegerValue (0), // default data rate is SF12
                   MakeUintegerAccessor (&LoRaWANEndDeviceApplication::GetDataRateIndex, &LoRaWANEndDeviceApplication::SetDataRateIndex),
                   MakeUintegerChecker<uint16_t> (0, LoRaWAN::m_supportedDataRates.size ()))
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (21),
                   MakeUintegerAccessor (&LoRaWANEndDeviceApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("ConfirmedDataUp",
                   "Send Upstream data as Confirmed Data UP MAC packets."
                   "False means Unconfirmed data up packets are sent.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LoRaWANEndDeviceApplication::m_confirmedData),
                   MakeBooleanChecker ())
    .AddAttribute ("ADR",
                   "Adaptive Data Rate mode."
                   "True means the Adaptive Data Rate mode is on.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&LoRaWANEndDeviceApplication::m_adr),
                   MakeBooleanChecker ())
    .AddAttribute ("ChannelRandomVariable", "A RandomVariableStream used to pick the channel for upstream transmissions.",
                   StringValue (channelRandomVariableSS.str ()),
                   MakePointerAccessor (&LoRaWANEndDeviceApplication::m_channelRandomVariable),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("UpstreamIAT", "A RandomVariableStream used to pick the time between subsequent US transmissions from this end device.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=600.0]"),
                   MakePointerAccessor (&LoRaWANEndDeviceApplication::m_upstreamIATRandomVariable),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("UpstreamSend", "A RandomVariableStream used to pick the time between subsequent US transmissions from this end device.",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=600.0]"),
                   MakePointerAccessor (&LoRaWANEndDeviceApplication::m_upstreamSendIATRandomVariable),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&LoRaWANEndDeviceApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddTraceSource ("USMsgTransmitted", "An US message is sent",
                     MakeTraceSourceAccessor (&LoRaWANEndDeviceApplication::m_usMsgTransmittedTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("DSMsgReceived", "An acknowledgement for an US message has been received.",
                     MakeTraceSourceAccessor (&LoRaWANEndDeviceApplication::m_dsMsgReceivedTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


LoRaWANEndDeviceApplication::LoRaWANEndDeviceApplication ()
  : m_socket (0),
    m_connected (false),
    m_lastTxTime (Seconds (0)),
    m_totBytes (0),
    m_framePort (0),
    m_fCntUp (0),
    m_fCntDown (0),
    m_setAck (false),
    m_totalRx (0),
    m_adrAckReq (false),
    m_classB(false),
    m_adrAckCnt(0),
    m_txPowerIndex(0),
    m_doSendLinkAdrAns(false),
    m_linkAdrAnsPowerAck(false),
    m_linkAdrAnsDataRateAck(false),
    m_linkAdrAnsChannelMaskAck(false),
    m_attemptedThroughput(0)
{
  NS_LOG_FUNCTION (this);

  //m_channelRandomVariable = CreateObject <UniformRandomVariable> (); // random variable between 0 and size(channels) - 2
  //m_channelRandomVariable->SetAttribute ("Min", DoubleValue (0.0));
  //const uint32_t max = (LoRaWAN::m_supportedChannels.size () - 1) - 1; // additional -1 as not to use the 10% RDC channel as an upstream channel
  //m_channelRandomVariable->SetAttribute ("Max", DoubleValue (max));
}

LoRaWANEndDeviceApplication::~LoRaWANEndDeviceApplication ()
{
  NS_LOG_FUNCTION (this); 
}

void
LoRaWANEndDeviceApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

uint32_t
LoRaWANEndDeviceApplication::GetDataRateIndex (void) const
{
  return m_dataRateIndex;
}

void
LoRaWANEndDeviceApplication::SetDataRateIndex (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);

  if (index <= LoRaWAN::m_supportedDataRates.size () - 1)
    m_dataRateIndex = index;
  else
    NS_LOG_ERROR (this << " " << index << " is an invalid data rate index");
}

Ptr<Socket>
LoRaWANEndDeviceApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t
LoRaWANEndDeviceApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_channelRandomVariable->SetStream (stream);
  m_upstreamIATRandomVariable->SetStream (stream + 1);
  m_upstreamSendIATRandomVariable->SetStream (stream + 2);
  return 2;
}

void
LoRaWANEndDeviceApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  PrintFinalDetails();
  // chain up
  Application::DoDispose ();
}

// Application Methods
void LoRaWANEndDeviceApplication::StartApplication () // Called at time specified by Start
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
      m_socket->SetRecvCallback (MakeCallback (&LoRaWANEndDeviceApplication::HandleRead, this));

      // TODO: remove?
      m_socket->SetConnectCallback (
        MakeCallback (&LoRaWANEndDeviceApplication::ConnectionSucceeded, this),
        MakeCallback (&LoRaWANEndDeviceApplication::ConnectionFailed, this));

      m_devAddr = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ()).Get();
    }

  // Insure no pending event
  CancelEvents ();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  //m_txEvent = Simulator::ScheduleNow (&LoRaWANEndDeviceApplication::SendPacket, this);

  Time nextSendTime (Seconds (this->m_upstreamSendIATRandomVariable->GetValue ()));
  NS_LOG_LOGIC (this << " upstream nextTime = " << nextSendTime);
  m_txEvent = Simulator::Schedule (nextSendTime,
                                       &LoRaWANEndDeviceApplication::SendPacket, this); 
}

void LoRaWANEndDeviceApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("LoRaWANEndDeviceApplication found null socket to close in StopApplication");
    }
}

void LoRaWANEndDeviceApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_txEvent);
}


// Private helpers
void LoRaWANEndDeviceApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      Time nextTime (Seconds (this->m_upstreamIATRandomVariable->GetValue ()));
      NS_LOG_LOGIC (this << " nextTime = " << nextTime);
      m_txEvent = Simulator::Schedule (nextTime,
                                       &LoRaWANEndDeviceApplication::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void LoRaWANEndDeviceApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_txEvent.IsExpired ());

  Ipv4Address myAddress = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ());

  LoRaWANFrameHeaderUplink fhdr;
  //LoRaWANFrameHeader fhdr;
  
  fhdr.setDevAddr (myAddress);
  fhdr.setAdr(m_adr);
  fhdr.setAck (m_setAck);
  fhdr.setClassB(m_classB);

  //TODO: support use of other MAC commands
  if(m_doSendLinkAdrAns) {
    if (fhdr.AddLoRaADRAns(m_linkAdrAnsPowerAck, m_linkAdrAnsDataRateAck, m_linkAdrAnsChannelMaskAck)) { //true indicates there was space to add the MAC layer message
      NS_LOG_INFO (this << "added ADRAns to frame");
      m_doSendLinkAdrAns = false;
      m_linkAdrAnsPowerAck = false;
      m_linkAdrAnsDataRateAck = false;
      m_linkAdrAnsChannelMaskAck = false;
    }
    else {
      NS_LOG_INFO (this << "tried but failed to add ADRAns to frame");
      //TODO: log, recovery
    }
  }
  //TODO: support use of MAC layer messages in frame payload, not just in FOpts
  m_fCntUp++; //TODO: this is changed from the original. Think the original was a bug? Read the very first transmission from a device as a retransmission, as 0=0
  fhdr.setFrameCounter (m_fCntUp); // increment frame counter
  //fhdr.setFrameCounter (m_fCntUp++); // increment frame counter
  
  //TODO: this should only be performed for a NEW uplink, repeated transmissions do not increase the counter.
  //but the NbTrans parameter is not currently implemented in the simulator, so on a device level there are not repeated transmissions
  //(the NS can still receive multiple packets with the same frameCounter value, this happens when a packet is received by multiple Gateways)
  if(m_adr) { 
    m_adrAckCnt++;
    AdaptiveDataRate(); //modifies the m_adrAckReq, m_txPowerIndex and m_dataRateIndex of the device.  
  }
  fhdr.setAdrAckReq(m_adrAckReq);
  

  // FPort: we will send FRMPayload so set the frame port
  fhdr.setFramePort (m_framePort);

  // Construct MACPayload
  // PHYPayload: MHDR | MACPayload | MIC
  // MACPayload: FHDR | FPort | FRMPayload
  Ptr<Packet> packet;
  uint8_t frmPayloadSize = m_pktSize  - fhdr.GetSerializedSize() - 1 - 4;  // subtract 8 bytes for frame header, 1B for MAC header and 4B for MAC MIC
  if (frmPayloadSize >= sizeof(uint64_t)) { // check whether payload size is large enough to hold 64 bit integer
    // send decrementing counter as payload (note: globally shared counter)
    uint8_t* payload = new uint8_t[frmPayloadSize](); // the parenthesis initialize the allocated memory to zero
    if (payload) {
      const uint64_t counter = LoRaWANCounterSingleton::GetCounter ();
      ((uint64_t*)payload)[0] = counter; // copy counter to beginning payload
      packet = Create<Packet> (payload, frmPayloadSize);
      delete[] payload;
    } else {
      packet = Create<Packet> (frmPayloadSize);
    }
  } else {
    packet = Create<Packet> (frmPayloadSize);
  }

  packet->AddHeader (fhdr); // Packet now represents MACPayload

  // Select channel to use:
  uint32_t channelIndex = m_channelRandomVariable->GetInteger ();
  NS_ASSERT (channelIndex <= LoRaWAN::m_supportedChannels.size () - 2); // -2 because end devices should not use the special high power channel for US traffic

  LoRaWANPhyParamsTag phyParamsTag;

  phyParamsTag.SetChannelIndex (channelIndex);
  phyParamsTag.SetDataRateIndex (m_dataRateIndex);

  NS_ASSERT_MSG(m_dataRateIndex != 6, "in ED sendPacket");

  phyParamsTag.SetCodeRate (1); //TODO: double-check the use of this to make sure it should be 1, not 3 as in original codebase

  //TODO: have SetSinrAvg set to be 0 in the constructor instead
  phyParamsTag.SetSinrAvg (0.0); //not actually used, has to be added here because PhyParamsTag, and PDDataIndication and DataIndication functions in net and mac layers are shared by GW and ED. Doesn't actually get used in NetDevice:Send()
  phyParamsTag.SetTxPowerIndex (m_txPowerIndex); 

  packet->AddPacketTag (phyParamsTag);

  // Set Msg type
  LoRaWANMsgTypeTag msgTypeTag;
  if (m_confirmedData)
    msgTypeTag.SetMsgType (LORAWAN_CONFIRMED_DATA_UP);
  else
    msgTypeTag.SetMsgType (LORAWAN_UNCONFIRMED_DATA_UP);
  packet->AddPacketTag (msgTypeTag);

  uint32_t deviceAddress = myAddress.Get ();
  m_usMsgTransmittedTrace (deviceAddress, msgTypeTag.GetMsgType (), packet);

  // Set NetDevice MTU Data rate before calling socket::Send
  Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (GetNode ()->GetDevice (0));
  netDevice->SetMTUSpreadingFactor(LoRaWAN::m_supportedDataRates [m_dataRateIndex].spreadingFactor);

  m_attemptedThroughput++;

  int16_t r = m_socket->Send (packet);
  if (r < 0) {
    NS_LOG_ERROR(this << "PacketSocket::Send failed and returned " << static_cast<int16_t>(r) << ". Errno is set to " << m_socket->GetErrno ());
  } else {
    m_setAck = false; // reset m_setAck
    m_totBytes += packet->GetSize ();

    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
        << "s LoRaWANEndDevice application on node "
        << myAddress.Get()
        << " sent "
        <<  packet->GetSize () << " bytes,"
        << " total Tx " << m_totBytes << " bytes");

    //NS_LOG_INFO("comparison of addresses: " << m_devAddr << " " << GetNode()->GetId());
  }
  

  m_lastTxTime = Simulator::Now ();
  ScheduleNextTx ();
}

void LoRaWANEndDeviceApplication::HandleRead (Ptr<Socket> socket)
{
   Ipv4Address myAddress = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ());
   
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
                       << "s end device (Receiver) on node "
                       << myAddress.Get()
                       <<  " received " << packet->GetSize () << " bytes from "
                       << PacketSocketAddress::ConvertFrom(from).GetPhysicalAddress ()
                       << ", total Rx " << m_totalRx << " bytes");

          this->HandleDSPacket (packet, from);
        }
      else
        {
          NS_LOG_WARN (this << " Unexpected address type");
        }
    }
}

void
LoRaWANEndDeviceApplication::HandleDSPacket (Ptr<Packet> p, Address from)
{
  NS_LOG_FUNCTION(this << p);

  
  if (m_adr) { //received a packet, reset m_adrAckCnt and m_adrAckReq
    m_adrAckCnt = 0;
    Ipv4Address myAddress = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ());
    NS_LOG_INFO("resetting adrAckCnt on node" << myAddress.Get());
    m_adrAckReq = false;
  }

  //TODO: this is where the packet is received, so should also remove and read the LoRaWANFrameHeader
  
  //this is a downlink packet, so should read: ADR, ACK, FPending, FOptsLen
  // Decode Frame header
  LoRaWANFrameHeaderDownlink frmHdr;
  frmHdr.setSerializeFramePort (true); // Assume that frame Header contains Frame Port so set this to true so that RemoveHeader will deserialize the FPort
  //TODO: support use of FPort=0, which is the case where MAC commands are stored in the frame payload, not in the header
  p->RemoveHeader (frmHdr);

  //The setting of the ADR flags on both sides indicates which ADR method should be used.
  //The ADR algorithm itself is ran after, every uplink frame is sent, here just changes the method used.
  //but we're only implementing the version which uses the NS for now. TODO: other methods
  /*
  if (frmHdr.getAdr () & m_adr) {
    //the NS is able to run its side of the ADR algorithm.
    // swap to regular ADR if not currently using it
    
  } else {
    //the NS is unable to run its side of the ADR algorithm
    if(m_adr) {
      //but an ADR-like scheme is still requested.
      //what happens now is dependent on whether the device is mobile or not:
      //mobile: unset m_adr, control it's uplink data rate following it's own strategy.
      //stationary:  keep m_adr set, and "apply normal data rate decay in the absense of ADR downlink commands"
      //leaving this as future work for now
    } else {
      //use no ADR scheme, just maintain static dr and tx power
    }
  }*/

  /*
  TODO: ACK bit handling

  "When receiving a confirmed data message, the receiver SHALL respond with a data frame
  that has the acknowledgment bit (ACK) set. If the sender is an end-device, the network will
  try to send the acknowledgement using one of the receive windows opened by the end-
  device after the send operation. I

  An acknowledgement is only sent in response to the latest message received and it is never
  retransmitted.

  If the sender is a gateway, the end-device transmits an acknowledgment at its own discretion (see note below).

  Note: To allow the end-devices to be as simple as possible and have
  as few states as possible it may transmit an explicit (possibly empty)
  acknowledgement data message immediately after the reception of a
  data message requiring a confirmation. Alternatively the end-device
  may defer the transmission of an acknowledgement to piggyback it
  with its next data message."

  
  */

  //TODO: FramePending bit handling
  //"The Frame Pending bit (FPending) is only used in downlink communication, and indicates that the network has more data pending to be sent and therefore asking
  //the end device to open another receive window as soon as possible by sending another uplink message."
  //note: this is not implemented yet.

  //the receive of a frame (any frame) should change the ADRAckReq counter too.

  //TODO: FOptsLen bit handling - loop through the m_macCommandsNS structure and handle any of the commands with bool set to true. The ADR-related command is the only implemented one for now.
  //but write in general.
  for(std::vector<LoRaWANMacCommandDownlink>::iterator it = frmHdr.m_macCommandsNS.begin(); it != frmHdr.m_macCommandsNS.end(); ++it) {
      if(it->m_isBeingUsed) {
        //TODO: write functions inside the frame-header to extract the MAC command properly
        // for now, since the ADR command is the only one implemented, we will just check for that one.
        if(it->m_commandID == LinkADRReq) {
          uint8_t new_dr = frmHdr.m_dataRateIndex;
          uint8_t new_tx = frmHdr.m_txPowerIndex;
          NS_ASSERT_MSG(new_dr != 6, "dr6 not supported! ed side. dr: " << new_dr << "and tx: " << new_tx << "and original val was: " << frmHdr.m_dataRateTXPowerByte);

          if (new_dr < 6) { // 15 indicates no change, 8-14 are RFU, 7 is FSK (not supported in this simulator), and 6 is DR6 with bw=250kHz (not supported in err model yet)
            NS_LOG_INFO (this << "received ADR mac, changing data rate from " << m_dataRateIndex << " to " << new_dr);
            m_dataRateIndex = new_dr;

            m_linkAdrAnsDataRateAck = true;
          } else {
            if (new_dr == 15) {
              //TODO: log ok
              NS_LOG_INFO (this << "received ADR mac, no change of dr needed");
              m_linkAdrAnsDataRateAck = true;
            } else {
              //TODO: log err
              NS_LOG_INFO (this << "received ADR mac, err dr");
              m_linkAdrAnsDataRateAck = false;
            }
          }

          if (new_tx < 8) { // 15 indicates no change, 8-14 are RFU
            NS_LOG_INFO (this << "received ADR mac, changing tx pow index from " << m_txPowerIndex << " to " << new_tx);
            m_txPowerIndex = new_tx;
            m_linkAdrAnsPowerAck = true;
          } else {
            if (new_tx == 15) {
              //TODO: log ok
              m_linkAdrAnsPowerAck = true;
              NS_LOG_INFO (this << "received ADR mac, no change of tx needed");
            } else {
              //TODO: log err
              m_linkAdrAnsPowerAck = false;
              NS_LOG_INFO (this << "received ADR mac, err tx");
            }
          }

          //TODO: support use of change of NbTrans and channel access
          m_linkAdrAnsChannelMaskAck = true;

          //indicate to send MAC level response in next uplink.
          m_doSendLinkAdrAns = true;
        }
        else {
          //TODO: err log
        }
      }
  }

  // set m_setAck to true in case a CONFIRMED_DATA_DOWN message was received:
  // Try to parse Packet tag:
  LoRaWANMsgTypeTag msgTypeTag;
  if (p->RemovePacketTag (msgTypeTag)) {
    LoRaWANMsgType msgType = msgTypeTag.GetMsgType ();
    if (msgType == LORAWAN_CONFIRMED_DATA_DOWN) {
      m_setAck = true; // next packet should set Ack bit
      NS_LOG_DEBUG (this << " Set Ack bit to 1");
    }
  } else {
    NS_LOG_WARN (this << " LoRaWANMsgTypeTag packet tag is missing from packet");
  }

  // Was packet received in first or second receive window?
  // -> Look at Mac state
  Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (GetNode ()->GetDevice (0));
  Ptr<LoRaWANMac> mac = netDevice->GetMac ();
  LoRaWANMacState state = mac->GetLoRaWANMacState ();
  NS_ASSERT (state == MAC_RW1 || state == MAC_RW2);

  // Log packet reception
  Ipv4Address myAddress = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ());
  uint32_t deviceAddress = myAddress.Get ();
  if (state == MAC_RW1)
    m_dsMsgReceivedTrace (deviceAddress, msgTypeTag.GetMsgType(), p, 1);
  else if (state == MAC_RW2)
    m_dsMsgReceivedTrace (deviceAddress, msgTypeTag.GetMsgType(), p, 2);
}

void LoRaWANEndDeviceApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void LoRaWANEndDeviceApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR (this << " Connection failed");
}

void LoRaWANEndDeviceApplication::AdaptiveDataRate ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO(GetNode()->GetId() << "is in ED ADR");
  //note that this is the 1.1 version, which includes tx_pow
  // in a nutshell, if the data rate or the tx power is set to below the default, the device periodically validates that messages are successfully being received.
 
 
  //TODO: choice of available channels is always all of them right now. Implement channel switches
  //TODO: the table included in the LoRaWAN ADR definition contradicts the algorithm? Double check?

  if( (m_txPowerIndex > 0) | (m_dataRateIndex > 0) /*| not all channels open*/)
  {
      if (m_adrAckCnt == ADR_ACK_LIMIT) {
        NS_LOG_INFO (this << " at time " << Simulator::Now ().GetSeconds ()  <<  " node " << GetNode()->GetId() << " adrAckCnt hit ADR_ACK_LIMIT");
        m_adrAckReq = true;
      }
      else if (m_adrAckCnt == ADR_ACK_LIMIT + ADR_ACK_DELAY) {
        NS_LOG_INFO (this << " at time " << Simulator::Now ().GetSeconds () <<  " node " << GetNode()->GetId()<< "adrAckCnt hit ADR_ACK_DELAY + ADR_ACK_LIMIT");
        if (m_txPowerIndex > 0) {
          m_txPowerIndex = 0; //increase tx power back to default (highest for the channel)
          NS_LOG_INFO (this << "... so increasing tx power on node " << GetNode()->GetId());
        } else if (m_dataRateIndex > 0) { //current DR is not the slowest, longest range data rate
          // index 0 is SF12, BW=125kHz (the slowest)
          m_dataRateIndex--; // slow data rate by 1
          NS_LOG_INFO (this << "... so slowing data rate to " << m_dataRateIndex << " on node " << GetNode()->GetId());
        } /* else if (not all channels open) {
          //Note: in the current implementation of the simulator all channels are always available
          //open all channels
        }*/
        m_adrAckCnt = ADR_ACK_LIMIT;  
      }

  } else {
    // link range cannot be improved
    m_adrAckReq = false;
  }
}

void
LoRaWANEndDeviceApplication::PrintFinalDetails ()
{
  std::cout << m_devAddr - 1 << "\t" <<  m_attemptedThroughput << std::endl;
}


} // Namespace ns3
