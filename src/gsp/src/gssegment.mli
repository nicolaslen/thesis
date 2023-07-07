open Statedelta;;
open Types;;

val gssegment_create : unit -> gsSegmentGSP
val gssegment_append : gsSegmentGSP -> round -> gsSegmentGSP