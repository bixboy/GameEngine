# GameEngine Setup

This project depends on the prebuilt SDL3 development libraries for Visual C++.
Because the official import libraries and DLLs are distributed as binaries they
are not stored in this repository.  Before opening the solution in Visual Studio
run the helper below from the repository root to download and unpack the correct
version automatically:

```bash
python GameEngine/Tools/fetch_sdl3.py
```

The script downloads the official archive from the SDL releases page, verifies
its checksum, and extracts the required files into `GameEngine/Libs/SDL3-3.2.22`.
Re-run it with `--force` whenever you want to refresh the binaries.
