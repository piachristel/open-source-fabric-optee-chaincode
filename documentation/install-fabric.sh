#!/bin/bash


# Script to install fabric

# Bases on installation guidelines of:
# https://hyperledger-fabric.readthedocs.io/en/release-1.4/CONTRIBUTING.html 
# https://hyperledger-fabric.readthedocs.io/en/release-1.4/getting_started.html
# https://hyperledger-fabric.readthedocs.io/en/release-1.4/dev-setup/devenv.html
# https://hyperledger-fabric.readthedocs.io/en/release-1.4/prereqs.html
# https://golang.org/doc/install
# https://golangtutorial.com/how-to-download-and-install-golang/
# https://stackoverflow.com/questions/8191459/how-do-i-update-node-js

# Its divided in the following sections:
# cMake
# go
# fabric
# Each section can be called with source `sudo ~/path/to/script/install-fabric sectionName`
# Due to dependencies the sections should be called in the order above

cMake() {
echo "Installing cmake..."
sudo apt-get install cmake -qq || exit 1 # make sure that version 3.5.1 or higher is installed
}

go() {
echo "Installing go..."
P=$(uname -m)
if ! [[ "$P" = "x86_64" || "$P" = "amd64" || "$P" = "X64" ]];
then
	echo "Wrong type of processor."
	exit 1
else
	wget https://dl.google.com/go/go1.11.4.linux-amd64.tar.gz || exit 1
	rm -r /usr/local/go || exit 1  # remove old version of go if any installed
	tar -xvf go1.11.4.linux-amd64.tar.gz || exit 1
	sudo mv go /usr/local	
	files='bashrc profile'
	for file in $files # TODO: do not export paths if already exported
	do
		echo 'export GOROOT=/usr/local/go' >> ~/.$file || exit 1
		echo 'export GOPATH=$HOME/go' >> ~/.$file || exit 1
		echo 'export PATH=$PATH:$GOROOT/bin:$GOPATH/bin' >> ~/.$file || exit 1
	done
	source ~/.bashrc
	# change permission of go dir: chown -R ubuntu ~/go; never use sudo together with go command
fi
}

remaining_prerequisites() {
echo "Installing npm and nano..."
sudo apt-get install npm -qq || exit 1
sudo npm install -g n -qq || exit 1
sudo n latest -qq || exit 1
sudo n 8.9.4 -qq || exit 1
sudo npm install npm@5.6.0 -g -qq || exit 1

echo "Installing docker and docker-compose..."
snap install docker || exit 1

echo "Print version of python..." # check that correct version of python is installed
dpkg -s python &> /dev/null
if [ $? -ne 0 ];
then
	echo "python not installed"
	exit 1
else
	V=$(python --version)
	echo "$V"
fi

echo "Installing pip..."
sudo apt-get install python-pip || exit 1
}

fabric() {
echo "Installing fabric..."
mkdir -p ~/go/src/github.com/hyperledger || exit 1
cd ~/go/src/github.com/hyperledger || exit 1
git clone https://github.com/hyperledger/fabric.git || exit 1
cd fabric || exit 1
git fetch --all --tags --prune || exit 1
git checkout release-1.4 || exit 1
git checkout v1.4.1
sudo chmod 666 /var/run/docker.sock || exit 1
make 
}

echo "Downloading and updating packages..."
sudo apt-get update -qq || exit 1

"$@"
exit 0
