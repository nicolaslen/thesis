open Types;;
open List;;
open Helpers;;

exception Foo of string

(* PRIVATE FUNCTIONS *)

let rvalue (r : read) (updates : update list) : value option = 
	match updates with
		[] -> None
	| 	us -> Some (last_element us)

let rec updates (rounds : round list) : update list =
	match rounds with
		[] 		-> []
	| 	r :: rs -> r.update :: updates rs 

let onReceive (c : clientGSP) (r : round) : clientGSP =
	let known = c.known@[r] in
	if r.origin <> c.id then
		{ id = c.id; known = known; pending = c.pending; round = c.round }
	else
		let len = length c.pending in
			print_endline ("round = " ^ (string_of_round r));
		if length c.pending > 0 && r = (hd c.pending) then
			{ id = c.id; known = known; pending = (tl c.pending); round = c.round }
		else
			failwith "RTOB total order"

let rec receiveMultipleRounds c rounds = 
	match rounds with
	  [] -> c
	| r::l -> let newC = (onReceive c r) in 
				receiveMultipleRounds newC l

(* PUBLIC FUNCTIONS *)
let client_create clientId = 
	{ id = clientId; known = []; pending = []; round = 0 }

let client_update c u =
	(*print_string "client_update with round = ";

	print_int c.round;
	print_string " and update = " ; 
	match u with w -> print_int w ;
	print_endline "" ;*)
    let round = { origin = c.id; number = c.round; update = u } in
    let pending = c.pending@[round] in

    rtobSubmit round;
    (*RTOB_submit( new Round { origin = this, number = round ++, update = u } );*)
    
    let newclient = { id = c.id; known = c.known; pending = pending; round = c.round + 1 } in
    newclient

let client_read c r = 
	let compositelog = (append c.known c.pending) in
	let us = (updates compositelog) in
	let result = rvalue r us in
	result