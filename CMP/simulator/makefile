CC = g++ -std=c++11 -Wall
ALL_OBJ = simulator.o instruction.o regfile.o memory.o
EXEC=CMP
simulator: ${ALL_OBJ}
	${CC} -o ${EXEC} ${ALL_OBJ}
allObj: ./*cpp
	${CC} -c ./*.cpp
simulator.o: simulator.cpp
	${CC} -c simulator.cpp
instruction.o: instruction.cpp
	${CC} -c instruction.cpp
regfile.o: regfile.cpp
	${CC} -c regfile.cpp
memory.o: memory.cpp
	${CC} -c memory.cpp
clean:
	rm -f ${EXEC} ${ALL_OBJ}