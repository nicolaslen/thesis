type read
type state
type delta
type value
type update

val initialstate : state
val emptydelta : delta
val readexample : read
val updatesexample : update list

val rvalue : read -> state -> value
val reduce : delta list -> delta
val apply : state -> delta list -> state
val append : delta -> update -> delta

val delta_of_string : string -> delta
val state_of_string : string -> state
val update_of_int : int -> update
val int_of_value : value -> int

val string_of_delta : delta -> string
val string_of_state : state -> string
val string_of_value : value -> string
val string_of_update : update -> string

val encode2Numbers : delta -> int -> int
val decode2Numbers : int -> (delta * int)
val encode4Numbers : bool -> bool -> string -> int -> int -> int
val decode4Numbers : int -> (bool * bool * string * int * int)
