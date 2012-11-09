CC=g++
CXX=g++
CXXFLAGS=  -I/usr/local/include/ImageMagick -pg `Magick++-config --cppflags --cxxflags --ldflags --libs`
LDFLAGS=  -I/usr/local/include/ImageMagick -lOpenCL `Magick++-config --cppflags --cxxflags --ldflags --libs`
LDLIBS=   -lstdc++ -lOpenCL `Magick++-config --cppflags --cxxflags --ldflags --libs`

CCFILES=$(wildcard *.cpp)
OFILES=$(CCFILES:%.cpp=%.o)
DFILES=$(CCFILES:%.cpp=%.d)

.PHONY: all clean check doc

all: magickTest preprocImg imageTest
#lcg lcg-m lcg-p

preprocImg: main.cpp img.cpp kernel.cl
	python ../preprocessor.py main.cpp img.cpp kernel.cl

imageTest: main-out.o lodepng.o img-out.o reader.o check.o
	$(CC) $(CXXFLAGS) $(MFlags) $(LDFLAGS) $+ $(LDLIBS) -o $@

magickTest: magicktest.o
	$(CC) $(CXXFLAGS) $(MFlags) $(LDFLAGS) $+ $(LDLIBS) -o $@
	
clean:
	rm -f $(OFILES) lcg lcg-m lcg-p test
	rm -f $(DFILES)
	rm -f time*.dat
	rm -rf doc

include $(DFILES)

%.d: %.cpp
	$(CXX) $(CXXFLAGS) $(MFlags) -MM $< > $@

