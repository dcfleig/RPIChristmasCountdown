CFLAGS=-Wall -O3 -g -J4
CXXFLAGS=$(CFLAGS)
OBJECTS=countdown.o 
BINARIES=countdown

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

CXXFLAGS+=-I/opt/vc/include -I/usr/include/boost -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -D_REENTRANT

LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) 
LDFLAGS+= -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/lib/arm-linux-gnueabihf -Wl,--enable-new-dtags -lboost_date_time -lfreeimage -lfreeimageplus -Wl,--no-undefined -lm -L/opt/vc/lib -lbcm_host -ldl -ldl -lpthread -lrt -L/usr/lib/arm-linux-gnueabihf
LDFLAGS+=-lrt -lm -lpthread

all : $(BINARIES)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

countdown : countdown.o $(RGB_LIBRARY) 
	$(CXX) $< -o $@ $(LDFLAGS)

test : test.o 
	$(CXX) $< -o $@ $(LDFLAGS)

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

%.o : %.c
	$(CC) -I$(RGB_INCDIR) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)

FORCE:
.PHONY: FORCE
