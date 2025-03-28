TAR_URL = https://www.openslr.org/resources/12/test-clean.tar.gz

all: eval

eval:
	$(MAKE) -f eval.mk

clean:
	$(MAKE) -f eval.mk clean

setup-venv:
	python3 -m venv venv
	./venv/bin/pip install -r requirements.txt

clean-venv:
	rm -r venv

get-audio:
	wget -c $(TAR_URL)
	tar -xf test-clean.tar.gz

.PHONY: all eval clean setup-venv clean-venv get-audio
