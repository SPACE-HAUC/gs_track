CXX = g++
COBJS = src/main.o src/track.o network/network.o
CXXFLAGS = -I ./include/ -I ./ -I ./network/ -Wall
EDLDFLAGS := -lsi446x -lpthread -lm
TARGET = track.out

all: $(COBJS)
	$(CXX) $(CXXFLAGS) $(COBJS) -o $(TARGET) $(EDLDFLAGS)
	./$(TARGET)

%.o: %.c
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	$(RM) *.out
	$(RM) *.o
	$(RM) src/*.o
	$(RM) network/*.o