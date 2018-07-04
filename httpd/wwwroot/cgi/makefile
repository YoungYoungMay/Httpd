ROOT=$(shell pwd)
INCLUDE=-I$(ROOT)/../../lib/include
LIB=-L$(ROOT)/../../lib/lib
LIB_NAME=-lmysqlclient
LIB_FLAG=-static

.PHONY:all
all:echoserver insertdata selectdata

echoserver:echoserver.c
	gcc -o $@ $^
insertdata:insertdata.c
	gcc -o $@ $^ $(INCLUDE) $(LIB) $(LIB_NAME)
selectdata:selectdata.c
	gcc -o $@ $^ $(INCLUDE) $(LIB) $(LIB_NAME)

.PHONY:clean
clean:
	rm -f echoserver insertdata selectdata
