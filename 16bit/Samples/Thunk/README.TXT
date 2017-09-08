SERVICE PROVIDER THUNK SAMPLE

THIS CODE IS PROVIDED "AS-IS" WITHOUT WARRANTY OF ANY KIND, EITHER
EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTY OF THE SPLIB PRODUCT.

This directory contains a sample for thunking to the TSP++ service provider
from a 32-bit companion application.  All the data passed from the provider
to the companion application is already thunked using the WM_COPYDATA message
available under Windows 95, Win32s and Windows NT.

This example gives a 32bit DLL which the companion links to, and a 16bit
function which is placed in the service provider to manage the thunking
back from the companion application.

The following files are in the directory:

32TO16.THK      16/32-bit thunk wrapper built for THUNK.EXE
DLL32.DEF       DEF file
DLL32.H         Interface file for 32-bit companion application
DLL32.MAK       VC4 makefile
DLL32.RC        Version resource for DLL
DLLMAIN.C       DeviceNotify32 definition

DLL16.C         16-bit DLL interface to add to service provider project

The DLLMAIN.C and DLL16.C must be modified to include the name of the
32-bit DLL being built and the name of the 16-bit service provider DLL
to be passed to the thunk connect code.

The thunk file must be compiled twice, once with the 32-bit project (the
makefile does this), and a second time for the 16-bit project.  The
16-bit command line is:

THUNK -t thk -o 16to32.asm -DIS_16 32to16.thk

This will create an .ASM file which would then be compiled into the 16-bit
service provider project.