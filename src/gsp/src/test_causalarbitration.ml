open Statedelta;;
open Streamingclient;;
open Unix;;

let causalArbitration_test1_c1_1 client =
	let client = streamingClient_update client (update_of_int 5) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let causalArbitration_test1_c1_2 client =
	sleep 4;
	client

let causalArbitration_test1_c1_3 client =
	let v = streamingClient_read client readexample in
	let client = streamingClient_update client (update_of_int (2 * (int_of_value v))) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let causalArbitration_test1_c2 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	sleep 1;
	client

let causalArbitration_test clientId = 
	if clientId = 1 
	then (causalArbitration_test1_c1_1 :: causalArbitration_test1_c1_2 :: causalArbitration_test1_c1_3 :: []) 
	else (causalArbitration_test1_c2 :: [])