/*
	acuphone.cpp

	$Id: acuphone.cpp,v 1.1 2000/10/18 16:58:42 lars Exp $

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

unsigned AculabPhone::startBeeps(int beeps)
{
	return 0;
}

bool AculabPhone::stopBeeps()
{
	return false;
}

unsigned AculabPhone::startTouchtones(const char* tt)
{
	struct sm_play_digits_parms digits;

	digits.channel = channel;
	digits.wait_for_completion = 0;
	digits.digits.type = kSMDTMFDigits;
	digits.digits.digit_duration = 64;
	digits.digits.inter_digit_delay = 16;

	strncpy(digits.digits.digit_string, tt, kSMMaxDigits);
	digits.digits.digit_string[kSMMaxDigits] = '\0';
	digits.digits.qualifier = 0; // unused parameter

	// warn if digits too long
	if (strlen(tt) > kSMMaxDigits)
	{
		std::cerr << "warning: digits string " << tt << " too long. truncated to: " << digits.digits.digit_string << std::endl;
	}

	int rc = sm_play_digits(&digits);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_play_digits", rc);

	// time played
	return 0;
}

bool AculabPhone::stopTouchtones()
{
	omni_mutex_lock l(mutex);

	if (current && typeid(current) == typeid(Telephone::Touchtones))
	{
		int rc = sm_play_tone_abort(channel);
		if (rc)
			throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_abort", rc);

		current->completed(0);
		client->completed(this, current, 0);

		current = NULL;

		return true;
	}

	return false;
}

unsigned AculabPhone::startEnergyDetector(unsigned qualTime)
{
	return 0;
}

bool AculabPhone::stopEnergyDetector()
{
	return false;
}

Buffers* AculabPhone::allocateBuffers(const char* aName, unsigned numBuffers, bool isRecording)
{
	char* dot = strrchr(aName, '.');
	Buffers* buffers;

	if (!dot)
		throw FileFormatError(__FILE__, __LINE__, "AculabPhone::allocateBuffers", "unknown format");

	dot++;

	if (_stricmp(dot, "ul") == 0)
	{
		buffers = new Buffers(new RawFileStorage(aName, isRecording), numBuffers, kSMMaxReplayDataBufferSize);
		buffers->setEncoding(kSMDataFormat8KHzULawPCM);
	}
	else if (_stricmp(dot, "al") == 0)
	{
		buffers = new Buffers(new RawFileStorage(aName, isRecording), numBuffers, kSMMaxReplayDataBufferSize);
		buffers->setEncoding(kSMDataFormat8KHzALawPCM);
	}
	else
	{
		throw FileFormatError(__FILE__, __LINE__, "AculabPhone::allocateBuffers", "unknown format");
	}

	return buffers;
}

unsigned AculabPhone::startPlaying()
{
	Telephone::FileSample *sample = dynamic_cast<Telephone::FileSample*>(current);
	Buffers *buffers = sample->getBuffers();

	struct sm_replay_parms start;

	start.channel = channel;
	start.background = kSMNullChannelId;
	start.speed = 100;
	start.agc = 0;
	start.volume = 0;
	start.type = buffers->getEncoding();
	start.data_length = buffers->getSize();

	int rc = sm_replay_start(&start);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_start", rc);

	int status;
	for (int i = 0; i < buffers->getNumBuffers(); ++i)
	{
		status = checkReplayStatus(sample);
		if (status == kSMReplayStatusComplete || status == kSMReplayStatusCompleteData)
			break;
	}

	return sample->position;
}

unsigned AculabPhone::submitPlaying()
{	
	Telephone::FileSample *sample = dynamic_cast<Telephone::FileSample*>(current);
	Buffers *buffers = sample->getBuffers();

	struct sm_ts_data_parms data;

	data.channel = channel;
	data.data = (char*)buffers->getCurrent();
	data.length = buffers->getCurrentSize();

	int rc = sm_put_replay_data(&data);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_put_replay_data", rc);

	++(*buffers);

	return data.length * 1000 / buffers->getBytesPerSecond();
}

bool AculabPhone::stopPlaying()
{
	Telephone::FileSample *sample = dynamic_cast<Telephone::FileSample*>(current);
	Buffers *buffers = sample->getBuffers();

	struct sm_replay_abort_parms p;

	p.channel = channel;

	int rc = sm_replay_abort(&p);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_abort", rc);

	int t = p.offset * 1000 / buffers->getBytesPerSecond();

	sample->status = r_aborted;

	current = NULL;

	return true;
}

int AculabPhone::checkReplayStatus(Telephone::FileSample *sample)
{
	struct sm_replay_status_parms status;

	status.channel = channel;
	
	int rc = sm_replay_status(&status);
	if (rc)
		throw ProsodyError(__FILE__, __LINE__, "sm_replay_status", rc);

	switch (status.status)
	{
	case kSMReplayStatusComplete:

		sample->completed(sample->getPos());
		client->completed(this, sample, sample->getPos());

		current = NULL;
		break;
	case kSMReplayStatusUnderrun:
		std::cerr << "underrun!" << std::endl;
	case kSMReplayStatusHasCapacity:

		sample->next(this);

		break;
	case kSMReplayStatusCompleteData:
		break;
	}

	return status.status;
}

unsigned AculabPhone::startRecording(unsigned maxTime)
{
	return 0;
}


unsigned AculabPhone::submitRecording()
{
	return 0;
}

bool AculabPhone::stopRecording()
{
	return true;
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
		if (typeid(*current) == typeid(Telephone::Touchtones))
		{
			struct sm_play_tone_status_parms status;

			status.channel = channel;

			int rc = sm_play_tone_status(&status);
			if (rc)
				throw ProsodyError(__FILE__, __LINE__, "sm_play_tone_status", rc);

			if (status.status == kSMPlayToneStatusComplete)
			{
				current->completed(0);
				client->completed(this, current, 0);

				current = NULL;
			}
		}
		else if (typeid(*current) == typeid(Telephone::FileSample))
		{
			checkReplayStatus(dynamic_cast<FileSample*>(current));
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
