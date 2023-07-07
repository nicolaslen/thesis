type client = int
type value = int
type update = value
type read = unit
type round = { origin : client; number : int; update : update }
type clientGSP = { id : client; known : round list; pending : round list; round : int }

type status = { lock : Mutex.t; mutable send : int; mutable receive : int; client : string }
type lineCounter = { mutable counter : int }