#!/bin/bash

perf stat -e uncore_imc_0/event=0x4,umask=0xc/,uncore_imc_1/event=0x4,umask=0xc/,uncore_imc_2/event=0x4,umask=0xc/,uncore_imc_3/event=0x4,umask=0xc/,uncore_imc_4/event=0x4,umask=0xc/,uncore_imc_5/event=0x4,umask=0xc/,uncore_imc_6/event=0x4,umask=0xc/,uncore_imc_7/event=0x4,umask=0xc/ -aA -o out -v -x, --pre ./wait.start ./wait.stop
