/*
	acuphone.cpp

	$Id: acuphone.cpp,v 1.2 2000/10/30 11:38:57 lars Exp $

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

ProsodyEventDispatcher ProsodyChannel::dispatcher;

void AculabSwitch::listen(const Timeslot &a, const Timeslot &b)
{
	OUTPUT_PARMS args;

    args.ost = a.st;
    args.ots = a.ts;
    args.mode = CONNECT_MODE;
    args.ist = b.st;
    args.its = b.ts;

#ifdef _DEBUG
	std::cout << "switch: " << a.st << ':' << a.ts << " := " << b.st << ':' << b.ts << std::endl;
#endif

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

#ifdef _DEBUG
	std::cout << "switch: " << a.st << ':' << a.ts << " := 0x" << std::setbase(16) << pattern << std::setbase(10) << std::endl;
#endif

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
		std::cerr << "ProsodyEventDispatcher::dispatch: offset " << offset << " out of bounds" << std::endl;
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
			std::cerr << "ProsodyEventDispatcherThread: WaitForMultipleObjects failed:" << GetLastError() << std::endl;

			continue;
		}
		if (rc - WAIT_OBJECT_0 > length)
		{
			std::cerr << "ProsodyEventDispatcherThread: WaitForMultipleObjects returned index we did not wait upon (" 
				<< rc - WAIT_OBJECT_0 << ")" << std::endl;

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
	return 0;
}

bool ProsodyChannel::Beep::stop(Telephone *phone)
{
	return false;
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
		std::cerr << "warning: digits string " << tt.c_str() << " too long. truncated to: " << digits.digits.digit_string << std::endl;
	}

	int rc = sm_play_digits(&digits);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_digits", rc);

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

	phone->completed(this);

	return true;
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

	int rc = sm_replay_start(&start);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_start", rc);

	process(phone);

	phone->started(this);

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

	return false;
}

// fills prosody buffers if space available, notifies about completion if done
int ProsodyChannel::FileSample::process(Telephone *phone)
{
	struct sm_replay_status_parms status;

	status.channel = prosody->channel;
	
	while (true)
	{
		int rc = sm_replay_status(&status);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_replay_status", rc);

		switch (status.status)
		{
		case kSMReplayStatusComplete:
			phone->completed(this);
			return status.status;
		case kSMReplayStatusUnderrun:
			std::cerr << "underrun!" << std::endl;
		case kSMReplayStatusHasCapacity:
			submit(phone);
			break;
		case kSMReplayStatusCompleteData:
			return status.status;
		case kSMReplayStatusNoCapacity:
			return status.status;
		}
	}

	// unreached statement
	return status.status;
}

unsigned ProsodyChannel::RecordFileSample::start(Telephone *phone)
{
	return 0;
}

bool ProsodyChannel::RecordFileSample::stop(Telephone *phone)
{
	return false;
}

void AculabPhone::connected(Trunk* aTrunk)
{
	if (receive.st == -1 || transmit.st == -1)
		std::cerr << "no transmit or receive timeslot" << std::endl;

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
		e.printOn(std::cerr);
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
	int x = 9;
}

void AculabPhone::onWrite(tSMEventId id)
{
	if (!current)
	{
		std::cerr << "got onWrite() event while no output active" << std::endl;
		return;
	}

	try
	{
		if (typeid(*current) == typeid(ProsodyChannel::Touchtones))
		{
			struct sm_play_tone_status_parms status;

			status.channel = channel;

			int rc = sm_play_tone_status(&status);
			if (rc)
				throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_status", rc);

			if (status.status == kSMPlayToneStatusComplete)
			{
				completed(current);
			}
		}
		else if (typeid(*current) == typeid(ProsodyChannel::FileSample))
		{
			dynamic_cast<FileSample*>(current)->process(this);
		}
	}
	catch (const Exception& e)
	{
		std::cerr << e << std::endl;
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
