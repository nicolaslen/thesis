open Statedelta;;
open Types;;
open Maps;;

val channel_create : client -> channelGSP
val channel_appendToServerstream : channelGSP -> gs -> channelGSP
val channel_appendToClientstream : channelGSP -> round -> channelGSP