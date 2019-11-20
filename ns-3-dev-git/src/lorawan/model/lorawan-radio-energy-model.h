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

#ifndef LORAWAN_RADIO_ENERGY_MODEL_H
#define LORAWAN_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"
#include "ns3/lorawan-phy.h"

namespace ns3 {

class LoRaWANCurrentModel;


// -------------------------------------------------------------------------- //

/**
 * \brief A LoRa radio energy model.
 * 
 * 6 states are defined for the radio: LORAWAN_PHY_TX_ON, LORAWAN_PHY_RX_ON, LORAWAN_PHY_IDLE, LORAWAN_PHY_TRX_OFF, LORAWAN_PHY_BUSY_RX, LORAWAN_PHY_BUSY_TX. Default state is
 * LORAWAN_PHY_TRX_OFF.
 *
 * Energy calculation: For each transaction, this model notifies EnergySource
 * object. The EnergySource object will query this model for the total current.
 * Then the EnergySource object uses the total current to calculate energy.
 *
 * Default values for power consumption are based on measurements reported in:
 * 
 * Joseph Finnegan, Stephen Brown, Ronan Farrell,
 * "Modeling the Energy Consumption of LoRaWAN in ns-3 Based on Real World Measurements", Proceedings of GIIS'18 
 * 
 * The dependence of the power consumption in the device based on extra parameters is achieved through a device-specific current model (for example, for the SX1272 in TX mode consumption is dependent on the nominal
 * transmit power and choice of PA circuit, and in RX mode on the use of LNA_Boost). See lorawan-current-model.h for details.
 *
 */

class LoRaWANRadioEnergyModel : public DeviceEnergyModel
{
public:
  /**
   * Callback type for energy depletion handling.
   */
  typedef Callback<void> LoRaWANRadioEnergyDepletionCallback;

  /**
   * Callback type for energy recharged handling.
   */
  typedef Callback<void> LoRaWANRadioEnergyRechargedCallback;

public:
  static TypeId GetTypeId (void);
  LoRaWANRadioEnergyModel ();
  virtual ~LoRaWANRadioEnergyModel ();

  /**
   * \brief Sets pointer to EnergySouce installed on node.
   *
   * \param source Pointer to EnergySource installed on node.
   *
   * Implements DeviceEnergyModel::SetEnergySource.
   */
  virtual void SetEnergySource (Ptr<EnergySource> source);

  /**
   * \returns Total energy consumption of the LoRa device.
   *
   * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
   */
  virtual double GetTotalEnergyConsumption (void) const;

  virtual TypeId GetInstanceTypeId ();

  // Setter & getters for state power consumption.
  double GetTxCurrentA (void) const;
  void SetTxCurrentA (double oldTxCurrentDbm, double newTxCurrentDbm);

  double GetRxCurrentA (void) const;
  void SetRxCurrentA (uint32_t oldBandwidth, uint32_t newBandwidth);

  double GetIdleCurrentA (void) const;
  void SetIdleCurrentA (double idleCurrentA);

  double GetSleepCurrentA (void) const;
  void SetSleepCurrentA (double sleepCurrentA);

  /**
   * \returns Current state.
   */
  LoRaWANPhyEnumeration GetCurrentState (void) const; //Note: type of this depends on LoRaWAN module used.

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy depletion handling.
   */
  void SetEnergyDepletionCallback (LoRaWANRadioEnergyDepletionCallback callback);

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy recharged handling.
   */
  void SetEnergyRechargedCallback (LoRaWANRadioEnergyRechargedCallback callback);

  /**
   * \param model the model containing the lora device current for each state.
   */
  void SetCurrentModel (Ptr<LoRaWANCurrentModel> model);

  
  virtual void ChangeState (int newState);
  
  /**
   * \brief Changes state of the LoRaWANRadioEnergyModel.
   *
   * \param newState New state the LoRa radio is in.
   *
   * Note: doesn't implement DeviceEnergyModel::ChangeState because of use of phy-layer type.
   */
  virtual void ChangeLoRaWANState (LoRaWANPhyEnumeration oldState, LoRaWANPhyEnumeration newState);

  /**
   * \brief Handles energy depletion.
   *
   * Implements DeviceEnergyModel::HandleEnergyDepletion
   */
  virtual void HandleEnergyDepletion (void);

  /**
   * \brief Handles energy recharged.
   *
   * Implements DeviceEnergyModel::HandleEnergyRecharged
   */
  virtual void HandleEnergyRecharged (void);

private:
  void DoDispose (void);

  /**
   * \returns Current draw of device, at current state.
   *
   * Implements DeviceEnergyModel::GetCurrentA.
   */
  virtual double DoGetCurrentA (void) const;

  /**
   * \param state New state the radio device is currently in.
   *
   * Sets current state. This function is private so that only the energy model
   * can change its own state.
   */
  void SetLoRaWANRadioState (const LoRaWANPhyEnumeration state);

private:
  Ptr<EnergySource> m_source;

  // Model containing the exact current consumption values for the set device
  Ptr<LoRaWANCurrentModel> m_currentModel;

  // This variable keeps track of the total energy consumed by this model.
  TracedValue<double> m_totalEnergyConsumption;

  // State variables.
  LoRaWANPhyEnumeration m_currentState;  // current state the radio is in

  Time m_lastUpdateTime;          // time stamp of previous energy update

  // Energy depletion callback
  LoRaWANRadioEnergyDepletionCallback m_energyDepletionCallback;

  // Energy recharged callback
  LoRaWANRadioEnergyRechargedCallback m_energyRechargedCallback;
};

} // namespace ns3

#endif /* LORAWAN_RADIO_ENERGY_MODEL_H */

