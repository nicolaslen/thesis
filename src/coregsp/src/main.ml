open Types;;
open Helpers;;
open Client;;
open Unix;;
open Inotify;;
open Thread;;

let handler = { lock = Mutex.create (); send = 0; receive = 0; client = Sys.argv.(1) } in
let currentLineReceive = { counter = 1 } in
let currentLineSend = { counter = 1 } in

let threadReceiving = Thread.create waitForNewMessagesToReceive handler in
let threadSending = Thread.create waitForNewMessagesToSend handler in


let rec mainPolling c s last p = 
	Mutex.lock s.lock;
	if s.receive > 0 && (s.send = 0 || last = 0) then
	begin
		let clientId = string_of_int c.id in
		let filename = "../broadcast/receive_" ^ clientId ^ ".txt" in
		let round = getRoundFromFile filename currentLineReceive in
		let newC = onReceive c round in
		print_endline ("clientGSP = " ^ (string_of_clientGSP newC));
		s.receive <- s.receive - 1;
		Mutex.unlock s.lock;
		mainPolling newC s 1 true
	end
	else if s.send > 0 && (s.receive = 0 || last = 1) then
	begin
		let clientId = string_of_int c.id in
		let filename = "usersender_" ^ clientId ^ ".txt" in
		let update = getUpdateFromFile filename currentLineSend in
		let newC = client_update c update in
		print_endline ("clientGSP = " ^ (string_of_clientGSP newC));
		s.send <- s.send - 1;
		Mutex.unlock s.lock;
		mainPolling newC s 0 true
	end
	else 
	begin
		Mutex.unlock s.lock;

		let readvalue = client_read c () in
		if p then
		(print_endline (match readvalue with 
			Some w ->  ("Thread inicial esperando... client_read = " ^ (string_of_int w))
		|	None -> "Thread inicial esperando..."));

		sleep 3;
		mainPolling c s last false
	end
in

let clientId = int_of_string Sys.argv.(1) in
let c = client_create clientId in
mainPolling c handler 0 true;

print_endline "end!";;