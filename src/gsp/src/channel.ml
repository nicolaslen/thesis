open Types;;
open List;;
open Helpers;;

(* PRIVATE FUNCTIONS *)

(* PUBLIC FUNCTIONS *)
let channel_create c = 
	{ client = c; clientstream = []; serverstream = []; accepted = false; receivebuffer = []; established = false }

let channel_appendToServerstream ch gs = 
	print_endline ("enviando gs = " ^ (string_of_gs gs));
	let channel = { ch with serverstream = ch.serverstream@[gs]; } in
	let target = if channel.client = 1 then true else false in
	rtobSegmentPrefixSubmit gs target; (* rtobSubmit *)
	channel

let channel_appendToClientstream ch r = 
	(*print_endline ("enviando round = " ^ (string_of_round r));*)
	let channel = { ch with clientstream = ch.clientstream@[r]; } in
	rtobRoundSubmit r; (* rtobSubmit *)
	channel