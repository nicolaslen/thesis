open Statedelta;;
open Types;;
open Helpers;;
open Channel;;
open Gssegment;;
open Gsprefix;;
open Streamingserver;;
open Streamingclient;;
open Unix;;
open Inotify;;
open Thread;;
open List;;
open Test_readmywrites;;
open Test_causalarbitration;;
open Test_causalvisibility;;
open Test_consistentprefix;;
open Test_monotonicreads;;
open Test_nocircularcausality;;
open Test_fixes;;

exception Foo of string;;

let argc = Array.length Sys.argv in

if argc = 0 then
	failwith "Arguments required";
let arg1 = Sys.argv.(1) in
if arg1 = "client" && argc = 1 then
	failwith "Arguments required";
let arg2 = if arg1 = "client" then Sys.argv.(2) else "0" in
let testToRun = if arg1 = "client" && argc > 3 then Sys.argv.(3) else "0" in
let testId = if arg1 = "client" && argc > 4 then (int_of_string Sys.argv.(4)) else 0 in

print_endline (if (arg1 = "client") then ("Soy el cliente " ^ Sys.argv.(2)) else ("Soy el servidor"));

let mutexHandler = { mutex = Mutex.create (); currentLine = 1; lines = 0; client = arg2; } in
ignore (Thread.create waitForNewMessagesToReceive mutexHandler);

let rec serverPolling server handler = 
	Mutex.lock handler.mutex;
	if handler.currentLine <= handler.lines then
	begin
		let filename = "../broadcast/receive_0.txt" in
		let (origin, message) = getOriginMessageFromFile filename handler.currentLine in
		let newServer = 
			(if origin > 0 then
			begin
				let round = getRoundFromMessage origin message in
				(*print_endline ("round recibido = " ^ (string_of_round round));*)
				if round.number = (-1) && round.delta = Statedelta.emptydelta
				then (streamingServer_acceptConnection server (channel_create round.origin))
				else (addRoundToClientstream server round)

				(* TODO mejora: addRound en el main habilita processBatch para que cumpla la precondición y processBatch no chequea nada (evita recorrer canales) *)
			end
			else
				server) in

		handler.currentLine <- handler.currentLine + 1;
		(*print_endline ("linePointer = " ^ (string_of_int handler.currentLine) ^ ", lines = " ^ (string_of_int handler.lines));*)
		Mutex.unlock handler.mutex;
		serverPolling newServer handler
	end
	else
	begin
		let newServer = streamingServer_processBatch server in
		if (server != newServer) then 
			print_endline ("streamingServer_processBatch: " ^ (string_of_server newServer));
		
		Mutex.unlock handler.mutex;
		sleep 3;
		serverPolling newServer handler
	end
in




(*
let test1_1 client =
	let client = streamingClient_pull client in
	print_endline ("streamingClient_pull_1: " ^ (string_of_client client));
	let value = streamingClient_read client readexample in
	print_endline ("streamingClient_read_2: " ^ (string_of_value value));
	let client = streamingClient_update client (List.nth updatesexample 0) in
	print_endline ("streamingClient_update_3: " ^ (string_of_client client));
	let value = streamingClient_read client readexample in
	print_endline ("streamingClient_read_4: " ^ (string_of_value value));
	let client = streamingClient_push client in
	print_endline ("streamingClient_push_5: " ^ (string_of_client client));
	let client = streamingClient_send client in
	print_endline ("streamingClient_send_6: " ^ (string_of_client client));	
	let client = streamingClient_update client (List.nth updatesexample 1) in
	print_endline ("streamingClient_update_7 " ^ (string_of_update (List.nth updatesexample 1)) ^ ": " ^ (string_of_client client));
	let client = streamingClient_update client (List.nth updatesexample 2) in
	print_endline ("streamingClient_update_8 " ^ (string_of_update (List.nth updatesexample 2)) ^ ": " ^ (string_of_client client));
	let client = streamingClient_push client in
	print_endline ("streamingClient_push_9: " ^ (string_of_client client));
	let client = streamingClient_send client in
	print_endline ("streamingClient_send_10: " ^ (string_of_client client));	
	let value = streamingClient_read client readexample in
	print_endline ("streamingClient_read_11: " ^ (string_of_value value));
	client
in

let test1_2 client = 
	client
in*)

let rec clientPolling client handler established tests =
	Mutex.lock handler.mutex;
	if handler.currentLine <= handler.lines then
	begin
		(*let filename = "../broadcast/receive_" ^ (if handler.client = "2" then "3" else handler.client) ^ ".txt" in*)
		let filename = "../broadcast/receive_" ^ handler.client ^ ".txt" in
		let (origin, message) = getOriginMessageFromFile filename handler.currentLine in
		let newClient, estab = 
			(if origin = 0 then
				let gs, target = getSegmentPrefixFromMessage origin message in

				let targetId = if target then 1 else 2 in
				let isForMe = match gs with
						GSSegment _ -> true
					| 	GSPrefix _ 	-> client.id = targetId in

				let client = 
					if isForMe 
					then (
						let c = streamingClient_receive (addSegmentPrefixToServerstream client gs) in
						match gs with
							GSSegment _ -> c
						| 	GSPrefix _ 	-> streamingClient_pull c
					)
					else client
				in

				client, (established || isForMe)
			else
				client, established) in

		handler.currentLine <- handler.currentLine + 1;
		(*print_endline ("linePointer = " ^ (string_of_int handler.currentLine) ^ ", lines = " ^ (string_of_int handler.lines));*)
		Mutex.unlock handler.mutex;
		clientPolling newClient handler estab tests
	end
	else if established && ((length tests) > 0) then
		begin
			Mutex.unlock handler.mutex;
			sleep 3;

			let newClient = (hd tests) client in
			
			let nextTests = 
				if (newClient.id = 2 && 
					(testToRun = "readmywrites" || 
					(testToRun = "monotonicreads" && (int_of_value (streamingClient_read newClient readexample)) = 0) ||
					testToRun = "causalvisibility" || 
					testToRun = "consistentprefix")) 
				then tests 
				else tl tests
			in
			
			clientPolling newClient handler established nextTests
		end
	else if established && ((length tests) = 0) then
		begin
			Mutex.unlock handler.mutex;
			sleep 3;
			
			let newClient = streamingClient_pull client in
				(*print_endline ("streamingClient_pull: " ^ (string_of_client newClient));*)
			clientPolling newClient handler established tests
		end
	else
		begin
			Mutex.unlock handler.mutex;
			sleep 3;
			clientPolling client handler established tests
		end
in


if arg1 = "server" then
begin
	let server = streamingServer_create () in
	serverPolling server mutexHandler
end
else if arg1 = "client" then
begin
	let clientId = int_of_string arg2 in
	let clientAux = streamingClient_create clientId in
	let client = streamingClient_sendConnectionRequest clientAux in
	let readMyWrites = readMyWrites_test1 clientId in
	let monotonicReads = monotonicReads_test clientId in
	let consistentPrefix = consistentPrefix_test clientId in
	let noCircularCausality = noCircularCausality_test clientId in
	let causalArbitration = causalArbitration_test clientId in
	let causalVisibility = causalVisibility_test clientId in
	let testFixes = fixes_test clientId in

	firstExport ((String.uppercase_ascii testToRun) ^ " " ^ (string_of_int testId));

	let runningTest = (
		if testToRun = "readmywrites" then
			readMyWrites testId
		else if testToRun = "monotonicreads" then
			monotonicReads testId
		else if testToRun = "consistentprefix" then
			consistentPrefix
		else if testToRun = "nocircularcausality" then
			noCircularCausality testId
		else if testToRun = "causalarbitration" then
			causalArbitration
		else if testToRun = "causalvisibility" then
			causalVisibility
		else testFixes) in

	clientPolling client mutexHandler false runningTest;
end