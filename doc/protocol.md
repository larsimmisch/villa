# The Villa protocol

The _Villa_ is a Telephone-based multi-user dungeon.
This document describes the _Villa protocol_ that is spoken between the 
game logic and an audio renderer. The protocol is a simple, text oriented 
protocol on top of TCP, inspired by IMAP. It provides sophisticated 
operations for the realtime manipulation of audio playlists and basic call 
control.

## Overview

The protocol has commands:

- `LSTN` listen
- `DISC` disconnect
- `ACPT` accept
- `MLCA` molecule add
- `MLCD` molecule discard
- `MLDP` molecule discard by priority
- `CNFO` conference open
- `CNFC` conference close

And events:

- `DTMF` DTMF detected
- `RDIS` Remote disconnect
- `ABEG` Atom beginning to run
- `AEND` Atom ending

A message is either a request, a completion or an event.

## Basic structure of a request

The basic structure of a request is:

`tid device command|event arguments`

`tid` is a transaction id. Most often, it will be a number, but it can 
also be alphanumerical. The string`-1` is reserved and must not be used
(it is used for unsolicited events).

Transactions IDs are used to correlate a completion to a request.

`device` is a name picked by the rendering engine. It uniquely identifies 
a channel, normally a telephone line and associated speech processing 
capabilities.

`arguments` depends on `command|event` and is a list of arguments to the
request, separated by whitespace.

A simple example of a request is:
```
0 GLOBAL LSTN any any
```

This instructs the audio renderer to wait for an incoming call on any trunk 
and any number.

When a call comes in, the audio renderer will respond with:

```
0 <channelId> LSTN 
```

## Molecules

notification := 'start' | 'stop' | 'both'

atom := play notification | record notification | dtmf notification | beep notification
	| silence notification | conference notification

play := `play` filename

record := `record` filename maxLengthMS

dtmf := `dtmf` digits 

beep := `beep` count

silence := `silence` lengthMS

conference := `conference` identifier