# RocketRAID
RocketRAID Linux Kernel drivers for rr232x card

## About
This is an un-official copy of the open-source Linux driver for the HighPoint RocketRaid 232x controller.

The original open-source driver used to be downloaded from the [official Highpoint website](http://www.highpoint-tech.com/USA_new/rr2300_download.htm).
- it seems this link has been removed, probably due to it being legacy hardware

## Disclaimer/ important information
This driver is provided as is, without warranty or liability. Use this at your own risk!
Please backup your data and try this in a safe way, ideally mounting the file system read only and gradually test - expert linux kernel recommended.
I've shared updates that I've had to do to make my system work with the RocketRAID hardware I have to keep my system going.

## Patches applied
- upto Kernel 5.x (I seem to have mine working on 5.3)
- new scsi
- timer

## Added support for DKMS
- this should support the Ubuntu kernel to dynamically build this module when a kernel update is needed

### Build and install with dkms

```bash
sudo git clone https://github.com/siratcliff/RocketRAID.git
cd rr232x-linux-src-v1.10
sudo cp -R * /usr/src/rr232x-1.10
sudo dkms add -m rr232x -v 1.10
sudo dkms build -m rr232x -v 1.10
sudo dkms install -m rr232x -v 1.10
sudo modprobe rr232x
```

## Build without dkms

Install required build packages and kernel headers:

```bash
sudo apt-get -y install build-essential checkinstall linux-headers-$(uname -r)
```

Clone git repo:

```bash
sudo cd /usr/src
sudo git clone https://github.com/siratcliff/RocketRAID.git
```

CD into patched directory and compile driver:

```bash
cd rr232x-linux-src-v1.10/product/rr232x/linux/
sudo make install
```

Load kernel module:

```bash
sudo modprobe rr232x
```

## Attributions

Need to add links to all the helpful posts and patches....
