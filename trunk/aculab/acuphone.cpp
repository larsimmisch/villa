/*
	acuphone.cpp

	$Id: acuphone.cpp,v 1.3 2000/11/06 13:10:59 lars Exp $

	Copyright 2000 ibp (uk) Ltd.

	Author: Lars Immisch <lars@ibp.de>
*/

#pragma warning (disable: 4786)

#include <iostream>
#include <iomanip>
#include <typeinfo.h>
#include "mvswdrvr.h"
#include "acuphone.h"
#include "prosody_error.i"
#include "beep.i"

const char* result_name(int r)
{
	switch (r)
	{
	case r_ok:
		return "r_ok";
	case r_timeout:
		return "r_timeout";
	case r_aborted: 
		return "r_aborted";
	case r_rejected: 
		return "r_rejected";
	case r_disconnected:
		return "r_disconnected";
	case r_failed:
		return "r_failed";
	case r_invalid: 
		return "r_invalid";
	case r_busy:
		return "r_busy";
	case r_not_available:
		return "r_not_available";
	case r_no_dialtone:
		return "r_no_dialtone";
	case r_empty:
		return "r_empty";
	case r_bad_state:
		return "r_bad_state";
	case r_number_changed:
		return "r_number_changed";
	default:
		return "unknown";
	}
};


ProsodyEventDispatcher ProsodyChannel::dispatcher;

void AculabSwitch::listen(const Timeslot &a, const Timeslot &b)
{
	OUTPUT_PARMS args;

    args.ost = a.st;
    args.ots = a.ts;
    args.mode = CONNECT_MODE;
    args.ist = b.st;
    args.its = b.ts;

	log(log_debug, "switch") << a.st << ':' << a.ts << " := " << b.st << ':' << b.ts << logend();

    int rc = sw_set_output(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::listen(Timeslot,Timeslot)", "set_output(CONNECT_MODE)", rc);

}

void AculabSwitch::listen(const Timeslot &a, char pattern)
{
	OUTPUT_PARMS args;

    args.ost = a.st;
    args.ots = a.ts;
    args.mode = PATTERN_MODE;
    args.pattern = pattern;

	log(log_debug, "switch") << a.st << ':' << a.ts << " := 0x" << std::setbase(16) << pattern << std::setbase(10) << logend();

    int rc = sw_set_output(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::listen(Timeslot,char)", "set_output(PATTERN_MODE)", rc);
}

void AculabSwitch::disable(const Timeslot &a)
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
}

char AculabSwitch::sample(const Timeslot &a)
{
	SAMPLE_PARMS args;

	args.ist = a.st;
	args.its = a.ts;

    int rc = sw_sample_input0(device, &args);
    if (rc != 0)    
		throw SwitchError(__FILE__,__LINE__,"AculabSwitch::sample(Timeslot)", "sw_sample_input0", rc);

	return args.sample;
}
	
Timeslot AculabSwitch::query(const Timeslot &a)
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
	if (running)
		throw Exception(__FILE__, __LINE__, "ProsodyEventDispatcher::add", "cannot add observers to running dispatcher");

	methods.push_back(ObserverMethod(observer, method));
	events.push_back(event);
}

void ProsodyEventDispatcher::start()
{
	running = true;

	for (int i = 0; i <= (events.size() / max_observers_per_thread); ++i)
	{
		int l = min(events.size() - i * max_observers_per_thread, max_observers_per_thread);

		DispatcherThread* d = new DispatcherThread(*this, i * max_observers_per_thread, l);
		threads.push_back(d);

		d->start();
	}
}

void ProsodyEventDispatcher::dispatch(int offset)
{
	if (offset >= methods.size())
	{
		log(log_error, "phone") << "ProsodyEventDispatcher::dispatch: offset " << offset << " out of bounds" << logend;
		return;
	}

	(methods[offset].observer->*methods[offset].method)(events[offset]);
}

void* ProsodyEventDispatcher::DispatcherThread::run_undetached(void *arg)
{
	HANDLE* handles = &dispatcher.events[offset];

	while (true)
	{
		DWORD rc = WaitForMultipleObjects(length, handles, FALSE, INFINITE);
		if (rc < WAIT_OBJECT_0 || rc > WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS)
		{
			log(log_error, "phone") << "ProsodyEventDispatcherThread: WaitForMultipleObjects failed:" << GetLastError() << logend();

			continue;
		}
		if (rc - WAIT_OBJECT_0 > length)
		{
			log(log_error, "phone") << "ProsodyEventDispatcherThread: WaitForMultipleObjects returned index we did not wait upon (" 
				<< rc - WAIT_OBJECT_0 << ")" << logend();

			continue;
		}

		dispatcher.dispatch(offset + rc - WAIT_OBJECT_0);
	}

	return NULL;
}

ProsodyChannel::ProsodyChannel()
{
	struct sm_channel_alloc_parms alloc;
	alloc.type		  = kSMChannelTypeFullDuplex;
	alloc.firmware_id = 0;
	alloc.group		  = 0;
	alloc.caps_mask	  = 0;
	
	int rc = sm_channel_alloc(&alloc);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_channel_alloc", rc);

	channel = alloc.channel;
	
	listenFor.channel = channel;
	listenFor.enable_pulse_digit_recognition = 0;
	listenFor.tone_detection_mode = kSMToneLenDetectionMinDuration64; // signal event at end of tone, to avoid recording the tone
	listenFor.active_tone_set_id = 0;	// use given (i.e. DTMF/FAX) tone set
	listenFor.map_tones_to_digits = kSMDTMFToneSetDigitMapping;
	listenFor.enable_cptone_recognition = 0;
	listenFor.enable_grunt_detection = 0;
	listenFor.grunt_latency = 0;
	
	rc = sm_listen_for(&listenFor);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_listen_for", rc);


	// initialize our events
	eventRead = set_event(kSMEventTypeReadData);
	eventWrite = set_event(kSMEventTypeWriteData);
	eventRecog = set_event(kSMEventTypeRecog);

	// and add them to the dispatcher
	dispatcher.add(eventRead, this, onRead);
	dispatcher.add(eventWrite, this, onWrite);
	dispatcher.add(eventRecog, this, onRecog);

	// cache channel info
	info.channel = channel;
	rc = sm_channel_info(&info);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_channel_info", rc);
}

tSMEventId ProsodyChannel::set_event(tSM_INT type)
{
	struct sm_channel_set_event_parms event;
	event.channel	   = channel;
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

unsigned ProsodyChannel::Beep::start(Telephone *phone)
{
	struct sm_replay_parms start;

	start.channel = prosody->channel;
	start.background = kSMNullChannelId;
	start.speed = 100;
	start.agc = 0;
	start.volume = 0;
	start.type = kSMDataFormat8KHzALawPCM;
	start.data_length = sizeof(beep);

	omni_mutex_lock(phone->getMutex());

	int rc = sm_replay_start(&start);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_start", rc);

	log(log_debug, "phone") << "beep " << beeps << " started" << logend();
	
	phone->started(this);

	process(phone);

	return position;
}

unsigned ProsodyChannel::Beep::submit(Telephone *phone)
{	
	struct sm_ts_data_parms data;
	data.channel = prosody->channel;
	data.length = min(sizeof(beep) - offset, kSMMaxReplayDataBufferSize);
	data.data = (char*)&beep[offset];

	int rc = sm_put_replay_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	position += data.length / 8;
	offset += data.length;

	return position;
}

bool ProsodyChannel::Beep::stop(Telephone *phone)
{
	struct sm_replay_abort_parms p;

	p.channel = prosody->channel;

	int rc = sm_replay_abort(&p);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_abort", rc);

	position = (count * sizeof(beep) + p.offset) / 8;

	status = r_aborted;

	log(log_debug+1, "phone") << "stopping beep " << beeps << logend();

	return false;
}

// fills prosody buffers if space available, notifies about completion if done
int ProsodyChannel::Beep::process(Telephone *phone)
{
	struct sm_replay_status_parms replay;

	replay.channel = prosody->channel;
	
	while (true)
	{
		int rc = sm_replay_status(&replay);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_replay_status", rc);

		switch (replay.status)
		{
		case kSMReplayStatusComplete:
			++count;
			if (count == beeps)
			{
				log(log_debug, "phone") << "beep " << beeps << " done [" << result_name(status) << ',' << position << ']' << logend();

				phone->completed(this);
			}
			else
			{
				offset = 0;
				start(phone);
			}
			return replay.status;
		case kSMReplayStatusUnderrun:
			log(log_error, "phone") << "underrun!" << logend();
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

unsigned ProsodyChannel::Touchtones::start(Telephone *phone)
{
	struct sm_play_digits_parms digits;

	digits.channel = prosody->channel;
	digits.wait_for_completion = 0;
	digits.digits.type = kSMDTMFDigits;
	digits.digits.digit_duration = 64;
	digits.digits.inter_digit_delay = 16;

	strncpy(digits.digits.digit_string, tt.c_str(), kSMMaxDigits);
	digits.digits.digit_string[kSMMaxDigits] = '\0';
	digits.digits.qualifier = 0; // unused parameter

	// warn if digits too long
	if (tt.length() > kSMMaxDigits)
	{
		log(log_warning, "phone") << "warning: digits string " << tt.c_str() << " too long. truncated to: " << digits.digits.digit_string << logend();
	}

	int rc = sm_play_digits(&digits);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_digits", rc);

	log(log_debug, "phone") << "touchtones " << tt.c_str() << " started" << logend();

	phone->started(this);

	// time played
	return 0;
}

bool ProsodyChannel::Touchtones::stop(Telephone *phone)
{
	int rc = sm_play_tone_abort(prosody->channel);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_abort", rc);

	status = r_aborted;

	log(log_debug, "phone") << "touchtones " << tt.c_str() << " stopped" << logend();

	phone->completed(this);

	return true;
}

int ProsodyChannel::Touchtones::process(Telephone *phone)
{
	struct sm_play_tone_status_parms tone;

	tone.channel = prosody->channel;

	int rc = sm_play_tone_status(&tone);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_status", rc);

	if (tone.status == kSMPlayToneStatusComplete)
	{
		log(log_debug, "phone") << "touchtones " << tt.c_str() << " done [" << result_name(status) << ',' << position << ']' << logend();
		phone->completed(this);
	}

	return tone.status;
}

void ProsodyChannel::startEnergyDetector(unsigned qualTime)
{
}

void ProsodyChannel::stopEnergyDetector()
{
}

Storage* ProsodyChannel::FileSample::allocateStorage(const char* aName, bool isRecording)
{
	char* dot = strrchr(aName, '.');
	Storage* storage;

	if (!dot)
		throw FileFormatError(__FILE__, __LINE__, "AculabPhone::allocateStorage", "unknown format");

	dot++;

	if (_stricmp(dot, "ul") == 0)
	{
		storage = new RawFileStorage(aName, isRecording);
		storage->encoding = kSMDataFormat8KHzULawPCM;
		storage->bytesPerSecond = 8000;
	}
	else if (_stricmp(dot, "al") == 0)
	{
		storage = new RawFileStorage(aName, isRecording);
		storage->encoding = kSMDataFormat8KHzALawPCM;
		storage->bytesPerSecond = 8000;
	}
	else
	{
		throw FileFormatError(__FILE__, __LINE__, "AculabPhone::allocateBuffers", "unknown format");
	}

	return storage;
}

unsigned ProsodyChannel::FileSample::start(Telephone *phone)
{
	struct sm_replay_parms start;

	start.channel = prosody->channel;
	start.background = kSMNullChannelId;
	start.speed = 100;
	start.agc = 0;
	start.volume = 0;
	start.type = storage->encoding;
	start.data_length = storage->getLength();

	omni_mutex_lock(phone->getMutex());

	int rc = sm_replay_start(&start);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_start", rc);

	log(log_debug, "phone") << "file sample " << name.c_str() << " started" << logend();

	phone->started(this);

	process(phone);

	return position;
}

unsigned ProsodyChannel::FileSample::submit(Telephone *phone)
{	
	char buffer[kSMMaxReplayDataBufferSize];

	struct sm_ts_data_parms data;
	data.channel = prosody->channel;
	data.length = storage->read(buffer, sizeof(buffer));

	data.data = buffer;

	int rc = sm_put_replay_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	position += data.length * 1000 / storage->bytesPerSecond;

	return position;
}

bool ProsodyChannel::FileSample::stop(Telephone *phone)
{
	struct sm_replay_abort_parms p;

	p.channel = prosody->channel;

	int rc = sm_replay_abort(&p);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_abort", rc);

	position = p.offset * 1000 / storage->bytesPerSecond;

	status = r_aborted;

	log(log_debug+1, "phone") << "stopping file sample " << name.c_str() << logend();

	return false;
}

// fills prosody buffers if space available, notifies about completion if done
int ProsodyChannel::FileSample::process(Telephone *phone)
{
	struct sm_replay_status_parms replay;

	replay.channel = prosody->channel;
	
	while (true)
	{
		int rc = sm_replay_status(&replay);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_replay_status", rc);

		switch (replay.status)
		{
		case kSMReplayStatusComplete:
			log(log_debug, "phone") << "file sample " << name.c_str() << " done [" << result_name(status) << ',' << position << ']' << logend();
			phone->completed(this);
			return replay.status;
		case kSMReplayStatusUnderrun:
			log(log_error, "phone") << "underrun!" << logend();
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

unsigned ProsodyChannel::RecordFileSample::start(Telephone *phone)
{
	struct sm_record_parms record;

	record.channel = prosody->channel;
	record.alt_data_source = kSMNullChannelId;
	record.type = storage->encoding;
	record.elimination = 0;
	record.max_octets = 0;
	record.max_elapsed_time = maxTime;
	// this shouldn't be hardcoded...
	record.max_silence = 1000;

	omni_mutex_lock(phone->getMutex());

	int rc = sm_record_start(&record);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_record_start", rc);

	phone->started(this);

	log(log_debug, "phone") << "recording " << name.c_str() << " started" << logend();

	return position;
}

bool ProsodyChannel::RecordFileSample::stop(Telephone *phone)
{
	struct sm_record_abort_parms abort;

	abort.channel = prosody->channel;
	abort.discard = 0;

	int rc = sm_record_abort(&abort);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_record_abort", rc);

	log(log_debug, "phone") << "stopping recording " << name.c_str() << logend();

	return false;
}

unsigned ProsodyChannel::RecordFileSample::receive(Telephone *phone)
{	
	char buffer[kSMMaxRecordDataBufferSize];

	struct sm_ts_data_parms data;
	data.channel = prosody->channel;
	data.length = sizeof(buffer);
	data.data = buffer;

	int rc = sm_get_recorded_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	storage->write(data.data, data.length);

	position += data.length * 1000 / storage->bytesPerSecond;

	return position;
}

int ProsodyChannel::RecordFileSample::process(Telephone *phone)
{
	struct sm_record_status_parms record;

	record.channel = prosody->channel;
	
	while (true)
	{
		int rc = sm_record_status(&record);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_record_status", rc);

		switch (record.status)
		{
		case kSMRecordStatusComplete:
			log(log_debug, "phone") << "recording " << name.c_str() << " done [" << result_name(status) << ',' << position << ']' << logend();
			phone->completed(this);
			return record.status;
		case kSMRecordStatusOverrun:
			log(log_error, "phone") << "overrun!" << logend();
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

void AculabPhone::connected(Trunk* aTrunk)
{
	if (receive.st == -1 || transmit.st == -1)
		log(log_error, "phone") << "no transmit or receive timeslot" << logend();

	try
	{
		sw.listen(aTrunk->getTimeslot(), transmit);
		sw.listen(receive, aTrunk->getTimeslot());

		if (info.card == -1)
		{
			struct sm_switch_channel_parms input;
			input.channel = channel;
			input.st	  = receive.st;
			input.ts	  = receive.ts;
			
			int rc = sm_switch_channel_input(&input);
			if (rc)
				throw ProsodyError(__FILE__, __LINE__, "sm_switch_channel_input", rc);

			struct sm_switch_channel_parms output;
			output.channel = channel;
			output.st	  = transmit.st;
			output.ts	  = transmit.ts;
			
			rc = sm_switch_channel_output(&input);
			if (rc)
				throw ProsodyError(__FILE__, __LINE__, "sm_switch_channel_output", rc);
		}
		else
		{
			Timeslot in(info.ist, info.its);
			Timeslot out(info.ost, info.ots);

			sw.listen(transmit, out);
			sw.listen(in, receive);
		}
	}
	catch (const Exception &e)
	{
		log(log_error, "phone") << e << logend();
	}

	client->connected(this);
}

void AculabPhone::disconnected(Trunk *trunk, int cause)
{
	abortSending();

	sw.disable(receive);
	sw.disable(transmit);

	client->disconnected(this);
}

void AculabPhone::onRead(tSMEventId id)
{
	omni_mutex_lock l(mutex);

	if (!current)
	{
		log(log_error, "phone") << "got onRead() event while no output active" << logend();
		return;
	}

	try
	{
		if (typeid(*current) == typeid(ProsodyChannel::RecordFileSample))
		{
			dynamic_cast<FileSample*>(current)->process(this);
		}
	}
	catch (const Exception& e)
	{
		log(log_error, "phone") << e << logend();
	}
}

void AculabPhone::onWrite(tSMEventId id)
{
	omni_mutex_lock l(mutex);

	if (!current)
	{
		log(log_error, "phone") << "got onWrite() event while no output active" << logend();
		return;
	}

	try
	{
		if (typeid(*current) == typeid(ProsodyChannel::Touchtones))
		{
			dynamic_cast<Touchtones*>(current)->process(this);
		}
		else if (typeid(*current) == typeid(ProsodyChannel::FileSample))
		{
			dynamic_cast<FileSample*>(current)->process(this);
		}
		else if (typeid(*current) == typeid(ProsodyChannel::Beep))
		{
			dynamic_cast<Beep*>(current)->process(this);
		}
	}
	catch (const Exception& e)
	{
		log(log_error, "phone") << e << logend();
	}
}

void AculabPhone::onRecog(tSMEventId id)
{
	struct sm_recognised_parms recog;

	recog.channel = channel;

	int rc = sm_get_recognised(&recog);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_abort", rc);

	switch(recog.type)
	{
	case kSMRecognisedDigit:
		client->touchtone(this, recog.param0);
		break;
	case kSMRecognisedGruntStart:
		// todo
		break;
	case kSMRecognisedGruntEnd:
		// todo
		break;
	default:
		break;
	}
}
