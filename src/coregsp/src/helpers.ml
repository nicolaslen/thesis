open Types;;
open List;;
open Unix;;
open Inotify;;
open Printf;;

let rec print_writes = function 
	[] -> ()
	| e::l -> print_string "wr(" ; print_int e.update ; print_string ")\n" ; print_writes l

let string_of_round r = 
	("{ origin: " ^ (string_of_int r.origin) ^ "; number: " ^ (string_of_int r.number) ^ "; update: " ^ (string_of_int r.update) ^ "; }")

let rec string_of_rounds rounds = 
	match rounds with
		[] 		-> ""
		| r::l 	-> ("\n      " ^ (string_of_round r) ^ (string_of_rounds l))

let string_of_clientGSP c = 
	let id = string_of_int c.id in
	let round = string_of_int c.round in
	("{\n   id = " ^ id ^ ";\n   known = [" ^ (string_of_rounds c.known) ^ "];\n   pending = [" ^ (string_of_rounds c.pending) ^ "];\n   round = " ^ round ^ "\n}")

let rec print_list = function 
	[] -> ()
	| e::l -> print_endline e ; print_list l

let input_line_opt in_channel = 
	try Some (input_line in_channel)
	with End_of_file -> None

let getLineFromFile filename currentLine =
	let n = currentLine.counter in
	let ic = open_in filename in
	let rec aux i =
		match input_line_opt ic with
		| Some line ->
			if i = n then begin
				currentLine.counter <- currentLine.counter + 1;
		  		line
			end else aux (succ i)
		| None ->
			close_in ic;
			failwith "Error de formato en el archivo"
	in
	aux 1

let encodeRound r =
	let bits0000ffff = 65535 in
	let roundNumber = r.number land bits0000ffff in
	let updateNumberShifted = r.update lsl 16 in
	let orResult = updateNumberShifted lor roundNumber in
	orResult (* updateNumber ++ roundNumber *)

let decodeRound n =
	let bits0000ffff = 65535 in
	let roundNumber = n land bits0000ffff in
	let updateNumber = n lsr 16 in
	{ origin = 0; number = roundNumber; update = updateNumber }

let getRoundFromFile filename currentLine =
	let line = getLineFromFile filename currentLine in
	let split = String.split_on_char ':' line in
	let origin = int_of_string (List.nth split 0) in
	let message = int_of_string (List.nth split 1) in
	let decodedRound = decodeRound message in
	let round = { origin = origin; number = decodedRound.number; update = decodedRound.update } in
	round

let getUpdateFromFile filename currentLine =
	let line = getLineFromFile filename currentLine in
	let intOfLine = int_of_string line in
	intOfLine

let rec getRoundsFromMessages messages =
	match messages with
	| [] -> []
	| m::l -> 
		let split = String.split_on_char ':' m in
		let origin = int_of_string (List.nth split 0) in
		let message = int_of_string (List.nth split 1) in
		let decodedRound = decodeRound message in
		let round = { origin = origin; number = decodedRound.number; update = decodedRound.update } in
		round :: (getRoundsFromMessages l)

let rtobSubmit r = 
	let clientIdString = string_of_int r.origin in
	let filename = ("../broadcast/send_" ^ clientIdString ^ ".txt") in
	let out_channel = open_out_gen [Open_append; Open_creat] 0o666 filename in
	let message = encodeRound r in
	Printf.fprintf out_channel "%d\n" message;
	close_out out_channel

let waitForNewMessagesToSend s = 
	let inotify = Inotify.create () in
	let filename = "usersender_" ^ s.client ^ ".txt" in
	close_out (open_out_gen [Open_trunc; Open_creat] 0o666 filename);
	
	let watch   = Inotify.add_watch inotify filename [Inotify.S_Close_write] in
	print_endline ("   Thread sender iniciado, esperando cambios en " ^ filename);

	while true do
		Inotify.read inotify;
		sleep 1;
		print_endline ("   Thread sender detectó un cambio en " ^ filename);

		Mutex.lock s.lock;
		s.send <- s.send + 1;
		Mutex.unlock s.lock
	done

let waitForNewMessagesToReceive s = 
	let inotify = Inotify.create () in
	let filename = "../broadcast/receive_" ^ s.client ^ ".txt" in
	let watch   = Inotify.add_watch inotify filename [Inotify.S_Modify] in
	print_endline ("   Thread receiver iniciado, esperando cambios en " ^ filename);

	while true do
		(*print_endline (Inotify.string_of_event (List.hd (Inotify.read inotify)));*)
		Inotify.read inotify;
		sleep 1;
		print_endline ("   Thread receiver detectó un cambio en " ^ filename);
		
		Mutex.lock s.lock;
		s.receive <- s.receive + 1;
		Mutex.unlock s.lock
	done

let rec last_element list = 
	match list with 
		| [] -> failwith "Empty list"
		| [x] -> x
		| f ::ls -> last_element ls