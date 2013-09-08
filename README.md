# NDNFD, Named Data Network Forwarding Daemon

## How to build

NDNFD code requires gcc 4.6+ or clang 3.1+ compiler. CCNx must be installed before compiling NDNFD.

Install prerequisites on Ubuntu 12.04

	sudo apt-get update
	sudo apt-get dist-upgrade -y
	sudo apt-get install -y git build-essential pandoc libpcap-dev libexpat1-dev libssl-dev
	
Install prerequisites on FreeBSD 9.0-RELEASE

	sudo pkg_add -r git
	sudo pkg_add -r gcc47
	sudo pkg_add -r hs-pandoc

Install prerequisites on OS X Mountain Lion

* [Xcode + command line tools](http://www.moncefbelyamani.com/how-to-install-xcode-homebrew-git-rvm-ruby-on-mac/#step-1)
* [Pandoc](http://johnmacfarlane.net/pandoc/installing.html#mac-os-x)

Configure and compile NDNFD

	./waf configure --gtest --markdown
	./waf
	./waf check

**--gtest** enables unit tests; `./waf check` executes unit tests.  
**--markdown** builds Markdown documents to HTML format.

