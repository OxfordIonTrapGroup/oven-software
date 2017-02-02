

OVERLAYS=OvenOverlay
OVERLAYS_DTBO=$(OVERLAYS:=-00A0.dtbo)

.PHONY: all
all: overlays makefile

.PHONY: overlays
overlays: $(OVERLAYS_DTBO) makefile

$(OVERLAYS_DTBO): makefile ${@:-00A0.dtbo=.dts}
	dtc -O dtb -o $@ -b 0 -@ $(@:-00A0.dtbo=.dts)


.PHONY: install
install: all
	cp $(OVERLAYS_DTBO) /lib/firmware/
	grep -q am33xx_pwm /sys/devices/bone_capemgr.9/slots || echo am33xx_pwm > /sys/devices/bone_capemgr.9/slots
	echo OvenOverlay > /sys/devices/bone_capemgr.9/slots