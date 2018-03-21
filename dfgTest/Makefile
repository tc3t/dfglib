# Note: this Makefile was created to quickly test cppcsv in the benchmark on Linux.
# I added -D defines to all this to work without google-test.

# The test won't compile without boost, so define your boost location here.
# Its needed for lexical_cast, endian and a few other things,
# and also the test uses boost::iostreams for a few things.
#
# Note: include the STATICALLY COMPILED version of libboost_iostreams for better performance.

# call like so: make BOOST_INCLUDE=/the/path/to/boost-1.ver.ver BOOST_IOSTREAMS_STATICLIB=/the/path/to/boost-1.ver.ver/stage/lib/libboost_iostreams-compiler-flags.a
#
# BOOST_INCLUDE=/the/path/to/boost-1.ver.ver
# BOOST_IOSTREAMS_STATICLIB=/the/path/to/boost-1.ver.ver/stage/lib/libboost_iostreams-compiler-flags.a

dfgTestCsvPerformance-O2:
	g++ -O2 -std=c++11 -pthread -o dfgTestCsvPerformance-O2 dfgTestCsvPerformance.cpp \
		-D"EXPECT_TRUE(x)=assert(x)" \
		-D"EXPECT_EQ(a,b)=assert((a)==(b))" \
		-D"TEST(a,b)=int main()" \
	  	-I .. \
		-I $(BOOST_INCLUDE) \
		$(BOOST_IOSTREAMS_STATICLIB)

clean:
	rm -v dfgTestCsvPerformance-O2
