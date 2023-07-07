#Compilador que será utilizado
CC=mpicc

# flags de compilación
CXXFLAGS=

# flags de linkeo
LDFLAGS=

# Agrear acá los directorios a incluir en la compilación [COMPLETAR]
INCDIR= .

# Agregar acá los archivos .cpp a compilar [COMPLETAR]

SOURCES=Main.c dpa_ApplicationLayerCommunication.c dpa_BEBroadcast.c dpa.c dpa_Common.c dpa_ConsensusBasedTOB.c dpa_EagerReliableBroadCast.c dpa_EventQueue.c dpa_FloodingConsensus.c dpa_Logger.c dpa_PerfectFailureDetector.c dpa_Runtime.c dpa_SerialNumbers.c


# Objetos que serán generados (no tocar)
OBJECTS=$(SOURCES:.c=.o)

# Nombre del ejecutable a generar
EXECUTABLE=broadcast

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS)

.c.o:
	$(CC) -I$(INCDIR) $(CXXFLAGS) $< -c -o $@

install: 
	sudo apt install libopenmpi-dev

