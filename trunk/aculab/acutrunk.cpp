/*
	acutrunk.cpp

	$Id: acutrunk.cpp,v 1.17 2001/09/30 09:51:57 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include <iostream>
#include <iomanip>
#include "acutrunk.h"

CallEventDispatcher AculabTrunk::s_dispatcher;
struct siginfo_xparms AculabTrunk::s_siginfo[MAXPORT];

void CallEventDispatcher::add(ACU_INT handle, AculabTrunk* trunk)
{
	m_handle_map[handle] = trunk;
}

void CallEventDispatcher::remove(ACU_INT handle)
{
	m_handle_map.erase(handle);
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
			log(log_error, "trunk") << "call_event failed: " << rc << logend();
		}
		else
		{
			if (event.handle)
			{
				lock();

				std::map<ACU_INT, AculabTrunk*>::iterator i = 
					m_handle_map.find(event.handle);

				if (i == m_handle_map.end())
				{
					unlock();

					log(log_error, "trunk") << 
						"no device registered for handle : 0x" 
						<< std::setbase(16) << event.handle 
						<< std::setbase(10) << logend();
				}
				else
				{
					unlock();

					i->second->onCallEvent(event.state);
				}
			}
		}
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

void AculabTrunk::start()
{ 
	memset(&AculabTrunk::s_siginfo, 0, sizeof(AculabTrunk::s_siginfo));

	call_signal_info(AculabTrunk::s_siginfo);

	s_dispatcher.start(); 
}

int AculabTrunk::listen()
{
	IN_XPARMS incoming;

	memset(&incoming, 0, sizeof(incoming));

	lock();
	m_stopped = false;
	m_state = waiting;

	incoming.net = m_port;
	unlock();

    incoming.ts = -1;
	incoming.cnf = CNF_REM_DISC | CNF_CALL_CHARGE;

	// we must lock the global dispatcher before we enter call_openin
	// to avoid that the the dispatcher dispatches an event for this channel
	// before it was added to it's map
	s_dispatcher.lock();

    int rc = call_openin(&incoming);
    if (rc != 0)
	{
		s_dispatcher.unlock();

		log(log_error, "trunk") << "call_openin failed: " << rc << logend();

		return r_failed;
	}

	s_dispatcher.add(incoming.handle, this);
	s_dispatcher.unlock();

	lock();
	m_handle = incoming.handle;
	unlock();

	return r_ok;
}

int AculabTrunk::connect(const SAP& local, const SAP& remote, unsigned aTimeout)
{
    OUT_XPARMS outdetail;

	memset(&outdetail, 0, sizeof(outdetail));

	lock();

	if (m_state != idle)
	{
		unlock();

		return r_bad_state;
	}

	unlock();

    if (remote.getAddress() == 0)   
	{
		return r_invalid;
	}

	int sigsys = call_type(m_port);

    outdetail.net = m_port;
    outdetail.ts = -1;
	outdetail.cnf = CNF_REM_DISC | CNF_CALL_CHARGE;
	outdetail.sending_complete = 1;

    strcpy(outdetail.destination_addr, remote.getAddress());
    if (local.getAddress()) 
	{
		strcpy(outdetail.originating_addr, local.getAddress());
	}

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
		outdetail.unique_xparms.sig_q931.service_octet = TELEPHONY;
		outdetail.unique_xparms.sig_q931.add_info_octet = ANALOGUE;
		outdetail.unique_xparms.sig_q931.dest_numbering_type = NT_UNKNOWN;
		outdetail.unique_xparms.sig_q931.dest_numbering_plan = NP_ISDN;
		outdetail.unique_xparms.sig_q931.orig_numbering_type = NT_UNKNOWN;
		outdetail.unique_xparms.sig_q931.orig_numbering_plan = NP_ISDN;
		break;
	case S_DASS:
	case S_DPNSS:
		break;
	case S_CAS:
	default:
		throw Exception(__FILE__, __LINE__, "AculabTrunk::connect()", "invalid signalling system");
	}

	lock();
	m_state = connecting;
	unlock();

	// we must lock the global dispatcher before we enter call_openout
	// to avoid that the the dispatcher dispatches an event for this channel
	// before it was added to it's map
	s_dispatcher.lock();

	int rc = call_openout(&outdetail);
	if (rc != 0)
	{
		log(log_error, "trunk") << "call_openout failed: " << rc << logend();

		s_dispatcher.unlock();

        return r_failed;
    }

	s_dispatcher.add(m_handle, this);
	s_dispatcher.unlock();

	lock();
    m_handle = outdetail.handle;
	unlock();

	return r_ok;
}
	
int AculabTrunk::accept()
{
	lock();

	if (m_state != collecting_details)
	{
		unlock();

		return r_bad_state;
	}

	m_state = accepting;
	unlock();

	int rc = call_accept(m_handle);

	if (rc)
	{
		release();

		log(log_error, "trunk", getName()) 
			<< "call_accept failed: " << rc << logend();

		return r_failed;
	}

	return r_ok;
}

int AculabTrunk::reject(int cause)
{
	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = m_handle;
	xcause.cause = cause;

	lock();
	m_state = rejecting;
	unlock();

	int rc = call_disconnect(&xcause);
	if (rc)
	{
		log(log_error, "trunk", getName()) 
			<< "call_disconnect failed: " << rc << logend();

		return r_failed;
	}

	return r_ok;
}

int AculabTrunk::disconnect(int cause)
{
	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = m_handle;
	xcause.cause = cause;

	lock();
	if (m_state == idle)
	{
		unlock();
		m_client->disconnectDone(this, r_ok);

		return r_ok;
	}

	m_state = disconnecting;
	unlock();

	int rc = call_disconnect(&xcause);
	if (rc)
	{
		log(log_error, "trunk", getName()) 
			<< "call_disconnect failed: " << rc << logend();

		return r_failed;
	}

	return r_ok;
}
	
int AculabTrunk::disconnectAccept()
{
	disconnect();

	return r_ok;
}

void AculabTrunk::release()
{
	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = m_handle;

	lock();
	m_state = idle;
	unlock();

	int rc = call_release(&xcause);
	if (rc)
	{
		log(log_error, "trunk", getName()) << "call_release failed: " << rc 
			<< " in state " << stateName(m_state) << logend();
	}

	s_dispatcher.lock();
	s_dispatcher.remove(m_handle);
	s_dispatcher.unlock();

}
	
void AculabTrunk::abort()
{
	release();
	setName(-1);
}

int AculabTrunk::getCause()
{
	CAUSE_XPARMS cause;

	cause.handle = m_handle;

	int rc = call_getcause(&cause);
	if (rc != 0)
	{
		log(log_error, "trunk", getName()) 
			<< "call_getcause failed: " << rc << " in state " 
			<< stateName(m_state) << logend();

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

	log(log_debug, "trunk", getName()) << eventName(event) << logend();

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
		log(log_warning, "trunk", getName()) 
			<< "unhandled call event " << eventName(event) << logend();
		break;
	}
}

void AculabTrunk::onIdle()
{
	int cause = getCause();

	bool restart(false);

	// stopTimer();

	switch (m_state)
	{
	case connecting:
		// outgoing failed or stopped
		release();
		m_client->connectDone(this, m_stopped ? r_aborted : cause);
		lock();
		setName(-1);
		m_stopped = false;
		unlock();
		listen();
		break;
	case connected:
		lock();
		m_state = idle;
		unlock();
		m_client->disconnectRequest(this, cause);
		break;
	case disconnecting:
		release();
		m_client->disconnectDone(this, r_ok);
		lock();
		setName(-1);
		unlock();
		listen();
		break;
	case rejecting:
		release();
		m_client->rejectDone(this, r_ok);
		lock();
		setName(-1);
		unlock();
		listen();
		break;
	case accepting:
		release();
		m_client->acceptDone(this, cause);
		lock();
		setName(-1);
		unlock();
		listen();
		break;
	default:
		log(log_warning, "trunk", getName()) 
			<< "call went idle in state " << stateName(m_state) << logend();

		// restart automatically
		release();
		lock();
		setName(-1);
		unlock();
		listen();
		break;
	}
}

void AculabTrunk::onWaitForIncoming()
{
	m_state = waiting;
}

void AculabTrunk::onIncomingCallDetected()
{
	SAP local, remote;
	DETAIL_XPARMS details;

	details.timeout = 0;
	details.handle = m_handle;

	int rc = call_details(&details);

	if (rc)
	{
		log(log_error, "trunk", getName()) << "call_details failed: " << rc << logend();

		// restart automatically

		release();
		listen();

		return;
	}


	remote.setAddress(details.originating_addr);
	local.setAddress(details.destination_addr);
	local.setService(details.stream);
	local.setSelector(details.ts);

	lock();
	setName(details.ts);
	m_timeslot.st = details.stream;
	m_timeslot.ts = details.ts;
	m_state = collecting_details;
	unlock();

	m_client->connectRequest(this, local, remote);
}

void AculabTrunk::onCallConnected()
{
	DETAIL_XPARMS details;

	details.timeout = 0;
	details.handle = m_handle;

	int rc = call_details(&details);

	if (rc)
	{
		log(log_error, "trunk", getName()) << "call_details failed: " << rc << logend();

		return;
	}

	m_state = connected;

	if (details.calltype == INCOMING)
	{
		m_client->acceptDone(this, r_ok);
	}
	else
	{
		m_client->connectDone(this, r_ok);
	}
}

void AculabTrunk::onWaitForOutgoing()
{
}

void AculabTrunk::onOutgoingRinging()
{
}

void AculabTrunk::onRemoteDisconnect()
{
	switch (m_state)
	{
	case disconnecting:
		break;
	case connected:
		m_state = remote_disconnect;
		m_client->disconnectRequest(this, getCause());
		break;
	case accepting:
		m_state = idle;
		m_client->acceptDone(this, getCause());
		disconnect();
		break;
	case collecting_details:
		m_client->disconnectRequest(this, getCause());
		break;
	default:
		log(log_error, "trunk", getName()) << "unhandled state " 
			<< stateName(m_state) << " in onRemoteDisconnect" << logend();
		break;
	}
}

void AculabTrunk::onWaitForAccept()
{
}

void AculabTrunk::onOutgoingProceeding()
{
}

void AculabTrunk::onDetails()
{
	DETAIL_XPARMS details;

	details.timeout = 0;
	details.handle = m_handle;

	int rc = call_details(&details);

	if (rc)
	{
		log(log_error, "trunk", getName()) << "call_details failed: " << rc << logend();

		return;
	}	
}

void AculabTrunk::onCallCharge()
{
}

void AculabTrunk::onChargeInt()
{
}

void AculabTrunk::setName(int ts)
{
	if (ts == -1)
	{
		Trunk::setName("");
	}
	else
	{
		char buffer[MAXSIGSYS + 16];

		sprintf(buffer, "%s[%d,%d]", s_siginfo[m_port].sigsys, m_port, ts);

		Trunk::setName(buffer);
	}
}