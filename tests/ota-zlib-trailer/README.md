# Compressed OTA boundary regression

Build with a C++11 compiler and the Espressif esptool flasher stub version of miniz. Set MINIZ_DIR to the directory containing miniz.c and include/miniz.h. The example uses a POSIX shell and Python 3 from the repository root.

~~~sh
testdir=tests/ota-zlib-trailer
flags="-DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_STDIO -DMINIZ_NO_TIME -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES"
cc $flags -I"$MINIZ_DIR/include" -c "$MINIZ_DIR/miniz.c" -o miniz.o
c++ -std=c++11 -pthread -D__arm__ $flags -I"$MINIZ_DIR/include" -I"$testdir/support" -Isrc "$testdir/main.cpp" miniz.o -o ota-test
for size in 32767 32768 32769 65536; do
  python3 -c 'import sys,zlib; open("fixture.z","wb").write(zlib.compress(bytes(i%251 for i in range(int(sys.argv[1])))))' "$size"
  ./ota-test fixture.z "$size" || exit 1
done
~~~

The real decompressor processes the final four bytes in a separate chunk. A flash double records all output bytes for comparison with the input image.
