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

#include "lorawan-current-model.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANCurrentModel");

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (LoRaWANCurrentModel);

TypeId 
LoRaWANCurrentModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANCurrentModel")
    .SetParent<Object> ()
    .SetGroupName ("LoRaWAN")
  ;
  return tid;
}

LoRaWANCurrentModel::LoRaWANCurrentModel()
{
}

LoRaWANCurrentModel::~LoRaWANCurrentModel()
{
}

TypeId
LoRaWANCurrentModel::GetInstanceTypeId ()
{
  return GetTypeId();
}

void
LoRaWANCurrentModel::SetVoltage (double voltage)
{
  NS_LOG_FUNCTION (this << voltage);
  m_voltage = voltage;
}

double
LoRaWANCurrentModel::GetVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  return m_voltage;
}

void
LoRaWANCurrentModel::SetIdleCurrent (double idleCurrent)
{
  NS_LOG_FUNCTION (this << idleCurrent);
  m_idleCurrent = idleCurrent;
}

double
LoRaWANCurrentModel::GetIdleCurrent (void) const
{
  NS_LOG_FUNCTION (this);
  return m_idleCurrent;
}

void
LoRaWANCurrentModel::SetSleepCurrent (double sleepCurrent)
{
  NS_LOG_FUNCTION (this << sleepCurrent);
  m_sleepCurrent = sleepCurrent;
}

double
LoRaWANCurrentModel::GetSleepCurrent (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sleepCurrent;
}







// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (SX1272LoRaWANCurrentModel);

  /*Tx power is currently set prior to going into Tx mode, so there's no situation where Tx power changes during a send (and so no current calculations have to be done here)*/

  /*
    The max EIRP for each subband, according to the ETSI regulations, are as follows:

    subband | spectrum access | edge frequencies | max eirp | equivalent dBm
    g       | 1% or LBT AFA   | 865-868MHz       | 10mW     | 10 dBm
    g1      | 1% or LBT AFA   | 868-868.6MHz     | 25mW     | 14 dBm
    g2      | 0.1% or LBT AFA | 868.7-869.2MHz   | 25mW     | 14 dBm
    g3      | 10% or LBT AFA  | 869.4-869.65MHz  | 500mW    | 27 dBm
    g4      | No Requirement  | 867-870MHz       | 5mW      | 7  dBm
    g4      | 1% or LBT AFA   | 867-870MHz       | 25mW     | 14 dBm

    The TXPower levels in LoRaWAN, which are used as part of the ADR, are defined as relative to the Max EIRP of the subband of the channel used for transmission, and are as follows:

    TXPower | Configuration (EIRP)
    0       | Max EIRP
    1       | Max EIRP - 2dB
    2       | Max EIRP - 4dB 
    3       | Max EIRP - 6dB
    4       | Max EIRP - 8dB
    5       | Max EIRP - 10dB
    6       | Max EIRP - 12dB
    7       | Max EIRP - 14dB
    8-14    | RFU (Reserved For Use)
    15      | Defined in LoRaWAN

    so, for example, the TXPower levels for a channel deployed in the g1 subband (e.g. 868.1, one of the mandetory ones) would be 14, 12, 10, 8, 6, 4, 2, 0.
    and the TXPower levels for a channel deployed in the g3 band would be 27, 25, 23, 21, 19, 17, 15, 13.
  
    The last one ("defined in LoRaWAN") is described in section 5.3 of the protocol: The LinkADRReq command is used by the NS to change the data rate and tx power of an end device.
    The requested data rate and tx power are encoded together in a single byte (4 bits each). A value of 15 specifies that the end device is to keep its current parameter value
     (i.e. a TXpower of 15 means stay at whatever TXpower the device is at already).  

    But, as this is a physical model of the device we provide values for each TX power option on the SX1272, which is for the PA_BOOST circuit 2dBm to 17dBm and 20dBm, and on the RFO pin -1dBm to 15dBm.
    The actual TXPower levels in the table above are part of the LoRaWAN protocol, not LoRa, and so should be controlled and modeled in the MAC layer.
  */


const double SX1272LoRaWANCurrentModel::m_txPowerUsePaBoost[] = {0.032018, 0.033157, 0.034224, 0.035168, 0.036302, 
0.037481, 0.038711, 0.040310, 0.042289, 0.044276, 0.046755, 0.050334, 0.054216, 0.061582, 0.068982, 0.077138, 0.0, 0.0, 0.105454}; // 2dBm to 17dBm, 1dB steps. Then high power mode of 20dBm.

const double SX1272LoRaWANCurrentModel::m_txPowerUseRfo[] = {0.0};

const double SX1272LoRaWANCurrentModel::m_rxWithLnaBoost[] = {0.010803, 0.011607}; //125kHz, 250kHz
const double SX1272LoRaWANCurrentModel::m_rxNoLnaBoost[]   = {0.009877, 0.010694}; //125kHz, 250kHz

TypeId 
SX1272LoRaWANCurrentModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SX1272LoRaWANCurrentModel")
    .SetParent<LoRaWANCurrentModel> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<SX1272LoRaWANCurrentModel> ()
    .AddAttribute ("Voltage", "The supply voltage (in Volts).",
                   DoubleValue (3.3),
                   MakeDoubleAccessor (&LoRaWANCurrentModel::SetVoltage,
                                       &LoRaWANCurrentModel::GetVoltage),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("IdleCurrent", "The current in the standby state.",
                   DoubleValue (0.001664), 
                   MakeDoubleAccessor (&LoRaWANCurrentModel::SetIdleCurrent,
                                       &LoRaWANCurrentModel::GetIdleCurrent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SleepCurrent", "The current in the SLEEP state.",
                   DoubleValue (0.000001),
                   MakeDoubleAccessor (&LoRaWANCurrentModel::SetSleepCurrent,
                                       &LoRaWANCurrentModel::GetSleepCurrent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxCurrent", "The current in the TX state.",
                   DoubleValue (0.054216),
                   MakeDoubleAccessor (&SX1272LoRaWANCurrentModel::SetTxCurrentDirectly,
                                       &SX1272LoRaWANCurrentModel::GetTxCurrent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxCurrent", "The current in the RX state.",
                   DoubleValue (0.010803),
                   MakeDoubleAccessor (&SX1272LoRaWANCurrentModel::SetRxCurrentDirectly,
                                       &SX1272LoRaWANCurrentModel::GetRxCurrent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UsePaBoost", "Choice of use of PaBoost pin or RFO pin.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SX1272LoRaWANCurrentModel::SetPaBoost,
                                        &SX1272LoRaWANCurrentModel::GetPaBoost),
                   MakeBooleanChecker ())
    .AddAttribute ("UseLnaBoost", "Choice of use of LnaBoost mode.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SX1272LoRaWANCurrentModel::SetLnaBoost,
                                        &SX1272LoRaWANCurrentModel::GetLnaBoost),
                   MakeBooleanChecker ())
  ;
  return tid;
}

SX1272LoRaWANCurrentModel::SX1272LoRaWANCurrentModel ()
{
  NS_LOG_FUNCTION (this);
}

SX1272LoRaWANCurrentModel::~SX1272LoRaWANCurrentModel()
{
}


//TODO: link this in helper
void
SX1272LoRaWANCurrentModel::SetRxCurrent(double bandwidth)
{
  if(m_useLnaBoost)
  {
    if(bandwidth == 125000)
    {
      m_rxCurrent = m_rxWithLnaBoost[0];
    } 
    else if(bandwidth == 250000)
    {
      m_rxCurrent = m_rxWithLnaBoost[1];
    }
    else
    {
      NS_FATAL_ERROR ("SX1272LoRaWANCurrentModel:current values for bandwidth chosen not available");
    } 
  }
  else
  {
    if(bandwidth == 125000)
    {
      m_rxCurrent = m_rxNoLnaBoost[0];
    } 
    else if(bandwidth == 250000)
    {
      m_rxCurrent = m_rxNoLnaBoost[1];
    } 
    else
    {
      NS_FATAL_ERROR ("SX1272LoRaWANCurrentModel:current values for bandwidth chosen not available");
    } 
  } 
}

void
SX1272LoRaWANCurrentModel::SetRxCurrentDirectly(double rx_current)
{
  m_rxCurrent = rx_current;
}

double
SX1272LoRaWANCurrentModel::GetRxCurrent (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxCurrent;
}

void
SX1272LoRaWANCurrentModel::SetPaBoost (bool paBoost)
{
  NS_LOG_FUNCTION (this << paBoost);
  m_usePaBoost = paBoost;
}

bool
SX1272LoRaWANCurrentModel::GetPaBoost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_usePaBoost;
}

void
SX1272LoRaWANCurrentModel::SetLnaBoost (bool lnaBoost)
{
  NS_LOG_FUNCTION (this << lnaBoost);
  m_useLnaBoost = lnaBoost;
}

bool
SX1272LoRaWANCurrentModel::GetLnaBoost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_useLnaBoost;
}

void
SX1272LoRaWANCurrentModel::SetTxCurrent (double txPowerDbm)
{
  NS_LOG_FUNCTION (this << txPowerDbm);

  if(m_usePaBoost)
  {
    if(txPowerDbm < 2)
    {
      NS_LOG_LOGIC (this << " Chosen dBm of " << txPowerDbm << " is less than the SX1272 min of 2dBm, using 2dBm instead.");
      txPowerDbm = 2;
     
    }
    else if(txPowerDbm == 18 || txPowerDbm == 19)
    {
      NS_FATAL_ERROR ("SX1272LoRaWANCurrentModel:18dBm and 19dBm are not available on the SX1272.");
    }
    else if(txPowerDbm > 20)
    {
      NS_LOG_LOGIC (this << " Chosen dBm of " << txPowerDbm << " is higher than the SX1272 max of 20dBm, using 20dBm instead.");
      txPowerDbm = 20; 
    }
    
    int ind = (int) txPowerDbm;
    ind -= 2;

    m_txCurrent =  m_txPowerUsePaBoost[ind];  
    
  }
  else 
  {
    //TODO
    NS_FATAL_ERROR ("SX1272LoRaWANCurrentModel:values for RFO not yet defined");
  }
}

void
SX1272LoRaWANCurrentModel::SetTxCurrentDirectly(double tx_current)
{
  m_txCurrent = tx_current;
}

double
SX1272LoRaWANCurrentModel::GetTxCurrent (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txCurrent;
}

// ------------------------------------------------------------------------- //

} // namespace ns3

