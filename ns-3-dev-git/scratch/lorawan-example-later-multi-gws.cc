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
  LogComponentEnable ("LoRaWANGatewayApplication", LOG_INFO);
   LogComponentEnable ("LoRaWANEndDeviceApplication", LOG_INFO);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  uint32_t nNodes = 4;
  uint8_t  dr = 0;

  float interval = 600.0;
  //uint32_t stream = 0;

  CommandLine cmd;
  cmd.AddValue("nNodes", "Number of nodes to add to simulation", nNodes);
  cmd.AddValue("dr", "Data rate to be used (up and down, a and b)", dr);
  //cmd.AddValue("stream", "Random stream var", stream);
  cmd.Parse (argc, argv);

  dr &= (0b111);

  NodeContainer endDeviceNodes;
  NodeContainer gatewayNodes;
  NodeContainer allNodes;

  NodeContainer laterNodes;
  laterNodes.Create(100);

  endDeviceNodes.Create (nNodes);
  gatewayNodes.Create (4);
  allNodes.Add (endDeviceNodes);
  allNodes.Add (gatewayNodes);

  double m_discRadius = 5000.0;
  MobilityHelper edMobility;
  edMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (0.0),
                                    "Y", DoubleValue (0.0),
                                    "rho", DoubleValue (m_discRadius));
  edMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  edMobility.Install (endDeviceNodes);

  edMobility.Install (laterNodes);

  // the gateway is placed at 0,0,0
  MobilityHelper gwMobility;
  Ptr<ListPositionAllocator> nodePositionList = CreateObject<ListPositionAllocator> ();
  nodePositionList->Add (Vector (3000.0, 3000.0, 0.0));  // gateway 1
  nodePositionList->Add (Vector (-3000.0, 3000.0, 0.0));  // gateway 2
  nodePositionList->Add (Vector (3000.0, -3000.0, 0.0));  // gateway 3
  nodePositionList->Add (Vector (-3000.0, -3000.0, 0.0));  // gateway 4
  gwMobility.SetPositionAllocator (nodePositionList);
  gwMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gwMobility.Install (gatewayNodes);

  LoRaWANHelper lorawanHelper;
  lorawanHelper.SetNbRep(1); // no retransmissions
  NetDeviceContainer lorawanEDDevices = lorawanHelper.Install (endDeviceNodes);

  NetDeviceContainer lorawanLaterDevices = lorawanHelper.Install (laterNodes);

  lorawanHelper.SetDeviceType (LORAWAN_DT_GATEWAY);
  NetDeviceContainer lorawanGWDevices = lorawanHelper.Install (gatewayNodes);


   //add an energy source to each end device
  BasicEnergySourceHelper sourceHelper;
  sourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(18000)); // = 5Wh
  EnergySourceContainer energySources = sourceHelper.Install(endDeviceNodes);

  EnergySourceContainer energySourcesLater = sourceHelper.Install(laterNodes);

  //add energy consumption model to each end device
  LoRaWANRadioEnergyModelHelper radioHelper;
  radioHelper.SetCurrentModel("ns3::SX1272LoRaWANCurrentModel");

  DeviceEnergyModelContainer deviceModels = radioHelper.Install (lorawanEDDevices, energySources);
  DeviceEnergyModelContainer deviceModelsLater = radioHelper.Install (lorawanLaterDevices, energySourcesLater);

  // Note to self: PacketSocketHelper::Install adds a PacketSocketFactory
  // object as an aggregate object to each of the nodes in the NodeContainer
  PacketSocketHelper packetSocket;
  packetSocket.Install (endDeviceNodes);
  packetSocket.Install (gatewayNodes);

  packetSocket.Install (laterNodes);

   // install end device application on nodes
  LoRaWANEndDeviceHelper enddevicehelper;
  enddevicehelper.SetAttribute ("DataRateIndex", UintegerValue (2));
  ApplicationContainer enddeviceApps = enddevicehelper.Install (endDeviceNodes);
  //enddevicehelper.AssignStreams(enddeviceApps, stream);
  enddevicehelper.SetAttribute ("DataRateIndex", UintegerValue (dr));
  ApplicationContainer enddeviceAppsLater = enddevicehelper.Install (laterNodes);

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
  std::cout << "later nodes start" << std::endl;
  for (d = laterNodes.Begin(); d != laterNodes.End(); ++d) { 
    Ptr<MobilityModel> mobility = (*d)->GetObject<MobilityModel>();
    Vector current = mobility->GetPosition();
    std::cout << (*d)->GetId() << " " << current.x << " " << current.y << " " << current.z << std::endl;
  }

  std::cout << "LOCATIONS END" << std::endl;


  enddeviceApps.Start (Seconds (0.0));
  enddeviceApps.Stop (Seconds (interval*500)); //250 is probably not long enough

  enddeviceAppsLater.Start(Seconds(interval*250)); //i.e. start after convergence.
  enddeviceAppsLater.Stop (Seconds (interval*500));
  //run for a day
  gatewayApps.Start (Seconds (0.0));
  gatewayApps.Stop (Seconds (interval*500));

  Ptr<Socket> recvSink = SetupPacketReceive (gatewayNodes.Get (0));

  Simulator::Stop (Seconds (interval*500));

  Simulator::Run ();

  std::cout << "starting energy print out" << std::endl;
  for (NodeContainer::Iterator it = endDeviceNodes.Begin (); it != endDeviceNodes.End (); ++it) {
      Ptr<EnergySourceContainer> energySourceC = (*it)->GetObject<EnergySourceContainer>();
      if(! energySourceC) 
      {
        std::cout << "Node " << (*it)->GetId() << "has no energy source" << std::endl;
      } else {
        Ptr<EnergySource> energySource = energySourceC->Get(0);
        std::cout << "Node " << (*it)->GetId() << " Energy used overall: " << 18000 - energySource->GetRemainingEnergy() << std::endl;
      }
  } 

  std::cout << "later nodes start" << std::endl;
  for (NodeContainer::Iterator it = laterNodes.Begin (); it != laterNodes.End (); ++it) {
      Ptr<EnergySourceContainer> energySourceC = (*it)->GetObject<EnergySourceContainer>();
      if(! energySourceC) 
      {
        std::cout << "Node " << (*it)->GetId() << "has no energy source" << std::endl;
      } else {
        Ptr<EnergySource> energySource = energySourceC->Get(0);
        std::cout << "Node " << (*it)->GetId() << " Energy used overall: " << 18000 - energySource->GetRemainingEnergy() << std::endl;
      }
  } 
  //std::cout << nNodes << std::endl;
  std::cout << "starting destroy" << std::endl;
  Simulator::Destroy ();
  std::cout << "finishing destroy" << std::endl;
  return 0;
}
