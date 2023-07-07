open Statedelta;;
open Types;;
open Maps;;
open Channel;;
open Helpers;;
open List;;

exception Foo of string

(* PRIVATE FUNCTIONS *)

let rec adjust_pending_queue (pending : round list) (maxround : int) : round list =
	if (length pending) = 0 then
		pending
	else
		(let round = hd pending in
		if maxround >= round.number then
			adjust_pending_queue (tl pending) maxround
		else
			pending)

let gsSegmentPull (c : streamingClientGSP) (segment : gsSegmentGSP) (gslist : gs list) : streamingClientGSP =
	(*assert(s instanceof GSSegment);*)

	let known = Statedelta.apply c.known [segment.delta] in 
	let maxround = match Rounds.find_opt c.id segment.maxround with
		| None -> 0
		| Some m -> m in

	let pending = adjust_pending_queue c.pending maxround in
	let ch = match c.channel with 
			Some a 	-> 	a 
		| 	None 	->	failwith "Channel disconnected" in
	let channel = { ch with receivebuffer = gslist; } in
	{ c with known 		= known; 
			 pending 	= pending;
			 channel 	= Some channel; }

let rec appendMultipleToClientsream (ch : channelGSP) (rounds : round list) : channelGSP =
	match rounds with
			[] 		-> ch
		| r :: rs  	-> appendMultipleToClientsream (channel_appendToClientstream ch r) rs
	
let gsPrefixPull (c : streamingClientGSP) (prefix : gsPrefixGSP) (gslist : gs list) : streamingClientGSP = 
	(*assert(s instanceof GSPrefix);*)
	let maxround = match Rounds.find_opt c.id prefix.maxround with
		| None -> 0
		| Some m -> m in

	let pending = adjust_pending_queue c.pending maxround in
	let ch = match c.channel with 
			Some a 	-> 	a 
		| 	None 	->	failwith "Channel disconnected" in

	let channelAux = appendMultipleToClientsream ch pending in

	let channel = { channelAux with 
							receivebuffer = gslist; 
							established = true } in
	
	{ c with known 		= prefix.state; 
			 pending 	= pending;
			 channel 	= Some channel; }
	
let clientPull (c : streamingClientGSP) (gs : gs) (gslist : gs list) : streamingClientGSP =
	let ch = match c.channel with 
			Some a 	-> a 
		| 	None 	-> failwith "Channel disconnected" in
	if ch.established then 
		(let segment = match gs with
			GSSegment a -> a
		| 	GSPrefix _ 	-> failwith "GS type error" in
		(gsSegmentPull c segment gslist))
	else
		(let prefix = match gs with 
			GSPrefix a 	-> a
		| 	GSSegment _ -> failwith "GS type error" in
		(gsPrefixPull c prefix gslist))

let rec deltas (seq : round list) : delta list = 
	match seq with
		[] 		-> []
	| 	r :: rs -> r.delta :: (deltas rs)

let curstate (c : streamingClientGSP) : state =
	let deltas1 = deltas c.pending in
	let deltas2 = if c.rds_in_pushbuf = 0 then deltas1 else deltas1@[c.pushbuf] in
	let deltas3 = if c.tbuf_empty then deltas2 else deltas2@[c.transactionbuf] in
	Statedelta.apply c.known deltas3

(* PUBLIC FUNCTIONS *)
let streamingClient_create clientId =
	{ id = clientId; known = Statedelta.initialstate; pending = []; round = 0; transactionbuf = Statedelta.emptydelta; tbuf_empty = true; channel = None; pushbuf = Statedelta.emptydelta; rds_in_pushbuf = 0 }

let streamingClient_read c r =
	let state = curstate c in
	let result = Statedelta.rvalue r state in

	let toprint = ("R" ^ (string_of_value result)) in
	print_endline toprint;
	export c.id toprint;

	result

let streamingClient_update c u =
	let toprint = ("W" ^ string_of_update u) in
	print_endline toprint;
	export c.id toprint;

	let transactionbuf = Statedelta.append c.transactionbuf u in
	{ c with transactionbuf = transactionbuf; 
			 tbuf_empty 	= false; }

let streamingClient_confirmed client =
	let confirmed = (client.pending = [] && client.rds_in_pushbuf = 0 && client.tbuf_empty) in
	confirmed

let streamingClient_send c =
	(*requires channel != null && channel.established && rds_in_pushbuf > 0*)
	let ch = match c.channel with 
			Some a 	-> a 
		| 	None 	-> failwith "Channel disconnected" in
	if ch.established = false then
		failwith "The channel is not established";

	if c.rds_in_pushbuf > 0 then
		(let round = { origin = c.id; number = c.round - 1; delta = c.pushbuf } in
		let channel = channel_appendToClientstream ch round in

		{ c with 	pending 		= c.pending@[round]; 
						channel 		= Some channel; 
						pushbuf 		= Statedelta.emptydelta; 
						rds_in_pushbuf 	= 0; })
	else
		c (*failwith "There is not any rounds into the pushbuffer";*)

let streamingClient_push client =
	if client.tbuf_empty then
		failwith "There is not any rounds into the transactionbuffer";

	let toprint = "PUSH" in
	print_endline toprint;
	export client.id toprint;

	
	let pushbuf = Statedelta.reduce (client.pushbuf :: [client.transactionbuf]) in
	{ client with 	round 			= client.round + 1; 
					transactionbuf 	= Statedelta.emptydelta; 
					tbuf_empty 		= true; 
					pushbuf 		= pushbuf; 
					rds_in_pushbuf 	= client.rds_in_pushbuf + 1 }

let rec streamingClient_pull client =
	let ch = match client.channel with 
			Some a 	-> a 
		| 	None 	-> failwith "Channel disconnected" in
	let newClient = match ch.receivebuffer with
		[] 		-> client
	| 	gs :: gslist -> streamingClient_pull (clientPull client gs gslist) in
	
	let established = if ch.established then false else
		match newClient.channel with 
			Some a -> a.established
		|   None -> false in

	(if established then
		let toprint = "ACCEPTED" in
		print_endline toprint;
		export client.id toprint);
	(if newClient != client && (length ch.receivebuffer = 1) then
		let toprint = "PULL" in
		print_endline toprint;
		export client.id toprint);
	newClient

let streamingClient_receive client =
	(*requires channel != None && channel.serverstream.length > 0*)	
	let ch = match client.channel with 
			Some a 	-> a 
		| 	None 	-> failwith "Channel disconnected" in

	if length ch.serverstream = 0 then
		failwith "There are not any packets to receive";

	let gs = hd ch.serverstream in
	let serverstream = tl ch.serverstream in
	let receivebuffer = ch.receivebuffer@[gs] in
	let channel = { ch with 	serverstream 	= serverstream; 
	 		  					receivebuffer 	= receivebuffer; } in
	let newClient = { client with channel = Some channel } in
	(*print_endline ("streamingClient_receive: " ^ (string_of_client newClient));*)
	newClient

let streamingClient_dropConnection client = 
	{ client with channel = None; }

let streamingClient_sendConnectionRequest c =
	(*requires channel == null;*) 
	let channelCreated = match c.channel with 
			Some _ 	-> true
		|	None -> false in

	if channelCreated then
		failwith "The channel was already connected";

	let channel = channel_create c.id in
	(*print_endline ("streamingClient_sendConnectionRequest - solicitando conexión...");*)
	rtobConnectionRequestSubmit c.id;
	{ c with channel = Some channel; }