
BOOST_PATH=/home/users/p02119/development/install/boost/
CXX=CC

INCLUDES=-I../include -I$(BOOST_PATH)/include

ROOT_PATH=$(BOOST_PATH)/lib/

RMAT2=-DNEW_GRAPH500_SPEC

#LDFLAGS=-static -L$(BOOST_PATH)/lib/
LDFLAGS=-L$(BOOST_PATH)/lib/

CXXFILES = main.cpp 

#CXXFLAGS = -O3 -o permute
CXXFLAGS =-Wall -g -fno-omit-frame-pointer -dynamic -fsanitize=address -o permute -DPRINT_DEBUG
#LIBS = $(ROOT_PATH)/libboost_thread.a $(ROOT_PATH)/libboost_mpi.a $(ROOT_PATH)/libboost_system.a \
	$(ROOT_PATH)/libboost_random.a $(ROOT_PATH)/libboost_serialization.a \
	$(ROOT_PATH)/libboost_graph_parallel.a $(ROOT_PATH)/libboost_graph.a

LIBS =

all: 
	$(CXX) $(CXXFILES) $(LIBS) $(CXXFLAGS) $(INCLUDES)
	#$(CXX) $(CXXFILES) $(LIBS) $(CXXFLAGS) $(INCLUDES) #$(RMAT2)

clean: 
	rm -f dstep *.o
