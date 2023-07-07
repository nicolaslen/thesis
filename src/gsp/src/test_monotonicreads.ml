open Statedelta;;
open Streamingclient;;
open Unix;;

let monotonicReads_test1_c1 client =
	let client = streamingClient_update client (update_of_int 5) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	client

let monotonicReads_test1_c2_1 client =
	sleep 2;
	let client = streamingClient_pull client in
	client

let monotonicReads_test1_c2_2 client =
	let client = streamingClient_update client (update_of_int 10) in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_push client in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_send client in
	ignore(streamingClient_read client readexample);
	client
	
let monotonicReads_test1_c2_3 client =
	let client = streamingClient_update client (update_of_int 20) in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_push client in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_send client in
	ignore(streamingClient_read client readexample);
	client
	
let monotonicReads_test clientId testId = 
	if clientId = 1 
	then (monotonicReads_test1_c1 :: [])
	else (monotonicReads_test1_c2_1 :: monotonicReads_test1_c2_2 :: monotonicReads_test1_c2_3 :: [])
	