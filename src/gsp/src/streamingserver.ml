open Statedelta;;
open Types;;
open Maps;;
open List;;
open Gsprefix;;
open Gssegment;;
open Channel;;
open Helpers;;

let countexample = 1

exception Foo of string

(* PRIVATE FUNCTIONS *)
let rec appendRoundsToSegment (rounds : round list) (segment : gsSegmentGSP) (count : int) : gsSegmentGSP =
	match rounds with
		[] 		-> segment
	| 	r :: rs -> 	if count == 0 then
						segment
					else
						appendRoundsToSegment rs (gssegment_append segment r) (count - 1)

let rec tailnth (ls : 'a list) (n : int) : 'a list =
	if (n > 0) then
		tailnth (tl ls) (n - 1)
	else
		ls

let receive (segment : gsSegmentGSP) (ch : channelGSP) (count : int) : (gsSegmentGSP * channelGSP) = 
	(*requires count <= ch.clientstream.length;*)
	if count > (length ch.clientstream) then
		failwith "There are not as many rounds as desired";

	let newsegment = appendRoundsToSegment ch.clientstream segment count in
	let channel = { ch with clientstream = (tailnth ch.clientstream count); } in
	newsegment, channel

let rec receiveConnections (connectionsList : (client * channelGSP) list) (segment : gsSegmentGSP) (connections : connections) : (gsSegmentGSP * connections) =
	match connectionsList with
		[] 		-> (segment, connections)
	|	(c, ch) :: cs -> 
			if (length ch.clientstream) >= countexample then
				(let (newsegment, channel) = (receive segment ch countexample) in
				let conns = Channels.add c channel connections in
				receiveConnections cs newsegment conns)
			else
				(let conns = Channels.add c ch connections in
				receiveConnections cs segment conns)

let rec appendSegmentToChannels (connectionsList : (client * channelGSP) list) (segment : gsSegmentGSP) (connections : connections) : connections =
	match connectionsList with
		[] 		-> connections
	| 	(c, ch) :: cs -> 	
		let channel = { ch with serverstream = ch.serverstream@[(GSSegment segment)]; } in
		let conns = (Channels.add c channel connections) in
		appendSegmentToChannels cs segment conns

let rec isAnyClientstreamWithRounds (connectionsList : (client * channelGSP) list) : bool =
	match connectionsList with
		[] 		-> false
	| 	(c, ch) :: cs -> if (length ch.clientstream) >= countexample then true else (isAnyClientstreamWithRounds cs)

(* PUBLIC FUNCTIONS *)
let streamingServer_create a = 
	{ serverstate = gsprefix_create (); connections = Channels.empty }

let streamingServer_acceptConnection server ch = 
	(*requires !ch.accepted && connections[ch.client] = null;*)
	if ch.accepted || (Channels.mem ch.client server.connections) then
		failwith "Attempt to accept a connection that has already been accepted";
	
	let channelaux = { ch with accepted = true; } in
	
	(*send first packet: current state*)
	let channel = channel_appendToServerstream channelaux (GSPrefix server.serverstate) in

	let connections = Channels.add channel.client channel server.connections in
	print_endline ("serverPolling - conexión aceptada para cliente " ^ (string_of_int ch.client) ^ "...");
	{ server with connections = connections }

let streamingServer_processBatch server = 
	let connectionsList = Channels.bindings server.connections in

	(* chequear que al menos una conexión tenga count elementos en clientstream *)
	if isAnyClientstreamWithRounds connectionsList then
		(let newSegment = gssegment_create () in

		(*collect updates from all incoming segments*)
		let segment, connections = receiveConnections connectionsList newSegment Channels.empty in
		
		(* atomically commit changes to persistent state *)
		let prefix = gsprefix_apply server.serverstate segment in

		(* notify connected clients *)
		let newconn = appendSegmentToChannels (Channels.bindings connections) segment Channels.empty in

		rtobSegmentPrefixSubmit (GSSegment segment) false; (* rtobSubmit *)
		
		(*print_endline ("serverPolling - broadcast de segment: " ^ (string_of_segment segment));*)
		
		{ serverstate = prefix; connections = newconn })
	else
		server 

let streamingServer_dropConnection server c = 
	let connections = Channels.remove c server.connections in
	{ server with connections = connections }

let streamingServer_crashAndRecover server =
	{ server with connections = Channels.empty }
