open Statedelta;;
open Maps;;

type maxround = int Rounds.t

type round = { 
	origin : client; 
	number : int; 
	delta : delta 
}

type gsPrefixGSP = { (* representa un prefijo de la secuencia de actualización global *)
	state : state; 
	maxround : maxround; 
}

type gsSegmentGSP = { (* representa un intervalo (delta) de la secuencia de actualización global *)
	delta : delta; 
	maxround : maxround; 
}

type gs = GSPrefix of gsPrefixGSP | GSSegment of gsSegmentGSP

type channelGSP = { 
	client : client; (* inmutable *)

	(* duplex streams *)
	clientstream : round list; (* client to server *) 
	serverstream : gs list; (* server to client *)
	
	(* server-side connection state *)
	accepted : bool; (* whether server has accepted connection *)
	
	(* client-side connection state *)
	receivebuffer : gs list;  (* locally buffered packets *)
	established : bool; (* whether client processed 1st packet *)
}

type connections = channelGSP Channels.t

type streamingServerGSP = { 
	serverstate : gsPrefixGSP; (* persistent state *)
	connections : connections; (* soft state *)
}

type streamingClientGSP = { 
	id : client; 
	known : state; (* known prefix *)
	pending : round list; (* sent, but unconfirmed rounds *)
	round : int; (* counts submitted rounds *)
	transactionbuf : delta;
	tbuf_empty : bool; 
	channel : channelGSP option; (* current connection (or null if there is none) *)
	pushbuf : delta; (* updates that were pushed, but not sent yet *)
	rds_in_pushbuf : int; (* counts the number of rounds in the pushbuffer *)
}

type status = { mutex : Mutex.t; mutable currentLine : int; mutable lines : int; client : string }
type lineCounter = { mutable counter : int }