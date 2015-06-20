TARGET			= torrija_host
CC 				= "C:/ti/ccsv6/tools/compiler/gcc_msp430_4.9.14r1_167/bin/msp430-elf-gcc.exe"
CFLAGS 			= -c -mmcu=msp430f5519 -g
LFLAGS			= --silicon_version=mspx --diag_warning=225 --printf_support=minimal -z -m${TARGET}.map --stack_size=160 --heap_size=160 --use_hw_mpy=F5 --warn_sections
INCLUDES	    = -I "C:\ti\ccsv6\ccs_base\msp430\include_gcc" -I "C:\ti\ccsv6\tools\compiler\gcc_msp430_4.9.14r1_167\lib" -I "C:\ti\ccsv6\tools\compiler\gcc_msp430_4.9.14r1_167" "C:\ti\ccsv6\tools\compiler\ti-cgt-msp430_4.4.3\include" -I "C:\ti\ccsv6\tools\compiler\gcc_msp430_4.9.14r1_167\msp430-elf\include" -I "C:\Users\eserflo\workspace_v6_1\torrija_host_coapserver_v0.2" -I "core" -I "apps" -I "apps/application-rfid" -I "apps/application-usb" -I "apps/coap-app" -I "apps/rest-coap" -I "apps/rest-common" -I "apps/application-usb" -I "platform/rfid" -I "platform/usb_api" -I "platform/usb_msp" -I "platform/torrija"  
LIBRARY			= -l "libc.a" "msp430f5519.cmd"

CONF_6LOWPAN_ND = 1
DEBUG			= 1

ifndef DEBUG
CFLAGS += -O3
endif

CP = copy
MOVE = move /Y
RM = del /F
RMDIR = rmdir /S /Q
BUILD_DIR	= Obj

SYSTEM  = core/sys/process.c \
core/sys/autostart.c \
core/sys/timer.c \
core/sys/etimer.c \
core/sys/stimer.c \

LIBS    = core/lib/memb.c \
core/lib/list.c \
platform/torrija/utils/random.c \

UIP   	= core/net/uip6.c \
platform/torrija/nd-opt/tcpip.c \
core/net/uip-udp-packet.c \

ifdef CONF_6LOWPAN_ND
NET   	= $(UIP) \
core/net/netstack.c \
core/net/queuebuf.c \
core/net/packetbuf.c \
core/net/uip-icmp6.c \
platform/torrija/nd-opt/uip-nd6.c \
platform/torrija/nd-opt/sicslowpan.c \
platform/torrija/nd-opt/uip-ds6.c \
core/net/mac/sicslowmac.c \
core/net/mac/nullmac.c \
core/net/mac/frame802154.c \
core/net/rime/rimeaddr.c

else

NET   	= $(UIP) \
core/net/netstack.c \
core/net/queuebuf.c \
core/net/packetbuf.c \
core/net/uip-icmp6.c \
core/net/uip-nd6.c \
core/net/sicslowpan.c \
core/net/uip-ds6.c \
core/net/mac/sicslowmac.c \
core/net/mac/nullmac.c \
core/net/mac/frame802154.c \
core/net/rime/rimeaddr.c

endif # CONF_6LOWPAN_ND


ARCH	= platform/torrija/clock_arch.c \
platform/torrija/dev/msp430_arch.c \
platform/torrija/dev/buttons.c \
platform/torrija/dev/leds_torrija.c \
platform/torrija/dev/cc2520ll.c \
platform/torrija/dev/hal_cc2520.c \
platform/torrija/utils/sense_utils.c \
platform/torrija/dev/cc2520_process.c \
platform/rfid/trf_hardware.c \
platform/rfid/trf_parallel.c \
platform/usb_msp/descriptors.c \
platform/usb_msp/usb_hardware.c \
platform/usb_api/usb.c \
platform/usb_api/usb_cdc.c
 

APP		= apps/coap-app/coap_app.c \
apps/coap-app/coap_app_client.c \
apps/rest-coap/coap-common.c \
apps/rest-coap/coap-server.c \
apps/rest-common/buffer.c \
apps/rest-common/rest.c \
apps/rest-common/rest-util.c \
apps/application-rfid/rfid-processes.c \
apps/application-usb/usb-processes.c

CONTIKI_TARGET_MAIN = platform/torrija/contiki-torrija-host-main.c

CONTIKI_TARGET_SOURCEFILES += $(ARCH) $(CONTIKI_TARGET_MAIN)

CONTIKIFILES = $(SYSTEM) $(LIBS) $(NET)

CONTIKI_SOURCEFILES += $(CONTIKIFILES) $(CONTIKI_TARGET_SOURCEFILES) $(APP)

CONTIKI_OBJECTFILES = $(CONTIKI_SOURCEFILES:.c=.obj)

CONTIKI_OBJECTFILES2 = \
process.obj \
autostart.obj \
timer.obj \
etimer.obj \
stimer.obj \
memb.obj \
list.obj \
random.obj \
uip6.obj \
tcpip.obj \
uip-udp-packet.obj \
netstack.obj \
queuebuf.obj \
packetbuf.obj \
uip-icmp6.obj \
uip-nd6.obj \
sicslowpan.obj \
uip-ds6.obj \
sicslowmac.obj \
nullmac.obj \
frame802154.obj \
rimeaddr.obj \
clock_arch.obj \
msp430_arch.obj \
buttons.obj \
cc2520ll.obj \
hal_cc2520.obj \
sense_utils.obj \
cc2520_process.obj \
trf_hardware.obj \
trf_parallel.obj \
leds_torrija.obj \
rfid-processes.obj \
usb_cdc.obj \
usb.obj \
descriptors.obj \
usb_hardware.obj \
usb-processes \
coap_app.obj \
coap_app_client.obj \
coap-common.obj \
coap-server.obj \
buffer.obj \
rest.obj \
rest-util.obj \
contiki-torrija-host-main.obj

${TARGET} : ${CONTIKI_OBJECTFILES} ${TARGET}.out postbuild

${TARGET}.out : ${CONTIKI_OBJECTFILES}
	${CC} ${CFLAGS} ${LFLAGS} ${INCLUDES}  --reread_libs --rom_model -o $@ $(CONTIKI_OBJECTFILES2) ${LIBRARY} lnk_msp430f5519.cmd
	
%.obj : %.c
	${CC} -c ${CFLAGS} ${INCLUDES} $<
	
clean:
	del /F *.obj 
	del /F *.o
	del /F ${TARGET}.out 
	del /F ${TARGET}.map
	del /F ${TARGET}.o
	del /F ${TARGET2}.out 
	del /F ${TARGET2}.map
	del /F ${TARGET2}.o
	@if exist $(BUILD_DIR) $(RMDIR) $(BUILD_DIR)
	@echo Done.
	
postbuild:
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@if  exist $(BUILD_DIR) $(MOVE) *.obj $(BUILD_DIR) 
	@if  exist $(BUILD_DIR) $(MOVE) *.out $(BUILD_DIR)
	@if  exist $(BUILD_DIR) $(MOVE) *.map $(BUILD_DIR) 
	@echo Done.