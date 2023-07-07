open Types;;

val client_create : client -> clientGSP
val client_update : clientGSP -> update -> clientGSP
val client_read : clientGSP -> read -> value option
val onReceive : clientGSP -> round -> clientGSP
val receiveMultipleRounds : clientGSP -> round list -> clientGSP