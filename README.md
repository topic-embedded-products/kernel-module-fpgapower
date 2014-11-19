kernel-module-fpgapower
=======================

Dummy power supply that indicates that the FPGA has been programmed.
This is used as a workaround for the SDIO interface, which is needed
for the SD card (boot device) and the WIFI chip (only available
after the FPGA has been programmed). Claiming that the WIFI power
is supplied by this module keeps the kernel from actually probing the
SDIO bus via EMIO until this module loads, which is done after
programmaming the FPGA.
