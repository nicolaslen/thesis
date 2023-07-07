open Statedelta;;
open Streamingclient;;
open Unix;;

let fixes_test_c1_1 client =
	sleep 30;
	client

let fixes_test_c1_2 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	client

let fixes_test_c2_1 client =
	let client = streamingClient_update client (update_of_int (-15)) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let fixes_test clientId = 
	if clientId = 1 
	then (fixes_test_c1_1 :: fixes_test_c1_2 :: fixes_test_c2_1 :: fixes_test_c1_2 :: [])
	else (fixes_test_c2_1 :: fixes_test_c1_1 :: fixes_test_c1_2 :: [])