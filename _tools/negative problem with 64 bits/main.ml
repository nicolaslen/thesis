(*int_of_string "0x7FFFFFFFFFFFFFFF";;
int_of_string "0x7FFFFFFFFFFFFFFF";;


int_of_string "0xFC";;
- : int = 252
# int_of_string "0o374";;
- : int = 252
# int_of_string "0b11111100";;
- : int = 252

negativos
0x4FFFFFFFFFFFFFFE
100111111111111111111111111111111111111111111111111111111111110

0x4FFFFFFFFFFFFFFF
100111111111111111111111111111111111111111111111111111111111111

0x5FFFFFFFFFFFFFFF
101111111111111111111111111111111111111111111111111111111111111

0x7FFFFFFFFFFFFFFF
111111111111111111111111111111111111111111111111111111111111111

int_of_string "0x27FFFFFFFFFFFFFE";;


[-2147483648, 2147483647]


1111111111111111, 1111111110110000
(-1, -80)


11111111 11111111 10000000 00000000  -32768

11111111111111111000000000000000
*)

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

let arg1 = (int_of_string Sys.argv.(1))
let arg2 = (int_of_string Sys.argv.(2))


let a = (encode2Numbers (arg1) (arg2))
let (b1, b2) = (decode2Numbers a)
(*
print_endline ("encoding (" ^ Sys.argv.(1) ^ ", " ^ Sys.argv.(2) ^ "): " ^ (string_of_int a));
print_endline ("decoding to: (" ^ (string_of_delta b1) ^ ", " ^ (string_of_int b2) ^ ")");
*)