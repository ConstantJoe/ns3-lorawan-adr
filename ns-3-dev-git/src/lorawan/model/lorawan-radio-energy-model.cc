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

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"
#include "lorawan-radio-energy-model.h"
#include "lorawan-current-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANRadioEnergyModel);

TypeId
LoRaWANRadioEnergyModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANRadioEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANRadioEnergyModel> ()
    .AddAttribute ("TxCurrentModel", "A pointer to the attached tx current model.",
                   PointerValue (),
                   MakePointerAccessor (&LoRaWANRadioEnergyModel::m_currentModel),
                   MakePointerChecker<LoRaWANCurrentModel> ())
    .AddTraceSource ("TotalEnergyConsumption",
                     "Total energy consumption of the radio device.",
                     MakeTraceSourceAccessor (&LoRaWANRadioEnergyModel::m_totalEnergyConsumption),
                     "ns3::TracedValueCallback::Double")
  ; 
  return tid;
}



LoRaWANRadioEnergyModel::LoRaWANRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_currentState = LoRaWANPhyEnumeration::LORAWAN_PHY_TRX_OFF;  // initially LORAWAN_PHY_TRX_OFF; same as in lorawan-phy
  m_currentModel = NULL;
  m_lastUpdateTime = Seconds (0.0);
  m_energyDepletionCallback.Nullify ();
  m_source = NULL;
}

LoRaWANRadioEnergyModel::~LoRaWANRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LoRaWANRadioEnergyModel::GetInstanceTypeId ()
{
  return GetTypeId();
}

void
LoRaWANRadioEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
}

double
LoRaWANRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);
  return m_totalEnergyConsumption;
}

double
LoRaWANRadioEnergyModel::GetIdleCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentModel->GetIdleCurrent();
}

void
LoRaWANRadioEnergyModel::SetIdleCurrentA (double idleCurrentA)
{
  NS_LOG_FUNCTION (this << idleCurrentA);
  m_currentModel->SetIdleCurrent(idleCurrentA);
}


double
LoRaWANRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentModel->GetTxCurrent();
}

void
LoRaWANRadioEnergyModel::SetTxCurrentA (double oldTxCurrentDbm, double newTxCurrentDbm)
{
  NS_LOG_FUNCTION (this << newTxCurrentDbm);
  m_currentModel->SetTxCurrent(newTxCurrentDbm);
}

double
LoRaWANRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentModel->GetRxCurrent();
}

void
LoRaWANRadioEnergyModel::SetRxCurrentA (uint32_t oldBandwidth, uint32_t newBandwidth)
{
  NS_LOG_FUNCTION (this << newBandwidth);
  m_currentModel->SetRxCurrent(newBandwidth);
}


double
LoRaWANRadioEnergyModel::GetSleepCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentModel->GetSleepCurrent();
}

void
LoRaWANRadioEnergyModel::SetSleepCurrentA (double sleepCurrentA)
{
  NS_LOG_FUNCTION (this << sleepCurrentA);
  m_currentModel->SetSleepCurrent(sleepCurrentA);
}

LoRaWANPhyEnumeration
LoRaWANRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentState;
}

void
LoRaWANRadioEnergyModel::SetEnergyDepletionCallback (
  LoRaWANRadioEnergyDepletionCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LoRaWANRadioEnergyModel:Setting NULL energy depletion callback!");
    }
  m_energyDepletionCallback = callback;
}

void
LoRaWANRadioEnergyModel::SetEnergyRechargedCallback (
  LoRaWANRadioEnergyRechargedCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LoRaWANRadioEnergyModel:Setting NULL energy recharged callback!");
    }
  m_energyRechargedCallback = callback;
}

void
LoRaWANRadioEnergyModel::SetCurrentModel (Ptr<LoRaWANCurrentModel> model)
{
  m_currentModel = model;
}

void
LoRaWANRadioEnergyModel::ChangeState (int state) 
{ 
}

void
LoRaWANRadioEnergyModel::ChangeLoRaWANState (LoRaWANPhyEnumeration oldState, LoRaWANPhyEnumeration newState)
{
  NS_LOG_FUNCTION (this << newState);

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.GetNanoSeconds () >= 0); // check if duration is valid

  // energy to decrease = current * voltage * time
  double energyToDecrease = 0.0;
  double supplyVoltage = m_source->GetSupplyVoltage ();

  switch (m_currentState)
    {
    case LoRaWANPhyEnumeration::LORAWAN_PHY_TRX_OFF:
      energyToDecrease = duration.GetSeconds () * GetSleepCurrentA() * supplyVoltage;
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_IDLE:
      energyToDecrease = duration.GetSeconds () * GetIdleCurrentA() * supplyVoltage;
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_RX_ON:
      energyToDecrease = duration.GetSeconds () * GetRxCurrentA() * supplyVoltage;
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_TX_ON:
      energyToDecrease = duration.GetSeconds () * GetTxCurrentA() * supplyVoltage;
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_BUSY_RX:
      energyToDecrease = duration.GetSeconds () * GetRxCurrentA() * supplyVoltage;
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_BUSY_TX:
      energyToDecrease = duration.GetSeconds () * GetTxCurrentA() * supplyVoltage;
      break;
    default:
      NS_FATAL_ERROR ("LoRaWANRadioEnergyModel:Undefined radio state: " << m_currentState);
    }

  // update total energy consumption
  m_totalEnergyConsumption += energyToDecrease;

  // update last update time stamp
  m_lastUpdateTime = Simulator::Now ();

  // notify energy source
  m_source->UpdateEnergySource ();


  SetLoRaWANRadioState (newState);

  NS_LOG_DEBUG ("LoRaWANRadioEnergyModel:Total energy consumption is " << m_totalEnergyConsumption << "J");
}

void
LoRaWANRadioEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoRaWANRadioEnergyModel:Energy is depleted!");
  // invoke energy depletion callback, if set.
  if (!m_energyDepletionCallback.IsNull ())
    {
      m_energyDepletionCallback ();
    }
}

void
LoRaWANRadioEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoRaWANRadioEnergyModel:Energy is recharged!");
  // invoke energy recharged callback, if set.
  if (!m_energyRechargedCallback.IsNull ())
    {
      m_energyRechargedCallback ();
    }
}


/*
 * Private functions start here.
 */

void
LoRaWANRadioEnergyModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_source = NULL;
  m_energyDepletionCallback.Nullify ();
}

double
LoRaWANRadioEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);

   switch (m_currentState)
    {
    case LoRaWANPhyEnumeration::LORAWAN_PHY_TRX_OFF:
      return GetSleepCurrentA();
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_IDLE:
      return GetIdleCurrentA();
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_RX_ON:
      return GetRxCurrentA();
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_TX_ON:
      return GetTxCurrentA();
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_BUSY_RX:
      return GetRxCurrentA();
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_BUSY_TX:
      return GetTxCurrentA();
      break;
    default:
      NS_FATAL_ERROR ("LoRaWANRadioEnergyModel:Undefined radio state: " << m_currentState);
    }
}

void
LoRaWANRadioEnergyModel::SetLoRaWANRadioState (const LoRaWANPhyEnumeration state)
{
  NS_LOG_FUNCTION (this << state);
  m_currentState = state;
  std::string stateName;

  switch (m_currentState)
    {
    case LoRaWANPhyEnumeration::LORAWAN_PHY_TRX_OFF:
      stateName = "SLEEP";
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_IDLE:
      stateName = "IDLE";
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_RX_ON:
      stateName = "RX_ON";
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_TX_ON:
      stateName = "TX_ON";
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_BUSY_RX:
      stateName = "RX_BUSY";
      break;
    case LoRaWANPhyEnumeration::LORAWAN_PHY_BUSY_TX:
      stateName = "TX_BUSY";
      break;
    default:
      NS_FATAL_ERROR ("LoRaWANRadioEnergyModel:Undefined radio state: " << m_currentState);
    }
  NS_LOG_DEBUG ("LoRaWANRadioEnergyModel:Switching to state: " << stateName <<
                " at time = " << Simulator::Now ());
}

} // namespace ns3

