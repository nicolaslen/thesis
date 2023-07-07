open Statedelta;;
open Types;;
open Maps;;

(*val print_writes : round list -> unit
val print_clientGSP_rounds : round list -> string

val print_rounds : round list -> unit
val print_list : string list -> unit*)

val firstExport : string -> unit
val export : int -> string -> unit


val string_of_list : 'a list -> ('a -> string) -> string
val string_of_round : round -> string
val string_of_option : 'a option -> ('a -> string) -> string
val string_of_bool : bool -> string
val string_of_maxround : (client * int) -> string
val string_of_prefix : gsPrefixGSP -> string
val string_of_segment : gsSegmentGSP -> string
val string_of_gs : gs -> string
val string_of_channel : channelGSP -> string
val string_of_connections : (client * channelGSP) -> string
val string_of_client : streamingClientGSP -> string 
val string_of_server : streamingServerGSP -> string 

val input_line_opt : in_channel -> string option 
val getLineFromFile : string -> int -> string
val getRoundsFromMessages : string list -> round list
val rtobRoundSubmit : round -> unit
val rtobSegmentPrefixSubmit : gs -> bool -> unit
val countFileLines : string -> int
val waitForNewMessagesToReceive : status -> unit
val rtobConnectionRequestSubmit : client -> unit
val getRoundFromMessage : int -> int -> round
val getOriginMessageFromFile : string -> int -> (int * int)
(*val getDeltaFromFile : string -> lineCounter -> delta*)
val getSegmentPrefixFromMessage : int -> int -> (gs * bool)
val addRoundToClientstream : streamingServerGSP -> round -> streamingServerGSP
val addSegmentPrefixToServerstream : streamingClientGSP -> gs -> streamingClientGSP
val last_element : 'a list -> 'a 
