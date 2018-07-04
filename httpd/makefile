ROOT=$(shell pwd)

.PHONY:all
all:myhttpd cgi

myhttpd:myhttpd.c
	gcc -o $@ $^ -lpthread
cgi:
	cd $(ROOT)/wwwroot/cgi;make clean;make;cd -

.PHONY:output
output:
	mkdir -p output/wwwroot/cgi
	mkdir -p output/wwwroot/lib
	cp myhttpd output
	cp start.sh output
	cp -rf lib/lib output/lib/
	cp -f wwwroot/*.html output/wwwroot
	cp -rf wwwroot/image/ output/wwwroot/
	cp -f wwwroot/cgi/echoserver output/wwwroot/cgi
	cp -f wwwroot/cgi/insertdata output/wwwroot/cgi
	cp -f wwwroot/cgi/selectdata output/wwwroot/cgi

.PHONY:clean
clean:
	rm -f myhttpd;cd $(ROOT)/wwwroot/cgi;make clean;cd -;rm -rf output
