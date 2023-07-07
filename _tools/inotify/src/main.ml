open List;;
open Unix;;
open Inotify;;
open Printf;;

let input_line_opt in_channel = 
	try Some (input_line in_channel)
	with End_of_file -> None
in

let countFileLines filename = 
	let ic = open_in filename in
	let rec aux a =
		match input_line_opt ic with
			| Some line -> (1 + (aux ()))
			| None -> close_in ic; 0 in
	aux ()
in


print_endline "waitForNewMessagesToReceive init";
let inotify = Inotify.create () in
let filename = "hola.txt" in
let watch = Inotify.add_watch inotify filename [Inotify.S_Modify] in
print_endline ("shared memory: " ^ filename);

let x = ref 0 in

while true do
	(*print_endline (Inotify.string_of_event (List.hd (Inotify.read inotify)));*)
	(*print_endline "waitingToReceive...";*)
	Inotify.read inotify;
	(*print_endline "waitingToReceive reading...";*)

	let lines = countFileLines filename in

	let previus = (!x) in
	x := lines;
	print_endline ("lines cambió de " ^ (string_of_int previus) ^ " a " ^ (string_of_int lines));
	
done