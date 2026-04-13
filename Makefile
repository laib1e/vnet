obj-m += vnet.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	install -d /lib/modules/$(shell uname -r)/extra/
	install -m 644 vnet.ko /lib/modules/$(shell uname -r)/extra/
	depmod -a

uninstall:
	rm -f /lib/modules/$(shell uname -r)/extra/vnet.ko
	depmod -a

install-service:
	cp vnet.service /etc/systemd/system/
	systemctl daemon-reload

uninstall-service:
	systemctl stop vnet || true
	systemctl disable vnet || true
	rm -f /etc/systemd/system/vnet.service
	systemctl daemon-reload