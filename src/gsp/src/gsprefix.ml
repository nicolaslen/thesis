open Statedelta;;
open Types;;
open Maps;;
open List;;
open Helpers;;

(* PRIVATE FUNCTIONS *)

let appendMap (orig : maxround) (newone : maxround) : maxround =
	let result = Rounds.merge (fun k xo yo -> match xo,yo with
		  _, Some y -> Some y
		| xo, None -> xo
	) orig newone in
	result

(*val merge : (key -> 'a option -> 'b option -> 'c option) -> 'a t -> 'b t -> 'c t*)

(* PUBLIC FUNCTIONS *)
let gsprefix_create u = 
	{ state = Statedelta.initialstate; maxround = Rounds.empty }

let gsprefix_apply prefix segment =
	let state = Statedelta.apply prefix.state [segment.delta] in
	let maxround = appendMap prefix.maxround segment.maxround in
	{ state = state; maxround = maxround }