/*
	acuphone.cpp

	$Id: acuphone.cpp,v 1.24 2004/01/12 21:48:06 lars Exp $

	Copyright 1995-2001 Lars Immisch

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include <iostream>
#include <iomanip>
#include <typeinfo.h>
#include "mvswdrvr.h"
#include "names.h"
#include "acuphone.h"
#include "fal.h"
#include "v3.h"
#include "prosody_error.i"
#include "beep.i"

ProsodyEventDispatcher ProsodyChannel::s_dispatcher;

void AculabSwitch::listen(const Timeslot &a, const Timeslot &b, const char *name)
{
	OUTPUT_PARMS args;

    args.ost = a.st; // sink
    args.ots = a.ts;
    args.mode = CONNECT_MODE;
    args.ist = b.st; // source
    args.its = b.ts;

	log(log_debug , "switch", name) << a.st << ':' << a.ts << " := " << b.st << ':' << b.ts << logend();

    int rc = sw_set_output(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::listen(Timeslot,Timeslot)", "set_output(CONNECT_MODE)", rc);

}

void AculabSwitch::listen(const Timeslot &a, char pattern, const char *name)
{
	OUTPUT_PARMS args;

    args.ost = a.st;
    args.ots = a.ts;
    args.mode = PATTERN_MODE;
    args.pattern = pattern;

	log(log_debug, "switch", name) << a.st << ':' << a.ts << " := 0x" 
		<< std::setbase(16) << pattern << std::setbase(10) << logend();

    int rc = sw_set_output(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::listen(Timeslot,char)", "set_output(PATTERN_MODE)", rc);
}

void AculabSwitch::disable(const Timeslot &a, const char *name)
{
	OUTPUT_PARMS args;

    args.ost = a.st;
    args.ots = a.ts;
    args.mode = DISABLE_MODE;
    args.ist = 0;            // these two zeros should not be necessary, but they are necessary
    args.its = 0;            // for the Aculab Switch Driver v1.05 (or below) and the Amtelco XDS

    int rc = sw_set_output(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::disable(Timeslot)", "sw_set_output(DISABLE_MODE)", rc);

	log(log_debug, "switch", name) << a.st << ':' << a.ts << " disabled"
		<< logend();
}

char AculabSwitch::sample(const Timeslot &a, const char *name)
{
	SAMPLE_PARMS args;

	args.ist = a.st;
	args.its = a.ts;

    int rc = sw_sample_input0(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::sample(Timeslot)", "sw_sample_input0", rc);

	return args.sample;
}
	
Timeslot AculabSwitch::query(const Timeslot &a, const char *name)
{
	OUTPUT_PARMS args;

    args.ost = a.st;
    args.ots = a.ts;

    int rc = sw_query_output(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::query(Timeslot)", "sw_query_output", rc);

    if (args.mode != CONNECT_MODE)
		return Timeslot();

    return Timeslot(args.ist, args.its);
}

void ProsodyEventDispatcher::add(tSMEventId event, ProsodyObserver* observer, ProsodyObserverMethod method)
{
	if (m_running)
		throw Exception(__FILE__, __LINE__, "ProsodyEventDispatcher::add", "cannot add observers to running dispatcher");

	m_methods.push_back(ObserverMethod(observer, method));
	m_events.push_back(event);
}

void ProsodyEventDispatcher::start()
{
	m_running = true;

	for (int i = 0; i <= (m_events.size() / max_observers_per_thread); ++i)
	{
		int l = min(m_events.size() - i * max_observers_per_thread, max_observers_per_thread);

		DispatcherThread* d = new DispatcherThread(*this, i * max_observers_per_thread, l);
		m_threads.push_back(d);

		d->start();
	}
}

void ProsodyEventDispatcher::dispatch(int offset)
{
	if (offset >= m_methods.size())
	{
		log(log_error, "phone") << "ProsodyEventDispatcher::dispatch: offset " << offset << " out of bounds" << logend();
		return;
	}

	(m_methods[offset].m_observer->*m_methods[offset].m_method)(
		m_events[offset]);
}

void* ProsodyEventDispatcher::DispatcherThread::run_undetached(void *arg)
{
	HANDLE* handles = &m_dispatcher.m_events[m_offset];

	while (true)
	{
		DWORD rc = WaitForMultipleObjects(m_length, handles, FALSE, INFINITE);
		if (rc < WAIT_OBJECT_0 || rc > WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS)
		{
			log(log_error, "phone") << "ProsodyEventDispatcherThread: WaitForMultipleObjects failed:" << GetLastError() << logend();

			continue;
		}
		if (rc - WAIT_OBJECT_0 > m_length)
		{
			log(log_error, "phone") << "ProsodyEventDispatcherThread: WaitForMultipleObjects returned index we did not wait upon (" 
				<< rc - WAIT_OBJECT_0 << ")" << logend();

			continue;
		}

		m_dispatcher.dispatch(m_offset + rc - WAIT_OBJECT_0);
	}

	return NULL;
}

ProsodyChannel::ProsodyChannel() : m_sending(0), m_receiving(0)
{
	struct sm_channel_alloc_parms alloc;
	alloc.type		  = kSMChannelTypeFullDuplex;
	alloc.firmware_id = 0;
	alloc.group		  = 0;
	alloc.caps_mask	  = 0;
	
	int rc = sm_channel_alloc(&alloc);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_channel_alloc", rc);

	m_channel = alloc.channel;
	
	m_listenFor.channel = m_channel;
	m_listenFor.enable_pulse_digit_recognition = 0;
	m_listenFor.tone_detection_mode = kSMToneLenDetectionMinDuration64; // signal event at end of tone, to avoid recording the tone
	m_listenFor.active_tone_set_id = 0;	// use given (i.e. DTMF/FAX) tone set
	m_listenFor.map_tones_to_digits = kSMDTMFToneSetDigitMapping;
	m_listenFor.enable_cptone_recognition = 0;
	m_listenFor.enable_grunt_detection = 0;
	m_listenFor.grunt_latency = 0;
	
	rc = sm_listen_for(&m_listenFor);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_listen_for", rc);


	// initialize our events
	m_eventRead = set_event(kSMEventTypeReadData);
	m_eventWrite = set_event(kSMEventTypeWriteData);
	m_eventRecog = set_event(kSMEventTypeRecog);

	// and add them to the dispatcher
	s_dispatcher.add(m_eventRead, this, onRead);
	s_dispatcher.add(m_eventWrite, this, onWrite);
	s_dispatcher.add(m_eventRecog, this, onRecog);

	// cache channel info
	m_info.channel = m_channel;
	rc = sm_channel_info(&m_info);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_channel_info", rc);
}

tSMEventId ProsodyChannel::set_event(tSM_INT type)
{
	struct sm_channel_set_event_parms event;
	event.channel	   = m_channel;
	event.issue_events = kSMChannelSpecificEvent;
	event.event_type   = type;
	
	int rc = smd_ev_create(&event.event,
				   event.channel,
				   event.event_type,
				   event.issue_events);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "smd_ev_create", rc);
	
	rc = sm_channel_set_event(&event);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_channel_set_event", rc);

	return event.event;
}

void ProsodyChannel::conferenceClone(ProsodyChannel *model)
{
	struct sm_conf_prim_clone_parms clone;

	memset(&clone, 0, sizeof(clone));
	clone.channel = m_channel;
	clone.model = model->m_channel;

	int rc = sm_conf_prim_clone(&clone);

	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_conf_prim_clone", rc);
	}
}

void ProsodyChannel::conferenceStart()
{
	struct sm_conf_prim_start_parms start;

	memset(&start, 0, sizeof(start));
	start.channel = m_channel;

	int rc = sm_conf_prim_start(&start);

	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_conf_prim_start", rc);
	}
}

void ProsodyChannel::conferenceAdd(ProsodyChannel *party)
{
	struct sm_conf_prim_add_parms add;

	memset(&add, 0, sizeof(add));

	add.channel = m_channel;
	add.participant = party->m_channel;

	int rc = sm_conf_prim_add(&add);

	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_conf_prim_add", rc);
	}

	party->m_conferenceId = add.id;
}

void ProsodyChannel::conferenceLeave(ProsodyChannel *party)
{
	struct sm_conf_prim_leave_parms leave;

	memset(&leave, 0, sizeof(leave));
	leave.channel = m_channel;
	leave.id = party->m_conferenceId;

	int rc = sm_conf_prim_leave(&leave);

	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_conf_prim_leave", rc);
	}
}

void ProsodyChannel::conferenceAbort()
{
	int rc = sm_conf_prim_abort(m_channel);

	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_conf_prim_abort", rc);
	}
}

void ProsodyChannel::conferenceEC()
{
	struct sm_condition_input_parms	condition;

	// Attempt to use echo cancelation in conference.
	memset(&condition, 0, sizeof(condition));

	condition.channel = m_channel;
	condition.reference	= m_channel;
	condition.reference_type = kSMInputCondRefUseOutput;
	condition.conditioning_type	= kSMInputCondEchoCancelation;
	condition.alt_dest_type			= kSMInputCondAltDestNone;

	int rc = sm_condition_input(&condition);
	
	if (rc == ERR_SM_WRONG_FIRMWARE_TYPE)
	{
		 // Fall back to sidetone suppression.
		struct sm_set_sidetone_channel_parms sidetone;

		memset(&sidetone, 0, sizeof(sidetone));

		sidetone.channel   = m_channel;
		sidetone.output    = m_channel;

		rc = sm_set_sidetone_channel(&sidetone);

		if (rc != 0) 
		{
			throw ProsodyError(__FILE__, __LINE__, "sm_set_sidetone_channel", rc);
		}
	}
	else if (rc != 0)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_condition_input", rc);
	}
}

unsigned ProsodyChannel::Beep::start(Media *phone)
{
	struct sm_replay_parms start;

	memset(&start, 0, sizeof(start));

	start.channel = m_prosody->m_channel;
	start.background = kSMNullChannelId;
	start.speed = 100;
	start.type = kSMDataFormat8KHzALawPCM;
	start.data_length = sizeof(beep);

	omni_mutex_lock(m_prosody->m_mutex);

	int rc = sm_replay_start(&start);
	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_start", rc);
	}

	m_state = active;
	m_prosody->m_sending = this;

	log(log_debug, "phone", phone->getName()) 
		<< "beep " << m_beeps << " started" << logend();
	
	process(phone);

	return m_position;
}

unsigned ProsodyChannel::Beep::submit(Media *phone)
{	
	struct sm_ts_data_parms data;
	data.channel = m_prosody->m_channel;
	data.length = min(sizeof(beep) - m_offset, kSMMaxReplayDataBufferSize);
	data.data = (char*)&beep[m_offset];

	int rc = sm_put_replay_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	m_position += data.length / 8;
	m_offset += data.length;

	return data.length;
}

bool ProsodyChannel::Beep::stop(Media *phone, unsigned status)
{
	struct sm_replay_abort_parms p;

	omni_mutex_lock l(m_prosody->m_mutex);

	if (m_state != active)
	{
		log(log_debug+1, "phone", phone->getName()) 
			<< "stopping beep " << m_beeps << " - not active" << logend();

		return false;
	}

	p.channel = m_prosody->m_channel;

	m_state = stopping;
	m_status = status;
	
	int rc = sm_replay_abort(&p);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_abort", rc);

	m_position = (m_count * sizeof(beep) + p.offset) / 8;

	log(log_debug+1, "phone", phone->getName()) 
		<< "stopping beep " << m_beeps << logend();

	return false;
}

// fills prosody buffers if space available, notifies about completion if done
int ProsodyChannel::Beep::process(Media *phone)
{
	struct sm_replay_status_parms replay;
	// we need a local copy because the client might delete us
	// in completed
	ProsodyChannel *p = m_prosody;

	replay.channel = m_prosody->m_channel;
	
	while (true)
	{
		int rc = sm_replay_status(&replay);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_replay_status", rc);

		switch (replay.status)
		{
		case kSMReplayStatusComplete:
			++m_count;
			if (m_count == m_beeps)
			{
				log(log_debug, "phone", phone->getName()) 
					<< "beep " << m_beeps << " done [" << result_name(m_status) 
					<< ',' << m_position << ']' << logend();

				m_state = idle;
				p->m_sending = 0;
				p->m_mutex.unlock();
				phone->completed(this);
				p->m_mutex.lock();
			}
			else
			{
				m_offset = 0;
				m_prosody->m_mutex.unlock();
				start(phone);
				m_prosody->m_mutex.lock();
			}

				return replay.status;
		case kSMReplayStatusUnderrun:
			log(log_error, "phone", phone->getName()) << "underrun!" << logend();
		case kSMReplayStatusHasCapacity:
			submit(phone);
			break;
		case kSMReplayStatusCompleteData:
			return replay.status;
		case kSMReplayStatusNoCapacity:
			return replay.status;
		}
	}

	// unreached statement
	return replay.status;
}

unsigned ProsodyChannel::Touchtones::start(Media *phone)
{
	struct sm_play_digits_parms digits;

	digits.channel = m_prosody->m_channel;
	digits.wait_for_completion = 0;
	digits.digits.type = kSMDTMFDigits;
	digits.digits.digit_duration = 64;
	digits.digits.inter_digit_delay = 16;

	strncpy(digits.digits.digit_string, m_tt.c_str(), kSMMaxDigits);
	digits.digits.digit_string[kSMMaxDigits] = '\0';
	digits.digits.qualifier = 0; // unused parameter

	// warn if digits too long
	if (m_tt.length() > kSMMaxDigits)
	{
		log(log_warning, "phone", phone->getName()) 
			<< "warning: digits string " << m_tt.c_str() 
			<< " too long. truncated to: " << digits.digits.digit_string 
			<< logend();
	}

	omni_mutex_lock(m_prosody->m_mutex);

	int rc = sm_play_digits(&digits);
	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_play_digits", rc);
	}

	m_state = active;
	m_prosody->m_sending = this;

	log(log_debug, "phone", phone->getName()) 
		<< "touchtones " << m_tt.c_str() << " started" << logend();

	// time played
	return 0;
}

bool ProsodyChannel::Touchtones::stop(Media *phone, unsigned status)
{
	omni_mutex_lock l(m_prosody->m_mutex);

	if (m_state != active)
	{
		log(log_debug+1, "phone", phone->getName()) 
			<< "stopping touchtones " << m_tt.c_str() << " - not active" << logend();

		return false;
	}

	m_state = stopping;
	m_status = status;

	int rc = sm_play_tone_abort(m_prosody->m_channel);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_abort", rc);

	log(log_debug, "phone", phone->getName()) 
		<< "touchtones " << m_tt.c_str() << " stopped" << logend();

	phone->completed(this);

	return true;
}

int ProsodyChannel::Touchtones::process(Media *phone)
{
	struct sm_play_tone_status_parms tone;
	// we need a local copy because the client might delete us
	// in completed
	ProsodyChannel *p = m_prosody;

	tone.channel = p->m_channel;

	int rc = sm_play_tone_status(&tone);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_status", rc);

	if (tone.status == kSMPlayToneStatusComplete)
	{
		log(log_debug, "phone", phone->getName()) 
			<< "touchtones " << m_tt.c_str() << " done [" 
			<< result_name(m_status) << ',' << m_position << ']' << logend();

		m_state = idle;
		p->m_sending = 0;
		p->m_mutex.unlock();
		phone->completed(this);
		p->m_mutex.lock();
	}

	return tone.status;
}

void ProsodyChannel::startEnergyDetector(unsigned qualTime)
{
}

void ProsodyChannel::stopEnergyDetector()
{
}

ProsodyChannel::FileSample::FileSample(ProsodyChannel *channel, 
									   const char* file, 
									   bool isRecordable)
 : m_recordable(isRecordable), m_storage(0), m_prosody(channel), m_name(file)
{
	m_storage = allocateStorage(file, isRecordable);
}

ProsodyChannel::FileSample::~FileSample()
{
	delete m_storage;
}

Storage* ProsodyChannel::FileSample::allocateStorage(const char* aName, bool isRecording)
{
	// find the last '/' or '\' if any
	const char *start = strrchr(aName, '/');
	if (!start)
	{
		start = strrchr(aName, '\\');
	}

	// no path delimiter
	if (!start)
	{
		start = aName;
	}
	
	char* dot = strrchr(start, '.');
	Storage* storage;

	// .wav is default
	if (!dot)
	{
		std::string file(aName);
		file += ".wav";

		storage = new WavFileStorage(file.c_str(), kSMDataFormat8KHzALawPCM, isRecording);

		return storage;
	}

	dot++;

	if (_stricmp(dot, "ul") == 0)
	{
		storage = new RawFileStorage(aName, kSMDataFormat8KHzULawPCM, isRecording);
	}
	else if (_stricmp(dot, "al") == 0)
	{
		storage = new RawFileStorage(aName, kSMDataFormat8KHzALawPCM, isRecording);
	}
	else if (_stricmp(dot, "wav") == 0)
	{
		storage = new WavFileStorage(aName, kSMDataFormat8KHzALawPCM, isRecording);
	}
	else
	{
		throw FileFormatError(__FILE__, __LINE__, "AculabMedia::allocateBuffers", "unknown format");
	}

	return storage;
}

unsigned ProsodyChannel::FileSample::getLength() 
{ 
	return m_storage ? 
		m_storage->getLength() * 1000 / m_storage->m_bytesPerSecond	: 0; 
}

unsigned ProsodyChannel::FileSample::start(Media *phone)
{
	struct sm_replay_parms start;

	long offset = m_position * m_storage->m_bytesPerSecond / 1000;

	memset(&start, 0, sizeof(start));

	start.channel = m_prosody->m_channel;
	start.background = kSMNullChannelId;
	start.speed = 100;
	start.type = m_storage->m_encoding;
	start.data_length = m_storage->getLength() - offset;

	m_storage->setPos(offset);

	omni_mutex_lock(m_prosody->m_mutex);

	int rc = sm_replay_start(&start);
	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_start", rc);
	}

	m_state = active;
	m_prosody->m_sending = this;

	process(phone);

	log(log_debug, "phone", phone->getName()) 
		<< "file sample " << m_name.c_str() << " started" << logend();

	return m_position;
}

unsigned ProsodyChannel::FileSample::submit(Media *phone)
{	
	char buffer[kSMMaxReplayDataBufferSize];

	struct sm_ts_data_parms data;

	data.channel = m_prosody->m_channel;
	data.length = m_storage->read(buffer, sizeof(buffer));
	data.data = buffer;

	int rc = sm_put_replay_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	m_position += data.length * 1000 / m_storage->m_bytesPerSecond;

	return data.length;
}

bool ProsodyChannel::FileSample::stop(Media *phone, unsigned status)
{
	struct sm_replay_abort_parms p;

	p.channel = m_prosody->m_channel;

	omni_mutex_lock(m_prosody->m_mutex);

	if (m_state != active)
	{
		log(log_debug+1, "phone", phone->getName()) 
			<< "stopping file sample " << m_name.c_str() << " - not active" << logend();

		return false;
	}

	m_state = stopping;
	m_status = status;

	int rc = sm_replay_abort(&p);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_abort", rc);

	m_position = p.offset * 1000 / m_storage->m_bytesPerSecond;


	log(log_debug+1, "phone", phone->getName()) 
		<< "stopping file sample " << m_name.c_str() << logend();

	return false;
}

// fills prosody buffers if space available, notifies about completion if done
int ProsodyChannel::FileSample::process(Media *phone)
{
	struct sm_replay_status_parms replay;
	// we need a local copy because the client might delete us
	// in completed
	ProsodyChannel *p = m_prosody;

	replay.channel = p->m_channel;
	
	while (true)
	{
		int rc = sm_replay_status(&replay);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_replay_status", rc);

		switch (replay.status)
		{
		case kSMReplayStatusComplete:
			log(log_debug, "phone", phone->getName()) 
				<< "file sample " << m_name.c_str() << " done [" 
				<< result_name(m_status) << ',' << m_position << ']' 
				<< logend();

			m_state = idle;
			p->m_sending = 0;
			p->m_mutex.unlock();
			phone->completed(this);
			p->m_mutex.lock();
			return replay.status;
		case kSMReplayStatusUnderrun:
			log(log_error, "phone", phone->getName()) << "underrun!" << logend();
		case kSMReplayStatusHasCapacity:
			if (!submit(phone))
			{
				// Villa test
				log(log_error, "phone", phone->getName()) << "premature end of data" << logend();
				stop(phone);
				return replay.status;
			}
			break;
		case kSMReplayStatusCompleteData:
			return replay.status;
		case kSMReplayStatusNoCapacity:
			return replay.status;
		}
	}

	// unreached statement
	return replay.status;
}

unsigned ProsodyChannel::RecordFileSample::start(Media *phone)
{
	struct sm_record_parms record;

	memset(&record, 0, sizeof(record));

	record.channel = m_prosody->m_channel;
	record.alt_data_source = kSMNullChannelId;
	record.type = m_storage->m_encoding;
	record.elimination = kSMDRecordToneElimination;
	record.max_octets = 0;
	record.max_elapsed_time = m_maxTime;
	// this shouldn't be hardcoded...
	record.max_silence = 2000;

	omni_mutex_lock(m_prosody->m_mutex);

	int rc = sm_record_start(&record);
	if (rc)
	{
		throw ProsodyError(__FILE__, __LINE__, "sm_record_start", rc);
	}

	m_state = active;
	m_prosody->m_receiving = this;

	log(log_debug, "phone", phone->getName()) 
		<< "recording " << m_name.c_str() << " started" << logend();

	return m_position;
}

bool ProsodyChannel::RecordFileSample::stop(Media *phone, unsigned status)
{
	struct sm_record_abort_parms abort;

	abort.channel = m_prosody->m_channel;
	abort.discard = 0;

	omni_mutex_lock(m_prosody->m_mutex);

	if (m_state != active)
	{
		log(log_debug+1, "phone", phone->getName()) 
			<< "stopping recording " << m_name.c_str() << " - not active" << logend();

		return false;
	}

	m_state = stopping;
	m_status = status;

	int rc = sm_record_abort(&abort);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_record_abort", rc);

	log(log_debug, "phone", phone->getName()) 
		<< "stopping recording " << m_name.c_str() << logend();

	return false;
}

unsigned ProsodyChannel::RecordFileSample::receive(Media *phone)
{	
	char buffer[kSMMaxRecordDataBufferSize];

	struct sm_ts_data_parms data;
	data.channel = m_prosody->m_channel;
	data.length = sizeof(buffer);
	data.data = buffer;

	int rc = sm_get_recorded_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	m_storage->write(data.data, data.length);

	m_position += data.length * 1000 / m_storage->m_bytesPerSecond;

	return m_position;
}

int ProsodyChannel::RecordFileSample::process(Media *phone)
{
	struct sm_record_status_parms record;
	struct sm_record_how_terminated_parms how;

	memset(&record, 0, sizeof(record));
	memset(&how, 0, sizeof(how));

	// we need a local copy because the client might delete us
	// in completed
	ProsodyChannel *p = m_prosody;

	record.channel = p->m_channel;
	how.channel = p->m_channel;
	
	while (true)
	{
		int rc = sm_record_status(&record);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_record_status", rc);

		switch (record.status)
		{
		case kSMRecordStatusComplete:
			log(log_debug, "phone", phone->getName()) 
				<< "recording " << m_name.c_str() << " done [" 
				<< result_name(m_status) << ',' << m_position << ']' 
				<< logend();

			m_state = idle;
			p->m_receiving = 0;
			p->m_mutex.unlock();
			
			/* Retain status codes in the range of successful completions - 
			   m_status may have been set in stop() */
			if (m_status >= V3_WARNING_OFFSET)
			{
				rc = sm_record_how_terminated(&how);
				if (rc)
					throw ProsodyError(__FILE__, __LINE__, "sm_record_how_terminated", rc);

				switch(how.termination_reason)
				{
				case kSMRecordHowTerminatedLength: 
				case kSMRecordHowTerminatedMaxTime:
					m_status = V3_WARNING_TIMEOUT;
					break;
				case kSMRecordHowTerminatedSilence:
					m_status = V3_ENDSILENCE;
					break;
				case kSMRecordHowTerminatedAborted:
					m_status = V3_STOPPED;
					break;
				default:
					m_status = V3_ERROR_FAILED;
					break;
				}
			}

			phone->completed(this);
			p->m_mutex.lock();
			return record.status;
		case kSMRecordStatusOverrun:
			log(log_error, "phone", phone->getName()) << "overrun!" << logend();
		case kSMRecordStatusData:
		case kSMRecordStatusCompleteData:
			receive(phone);
			break;
		case kSMRecordStatusNoData:
			return record.status;
		}
	}

	// unreached statement
	return record.status;
}

void AculabMedia::connected(Trunk* aTrunk)
{
	m_trunk = aTrunk;

	if (m_receive.st == -1 || m_transmit.st == -1)
		log(log_error, "phone", getName()) 
			<< "no transmit or receive timeslot" << logend();

	try
	{
		if (m_info.card == -1)
		{
			struct sm_switch_channel_parms input;
			input.channel = m_channel;
			input.st	  = m_receive.st;
			input.ts	  = m_receive.ts;
			
			int rc = sm_switch_channel_input(&input);
			if (rc)
				throw ProsodyError(__FILE__, __LINE__, "sm_switch_channel_input", rc);

			struct sm_switch_channel_parms output;
			output.channel = m_channel;
			output.st	  = m_transmit.st;
			output.ts	  = m_transmit.ts;
			
			rc = sm_switch_channel_output(&output);
			if (rc)
				throw ProsodyError(__FILE__, __LINE__, "sm_switch_channel_output", rc);
		}
		else
		{
			Timeslot in(m_info.ist, m_info.its);
			Timeslot out(m_info.ost, m_info.ots);

			m_sw.listen(m_transmit, out, getName());
			m_sw.listen(in, m_receive, getName());
		}

		m_sw.listen(aTrunk->getTimeslot(), m_transmit, getName());
		m_sw.listen(m_receive, aTrunk->getTimeslot(), getName());
	}
	catch (const Exception &e)
	{
		log(log_error, "phone") << e << logend();
	}
}

void AculabMedia::disconnected(Trunk *trunk)
{
	m_trunk = 0;

	m_sw.disable(m_receive);
	m_sw.disable(m_transmit);
}

void AculabMedia::onRead(tSMEventId id)
{
	omni_mutex_lock l(m_mutex);

	if (!m_receiving)
	{
		log(log_error, "phone", getName()) 
			<< "got onRead() event while no input active" << logend();
		return;
	}

	try
	{
		if (typeid(*m_receiving) == typeid(ProsodyChannel::RecordFileSample))
		{
			dynamic_cast<RecordFileSample*>(m_receiving)->process(this);
		}
	}
	catch (const Exception& e)
	{
		log(log_error, "phone", getName()) << e << logend();
	}
}

void AculabMedia::onWrite(tSMEventId id)
{
	omni_mutex_lock l(m_mutex);

	if (!m_sending)
	{
		log(log_error, "phone", getName()) 
			<< "got onWrite() event while no output active" << logend();
		return;
	}

	try
	{
		if (typeid(*m_sending) == typeid(ProsodyChannel::Touchtones))
		{
			dynamic_cast<Touchtones*>(m_sending)->process(this);
		}
		else if (typeid(*m_sending) == typeid(ProsodyChannel::FileSample))
		{
			dynamic_cast<FileSample*>(m_sending)->process(this);
		}
		else if (typeid(*m_sending) == typeid(ProsodyChannel::Beep))
		{
			dynamic_cast<Beep*>(m_sending)->process(this);
		}
	}
	catch (const Exception& e)
	{
		log(log_error, "phone", getName()) << e << logend();
	}
}

void AculabMedia::onRecog(tSMEventId id)
{
	struct sm_recognised_parms recog;

	memset(&recog, 0, sizeof(recog));

	for(;;)
	{
		recog.channel = m_channel;

		int rc = sm_get_recognised(&recog);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_abort", rc);

		switch(recog.type)
		{
		case kSMRecognisedDigit:
			if (m_client)
				m_client->touchtone(this, recog.param0);
			else
				log(log_error, "phone", getName()) << "no client for touchtone"<< logend();
			break;
		case kSMRecognisedGruntStart:
			// todo
			break;
		case kSMRecognisedGruntEnd:
			// todo
			break;
		case kSMRecognisedNothing:
			return;
		default:
			break;
		}
	}
}

void AculabMedia::loopback()
{
	m_sw.listen(m_receive, Timeslot(m_info.ost, m_info.ots));
	m_sw.listen(Timeslot(m_info.ist, m_info.its), m_receive);
}