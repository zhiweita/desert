//
// Copyright (c) 2017 Regents of the SIGNET lab, University of Padova.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Padova (SIGNET lab) nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * @file   uwpolling_AUV.h
 * @author Federico Favaro
 * @version 2.0.0
 *
 * \brief Class that represents the AUV of UWPOLLING
 *
 */

#ifndef UWPOLLING_HDR_AUV_H
#define UWPOLLING_HDR_AUV_H

#include "uwpolling_cmn_hdr.h"

#include <mmac.h>
#include <mphy.h>
#include <clmessage.h>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <fstream>
#include <ostream>
#include <chrono>
#include "uwsmposition.h"

#define UWPOLLING_AUV_DROP_REASON_ERROR "DERR" /**< Packet corrupted */
#define UWPOLLING_AUV_DROP_REASON_UNKNOWN_TYPE \
	"DUT" /**< Packet of unknown type */
#define UWPOLLING_AUV_DROP_REASON_WRONG_RECEIVER \
	"DWR" /**< Packet for another node */
#define UWPOLLING_AUV_DROP_REASON_WRONG_STATE \
	"DWS" /**< The AUV cannot receive this kind of packet in this state */
#define UWPOLLING_AUV_DROP_REASON_BUFFER_FULL "ADBF"

#define ENTRY_MAX_SIZE 256

/** 
 * Struct used for handling the number of probes detected and received to estimate 
 * the number of neighbors.
**/
typedef struct probe_cicle_counters {
	uint n_probe_detected; /**< Number of probe (prehamble) detected in a round by the MAC*/
	uint n_probe_received; /**< Number of probe received in a round (i.e., prehamble received)*/
	/**
	 * Increment both counters 
	**/
	inline void incrementCounters() {n_probe_detected++; n_probe_received++;}
	/**
	 * Reset both counters 
	**/
	inline void resetCounters() {n_probe_detected = 0; n_probe_received = 0;}
	/**
	 * Get estimate number of neighbors
	**/
	inline uint getNumberOfNeighbors() {
		return n_probe_detected + (n_probe_detected-n_probe_received);}
};

/**
 * Internal structure where the AUV store the informations about
 * the node to POLL
 */
typedef struct probbed_node {
	uint id_node; /**< Id of the node **/
	int
			n_pkts; /**< Number of packets that the node transmit when he's
					   polled. */
	int mac_address; /**< Mac Address of the node */
	double time_stamp; /**< Timestamp of the most recent data packet generated
						  by the node */
	//double backoff_time; /**< Backoff time chosen by the node before
						//	transmitting the PROBE */
	double
			Tmeasured; /**< Time elapsed between the transmisssion of the
						  TRIGGER and the reception of the PROBE */
	bool is_sink_; /**< Ture if the probe comes from a sink */

	uint16_t id_ack; /**< Id used for ack purpose is the proebbed node is a sink */

	double policy_weight; /**< Weigth used to choose the order to poll 
				the nodes. The higher weight, the higher priority */

	/**
	 * Reference to the time_stamp variable
	 */
	inline double &
	get_time_stamp()
	{
		return (time_stamp);
	}

	/**
	 * Reference to the id_node variable
	 */
	inline uint &
	get_id_node()
	{
		return (id_node);
	}

	/**
	 * Reference to the n_pkts variable
	 */
	inline int &
	get_n_pkts()
	{
		return (n_pkts);
	}

	/**
	 * Reference to the mac_address variable
	 */
	inline int &
	get_mac_address()
	{
		return (mac_address);
	}

	/**
	 * Reference to the is_sink variable
	 */
	inline bool &
	is_sink()
	{
		return is_sink_;
	}

	/**
	 * Reference to the id_ack variable
	 */
	inline uint16_t &
	get_id_ack()
	{
		return id_ack;
	}

	/**
	 * Reference to the policy_weight variable
	 */
	inline double &
	get_weight()
	{
		return policy_weight;
	}

} probbed_node;

/**
 * Class used to represent the UWPOLLING MAC layer of the AUV
 */
class Uwpolling_AUV : public MMac
{
public:
	/**
	 * Constructor of the Uwpolling_AUV class
	 */
	Uwpolling_AUV();
	/**
	 * Destructor of the Uwpolling_AUV class
	 */
	virtual ~Uwpolling_AUV();

	/**
	 * TCL command interpreter. It implements the following OTcl methods:
	 *
	 * @param argc Number of arguments in <i>argv</i>.
	 * @param argv Array of strings which are the command parameters (Note that
	 *<i>argv[0]</i> is the name of the object).
	 * @return TCL_OK or TCL_ERROR whether the command has been dispatched
	 *successfully or not.
	 *
	 **/
	virtual int command(int argc, const char *const *argv);
	/**
	 * Cross-Layer messages interpreter
	 *
	 * @param ClMessage* an instance of ClMessage that represent the message
	 * received
	 * @return <i>0</i> if successful.
	 */
	virtual int crLayCommand(ClMessage *m);

protected:
	/***********************************
	 | Internal variable and functions |
	 ***********************************/

	/**< Variable that rapresent the status of the protocol machine state */
	enum UWPOLLING_AUV_STATUS {
		UWPOLLING_AUV_STATUS_IDLE = 1,
		UWPOLLING_AUV_STATUS_TX_TRIGGER,
		UWPOLLING_AUV_STATUS_RX_PROBES,
		UWPOLLING_AUV_STATUS_RX_DATA,
		UWPOLLING_AUV_STATUS_RX_ACK,
		UWPOLLING_AUV_STATUS_TX_POLL,
		UWPOLLING_AUV_STATUS_TX_DATA,
		UWPOLLING_AUV_STATUS_WAIT_PROBE,
		UWPOLLING_AUV_STATUS_WAIT_DATA,
		UWPOLLING_AUV_STATUS_WAIT_ACK
	};

	/**< Reason for the changing of the state */
	enum UWPOLLING_AUV_REASON {
		UWPOLLING_AUV_REASON_DATA_RX = 1,
		UWPOLLING_AUV_REASON_TX_TRIGGER,
		UWPOLLING_AUV_REASON_TX_POLL,
		UWPOLLING_AUV_REASON_PROBE_RECEIVED,
		UWPOLLING_AUV_REASON_RX_DATA_TO,
		UWPOLLING_AUV_REASON_PACKET_ERROR,
		UWPOLLING_AUV_REASON_LAST_PACKET_RECEIVED,
		UWPOLLING_AUV_REASON_MAX_PROBE_RECEIVED,
		UWPOLLING_AUV_REASON_LAST_POLLED_NODE,
		UWPOLLING_AUV_REASON_PROBE_TO_EXPIRED
	};

	/**< Type of the packet */
	enum UWPOLLING_PKT_TYPE {
		UWPOLLING_DATA_PKT = 1,
		UWPOLLING_POLL_PKT,
		UWPOLLING_TRIGGER_PKT,
		UWPOLLING_PROBE_PKT
	};

	/**< Status of the timer */
	enum UWPOLLING_TIMER_STATUS {
		UWPOLLING_IDLE = 1,
		UWPOLLING_RUNNING,
		UWPOLLING_FROZEN,
		UWPOLLING_EXPIRED
	};

	/**
	 * Class that describes the timer in the AUV
	 */
	class Uwpolling_AUV_Timer : public TimerHandler
	{
	public:
		/**
		 * Constructor of the Uwpolling_AUV_Timer class
		 * @param Uwpolling_AUV* a pointer to an object of type Uwpolling_AUV
		 */
		Uwpolling_AUV_Timer(Uwpolling_AUV *m)
			: TimerHandler()
			, start_time(0.0)
			, left_duration(0.0)
			, counter(0)
			, module(m)
			, timer_status(UWPOLLING_IDLE)
		{
			assert(m != NULL);
		}

		/**
		 * Destructor of the Uwpolling_AUV_Timer class
		 */
		virtual ~Uwpolling_AUV_Timer()
		{
		}

		/**
		 * Freeze the timer
		 */
		virtual void
		freeze()
		{
			assert(timer_status == UWPOLLING_RUNNING);
			left_duration -= (NOW - start_time);
			if (left_duration <= 0.0) {
				left_duration = module->mac2phy_delay_;
			}
			force_cancel();
			timer_status = UWPOLLING_FROZEN;
		}

		/**
		 * unFreeze is used to resume the timer starting from the point where it
		 * was freezed
		 */
		virtual void
		unFreeze()
		{
			assert(timer_status == UWPOLLING_FROZEN);
			start_time = NOW;
			assert(left_duration > 0);
			sched(left_duration);
			timer_status = UWPOLLING_RUNNING;
		}

		/**
		 * stops the timer
		 */
		virtual void
		stop()
		{
			timer_status = UWPOLLING_IDLE;
			force_cancel();
		}

		/**
		 * Schedules a timer
		 * @param double the duration of the timer
		 */
		virtual void
		schedule(double val)
		{
			start_time = NOW;
			left_duration = val;
			timer_status = UWPOLLING_RUNNING;
			resched(val);
		}

		/**
		 * Checks if the timer is IDLE
		 * @return bool <i>true</i> or <i>false</i>
		 */
		bool
		isIdle()
		{
			return (timer_status == UWPOLLING_IDLE);
		}

		/**
		 * Checks if the timer is RUNNING
		 * @return <i>true</i> or <i>false</i>
		 */
		bool
		isRunning()
		{
			return (timer_status == UWPOLLING_RUNNING);
		}

		/**
		 * Checks if the timer is EXPIRED
		 * @return <i>true</i> or <i>false</i>
		 */
		bool
		isExpired()
		{
			return (timer_status == UWPOLLING_EXPIRED);
		}

		/**
		 * Checks if the timer is FROZEN
		 * @return <i>true</i> or <i>false</i>
		 */
		bool
		isFrozen()
		{
			return (timer_status == UWPOLLING_FROZEN);
		}

		/**
		 * Checks if the timer is ACTIVE
		 * @return <i>true</i> or <i>false</i>
		 */
		bool
		isActive()
		{
			return (timer_status == UWPOLLING_FROZEN ||
					timer_status == UWPOLLING_RUNNING);
		}

		/**
		 * Resets the counter of the timer
		 */
		void
		resetCounter()
		{
			counter = 0;
		}

		/**
		 * Increments the counter of the timer
		 */
		void
		incrCounter()
		{
			++counter;
		}

		/**
		 * Returns the counter of the timer
		 * @return the value of the counter of the timer
		 */
		int
		getCounter()
		{
			return counter;
		}

		/**
		 * Returns the left duration of the timer
		 * @return the value of the counter of the timer
		 */
		double
		getDuration()
		{
			return left_duration;
		}

	protected:
		double start_time; /**< Start Time of the timer */
		double left_duration; /**< Left duration of the timer */
		int counter; /**< counter of the timer */
		Uwpolling_AUV
				*module; /**< Pointer to an object of type Uwpolling_AUV */
		UWPOLLING_TIMER_STATUS timer_status; /**< Timer status */
	};

	/**
	 * Class (inherited from Uwpolling_AUV_Timer) used to handle the timer of
	 * data packets
	 * When the AUV give the POLL packet to a node, he set up the timer in which
	 * the node has to transmit
	 * his packets. The duration of the timer is calculated based on the RTT
	 * between the AUV and the node and
	 * the duration of the transmission of a packet
	 */
	class DataTimer : public Uwpolling_AUV_Timer
	{
	public:
		/**
		 * Conscructor of DataTimer class
		 * @param Uwpolling_AUV* pointer to an object of type Uwpolling_AUV
		 */
		DataTimer(Uwpolling_AUV *m)
			: Uwpolling_AUV_Timer(m)
		{
		}

		/**
		 * Destructor of DataTimer class
		 */
		virtual ~DataTimer()
		{
		}

	protected:
		/**
		 * Method called when the timer expire
		 * @param Event*  pointer to an object of type Event
		 */
		virtual void expire(Event *e);
	};

	/**
	 * Class (inherited from Uwpolling_AUV_Timer) used to handle the Probe
	 * Timer. The duration of
	 * this timer is the time in which the nodes can transmit their PROBE
	 * packets to "book" the possibility
	 * to transmit data packets to the AUV */
	class ProbeTimer : public Uwpolling_AUV_Timer
	{
	public:
		/**
		 * Conscructor of ProbeTimer class
		 */
		ProbeTimer(Uwpolling_AUV *m)
			: Uwpolling_AUV_Timer(m)
		{
		}

		/**
		 * Destructor of ProbeTimer class
		 * @param Uwpolling_AUV* Pointer of an object of type Uwpolling_AUV
		 */
		virtual ~ProbeTimer()
		{
		}

	protected:
		/**
		 * Method call when the timer expire
		 * @param Event* pointer to an object of type Event
		 */
		virtual void expire(Event *e);
	};

	class PollTimer : public Uwpolling_AUV_Timer
	{
	public:
		/**
		 * Conscructor of ProbeTimer class
		 */
		PollTimer(Uwpolling_AUV *m)
			: Uwpolling_AUV_Timer(m)
		{
		}

		/**
		 * Destructor of ProbeTimer class
		 * @param Uwpolling_AUV* Pointer of an object of type Uwpolling_AUV
		 */
		virtual ~PollTimer()
		{
		}

	protected:
		/**
		 * Method call when the timer expire
		 * @param Event* pointer to an object of type Event
		 */
		virtual void expire(Event *e);
	};

	class AckTimer : public Uwpolling_AUV_Timer
	{
	public:
		/**
		 * Conscructor of AckTimer class
		 */
		AckTimer(Uwpolling_AUV *m)
			: Uwpolling_AUV_Timer(m)
		{
		}

		/**
		 * Destructor of AckTimer class
		 * @param Uwpolling_AUV* Pointer of an object of type Uwpolling_AUV
		 */
		virtual ~AckTimer()
		{
		}

	protected:
		/**
		 * Method call when the timer expire
		 * @param Event* pointer to an object of type Event
		 */
		virtual void expire(Event *e);
	};

	
	/**
	 * Reference to the backoff_time variable
	 */
	double getMaxBackoffTime();

	/**
	 * Calculate the linear interpolation between two 2-D points
	 *
	 * @param x x-coordinate of which we need to finde the value.
	 * @param x1 x-coordinate of the first point
 	 * @param x2 x-coordinate of the second point
	 * @param y1 y-coordinate of the first point
	 * @param y2 y-coordinate of the second point
	 * @return the value assumed by y obtained by linear interpolation
	 */
	virtual double linearInterpolator(
			double x, double x1, double x2, double y1, double y2);

	
	/**
	 * Pass the packet to the PHY layer
	 * @param Event* Pointer to an object of type Packet that rapresent the
	 * Packet to transmit
	 */
	virtual void Mac2PhyStartTx(Packet *p);
	
	/**
	 * Method called when the PHY layer finish to transmit the packet.
	 * @param Packet* Pointer to an object of type Packet that rapresent the
	 * Packet transmitted
	 */
	virtual void Phy2MacEndTx(const Packet *p);
	/**
	 * Method called when the Phy Layer start to receive a Packet
	 * @param const Packet* Pointer to an object of type Packet that rapresent
	 * the Packet that is in reception
	 */
	virtual void Phy2MacStartRx(const Packet *p);
	/**
	 * Method called when the Phy Layer finish to receive a Packet
	 * @param const Packet* Pointer to an object of type Packet that rapresent
	 * the packet received
	 */
	virtual void Phy2MacEndRx(Packet *p);

	/**
	 * Increases the number of TRIGGER packets sent. Used for statistical
	 * purposes
	 * @param Packet*
	 */
	inline void
	incrTriggerTx()
	{
		n_trigger_tx++;
	}

	/**
	 * Increases the number of PROBE packets received. Used for statistical
	 * purposes
	 */
	inline void
	incrProbeRx()
	{
		n_probe_rx++;
	}

	/**
	 * Increases the number of Ack packets received. Used for statistical
	 * purposes
	 */
	inline void
	incrAckRx()
	{
		n_ack_rx++;
	}

	/**
	 * Increases the number of DATA packets received from node that are not
	 * POLLED anymore (i.e. they transmit a data packet when the DataTimer is
	 * expired.
	 * Used for statistical purposes.
	 */
	inline void
	incrWrongNodeDataSent()
	{
		wrong_node_data_sent++;
	}

	/**
	 * Increases the number of wrong PROBE packet received. 
	 * Used for statistical purposes
	 */
	inline void
	incrDroppedProbePkts()
	{
		N_dropped_probe_pkts++;
	}

	/**
	 * Increases the number of wrong ACK packet received. 
	 * Used for statistical purposes
	 */
	inline void
	incrDroppedAckPkts()
	{
		n_dropped_ack_pkts++;
	}

	/**
	 * Increase the number of PROBEs packets dropped because the AUV was not
	 * in the RX_PROBE state. This probes are sent from sensors after the
	 * maximum time
	 * allowed for PROBE transmission.
	 */
	inline void
	incrDroppedProbeWrongState()
	{
		N_dropped_probe_wrong_state++;
	}

	/**
	 * Incrase the number of POLL transmitted
	 */
	inline void
	incrPollTx()
	{
		n_poll_tx++;
	}

	/**
	 * Returns the number of TRIGGER sent during the simulation
	 * @return int n_trigger_tx the number of TRIGGER packet sent
	 */
	inline int
	getTriggerTx()
	{
		return n_trigger_tx;
	}

	/**
	 * Returns the number of PROBE received during the simulation
	 * @return int n_probe_rx the number of PROBE received
	 */
	inline int
	getProbeRx()
	{
		return n_probe_rx;
	}

	/**
	 * Returns the number of ACK received during the simulation
	 * @return int n_probe_rx the number of PROBE received
	 */
	inline int
	getAckRx()
	{
		return n_ack_rx;
	}

	/**
	 * Returns the number of Data Packet sent by Nodes not polled
	 * (used for debug purposes)
	 * @return int wrong_node_data_sent
	 */
	inline int
	getWrongNodeDataSent()
	{
		return wrong_node_data_sent;
	}

	/**
	 * Return the number of POLL packets sent during the simulation
	 * @return int n_poll_tx number of POLL packets sent during the simulation
	 */
	inline int
	getPollSent()
	{
		return n_poll_tx;
	}

	/**
	 * Return the number of PROBE packets discarded because of wrong CRC
	 * @return int N_dropped_probe_pkts number of PROBE pkts dropped
	 */
	inline int
	getDroppedProbePkts()
	{
		return N_dropped_probe_pkts;
	}

	/**
	 * Return the number of PROBE packets discarded because of wrong CRC
	 * @return int N_dropped_probe_pkts number of PROBE pkts dropped
	 */
	inline int
	getDroppedAckPkts()
	{
		return n_dropped_ack_pkts;
	}

	/**
	 * Return the number of PROBE packets discarded because sent after the
	 * maximum time allowed
	 * @return int N_dropped_probe_wrong_state number of PROBE packets discarded
	 * because sent after the
	 * maximum time allowed
	 */
	inline int
	getDroppedProbeWrongState()
	{
		return N_dropped_probe_wrong_state;
	}

	/**
	 * Refresh the reason for the changing of the state
	 * @param UWPOLLING_AUV_REASON The reason of the change of the state
	 */
	virtual void
	refreshReason(UWPOLLING_AUV_REASON reason)
	{
		last_reason = reason;
	}

	/**
	 * Refresh the state of the protocol
	 * @param UWPOLLING_AUV_STATUS current state of the protcol
	 */
	virtual void
	refreshState(UWPOLLING_AUV_STATUS state)
	{
		prev_state = curr_state;
		curr_state = state;
	}
	/**
	 * State of the protocol in which there's a reception of a PROBE packet
	 * @param UWPOLLING_AUV_STATUS state
	 */
	virtual void stateRxProbe();
	/**
	 * State of the protocol in which there's a reception of a DATA packet
	 */
	virtual void stateRxData();
	/**
	 * State of the protocol in which the PROBE timer is set up.
	 */
	virtual void stateWaitProbe();
	/**
	 * State of the protocol in which the DATA timer is set up
	 */
	virtual void stateWaitData();
	/**
	 * IDLE state. Each variable is resetted
	 */
	virtual void stateIdle();
	/**
	 * State of the protocol in which a TRIGGER packet is initialized.
	 */
	virtual void stateTxTrigger();
	/**
	 * State of the protocol in which a POLL packet is initialized
	 */
	virtual void stateTxPoll();

	/**
	 * DATA TIMER is Expired. In this method the reception of DATA is disabled.
	 */
	virtual void DataTOExpired();
	/**
	 * PROBE TIMER is Expired. In this method the reception of PROBE is disabled
	 */
	virtual void ProbeTOExpired();
	/**
	 * Prints a file with every state change for debug purposes.
	 * @param double delay
	 */
	// virtual void printStateInfo(double delay = 0);
	/**
	 * Initializes the protocol at the beginning of the simulation. This method
	 * is called by
	 * a command in tcl.
	 * @param double delay
	 * @see command method
	 */
	virtual void initInfo();
	/**
	 * Transmission of the TRIGGER packet
	 */
	virtual void TxTrigger();
	/**
	 * Transmisssion of the POLL packet
	 */
	virtual void TxPoll();
	/**
	 * Method in which the node that has sent correctly the PROBE packet is
	 * sorted in order to
	 * give the priority to the nodes that has the most recent data packet.
	 */
	virtual void SortNode2Poll();
	/**
	 * The first element of the list of nodes is trimmed and the next node is
	 * polled.
	 */
	virtual void ChangeNodePolled();
	/**
	 * Calculation of DATA TIMER value based on the number of packet that a node
	 * need to transmit,
	 * the packet time and the RTT between the node and the AUV
	 * @return the duration of the timer.
	 */
	virtual double GetDataTimerValue();
	/**
	 * Compute the Transmission Time for various type of packet using a
	 * CrossLayer Message
	 * to ask the PHY to compute the transmission time
	 * @param UWPOLLING_PKT_TYPE the type of the packet
	 * @return the transmission time
	 */
	virtual void computeTxTime(UWPOLLING_PKT_TYPE pkt);
	/**
	 * Update the RTT between a node and the AUV based on the time elapsed
	 * between the transmisison
	 * of the TRIGGER and the reception of the PROBE
	 */
	virtual void UpdateRTT();
	/**
	 * Informs the protocol to stop counting the time in which the AUV receive
	 * DATA packets from the nodes
	 * It is used to compute the throughput at MAC layer.
	 */
	virtual void stop_count_time();

	/**
	 * Permits to retrieve the total time in which the AUV has received DATA
	 * packets.
	 * @return the amount of time
	 */
	inline double
	GetTotalReceivingTime()
	{
		return total_time;
	}
	/**
	 * Used for debug purposes. (Permit to have a "step by step" behaviour of
	 * the protocol)
	 */
	virtual void waitForUser();
	/**
	 * Calculate the epoch of the event. Used in sea-trial mode
	 * @return the epoch of the system
	 */
	inline unsigned long int
	getEpoch()
	{
	  unsigned long int timestamp =
		  (unsigned long int) (std::chrono::duration_cast<std::chrono::milliseconds>(
			  std::chrono::system_clock::now().time_since_epoch()).count() );
	  return timestamp;
	}

	/** 
   	 * Handle a packet coming from upper layers
   	 * 
   	 * @param p pointer to the packet
   	 */
	virtual void recvFromUpperLayers(Packet *p);

	/**
	 * Handle the transmission after poll reception. Decide if transmit data to 
	 * the sink or to trnasmit poll to other nodes in the list
	 */
	virtual void stateTx();

	/**
	 * Handle the data transmission after poll reception. 
	 */
	virtual void stateTxData();

	/**
	 * Transmit data transmission after poll reception. 
	 */
	virtual void txData();

	/**
	 * State of the protcol in which the ACK timer is set up
	 */
	virtual void stateWaitAck();

	/**
	 * ACK TIMER is Expired. In this method the reception of ACK is disabled and
	 * the protocol is moved to StateIdle
	 */
	virtual void ackTOExpired();

	/**
	 * Handle the recption of an ACK from the sink
	 */
	virtual void stateAckRx();

	/**
	 * Handle a received ack, reinserting in the buffer the packet not received
	 * by the sink.
	 */
	virtual void handleAck();

	/**
	 * Handle case with no ack received. Reinsert packets in the tx buffer
	 */
	virtual void handleNoAck();

	/**
	 * Handle the ack received in the probe packet tx by the sink.
	 */
	virtual void handleProbeAck();

	/**
	 * Add a node to the list of probbed nodes
	 */
	virtual void addNode2List();

	/**
	 * Add a sink node to the list of probbed nodes
	 */
	virtual void addSink2List();

	/**
	 * Estimate the time needed by the AUV to POLL all the remaining node in the
	 * probbed node list
	 * @return poll time
	 */
	virtual uint16_t getPollTime();

	/**
	 * Get the packets received from the node with the given mac address
	 */
	virtual uint getRxPkts(int mac_addr);

	/**
	 * Initialize the backoff LUT.
	 * Erase it an re initialize it if already populated
	 * return false if file not found
	 */
	bool initBackoffLUT();
	
	// timers
	DataTimer data_timer; /**< Data timer*/
	ProbeTimer probe_timer; /**< PROBE Timer */
	PollTimer poll_timer; /**< POLL Timer */
	AckTimer ack_timer; /**< ACK Timer */

	// internal AUV structure for list of polled node
	std::vector<probbed_node> list_probbed_node; /**< list of nodes that have
												   sent correctly the PROBE */
	probbed_node probbed_sink; /**<Element with sink probe data */
	int polling_index; /**< Index of the node that the AUV is polling */
	bool sink_inserted; /** true if the sink has been inserted in the list*/
	// pointer to packets
	Packet *curr_trigger_packet; /**< Pointer to the current TRIGGER packet */
	Packet *curr_poll_packet; /**< Pointer to the current POLL packet */
	Packet *curr_data_packet; /**< Pointer to the current DATA packet */
	Packet *curr_probe_packet; /**< Pointer to the current PROBE packet */
	Packet *curr_tx_data_packet; /**< Pointer to the current DATA packet*/
	Packet *curr_ack_packet; /**< Pointer to the current ACK packet*/

	// input parameters via TCL
	double T_probe; /**< Duration of PROBE TIMER */
	double T_probe_guard; /**< Guard time for PROBE packet: 
							T_probe=T_max+T_probe_guard */
	double T_ack_timer; /** Duration of ACK_TIMER */
	int max_payload; /**< Dimension of the DATA payload */
	double T_min; /**< Minimum value in which the node can choose his backoff
					 time */
	double T_max; /**< Maximum value in which the node can choose his backoff
					 time */
	double T_guard; /**< Guard time added to the calculation of the RTT */
	int
			max_polled_node; /**< Maximum number of node that the AUV can poll
								each time. */
	int
			sea_trial_; /**< Sea Trial flag: To activate if the protocol is
						   going to be tested at the sea */
	int print_stats_; /**< Print protocol's statistics of the protocol */
	int modem_data_bit_rate; /**< Bit rate of the modem used */
	int
			DATA_POLL_guard_time_; /**< Guard time between the reception of the
									  last data and the transmission of the
									  following POLL */
	int n_run; /*< ID of the experiments (used during sea trial) */

	// states of protocol and reasons
	UWPOLLING_AUV_STATUS curr_state,
			prev_state; /**< Current and previous state variable */
	UWPOLLING_AUV_REASON reason, last_reason; /**< Current and previous reason
												 for the change of the state */

	// mapping
	static std::
			map<UWPOLLING_PKT_TYPE, std::string>
					pkt_type_info; /**< Map the UWPOLLING_PKT_TYPE to the
									  description of each type of packet */
	static std::
			map<UWPOLLING_AUV_STATUS, std::string>
					status_info; /** Map the UWPOLLING_AUV_STATUS to the
									description of each state */
	static std::
			map<UWPOLLING_AUV_REASON, std::string>
					reason_info; /** Map the UWPOLLING_AUV_REASON to the
									description of each reason */

	static bool
			initialized; /**< Indicate if the protocol has been initialized or
							not */

	bool RxDataEnabled; /**< <i>true</i> if the AUV is enabled to receive DATA
						   packets, <i>false</i> otherwise */
	bool
			RxProbeEnabled; /**< <i>true</i> if the AUV is enabled to receive
							   PROBE packets, <i>false</i> otherwise */
	bool TxEnabled; /**< <i>true</i> if the AUV is enabled to receive POLL
						   packets, <i>false</i> otherwise */
	double distance; /**< Distance between the AUV and the current node */
	int N_expected_pkt; /**< Number of packets that the node polled wish to
						   transmit to the AUV */
	int
			packet_index; /**< Variable that indicate the number of the packet
							 that has been just received by the AUV */
	int curr_polled_node_address; /**< MAC address of the node polled */

	uint curr_node_id; /**< ID of the node polled */

	// statistics

	//double curr_backoff_time; /**< backoff time of the node polled */
	double curr_RTT; /**< Round Trip time of the node polled */
	double curr_Tmeasured; /**< Time elapsed between the transmission of the
							  TRIGGER and the reception of the PROBE packet by
							  the polled node */
	double probe_rtt; /**< RTT calculation between AUV and NODE based on the
						 time of PROBE transmission and reception */
	int wrong_node_data_sent; /**< Number of data sent by a node that isn't
								 polled anymore */
	double initial_time; /**< Timestamp in which the AUV receive the first data
							packet */
	int n_trigger_tx; /**< Number of TRIGGER packets sent */
	uint n_probe_rx; /**< Number of PROBE packets received */
	int n_ack_rx; /**< Number of ack packets received */
	int n_poll_tx; /**< Number of POLL packets sent */
	bool begin; /**< indicate the first PROBE received */
	double stop_time; /**< Time stamp in which the AUV finish to receive data
						 packets */
	double pkt_time; /**< Time needed to transmit a data packet */
	double total_time; /**< Total time in which the AUV has received data
						  packets */
	double Tdata; /**< Time needed to transmit a DATA packet */
	double Tprobe; /**< Time needed to transmit a PROBE packet */
	double Ttrigger; /**< Time needed to transmit a TRIGGER packet */
	double Tpoll; /**< Time needed to transmit a POLL packet */
	uint TRIGGER_uid; /**< TRIGGER Unique ID */
	uint POLL_uid; /**< POLL Unique ID */

	int N_dropped_probe_pkts; /**< Number of PROBE dropped because of CRC
									 error */
	int n_dropped_ack_pkts; /**<Number of ACK dropped pkts */
	int N_dropped_probe_wrong_state; /**< Number of PROBE dropped because the
										AUV was not in RX_PROBE mode */

	std::
			ofstream fout; /**< Variable that handle the file in which the
							  protocol write the state transition for debug
							  purposes */
	std::ofstream out_file_stats; /**< Variable that handle the file in which
									 the protocol write the statistics */

	std::deque<Packet *> tx_buffer; /**< Queue of DATA in number of packets */
	std::deque<Packet *> temp_buffer; /** Temp buffer where packets are insert
									waiting for an ACK */
	uint max_buffer_size; /**< Max size for the transmission buffer */
	uint16_t uid_tx_pkt; /**< Unique ID of the transmitted packets */
	
	bool curr_is_sink; /**< True if the current node of the list is a sink.*/
	uint max_tx_pkts; /**< Max number of packets can be transmitted by the AUV
						during a TxData session */
	uint n_pkts_to_tx; /**< Number of packets to transmit during a TxData 
						session */
	uint n_tx_pkts; /**< Number of packets transmitted by the AUV during a 
					TxData session */
	uint16_t last_pkt_uid; /**< ID of the last packet transmitted in the round*/

	bool enableAckRx; /**< True if the ack reception is enabled */
	bool acked; /**< True if an ack has been received*/

	int ack_enabled; /**< True if ack is enabled, false if disabled, default true*/

	std::map<int, uint>rx_pkts_map; /**< Map (mac_addr,rx_pkts) with the received  
					packets for each node*/
	bool enable_adaptive_backoff; /**< Set to true if backoff is chosen adaptively*/
	std::string backoff_LUT_file; /**< File name of the backoff LUT */
	char lut_token_separator; /**< LUT token separator */
	std::map<double, double>backoff_LUT; /**< Map with the backoff LUT */
	probe_cicle_counters probe_counters; /**< Number of probe detected in a round (i.e., prehamble received)*/
	int full_knowledge; /**< Set to a number != 0 means we have full_knowledge 
						about the estimate of neighbors*/
	int last_probe_lost; /**Number of probe packets lost since last round;*/

};
#endif
