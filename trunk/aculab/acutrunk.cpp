/*
	acuphone.cpp

	$Id: acutrunk.cpp,v 1.1 2000/10/02 15:52:14 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include <iostream>
#include <iomanip>
#include "acutrunk.h"

CallEventDispatcher AculabTrunk::dispatcher;

void CallEventDispatcher::add(ACU_INT handle, AculabTrunk* trunk)
{
	handle_map[handle] = trunk;
}

void CallEventDispatcher::remove(ACU_INT handle)
{
	handle_map.erase(handle);
}

void* CallEventDispatcher::run_undetached(void* arg)
{
	state_xparms event;
	int rc;

	// we don't provide a method to stop this thread.
	// when this thread has to be stopped, the application exits anyway
	while (true)
	{
		event.handle = 0;
		event.timeout = 10000;

		rc = call_event(&event);
		if (rc)
		{
			std::cerr << "call_event failed: " << rc << std::endl;
		}
		else
		{
			if (event.handle)
			{
				lock();

				std::map<ACU_INT, AculabTrunk*>:: iterator i = handle_map.find(event.handle);

				unlock();

				if (i == handle_map.end())
				{
					std::cerr << "no device registered for handle : 0x" << std::setbase(16) << event.handle << std::setbase(10) << std::endl;
				}
				else
				{
					i->second->onCallEvent(event.state);
				}
			}
		}
	}
}

const char* AculabTrunk::stateName(states state)
{
	switch (state)
	{
	case idle:
		return "idle";
	case listening:
		return "listening";
	case connecting:
		return "connecting";
	case connected:
		return "connected";
	case disconnecting:
		return "disconnecting";
	case transferring:
		return "transferring";
	case waiting:
		return "waiting";
	case collecting_details:
		return "collecting_details";
	case accepting:
		return "accepting";
	case rejecting:
		return "rejecting";
	default:
		return "illegal state";
	}
}

const char* AculabTrunk::eventName(ACU_LONG event)
{
	static char name[32];

	switch(event)
	{
	case EV_IDLE:
		return "EV_IDLE";
	case EV_WAIT_FOR_INCOMING:
		return "EV_WAIT_FOR_INCOMING";
	case EV_INCOMING_CALL_DET:
		return "EV_INCOMING_CALL_DET";
	case EV_CALL_CONNECTED:
		return "EV_CALL_CONNECTED";
	case EV_EMERGENCY_CONNECT:
		return "EV_EMERGENCY_CONNECT";
	case EV_TEST_CONNECT:
		return "EV_TEST_CONNECT";
	case EV_WAIT_FOR_OUTGOING:
		return "EV_WAIT_FOR_OUTGOING";
	case EV_OUTGOING_RINGING:
		return "EV_OUTGOING_RINGING";
	case EV_REMOTE_DISCONNECT:
		return "EV_REMOTE_DISCONNECT";
	case EV_WAIT_FOR_ACCEPT:
		return "EV_WAIT_FOR_ACCEPT";
	case EV_HOLD:
		return "EV_HOLD";
	case EV_HOLD_REJECT:
		return "EV_HOLD_REJECT";
	case EV_TRANSFER_REJECT:
		return "EV_TRANSFER_REJECT";
	case EV_RECONNECT_REJECT:
		return "EV_RECONNECT_REJECT";
	case EV_PROGRESS:
		return "EV_PROGRESS";
	case EV_OUTGOING_PROCEEDING:
		return "EV_OUTGOING_PROCEEDING";
	case EV_DETAILS:
		return "EV_DETAILS";
	case EV_CALL_CHARGE:
		return "EV_CALL_CHARGE";
	case EV_CHARGE_INT:
		return "EV_CHARGE_INT";
	case EV_NOTIFY:
		return "EV_NOTIFY";
	case EV_EXTENDED:
		return "EV_EXTENDED";
	default:
		sprintf(name, "unknown event 0x%x", event);
		return name;
	}
}

int AculabTrunk::listen()
{
	IN_XPARMS incoming;

	omni_mutex_lock l(mutex);

	incoming.net = port;
    incoming.ts = -1;
	incoming.cnf = CNF_REM_DISC | CNF_CALL_CHARGE;

	// we must lock the global dispatcher before we enter call_openin
	// to avoid that the the dispatcher gets an event for this channel
	// before it was added to it's map
	dispatcher.lock();

    int rc = call_openin(&incoming);
    if (rc != 0)
	{
		dispatcher.unlock();

		std::cerr << "call_openin failed: " << rc << std::endl;
	}

	handle = incoming.handle;
	stopped = false;

	dispatcher.add(incoming.handle, this);
	dispatcher.unlock();

	state = waiting;

	return r_ok;
}

int AculabTrunk::connect(SAP& aLocalSAP, SAP& aRemoteSAP, unsigned aTimeout)
{
    OUT_XPARMS outdetail;

	omni_mutex_lock l(mutex);

	if (state != idle)
	{
		return r_bad_state;
	}

    if (aRemoteSAP.getAddress() == 0)   
	{
		return r_invalid;
	}

	int sigsys = call_type(port);

    outdetail.net = port;
    outdetail.ts = -1;
    strcpy(outdetail.destination_addr, aRemoteSAP.getAddress());
    if (aLocalSAP.getAddress()) 
	{
		strcpy(outdetail.originating_addr, aLocalSAP.getAddress());
	}
	else
	{
		outdetail.originating_addr[0] = '\0';
	}

	memset(&outdetail.unique_xparms, 0, sizeof(UNIQUEXU));
	switch (sigsys)
	{
	case S_ETS300:
	case S_ETSNET:
	case S_SWETS300:
	case S_SWETSNET:
	case S_QSIG:
	case S_AUSTEL:
	case S_AUSTNET:
	case S_VN3:
	case S_VN3NET:
	case S_TNA_NZ:
	case S_TNANET:
	case S_IDAP:
	case S_IDAPNET:
	case S_INS:
	case S_INSNET:
	case S_FETEX_150:
	case S_FETEXNET:
	case S_ATT:
	case S_ATTNET:
	case S_NI2:
	case S_NI2NET:
	case BR_ETS300:
	case BR_ETSNET:
	case BR_ATT:
	case BR_ATTNET:
		outdetail.sending_complete = 1;
		outdetail.unique_xparms.sig_q931.service_octet = TELEPHONY;
		outdetail.unique_xparms.sig_q931.add_info_octet = ANALOGUE;
		outdetail.unique_xparms.sig_q931.dest_numbering_type = NT_UNKNOWN;
		outdetail.unique_xparms.sig_q931.dest_numbering_plan = NP_ISDN;
		break;
	case S_DASS:
	case S_DPNSS:
		break;
	case S_CAS:
	default:
		throw Exception(__FILE__, __LINE__, "AculabTrunk::connect()", "invalid signalling system");
	}

	int rc = call_openout(&outdetail);
	if (rc != 0)
	{
		std::cerr << "call_openout failed: " << rc << std::endl;
        return r_failed;
    }

    handle = outdetail.handle;

	dispatcher.add(handle, this);

	return r_ok;
}
	
int AculabTrunk::accept()
{
	omni_mutex_lock l(mutex);

	if (state != collecting_details)
		return r_bad_state;

	int rc = call_accept(handle);

	if (rc)
	{
		release();

		std::cerr << "call_accept failed: " << rc << std::endl;

		return r_failed;
	}

	state = accepting;

	return r_ok;
}

int AculabTrunk::reject(int cause)
{
	return disconnect(cause);
}

int AculabTrunk::disconnect(int cause)
{
	omni_mutex_lock l(mutex);

	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = handle;
	xcause.cause = cause;

	int rc = call_disconnect(&xcause);
	if (rc)
	{
		release();

		std::cerr << "call_disconnect failed: " << rc << std::endl;

		return r_failed;
	}

	state = disconnecting;

	return r_ok;
}
	
int AculabTrunk::disconnectAccept()
{
	return disconnect();
}

void AculabTrunk::release()
{
	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = handle;

	int rc = call_release(&xcause);
	if (rc)
	{
		std::cerr << "call_release failed: " << rc << std::endl;
	}

	dispatcher.lock();
	dispatcher.remove(handle);
	dispatcher.unlock();

	state = idle;
}
	
void AculabTrunk::abort()
{
	release();
}

int AculabTrunk::getCause()
{
	CAUSE_XPARMS cause;

	cause.handle = handle;

	int rc = call_getcause(&cause);
	if (rc != 0)
	{
		std::cerr << "call_getcause failed: " << rc << std::endl;

		return r_failed;
	}

	switch(cause.cause)
	{
	case LC_NORMAL:
		return r_ok;
	case LC_NUMBER_BUSY:
		return r_busy;
	case LC_NO_ANSWER:
		return r_timeout;
	case LC_NUMBER_UNOBTAINABLE:
		return r_not_available;
	case LC_NUMBER_CHANGED:
		return r_number_changed;
	case LC_OUT_OF_ORDER:
		return r_failed;
	case LC_INCOMING_CALLS_BARRED:
		return r_failed;
	case LC_CALL_REJECTED:
		return r_rejected;
	case LC_CALL_FAILED:
		return r_failed;
	case LC_CHANNEL_BUSY:
		return r_busy;
	case LC_NO_CHANNELS:
		return r_busy;
	case LC_CONGESTION:
		return r_busy;
	default:
		return r_invalid;
	}
}

void AculabTrunk::onCallEvent(ACU_LONG event)
{
	omni_mutex_lock l(mutex);

	switch (event)
	{
	case EV_IDLE:
		onIdle();
		break;
	case EV_WAIT_FOR_INCOMING:
		onWaitForIncoming();
		break;
	case EV_INCOMING_CALL_DET:
		onIncomingCallDetected();
		break;
	case EV_CALL_CONNECTED:
		onCallConnected();
		break;
	case EV_WAIT_FOR_OUTGOING:
		onWaitForOutgoing();
		break;
	case EV_OUTGOING_RINGING:
		onOutgoingRinging();
		break;
	case EV_REMOTE_DISCONNECT:
		onRemoteDisconnect();
		break;
	case EV_WAIT_FOR_ACCEPT:
		onWaitForAccept();
		break;
	case EV_OUTGOING_PROCEEDING:
		onOutgoingProceeding();
		break;
	case EV_DETAILS:
		onDetails();
		break;
	case EV_CALL_CHARGE:
		onCallCharge();
		break;
	case EV_CHARGE_INT:
		onChargeInt();
		break;
	case EV_HOLD:
	case EV_HOLD_REJECT:
	case EV_TRANSFER_REJECT:
	case EV_RECONNECT_REJECT:
	case EV_PROGRESS:
	case EV_NOTIFY:
	case EV_EMERGENCY_CONNECT:
	case EV_TEST_CONNECT:
	case EV_EXTENDED:
	default:
		std::cerr << "unhandled call event " << eventName(event) << std::endl;
		break;
	}
}

void AculabTrunk::onIdle()
{
	int cause = getCause();

	release();

	// stopTimer();

	switch (state)
	{
	case collecting_details:
	case waiting:
		std::cerr << "incoming call went idle in state " << stateName(state) << std::endl;

		// restart automatically
		release();
		listen();

		break;
	case connecting:
		// outgoing failed or stopped
		client->connectDone(this, stopped ? r_aborted : cause);
		stopped = false;
		release();
		break;
	case connected:
		client->disconnectRequest(this, cause);
		if (phone)
			phone->disconnected(this, cause);
		state = disconnecting;
		break;
	case disconnecting:
		client->disconnectDone(this, r_ok);
		state = idle;
		break;
	case accepting:
		client->acceptDone(this, cause);
		release();
		break;
	default:
		break;
	}
}

void AculabTrunk::onWaitForIncoming()
{
	state = waiting;
}

void AculabTrunk::onIncomingCallDetected()
{
	SAP local, remote;
	DETAIL_XPARMS details;

	details.timeout = 0;
	details.handle = handle;

	int rc = call_details(&details);

	if (rc)
	{
		std::cerr << "call_details failed: " << rc << std::endl;

		// restart automatically

		release();
		listen();

		return;
	}

	slot.st = details.stream;
	slot.ts = details.ts;

	remote.setAddress(details.originating_addr);
	local.setAddress(details.destination_addr);
	local.setService(details.stream);
	local.setSelector(details.ts);

	state = collecting_details;

	client->connectRequest(this, local, remote);
}

void AculabTrunk::onCallConnected()
{
	DETAIL_XPARMS details;

	details.timeout = 0;
	details.handle = handle;

	int rc = call_details(&details);

	if (rc)
	{
		std::cerr << "call_details failed: " << rc << std::endl;

		return;
	}

	state = connected;

	if (details.calltype == INCOMING)
	{
		client->acceptDone(this, r_ok);
	}
	else
	{
		client->connectDone(this, r_ok);
	}
}

void AculabTrunk::onWaitForOutgoing()
{
}

void AculabTrunk::onOutgoingRinging()
{
	if (phone)
		phone->remoteRinging(this);
}

void AculabTrunk::onRemoteDisconnect()
{
	switch (state)
	{
	case connected:
		state = disconnecting;
		client->disconnectRequest(this, getCause());
		break;
	case accepting:
		client->acceptDone(this, getCause());
		release();
		break;
	default:
		std::cerr << "unhandled state " << stateName(state) << " in onRemoteDisconnect" << std::endl;
		break;
	}
}

void AculabTrunk::onWaitForAccept()
{
	state = accepting;
}

void AculabTrunk::onOutgoingProceeding()
{
}

void AculabTrunk::onDetails()
{
	DETAIL_XPARMS details;

	details.timeout = 0;
	details.handle = handle;

	int rc = call_details(&details);

	if (rc)
	{
		std::cerr << "call_details failed: " << rc << std::endl;

		return;
	}	
}

void AculabTrunk::onCallCharge()
{
}

void AculabTrunk::onChargeInt()
{
}
