(* REGISTER *)
type read = unit
type state = int
type delta = int
type value = int
type update = int

let initialstate = 0
let emptydelta = 0
let readexample = ()
let updatesexample = [(+5); (-100); (+110)]

let rec lastElement (deltas : delta list) (default : int) : 'a = 
	match deltas with 
		| [] -> default
		| [x] -> x
		| f ::ls -> lastElement ls default

let rvalue r s = 
	s

let reduce deltas = 
	lastElement deltas emptydelta

let rec apply s deltas = 
	lastElement deltas s

let append d u = 
	u

let delta_of_string s =
	int_of_string s

let state_of_string s =
	int_of_string s

let update_of_int i =
	i

let int_of_value v =
	v

let string_of_delta s =
	string_of_int s

let string_of_state s =
	string_of_int s

let string_of_value s =
	string_of_int s

let string_of_update u =
	string_of_int u

let encode2Numbers n1 n2 =
	(*print_endline ("enconding (" ^ (string_of_int n1) ^ ", " ^ (string_of_int n2) ^ ")");*)
	let bits0000ffff = (int_of_string "0x0000ffff") in
	let bits00008000 = (int_of_string "0x80000000") in (*0s, 1 en el 32-esimo lugar *)
	let bitsffffffff = (int_of_string "0xffffffff") in (*32 1s*)
	let bitsffffffff00000000 = (bitsffffffff lsl 32) in (*32 1s, 32 0s*)

	let truncate16bits = (fun n -> (n land bits0000ffff)) in

	let deltaNumber = (truncate16bits n1) in
	let roundNumber = (truncate16bits n2) in

	let deltaNumberShifted = deltaNumber lsl 16 in
	let orResult = deltaNumberShifted lor roundNumber in

	let isNegative = (fun a -> (a lor bits00008000) = a) in
	let signalBit = (fun a -> (if (isNegative a) then bitsffffffff00000000 else 0)) in
	let finalNumber = orResult lor (signalBit orResult) in

	(*print_endline ("encoded to: " ^ (string_of_int finalNumber));*)
	finalNumber (* deltaNumber ++ roundNumber *)

let decode2Numbers n =
	(*print_endline ("decoding " ^ (string_of_int n));*)
	let bits0000ffff = (int_of_string "0x0000ffff") in (*16 0s, 16 1s*)
	let bits00008000 = (int_of_string "0x00008000") in (*0s, 1 en el 16-esimo lugar *)
	let bitsffff0000 = (int_of_string "0xffff0000") in (*16 1s, 16 0s*)
	let bitsffffffff = (int_of_string "0xffffffff") in (*32 1s*)
	let bitsffffffffffff0000 = (bitsffffffff lsl 32) lor bitsffff0000 in (*48 1s, 16 0s*)

	let n1 = n lsr 16 in
	let n2 = n in

	let isNegative = (fun a -> (a lor bits00008000) = a) in
	let truncate16bits = (fun a -> (a land bits0000ffff)) in
	let signalBit = (fun a -> (if (isNegative a) then bitsffffffffffff0000 else 0)) in

	let deltaNumber = (truncate16bits n1) lor (signalBit n1) in
	let roundNumber = (truncate16bits n2) lor (signalBit n2) in
	(*print_endline ("decoded to: (" ^ (string_of_int deltaNumber) ^ ", " ^ (string_of_int roundNumber) ^ ")");*)
	(deltaNumber, roundNumber)
	(*{ origin = 0; number = roundNumber; delta = deltaNumber }*)

let encode4Numbers b1 b2 s n1 n2 =
	(*print_endline ("enconding (" ^ (if b1 then "true" else "false") ^ ", " ^ (if b2 then "true" else "false") ^ ", " ^ s ^ ", " ^ (string_of_int n1) ^ ", " ^ (string_of_int n2) ^ ")");*)

	let isSegmentNumber = if b1 then 1 else 0 in
	let targetNumber = if b2 then 1 else 0 in
	let deltastate = if b1 then (delta_of_string s) else (state_of_string s) in

	let bits00000001 = (int_of_string "0x00000001") in (* 31 0s, 1 1s *)
	let bits000003ff = (int_of_string "0x000003ff") in (* 22 0s, 10 1s *)
	let bits00008000 = (int_of_string "0x80000000") in (*0s, 1 en el 32-esimo lugar *)
	let bitsffffffff = (int_of_string "0xffffffff") in (*32 1s*)
	let bitsffffffff00000000 = (bitsffffffff lsl 32) in (*32 1s, 32 0s*)

	let truncate10bits = (fun a -> (a land bits000003ff)) in

	let isSegmentShifted = (isSegmentNumber land bits00000001) lsl 31 in
	let targetNumberShifted = (targetNumber land bits00000001) lsl 30 in
	let statedeltaShifted = (truncate10bits deltastate) lsl 20 in
	let maxround1Shifted = (truncate10bits n1) lsl 10 in
	let maxround2Shifted = (truncate10bits n2) in

	let code = (((isSegmentShifted lor targetNumberShifted) lor statedeltaShifted) lor maxround1Shifted) lor maxround2Shifted in

	let isNegative = (fun a -> (a lor bits00008000) = a) in
	let signalBit = (fun a -> (if (isNegative a) then bitsffffffff00000000 else 0)) in
	let finalNumber = code lor (signalBit code) in

	(*print_endline ("encoded to: " ^ (string_of_int finalNumber));*)
	finalNumber
	(*b ++ s ++ n1 ++ n2*)

let decode4Numbers n =
	let bits00000001 = (int_of_string "0x00000001") in (* 31 0s, 1 1s *)
	let bits000003ff = (int_of_string "0x000003ff") in (* 22 0s, 10 1s *)
	let bits00000200 = (int_of_string "0x00000200") in (* 0s, 1 en el 10-esimo lugar *)
	let bitsffffffff = (int_of_string "0xffffffff") in (*32 1s*)
	let bitsfffffc00 = (int_of_string "0xfffffc00") in (*32 1s*)
	let bitsfffffffffffffc00 = (bitsffffffff lsl 32) lor bitsfffffc00 in (*54 1s, 10 0s*)

	(*print_endline ("decoding " ^ (string_of_int n));*)

	let truncate1bit = (fun a -> (a land bits00000001)) in

	let isNegative = (fun a -> (a lor bits00000200) = a) in
	let signalBit = (fun a -> (if (isNegative a) then bitsfffffffffffffc00 else 0)) in
	let truncate10bits = (fun a -> (a land bits000003ff)) in

	let b1 = (n lsr 31) in
	let b2 = (n lsr 30) in
	let s = (n lsr 20) in (* can be negative *)
	let n1 = (n lsr 10) in (* positive *)
	let n2 = n in (* positive *)

	let isSegment = ((truncate1bit b1) = 1) in
	let target = ((truncate1bit b2) = 1) in
	let statedelta = string_of_int ((truncate10bits s) lor (signalBit s)) in
	let maxround1 = truncate10bits n1 in
	let maxround2 = truncate10bits n2 in

	(*print_endline ("decoded to: (" ^ (if isSegment then "true" else "false") ^ ", " ^ (if target then "true" else "false") ^ ", " ^ statedelta ^ ", " ^ (string_of_int maxround1) ^ ", " ^ (string_of_int maxround2) ^ ")");*)
	(isSegment, target, statedelta, maxround1, maxround2)