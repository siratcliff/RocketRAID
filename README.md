# RocketRAID
RocketRAID Linux Kernel drivers for rr232x card

## Disclaimer/ important information
This driver is provided as is, without warranty or liability. Use this at your own risk!
Please backup your data and try this in a safe way, ideally mounting the file system read only and gradually test - expert linux kernel recommended.
I've shared updates that I've had to do to make my system work with the RocketRAID hardware I have to keep my system going.

## Patches applied
- upto Kernel 4.7
- new scsi
- timer

## Added support for DKMS
- this should support the Ubuntu kernel to dynamically build this module when a kernel update is needed

