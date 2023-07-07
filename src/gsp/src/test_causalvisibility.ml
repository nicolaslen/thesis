open Statedelta;;
open Streamingclient;;
open Unix;;

let causalVisibility_test1_c1_1 client =
	let client = streamingClient_update client (update_of_int 5) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let causalVisibility_test1_c1_2 client =
	sleep 4;
	client

let causalVisibility_test1_c1_3 client =
	let v = streamingClient_read client readexample in
	let client = streamingClient_update client (update_of_int (2 * (int_of_value v))) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let causalVisibility_test1_c2 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	sleep 1;
	client

let causalVisibility_test clientId = 
	if clientId = 1 
	then (causalVisibility_test1_c1_1 :: causalVisibility_test1_c1_2 :: causalVisibility_test1_c1_3 :: []) 
	else (causalVisibility_test1_c2 :: [])