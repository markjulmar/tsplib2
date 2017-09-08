/*****************************************************************************/
//
// DSSPINT.H - Digital Switch Service Provider Sample
//                                                                        
// This file contains the interface between the companion application
// and the service provider code.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/*****************************************************************************/

// New command codes sent from the service provider to the companion application.
// These go directly through to the emulator.
#define COMMAND_GETEMULATORSETTINGS         (COMMAND_END+1)
#define COMMAND_GETEMULATORADDRESSINFO      (COMMAND_END+2)
#define COMMAND_EMULATORPREPARECALL         (COMMAND_END+3)
#define COMMAND_EMULATORDROPCALL            (COMMAND_END+4)
#define COMMAND_EMULATORDIAL                (COMMAND_END+5)
#define COMMAND_GETVERSIONINFO              (COMMAND_END+6)
#define COMMAND_EMULATORANSWERCALL          (COMMAND_END+7)
#define COMMAND_EMULATORHOLDCALL            (COMMAND_END+8)
#define COMMAND_EMULATORUNHOLDCALL          (COMMAND_END+9)
#define COMMAND_EMULATORTRANSFER            (COMMAND_END+10)
#define COMMAND_EMULATORFLASH               (COMMAND_END+11)
#define COMMAND_EMULATORFORWARD             (COMMAND_END+12)
#define COMMAND_SETGAIN                     (COMMAND_END+13)
#define COMMAND_SETVOL                      (COMMAND_END+14)
#define COMMAND_SETHOOKSWITCH               (COMMAND_END+15)
#define COMMAND_EMULATORCONFERENCE          (COMMAND_END+16)
#define COMMAND_EMULATORREDIRECT            (COMMAND_END+17)
#define COMMAND_SETRINGMODE                 (COMMAND_END+18)
#define COMMAND_COMPLETECALL                (COMMAND_END+19)
#define COMMAND_PARKCALL                    (COMMAND_END+20)
#define COMMAND_UNPARKCALL                  (COMMAND_END+21)
// This forces a context switch to a 16-bit context for INIT
#define COMMAND_PERFORMINIT                 (COMMAND_END+22)
#define RESULT_PERFORMINIT                  (RESULT_END+1)
