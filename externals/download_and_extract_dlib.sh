wget https://github.com/tc3t/misc_helpers/raw/master/dfglib_test_dlib_19.1.7z
dlib_md5=$(md5sum dfglib_test_dlib_19.1.7z | cut -b -32 -)
if [ ! "$dlib_md5" = "4b314037160d8eed937f1b26dc591f32" ] ; then
	echo "Unexpected dfglib_test_dlib_19.1.7z md5sum"
	exit 1
fi
7z x dfglib_test_dlib_19.1.7z
