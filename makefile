listener_test:
	clang listener.c cryo.c interpolator.c listener_test.c -o listener_test -lcurl -ljansson -lm -L.
	
curl_test:
	clang curl_test.c -o curl_test -lcurl
	
clean:
	rm -rf listener_test curl_test
