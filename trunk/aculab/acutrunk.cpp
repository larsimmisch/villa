/*
	acutrunk.cpp

	$Id: acutrunk.cpp,v 1.19 2003/11/26 00:09:28 lars Exp $

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

		return PHONE_ERROR_FAILED;
	}

	s_dispatcher.add(incoming.handle, this);
	s_dispatcher.unlock();

	lock();
	m_handle = incoming.handle;
	unlock();

	return PHONE_OK;
}

int AculabTrunk::connect(const SAP& local, const SAP& remote, unsigned aTimeout)
{
    OUT_XPARMS outdetail;

	memset(&outdetail, 0, sizeof(outdetail));

	lock();

	if (m_cmd != t_none)
	{
		unlock();

		return PHONE_ERROR_INVALID_STATE;
	}

    if (remote.getAddress() == 0)   
	{
		unlock();
		return PHONE_ERROR_INVALID_ARGUMENT;
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

	m_cmd = t_connect;
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

        return PHONE_ERROR_FAILED;
    }

	s_dispatcher.add(m_handle, this);
	s_dispatcher.unlock();

	lock();
    m_handle = outdetail.handle;
	unlock();

	return PHONE_OK;
}
	
int AculabTrunk::accept()
{
	lock();

	if (m_cmd != t_none)
	{
		unlock();

		return PHONE_ERROR_INVALID_STATE;
	}

	m_cmd = t_accept;
	unlock();

	int rc = call_accept(m_handle);

	if (rc)
	{
		release();

		log(log_error, "trunk", getName()) 
			<< "call_accept failed: " << rc << logend();

		return PHONE_ERROR_FAILED;
	}

	return PHONE_OK;
}

int AculabTrunk::disconnect(int cause)
{
	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = m_handle;
	xcause.cause = cause;

	lock();
	m_cmd = t_disconnect;
	unlock();

	int rc = call_disconnect(&xcause);
	if (rc)
	{
		log(log_error, "trunk", getName()) 
			<< "call_disconnect failed: " << rc << logend();

		return PHONE_ERROR_FAILED;
	}

	return PHONE_OK;
}

void AculabTrunk::release()
{
	CAUSE_XPARMS xcause;

	memset(&xcause, 0, sizeof(xcause));

	xcause.handle = m_handle;

	int rc = call_release(&xcause);
	if (rc)
	{
		log(log_error, "trunk", getName()) << "call_release failed: " << rc 
			<< " in command " << commandName(m_cmd) << logend();
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
			<< "call_getcause failed: " << rc << " in command " 
			<< commandName(m_cmd) << logend();

		return PHONE_ERROR_FAILED;
	}

	switch(cause.cause)
	{
	case LC_NORMAL:
		return PHONE_OK;
	case LC_NUMBER_BUSY:
		return PHONE_ERROR_BUSY;
	case LC_NO_ANSWER:
		return PHONE_ERROR_TIMEOUT;
	case LC_NUMBER_UNOBTAINABLE:
		return PHONE_ERROR_UNREACHABLE;
	case LC_NUMBER_CHANGED:
		return PHONE_ERROR_NUMBER_CHANGED;
	case LC_OUT_OF_ORDER:
		return PHONE_ERROR_FAILED;
	case LC_INCOMING_CALLS_BARRED:
		return PHONE_ERROR_FAILED;
	case LC_CALL_REJECTED:
		return PHONE_ERROR_REJECTED;
	case LC_CALL_FAILED:
		return PHONE_ERROR_FAILED;
	case LC_CHANNEL_BUSY:
		return PHONE_ERROR_BUSY;
	case LC_NO_CHANNELS:
		return PHONE_ERROR_BUSY;
	case LC_CONGESTION:
		return PHONE_ERROR_BUSY;
	default:
		return PHONE_ERROR_FAILED;
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

	// stopTimer();

	TrunkCommand cmd;
	bool remote_disconnect;

	lock();
	cmd = m_cmd;
	remote_disconnect = m_remote_disconnect;
	unlock();

	if (!remote_disconnect)
	{
		switch (cmd)
		{
		case t_connect:
			// outgoing failed or stopped
			m_client->connectDone(this, m_stopped ? PHONE_ERROR_ABORTED : cause);
			break;
		case t_disconnect:
			m_client->disconnectDone(this, PHONE_OK);
			break;
		case t_accept:
			m_client->acceptDone(this, cause);
			break;
		default:
			m_client->disconnectRequest(this, cause);
			break;
		}
	}

	lock();
	setName(-1);
	m_remote_disconnect = false;
	m_stopped = false;
	m_cmd = t_none;
	unlock();

	release();
	listen();
}

void AculabTrunk::onWaitForIncoming()
{
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

	if (details.calltype == INCOMING)
	{
		m_client->acceptDone(this, PHONE_OK);
	}
	else
	{
		m_client->connectDone(this, PHONE_OK);
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
	TrunkCommand cmd;

	lock();
	cmd = m_cmd;
	m_cmd = t_none;
	m_remote_disconnect = true;
	unlock();

	switch (cmd)
	{
	case t_none:
		m_client->disconnectRequest(this, getCause());
		break;
	case t_disconnect:
		m_client->disconnectDone(this, PHONE_OK);
		break;
	case t_connect:
		m_client->connectDone(this, PHONE_ERROR_REJECTED);
		disconnect();
		break;
	case t_accept:
		m_client->acceptDone(this, PHONE_ERROR_REJECTED);
		disconnect();
		break;
	default:
		log(log_error, "trunk", getName()) << "unhandled command " 
			<< commandName(cmd) << " in onRemoteDisconnect" << logend();
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