define \n


endef

PREFIX ?= /usr
VENDOR ?= $(shell dpkg-vendor --query Vendor | tr '[:upper:]' '[:lower:]')

build: ;

install:
	install -d $(DESTDIR)$(PREFIX)/share/distro-info
	install -m 644 $(wildcard data/*.csv) $(DESTDIR)$(PREFIX)/share/distro-info
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 $(wildcard doc/*.1) $(DESTDIR)$(PREFIX)/share/man/man1
	cd python && python setup.py install --root="$(DESTDIR)" --no-compile --install-layout=deb
	ln -s $(VENDOR)-distro-info $(DESTDIR)$(PREFIX)/bin/distro-info

test:
	$(foreach python,$(shell pyversions -r),cd python && $(python) setup.py test$(\n))

clean:
	rm -rf python/build python/*.egg-info
	find python -name '*.pyc' -delete

.PHONY: build clean install test
