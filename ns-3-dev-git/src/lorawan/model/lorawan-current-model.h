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

#ifndef LORAWAN_CURRENT_MODEL_H
#define LORAWAN_CURRENT_MODEL_H

#include "ns3/object.h"

namespace ns3 {

/**
 * \ingroup lorawan
 * 
 * \brief Encapsulate the current consumption of a particular LoRaWAN chip.
 *
 */
class LoRaWANCurrentModel : public Object
{
public:
  static TypeId GetTypeId (void);

  LoRaWANCurrentModel ();
  virtual ~LoRaWANCurrentModel ();

  virtual TypeId GetInstanceTypeId ();
  
  /**
   * \param txPowerDbm (dBm)
   *
   * Set the current consumption in the TX state, based on the TX Power.
   */
  virtual void SetTxCurrent (double txPowerDbm) = 0;
  
  /**
   * \return the current in the TX state, which is dependent on the current TX Power and choice of PA circuitry.
   */
  virtual double GetTxCurrent () const = 0;

  /**
   * \param bandwidth (Hz)
   *
   * Set the current consumption in the RX state, based on the bandwidth of the channel.
   */
  virtual void SetRxCurrent (double bandwidth) = 0;

  /**
   * \return the current in the RX state, which is dependent on the bandwidth of the channel and use of LnaBoost.
   */
  virtual double GetRxCurrent () const = 0;

  /**
   * \return the supply voltage.
   */
  double GetVoltage (void) const;

  /**
   * \param voltage (Volts)
   *
   * Set the supply voltage.
   */
  void SetVoltage (double voltage);

  /**
   * \param idleCurrent (Ampere)
   *
   * Set the current in the IDLE state.
   */
  void SetIdleCurrent (double idleCurrent);

  /**
   * \return the current in the IDLE state.
   */
  double GetIdleCurrent (void) const;
  

  /**
   * \param sleepCurrent (Ampere)
   *
   * Set the current in the SLEEP state.
   */
  void SetSleepCurrent (double sleepCurrent);

  /**
   * \return the current in the SLEEP state.
   */
  double GetSleepCurrent (void) const;

  

private:
  
  double m_voltage;
  double m_idleCurrent;
  double m_sleepCurrent;
  
};

// ------------------------------------------------------------------------- //

/**
 * \ingroup lorawan
 *
 * Models the current consumption of the SX1272.  
 * 
 * Default values for current consumption are based on measurements reported in:
 * 
 * Joseph Finnegan, Stephen Brown, Ronan Farrell,
 * "Modeling the Energy Consumption of LoRaWAN in ns-3 Based on Real World Measurements", Proceedings of GIIS'18 
 */



class SX1272LoRaWANCurrentModel : public LoRaWANCurrentModel
{
public:
  static TypeId GetTypeId (void);

  SX1272LoRaWANCurrentModel ();
  virtual ~SX1272LoRaWANCurrentModel ();


  /**
   * \param txPowerDbm (dBm)
   *
   * Set the current in the TX state of the model, based on the Tx power.
   */
  void SetTxCurrent (double txPowerDbm);

  /**
   * \param tx_current (Ampere)
   *
   * Set the current in the TX state of the model directly.
   */
  void SetTxCurrentDirectly(double tx_current);
  
  /**
   * \return the current in the TX state, which is dependent on the current TX Power and choice of PA circuitry.
   */
  double GetTxCurrent (void) const;


  /**
   * \param bandwidth (Hz)
   *
   *  Set the bandwidth in the RX state, which is dependent on the bandwidth of the channel.
   */
  void SetRxCurrent (double bandwidth);
  
  /**
   * \param rx_current (Ampere)
   *
   * Set the current in the RX state of the model directly.
   */
  void SetRxCurrentDirectly(double rx_current);

  /**
   * \return the current in the RX state, which is dependent on the bandwidth of the channel and use of LnaBoost.
   */
  double GetRxCurrent () const;


  /**
   * \param sleepCurrent (Ampere)
   *
   * Set PaBoost .
   */
  void SetPaBoost (bool paBoost);
 
  /**
   * \return the current in the RX state, which is dependent on the bandwidth of the channel and use of LnaBoost.
   */
  bool GetPaBoost (void) const;
  

  /**
   * \param sleepCurrent (Ampere)
   *
   * Set the current in the SLEEP state.
   */
  void SetLnaBoost (bool lnaBoost);

  /**
   * \return the current in the RX state, which is dependent on the bandwidth of the channel and use of LnaBoost.
   */
  bool GetLnaBoost (void) const;
  

private:

  const static double m_txPowerUsePaBoost[];
  const static double m_txPowerUseRfo[];

  const static double m_rxWithLnaBoost[];
  const static double m_rxNoLnaBoost[];

  double m_txCurrent; // current value of TX current consumption

  double m_rxCurrent; // current value of RX current consumption

  bool m_usePaBoost; // choice of whether to use PaBoost mode or not

  bool m_useLnaBoost; // choice of whether to use LnaBoost mode or not
  
  
};

} // namespace ns3

#endif /* LORAWAN_CURRENT_MODEL_H */

