define \n


endef

PREFIX ?= /usr
VENDOR ?= $(shell dpkg-vendor --query Vendor | tr '[:upper:]' '[:lower:]')

build: debian-distro-info ubuntu-distro-info

%-distro-info: %-distro-info.in distro-info-util.sh
	sed -e '/^\. .*distro-info-util.sh\"$$/r distro-info-util.sh' $< | \
		sed -e '/^##/d;/^\. .*distro-info-util.sh\"$$/d' | \
		python -c 'import re,sys;print re.sub("(?<=\n)#BEGIN \w*#\n(.|\n)*?\n#END \w*#\n", "", re.sub("(?<=\n)#(BEGIN|END) $*#\n", "", sys.stdin.read())),' > $@
	chmod +x $@

install: debian-distro-info ubuntu-distro-info
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $^ $(DESTDIR)$(PREFIX)/bin
	ln -s $(VENDOR)-distro-info $(DESTDIR)$(PREFIX)/bin/distro-info
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 $(wildcard doc/*.1) $(DESTDIR)$(PREFIX)/share/man/man1
	install -d $(DESTDIR)$(PREFIX)/share/perl5/Debian
	install -m 644 $(wildcard perl/Debian/*.pm) $(DESTDIR)$(PREFIX)/share/perl5/Debian
	cd python && python setup.py install --root="$(DESTDIR)" --no-compile --install-layout=deb

test: test-commandline test-perl test-python

test-commandline: debian-distro-info ubuntu-distro-info
	./commandlinetest Debian.tests Ubuntu.tests

test-perl:
	cd perl && ./test.pl

test-python:
	$(foreach python,$(shell pyversions -r),cd python && $(python) setup.py test$(\n))

clean:
	rm -rf *-distro-info python/build python/*.egg-info
	find python -name '*.pyc' -delete

.PHONY: build clean install test test-commandline test-perl test-python
