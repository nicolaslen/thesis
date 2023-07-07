open Statedelta;;
open Types;;
open Maps;;

val streamingServer_create : unit -> streamingServerGSP

(* A connection is started by the accept_connection transition, which adds 
it to the active connections "connections" and sets the "accepted" flag. 
It then sends the current state (i.e. the reduced prefix of the global 
sequence) on the channel. *)
val streamingServer_acceptConnection : streamingServerGSP -> channelGSP -> streamingServerGSP

(* During normal operation, the server repeatedly performs the processbatch operation.
It combines a nondeterministic number of rounds (we use the * in the pseudocode to denote
a nondeterministic choice) from each active connection into a single segment. This segment
stores all updates in reduced form as a delta object. We then append this segment to the
persistent state (which applies the delta to the current state, and updates the maximum
round number per client), and send it out on all active channels. *)
val streamingServer_processBatch : streamingServerGSP -> streamingServerGSP

(* drop_connection models the abrupt failure or disconnection of a channel
at the server side - but not on the client, who may still send and receive 
packets until it in turn drops the connection. 
Note that a client may reconnect later, using a fresh channel object, and will 
resend rounds that were lost in transit when the channel was dropped. *)
val streamingServer_dropConnection : streamingServerGSP -> client -> streamingServerGSP

(* crash_and_recover models a failure and recovery of the server, which
loses all soft state but preserves the persistent state. *)
val streamingServer_crashAndRecover : streamingServerGSP -> streamingServerGSP
