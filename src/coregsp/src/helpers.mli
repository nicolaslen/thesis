open Types;;

val print_writes : round list -> unit
val string_of_round : round -> string
val string_of_rounds : round list -> string
val string_of_clientGSP : clientGSP -> string
val print_list : string list -> unit

val input_line_opt : in_channel -> string option 
val getLineFromFile : string -> lineCounter -> string
val encodeRound : round -> int
val decodeRound : int -> round
val getRoundsFromMessages : string list -> round list
val waitForNewMessagesToSend : status -> unit
val waitForNewMessagesToReceive : status -> unit
val rtobSubmit : round -> unit
val getRoundFromFile : string -> lineCounter -> round
val getUpdateFromFile : string -> lineCounter -> update
val last_element : 'a list -> 'a 