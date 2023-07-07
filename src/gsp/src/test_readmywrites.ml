open Statedelta;;
open Streamingclient;;
open Unix;;

let readMyWrites_test1_c1 client =
	let client = streamingClient_update client (update_of_int 10) in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_push client in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_send client in
	ignore(streamingClient_read client readexample);
	client

let readMyWrites_test1_c2 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	client

let readMyWrites_test1 clientId testId = 
	if clientId = 1 
	then (readMyWrites_test1_c1 :: [])
	else (readMyWrites_test1_c2 :: [])