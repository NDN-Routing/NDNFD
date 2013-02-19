# NDNFD, Named Data Network Forwarding Daemon

## How to build

NDNFD requires gcc 4.6+ or clang 3.1+.

	./waf configure --gtest --markdown
	./waf
	./waf check

**--gtest** enables unit tests; `./waf check` executes unit tests.
**--markdown** builds Markdown documents to HTML format, and this requires pandoc.

