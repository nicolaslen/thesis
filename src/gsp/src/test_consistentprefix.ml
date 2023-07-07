open Statedelta;;
open Streamingclient;;
open Unix;;

let consistentPrefix_test1_c1_1 client =
	let client = streamingClient_update client (update_of_int 5) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let consistentPrefix_test1_c1_2 client =
	sleep 4;
	client

let consistentPrefix_test1_c1_3 client =
	let client = streamingClient_update client (update_of_int 10) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let consistentPrefix_test1_c2 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	sleep 1;
	client

let consistentPrefix_test clientId = 
	if clientId = 1 
	then (consistentPrefix_test1_c1_1 :: consistentPrefix_test1_c1_2 :: consistentPrefix_test1_c1_3 :: []) 
	else (consistentPrefix_test1_c2 :: [])