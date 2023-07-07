open Statedelta;;
open Types;;
open Maps;;

val streamingClient_create : client -> streamingClientGSP

(* read transition computes the visible state by calling
curstate which (nondestructively) applies the delta 
objects in pending, pushbuf and transactionbuf to the 
state object in known. *)
val streamingClient_read : streamingClientGSP -> read -> value

(* update transition adds the given update to the 
transaction buffer, and clears the tbuf_empty flag 
(since delta objects do not have a function to query 
whether they are empty, we use a flag to determine 
whether the transaction buffer is empty). *)
val streamingClient_update : streamingClientGSP -> update -> streamingClientGSP

(* confirmed transition checks whether updates are 
pending in pending or transactionbuf or pushbuffer.*)
val streamingClient_confirmed : streamingClientGSP -> bool

(* send transition requires that there is an 
established channel (this is is important to 
handle channel recovery correctly, as explained
earlier). It sends one or more rounds (stored in
pushbuf ) as a single cumulative round with the 
latest pushed round number, and appends it to the
pending queue. *)
val streamingClient_send : streamingClientGSP -> streamingClientGSP

(* push transition moves the content of the 
transactionbuffer to the pushbuffer, by combining 
and reducing the respective delta objects.*)
val streamingClient_push : streamingClientGSP -> streamingClientGSP

(* pull transition processes all packets
in the receive buffer (which we model as 
part of the channel object), if any. When 
processing a packet, we track if this is 
the first packet received on this channel 
by checking the established flag *)
val streamingClient_pull : streamingClientGSP -> streamingClientGSP

(* receive transition straightforwardly 
moves a packet from the serverstream into
the receive buffer. *)
val streamingClient_receive : streamingClientGSP -> streamingClientGSP

(* drop_connection transition models loss of 
connection at the client side. It removes the 
channel object from the client - but not from
 the server, who may still send and receive 
packets on the channel until it in turn drops 
the connection.*)
val streamingClient_dropConnection : streamingClientGSP -> streamingClientGSP

val streamingClient_sendConnectionRequest : streamingClientGSP -> streamingClientGSP