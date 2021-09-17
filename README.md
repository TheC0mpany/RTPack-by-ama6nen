# Modified RTPack

## Features
* Unpack rttex files into png images
* Automatically compress rttex files when packing png images into rttex files
* Alpha premultiplication colors are fixed by default
* Handle files in batches (Drag & Drop multiple rttex/png files)
* Automatically identifies whether to unpack or pack a file based on extension, is able to pack and unpack mixed png/rttex files
* No need for command line, everything is automatic, just drag and drop your files
* Only decompress rttex files instead of unpacking with -decomp
* Disable automatic compression with -nocomp


## How to use
  1. Download proton from [here](https://github.com/SethRobinson/proton/)
  1. Download this source, replace the existing files in proton with ones from this one
  1. Build using the modified solution in RTPack/windows_vc2017 (Debug, Win32)
  1. Or download the built executable from [here](https://github.com/ama6nen/RTPack/releases/tag/V1.6)
