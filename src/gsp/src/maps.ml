type client = int

module Client = 
	struct
		type t = client
		let compare = Pervasives.compare
	end


module Rounds = Map.Make(Client)

module Channels = Map.Make(Client)
