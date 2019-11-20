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

/*
 * Try to send data from two class A end devices to a gateway, data is
 * unconfirmed upstream data. Chain is LoRaWANMac -> LoRaWANPhy ->
 * SpectrumChannel -> LoRaWANPhy -> LoRaWANMac
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ipv4-address.h>
#include <ns3/lorawan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/mobility-module.h>
#include <ns3/applications-module.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include "ns3/basic-energy-source-helper.h"


#include <iostream>

using namespace ns3;

void
ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  uint64_t bytes = 0;
  while ((packet = socket->Recv ()))
    {
      bytes += packet->GetSize ();
    }

  //std::cout << "SOCKET received " << bytes << " bytes" << std::endl;
}

Ptr<Socket>
SetupPacketReceive (Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  sink->Bind ();
  sink->SetRecvCallback (MakeCallback (&ReceivePacket));
  return sink;
}

int main (int argc, char *argv[])
{
  NodeContainer endDeviceNodes;
  NodeContainer gatewayNodes;
  NodeContainer allNodes;

  endDeviceNodes.Create (1);
  gatewayNodes.Create (1);
  allNodes.Add (endDeviceNodes);
  allNodes.Add (gatewayNodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> nodePositionList = CreateObject<ListPositionAllocator> ();
  nodePositionList->Add (Vector (2000.0, 2000.0, 0.0));  // end device1
  //nodePositionList->Add (Vector (-5.0, 5.0, 0.0));  // end device2
  //nodePositionList->Add (Vector (5.0, -5.0, 0.0));  // end device3
  //nodePositionList->Add (Vector (-5.0, -5.0, 0.0));  // end device4
  nodePositionList->Add (Vector (0.0, 0.0, 0.0));  // gateway
  mobility.SetPositionAllocator (nodePositionList);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (allNodes);

  /*double m_discRadius = 6000.0;
  MobilityHelper edMobility;
  edMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (0.0),
                                    "Y", DoubleValue (0.0),
                                    "rho", DoubleValue (m_discRadius));
  edMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  edMobility.Install (endDeviceNodes);*/

  // the gateway is placed at 0,0,0
  /*MobilityHelper gwMobility;
  Ptr<ListPositionAllocator> nodePositionList = CreateObject<ListPositionAllocator> ();
  nodePositionList->Add (Vector (0.0, 0.0, 0.0));  // gateway
  gwMobility.SetPositionAllocator (nodePositionList);
  gwMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gwMobility.Install (gatewayNodes);*/

  LoRaWANHelper lorawanHelper;
  lorawanHelper.SetNbRep(1); // no retransmissions
  NetDeviceContainer lorawanEDDevices = lorawanHelper.Install (endDeviceNodes);

  lorawanHelper.SetDeviceType (LORAWAN_DT_GATEWAY);
  NetDeviceContainer lorawanGWDevices = lorawanHelper.Install (gatewayNodes);


   //add an energy source to each end device
  BasicEnergySourceHelper sourceHelper;
  sourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(18000)); // = 5Wh
  EnergySourceContainer energySources = sourceHelper.Install(endDeviceNodes);



  //add energy consumption model to each end device
  LoRaWANRadioEnergyModelHelper radioHelper;
  radioHelper.SetCurrentModel("ns3::SX1272LoRaWANCurrentModel");

  DeviceEnergyModelContainer deviceModels = radioHelper.Install (lorawanEDDevices, energySources);

  PacketSocketHelper packetSocket;
  packetSocket.Install (endDeviceNodes);
  packetSocket.Install (gatewayNodes);

  LoRaWANEndDeviceHelper enddevicehelper;



  ApplicationContainer enddeviceApps = enddevicehelper.Install (endDeviceNodes);
  //enddevicehelper.AssignStreams(enddeviceApps, stream);

  // install gw application on gateways
  LoRaWANGatewayHelper gatewayhelper;
  //gatewayhelper.SetAttribute ("DefaultClassBDataRateIndex", UintegerValue (dr));
  ApplicationContainer gatewayApps = gatewayhelper.Install (gatewayNodes);
  //gatewayhelper.AssignStreams(gatewayApps, stream); //assigning streams on GW sets them on the NS

  std::cout << "LOCATIONS START" << std::endl;
  NodeContainer::Iterator d;
  for (d = endDeviceNodes.Begin(); d != endDeviceNodes.End(); ++d) { 
    Ptr<MobilityModel> mobility = (*d)->GetObject<MobilityModel>();
    Vector current = mobility->GetPosition();
    std::cout << (*d)->GetId() << " " << current.x << " " << current.y << " " << current.z << std::endl;
  }
  std::cout << "LOCATIONS END" << std::endl;

  
  uint32_t stream = 0;
  enddevicehelper.AssignStreams(gatewayApps, stream);
  gatewayhelper.AssignStreams(enddeviceApps, stream);

  enddeviceApps.Start (Seconds (0.0));
  enddeviceApps.Stop (Seconds (86400*1));

  //run for a day
  gatewayApps.Start (Seconds (0.0));
  gatewayApps.Stop (Seconds (86400*1));

  Ptr<Socket> recvSink = SetupPacketReceive (gatewayNodes.Get (0));

  Simulator::Stop (Seconds (86400.0*1));

  Simulator::Run ();


  
  //std::cout << nNodes << std::endl;
  std::cout << "starting destroy" << std::endl;
  Simulator::Destroy ();
  std::cout << "finishing destroy" << std::endl;
  return 0;
}
