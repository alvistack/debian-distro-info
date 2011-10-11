define \n


endef

PREFIX ?= /usr
VENDOR ?= $(shell dpkg-vendor --query Vendor | tr '[:upper:]' '[:lower:]')

build: debian-distro-info ubuntu-distro-info

debian-distro-info: DebianDistroInfo.hs DistroInfo.hs
	ghc -Wall -o $@ --make -main-is DebianDistroInfo $<

test-distro-info: TestDistroInfo.hs DistroInfo.hs
	ghc -Wall -o $@ --make -main-is TestDistroInfo $<

ubuntu-distro-info: UbuntuDistroInfo.hs DistroInfo.hs
	ghc -Wall -o $@ --make -main-is UbuntuDistroInfo $<

install: debian-distro-info ubuntu-distro-info
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $^ $(DESTDIR)$(PREFIX)/bin
	ln -s $(VENDOR)-distro-info $(DESTDIR)$(PREFIX)/bin/distro-info
	install -d $(DESTDIR)$(PREFIX)/share/distro-info
	install -m 644 $(wildcard data/*.csv) $(DESTDIR)$(PREFIX)/share/distro-info
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 $(wildcard doc/*.1) $(DESTDIR)$(PREFIX)/share/man/man1
	install -d $(DESTDIR)$(PREFIX)/share/perl5/Debian
	install -m 644 $(wildcard perl/Debian/*.pm) $(DESTDIR)$(PREFIX)/share/perl5/Debian
	cd python && python setup.py install --root="$(DESTDIR)" --no-compile --install-layout=deb

test: test-distro-info
	./test-distro-info
	cd perl && ./test.pl
	$(foreach python,$(shell pyversions -r),cd python && $(python) setup.py test$(\n))

clean:
	rm -rf *-distro-info *.hi *.o python/build python/*.egg-info
	find python -name '*.pyc' -delete

.PHONY: build clean install test
