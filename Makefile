CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=EDIT_MAKE_FILE

all: web-server web-client

web-server: server.cpp
	$(CXX) $(CXXFLAGS) server.cpp -o web-server

web-client: client.cpp
	$(CXX) $(CXXFLAGS) client.cpp -o web-client

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server web-client *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
