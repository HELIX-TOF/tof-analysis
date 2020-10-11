ROOTLIBS      	= $(shell root-config --libs)
CFLAGS		= -O3 -std=c++11 $(shell root-config --cflags)

	
tof2root2:	tof2root2.o
	g++ -o tof2root2 tof2root2.o -Wl,-rpath,$ROOTSYS/lib $(ROOTLIBS) $(CFLAGS)
	

.cpp.o: 		$<.cpp
		g++ -c $< $(CFLAGS)
		
clean:
	@rm -f *.o 

