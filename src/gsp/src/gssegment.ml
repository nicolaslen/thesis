open Statedelta;;
open Types;;
open Maps;;

(* PRIVATE FUNCTIONS *)


(* PUBLIC FUNCTIONS *)
let gssegment_create u = 
	{ delta = Statedelta.emptydelta; maxround = Rounds.empty }

let gssegment_append (segment : gsSegmentGSP) (r : round) =
	let deltalist = segment.delta :: r.delta :: [] in
	let delta = Statedelta.reduce deltalist in
	let maxround = (Rounds.add r.origin r.number segment.maxround) in
	{ delta = delta; maxround = maxround }