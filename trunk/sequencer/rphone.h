/*
    rphone.h

    Author: Lars Immisch lars@ibp.de
 
*/

#ifndef _RPHONE_H_
#define _RPHONE_H_

#define sequencer_port 2103
#define interface_port 2104

enum { notify_start = 0x01, notify_done = 0x02 };

enum { recognizer_isolated, recognizer_contiguous };

enum
{
	if_describe,
	if_describe_done,
    if_open_conference,
    if_open_conference_done,
    if_close_conference,
    if_close_conference_done,
	if_listen,
	if_listen_done,
	if_connect,
	if_connect_done,
	if_stop_listening,
	if_stop_listening_done,
	if_shutdown
};

/*
    currently the discard_*_done packets are not sent when an active entity is discarded.
    This is probably undesireable in some cases. If anyone needs these packets, please let me know.
*/

enum 
{ 
    phone_connect = 1,
    phone_listen,
    phone_accept,
    phone_accept_done,
    phone_reject,
    phone_reject_done,
    phone_disconnect,
    phone_disconnect_done,
    phone_connect_request,
    phone_connect_done,
	phone_connect_remote_ringing,
	phone_transfer,
	phone_transfer_done,
    phone_stop_listening,
    phone_add_activity,
    phone_add_activity_done,
    phone_activity_done,
    phone_add_molecule,
    phone_add_molecule_done,
    phone_molecule_done,
    phone_discard_activity,
    phone_discard_activity_done,            // is sent when an inactive activity is discarded, otherwise expect phone_activity_done
    phone_discard_molecule,
    phone_discard_molecule_done,            // is sent when an inactive molecule is discarded, otherwise expect phone_molecule_done
    phone_discard_molecule_priority,
    phone_discard_molecule_priority_done,    // is sent when an inactive molecule is discarded, otherwise expect phone_molecule_done
    phone_switch_activity,
    phone_switch_activity_done,
    phone_touchtones,
    phone_atom_started,
    phone_atom_done,
	phone_start_recognition,
	phone_stop_recognition,
	phone_recognition,
	phone_shutdown,
	phone_abort
};

enum
{
    _ok,
    _timeout,
    _aborted,
    _rejected,
	_disconnected,
	_failed,
	_not_implemented,
	_invalid, 
	_busy,
	_not_available,
	_network_unreachable,
    _duplicate,
    _empty,
	_server_down,
	_out_of_service
};

enum
{
    atom_record_file_sample,
    atom_play_file_sample,
    atom_beep,
    atom_conference,
	atom_touchtones,
	atom_silence
};


enum
{ 
    conf_listen = 0x01, 
    conf_speak = 0x02, 
    conf_play = 0x04
};

#endif
