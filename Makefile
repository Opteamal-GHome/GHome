#Variables utilisées :
CC=gcc #Compilateur
EDL=gcc #Linker
CCFLAGS=-Wall -m32 #Options de compilations
EDLFLAGS=-Wall -m32
EXE=ghome #Nom du binaire à construire

OBJ=tcpserver.o mere.o sensorServer.o gestion_capteurs.o gestion_regles.o dispatchServer.o\
		restRcv.o engine.o
LIBS=-lpthread -lrt -ljansson


$(EXE): $(OBJ)
	@echo building $<
	$(EDL)  -o $(EXE) $(EDLFLAGS) $(OBJ) $(LIBS)
	@echo done

%.o : %.c *.h 
	@echo building $< ...
	$(CC) $(CCFLAGS) -c $<
	@echo done
	
clean: 
	@echo -n cleaning repository... 
	@rm -f *.o
	@rm -f .*.swp
	@rm -f *~
	@rm -f *.log
	@rm -f *.pid
	@rm -f *.out
	@echo cleaned.

coffee : clean
	@echo No!
