open Statedelta;;
open Types;;
open List;;
open Unix;;
open Inotify;;
open Printf;;
open Maps;;

exception Foo of string
(*
let rec print_writes = function 
	[] -> ()
	| e::l -> match e.update with
		| Wr a -> print_string "wr(" ; print_int a ; print_string ")\n" ; print_writes l

let rec print_clientGSP_rounds rounds = 
	match rounds with
		[] -> ""
		| r::l -> 
		let updateNumber = match r.update with Wr a -> a in
		("\n      { origin: " ^ 
		(string_of_int r.origin) ^ "; number: " ^
		(string_of_int r.number) ^ "; update: " ^ (string_of_int updateNumber) ^ "; }" ^
		print_clientGSP_rounds l)

let print_clientGSP c = 
	let id = string_of_int c.id in
	let round = string_of_int c.round in
	print_endline ("clientGSP = {\n   id = " ^ id ^ ";\n   known = [" ^ (print_clientGSP_rounds c.known) ^ "];\n   pending = [" ^ (print_clientGSP_rounds c.pending) ^ "];\n   round = " ^ round ^ "\n}")

let rec print_rounds = function 
	[] -> ()
	| r::l -> print_round r; print_rounds l

let rec print_list = function 
	[] -> ()
	| e::l -> print_endline e ; print_list l
*)
let firstExport s =
	let out_channel = open_out_gen [Open_append; Open_creat] 0o666 "output.txt" in
	let strtoprint = (s ^ "\n") in
	Printf.fprintf out_channel "%s" strtoprint;
	close_out out_channel

let export c s =
	let cstr = string_of_int c in
	let out_channel = open_out_gen [Open_append; Open_creat] 0o666 "output.txt" in
	let strtoprint = ("C" ^ cstr ^ "_" ^ s ^ "\n") in
	Printf.fprintf out_channel "%s" strtoprint;
	close_out out_channel

let rec string_of_list l fc =
	let rec aux = (fun lst f ->
		(match lst with
			[] 		-> "]"
		|	[a]		-> (f a) ^ "]"
		|	a :: ls -> ((f a) ^ ", " ^ (aux ls f)))) in
	"[" ^ (aux l fc)

let string_of_round r = 
	"{ 
			origin: " ^ (string_of_int r.origin) ^ "; 
			number: " ^ (string_of_int r.number) ^ "; 
			delta: " ^ (string_of_delta r.delta) ^ ";
	}"

let string_of_option o f = 
	match o with 
			Some a 	-> "Some " ^ (f a)
		| 	None 	-> "None"

let string_of_bool b =
	(if b then "true" else "false")

let rec string_of_maxround (c, m) =
	(string_of_int c) ^ ": " ^ (string_of_int m)


let string_of_prefix prefix =
	"{
		state : " ^ (string_of_state prefix.state) ^ "; 
		maxround : " ^ (string_of_list (Rounds.bindings prefix.maxround) string_of_maxround) ^ "; 
	}"

let string_of_segment segment =
	"{
		delta : " ^ (string_of_delta segment.delta) ^ "; 
		maxround : [" ^ (string_of_list (Rounds.bindings segment.maxround) string_of_maxround)  ^ "]; 
	}"

let string_of_gs gs =
	match gs with
		GSSegment a -> (string_of_segment a)
	| 	GSPrefix a 	-> (string_of_prefix a)

let string_of_channel (ch : channelGSP) = 
		"{ 
			client : " ^ (string_of_int ch.client) ^ ";
			clientstream : " ^ (string_of_list ch.clientstream string_of_round) ^ ";
			serverstream : " ^ (string_of_list ch.serverstream string_of_gs) ^ ";
			accepted : " ^ (string_of_bool ch.accepted) ^ ";
			receivebuffer : " ^ (string_of_list ch.receivebuffer string_of_gs) ^ ";
			established : " ^ (string_of_bool ch.established) ^ ";
		}"

let rec string_of_connections (c, m) =
	(string_of_int c) ^ ": " ^ (string_of_channel m)

let string_of_client c = 
	"{ 
		id : " ^ (string_of_int c.id) ^ "; 
		known : " ^ (string_of_state c.known) ^ ";
		pending : " ^ (string_of_list c.pending string_of_round) ^ ";
		round : " ^ (string_of_int c.round) ^ ";
		transactionbuf : " ^ (string_of_delta c.transactionbuf) ^ ";
		tbuf_empty : " ^ (string_of_bool c.tbuf_empty) ^ "; 
		channel : " ^ (string_of_option c.channel string_of_channel) ^ ";
		pushbuf : " ^ (string_of_delta c.pushbuf) ^ ";
		rds_in_pushbuf : " ^ (string_of_int c.rds_in_pushbuf) ^ ";
	}"

let string_of_server s = 
	"{ 
		serverstate : " ^ (string_of_prefix s.serverstate) ^ ";
		connections : " ^ (string_of_list (Channels.bindings s.connections) string_of_connections) ^ ";
	}"

let input_line_opt in_channel = 
	try Some (input_line in_channel)
	with End_of_file -> None

let getLineFromFile filename currentLine =
	let n = currentLine in
	let ic = open_in filename in
	let rec aux i =
		match input_line_opt ic with
		| Some line ->
			if i = n then line else aux (succ i)
		| None ->
			close_in ic;
			failwith "Error de formato en el archivo"
	in
	aux 1

let getRoundFromMessage origin message =
	let (delta, number) = Statedelta.decode2Numbers message in
	let round = { origin = origin; number = number; delta = delta } in
	round

let getOriginMessageFromFile filename currentLine =
	let line = getLineFromFile filename currentLine in
	let split = String.split_on_char ':' line in
	(int_of_string (List.nth split 0), int_of_string (List.nth split 1))

let getSegmentPrefixFromMessage origin message =
	let (isSegment, target, data, maxround1, maxround2) = Statedelta.decode4Numbers message in

	let maxround = Rounds.add 2 maxround2 (Rounds.add 1 maxround1 Rounds.empty) in
	let gs = 
		if isSegment 
		then GSSegment { delta = delta_of_string data; maxround = maxround } 
		else GSPrefix { state = state_of_string data; maxround = maxround } in
	gs, target

let rec getRoundsFromMessages messages =
	match messages with
	| [] -> []
	| m::l -> 
		let split = String.split_on_char ':' m in
		let origin = int_of_string (List.nth split 0) in
		let message = int_of_string (List.nth split 1) in
		let (delta, number) = Statedelta.decode2Numbers message in
		let round = { origin = origin; number = number; delta = delta } in
		round :: (getRoundsFromMessages l)

let rtobRoundSubmit r = 
	let clientIdString = string_of_int r.origin in
	let filename = ("../broadcast/send_" ^ clientIdString ^ ".txt") in
	let out_channel = open_out_gen [Open_append; Open_creat] 0o666 filename in
	let message = Statedelta.encode2Numbers r.delta r.number in
	Printf.fprintf out_channel "%d\n" message;
	close_out out_channel

let rtobSegmentPrefixSubmit gs target = 
	let filename = ("../broadcast/send_0.txt") in
	let out_channel = open_out_gen [Open_append; Open_creat] 0o666 filename in
	
	let isSegment = match gs with
						GSSegment _ -> true
					| 	GSPrefix _ 	-> false in
	
	let data = match gs with
						GSSegment a -> Statedelta.string_of_delta a.delta
					| 	GSPrefix a 	-> Statedelta.string_of_state a.state in

	let maxround = match gs with
						GSSegment a -> a.maxround
					| 	GSPrefix a 	-> a.maxround in
	
	let maxround1_opt = Rounds.find_opt 1 maxround in
	let maxround2_opt = Rounds.find_opt 2 maxround in

	let maxround1 = match maxround1_opt with
						Some a -> a
					| 	None -> 0 in

	let maxround2 = match maxround2_opt with
						Some a -> a
					| 	None -> 0 in

	let message = Statedelta.encode4Numbers isSegment target data maxround1 maxround2 in
	
	Printf.fprintf out_channel "%d\n" message;
	close_out out_channel

let rtobConnectionRequestSubmit c = 
	let clientIdString = string_of_int c in
	let filename = ("../broadcast/send_" ^ clientIdString ^ ".txt") in
	let out_channel = open_out_gen [Open_append; Open_creat] 0o666 filename in
	let message = Statedelta.encode2Numbers Statedelta.emptydelta (-1) in
	Printf.fprintf out_channel "%d\n" message;
	close_out out_channel

let countFileLines filename = 
	let ic = open_in filename in
	let rec aux a =
		match input_line_opt ic with
			| Some line -> (1 + (aux ()))
			| None -> close_in ic; 0 in
	aux ()

let waitForNewMessagesToReceive s = 
	(*print_endline "waitForNewMessagesToReceive init";*)
	let inotify = Inotify.create () in
	(*let filename = "../broadcast/receive_" ^ (if s.client = "2" then "3" else s.client) ^ ".txt" in*)
	let filename = "../broadcast/receive_" ^ s.client ^ ".txt" in
	ignore (Inotify.add_watch inotify filename [Inotify.S_Modify]);
	(*print_endline ("shared memory: " ^ filename);*)

	while true do
		(*print_endline (Inotify.string_of_event (List.hd (Inotify.read inotify)));*)
		(*print_endline "waitingToReceive...";*)
		ignore (Inotify.read inotify);
		(*print_endline "waitingToReceive reading...";*)

		let lines = countFileLines filename in

		Mutex.lock s.mutex;

		s.lines <- lines;
		(*print_endline ("linePointer = " ^ (string_of_int s.currentLine) ^ ", lines = " ^ (string_of_int s.lines));*)

		Mutex.unlock s.mutex;
	done

let addRoundToClientstream server round =
	(*let channel = Rounds.find round.origin server.connections in*)
	let channel_opt = Rounds.find_opt round.origin server.connections in
	
	let channel = match channel_opt with
						Some ch -> ch
					| 	None -> failwith "No hay canal" in

	let clientstream = channel.clientstream@[round] in
	let newChannel = { channel with clientstream = clientstream } in

	let newConnections = Channels.add round.origin newChannel server.connections in
	print_endline ("serverPolling - agregando un round a clientstream: " ^ (string_of_round round));
	{ server with connections = newConnections }

let addSegmentPrefixToServerstream client gs =
	let ch = match client.channel with 
			Some a 	-> a 
		| 	None 	-> failwith "Channel disconnected" in
	
	let serverstream = ch.serverstream@[gs] in
	let channel = { ch with serverstream = serverstream } in
	(*print_endline ("clientPolling - agregando un gs a serverstream: " ^ (string_of_gs gs));*)
	{ client with channel = Some channel }

let rec last_element list = 
	match list with 
		| [] -> failwith "Empty list"
		| [x] -> x
		| f ::ls -> last_element ls