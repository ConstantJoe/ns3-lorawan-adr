/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 National University of Ireland, Maynooth
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
 * Author: Joseph Finnegan <joseph.finnegan@mu.ie>
 */

#include "ns3/lorawan-radio-energy-model-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/lorawan-net-device.h"
#include "ns3/lorawan-phy.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/lorawan-current-model.h"

namespace ns3 {

LoRaWANRadioEnergyModelHelper::LoRaWANRadioEnergyModelHelper ()
{
  m_radioEnergy.SetTypeId ("ns3::LoRaWANRadioEnergyModel");
  m_depletionCallback.Nullify ();
  m_rechargedCallback.Nullify ();
}

LoRaWANRadioEnergyModelHelper::~LoRaWANRadioEnergyModelHelper ()
{
}

void
LoRaWANRadioEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_radioEnergy.Set (name, v);
}

void
LoRaWANRadioEnergyModelHelper::SetDepletionCallback (
  LoRaWANRadioEnergyModel::LoRaWANRadioEnergyDepletionCallback callback)
{
  m_depletionCallback = callback;
}

void
LoRaWANRadioEnergyModelHelper::SetRechargedCallback (
  LoRaWANRadioEnergyModel::LoRaWANRadioEnergyRechargedCallback callback)
{
  m_rechargedCallback = callback;
}

void
LoRaWANRadioEnergyModelHelper::SetCurrentModel (std::string currentModel)
{
  ObjectFactory factory;
  factory.SetTypeId (currentModel);
  m_currentModel = factory;
}


/*
 * Private function starts here.
 */

Ptr<DeviceEnergyModel>
LoRaWANRadioEnergyModelHelper::DoInstall (Ptr<NetDevice> device,
                                       Ptr<EnergySource> source) const
{
  NS_ASSERT (device != NULL);
  NS_ASSERT (source != NULL);

  // check if device is LoRaWANNetDevice
  std::string deviceName = device->GetInstanceTypeId ().GetName ();

  if (deviceName.compare ("ns3::LoRaWANNetDevice") != 0)
  {
    NS_FATAL_ERROR ("NetDevice type is not LoRaWANNetDevice!");
  }

  Ptr<Node> node = device->GetNode ();

  Ptr<LoRaWANRadioEnergyModel> model = m_radioEnergy.Create ()->GetObject<LoRaWANRadioEnergyModel> ();
  NS_ASSERT (model != NULL);

  // set energy source pointer
  model->SetEnergySource (source);
  

  Ptr<LoRaWANNetDevice> LoRaWANDevice = DynamicCast<LoRaWANNetDevice> (device);
  Ptr<LoRaWANPhy> LoRaWANPhy = LoRaWANDevice->GetPhy ();

  // TODO: leaving these commented out for now as current in the PHY model there is no distinction between TRX_OFF and (e.g.) DEVICE_DEAD
  // note that the states mentioned in the functions in the callbacks do not yet exist, are just an example of future planned structure.
  // set energy depletion callback
  // if none is specified, make a callback to WifiPhy::SetSleepMode
  /*if (m_depletionCallback.IsNull ())
  {
      model->SetEnergyDepletionCallback (MakeCallback (&LoRaWANPhy::ChangeTrxState(DEAD), LoRaWANPhy));
  }
  else
  {
      model->SetEnergyDepletionCallback (m_depletionCallback);
  }
  
  // set energy recharged callback
  // if none is specified, make a callback to WifiPhy::ResumeFromSleep
  if (m_rechargedCallback.IsNull ())
  {
      model->SetEnergyRechargedCallback (MakeCallback (&LoRaWANPhy::ChangeTrxState(TRANSMITTER_OFF), LoRaWANPhy));
  }
  else
  {
      model->SetEnergyRechargedCallback (m_rechargedCallback);
  }*/

  // add model to device model list in energy source
  source->AppendDeviceEnergyModel (model);

  // create and register energy model phy listener
  LoRaWANPhy -> TraceConnectWithoutContext ("TrxState",MakeCallback(&LoRaWANRadioEnergyModel::ChangeLoRaWANState, model));
  LoRaWANPhy -> TraceConnectWithoutContext ("TxPower",MakeCallback(&LoRaWANRadioEnergyModel::SetTxCurrentA, model));
  LoRaWANPhy -> TraceConnectWithoutContext ("BandwidthOfCurrentChannel",MakeCallback(&LoRaWANRadioEnergyModel::SetRxCurrentA, model));

  if (m_currentModel.GetTypeId ().GetUid ())
  { 
      Ptr<LoRaWANCurrentModel> current = m_currentModel.Create<SX1272LoRaWANCurrentModel> ();
      model->SetCurrentModel (current);
  }

  return model;
}

} // namespace ns3

