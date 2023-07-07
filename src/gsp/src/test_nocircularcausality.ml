open Statedelta;;
open Streamingclient;;
open Unix;;

let noCircularCausality_test1_c1 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_update client (update_of_int 10) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	sleep 10;
	client

let noCircularCausality_test1_c2 client =
	let client = streamingClient_pull client in
	ignore(streamingClient_read client readexample);
	let client = streamingClient_update client (update_of_int 5) in
	let client = streamingClient_push client in
	let client = streamingClient_send client in
	sleep 10;
	client

let noCircularCausality_test clientId testId = 
	if clientId = 1 
	then (noCircularCausality_test1_c1 :: noCircularCausality_test1_c1 :: [])
	else (noCircularCausality_test1_c2 :: noCircularCausality_test1_c2 :: [])