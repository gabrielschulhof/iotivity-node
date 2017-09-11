IOTIVITY_PREFIX = /home/nix/iot/iotivity-node/iotivity-native
BIN_PATH = out/linux/x86_64/release

INCLUDES = \
	-I$(IOTIVITY_PREFIX)/resource/csdk/stack/include \
	-I$(IOTIVITY_PREFIX)/resource/csdk/include \
	-I$(IOTIVITY_PREFIX)/resource/c_common

all: client server server.oic_svr_db.dat client.oic_svr_db.dat
.PHONY: all

client: client.c boilerplate.c
	cc -g -Wall -O0 `pkg-config --cflags --libs glib-2.0` \
		-DROUTING_EP \
		-D__WITH_DTLS__ \
		$(INCLUDES) \
		-loctbstack \
		-L$(IOTIVITY_PREFIX)/$(BIN_PATH)/resource/csdk/stack \
		-Wl,-rpath $(IOTIVITY_PREFIX)/$(BIN_PATH)/resource/csdk/stack \
		client.c boilerplate.c -o client

server: server.c boilerplate.c
	cc -g -Wall -O0 `pkg-config --cflags --libs glib-2.0` \
		-DROUTING_EP \
		-D__WITH_DTLS__ \
		$(INCLUDES) \
		-loctbstack \
		-L$(IOTIVITY_PREFIX)/$(BIN_PATH)/resource/csdk/stack \
		-Wl,-rpath $(IOTIVITY_PREFIX)/$(BIN_PATH)/resource/csdk/stack \
		server.c boilerplate.c -o server

%.dat: %.json
	$(IOTIVITY_PREFIX)/$(BIN_PATH)/resource/csdk/security/tool/json2cbor $< $@

clean:
	rm -rf *dat client server
.PHONY: clean
