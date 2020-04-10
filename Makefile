ROOTLIBS      	= $(shell root-config --libs)
CFLAGS		= -O3 -std=c++11 $(shell root-config --cflags)

	
tof2root:	tof2root.o
	g++ -o tof2root tof2root.o -Wl,-rpath,$ROOTSYS/lib $(ROOTLIBS) $(CFLAGS)
	

.cpp.o: 		$<.cpp
		g++ -c $< $(CFLAGS)
		
clean:
	@rm -f *.o 

