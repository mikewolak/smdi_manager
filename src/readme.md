# SMDI Sampler Manager for IRIX 5.3

A Motif-based graphical user interface for managing SCSI samplers using the SMDI (SCSI Musical Data Interchange) protocol on SGI IRIX 5.3 systems.

![SMDI Sampler Manager](screenshot.png)

## Features

- Connect to SCSI samplers using the SMDI protocol
- Scan SCSI bus for available devices
- View, upload, download, and delete samples
- Support for AIF audio file format
- Batch upload multiple samples at once
- Configurable sample ID assignment
- Custom grid widget for sample browsing

## Requirements

- SGI workstation running IRIX 5.3
- ANSI C compiler (SGI cc)
- Motif 1.2 development libraries
- SCSI sampler supporting the SMDI protocol

## Building

The application uses a standard IRIX Makefile:

```
make clean    # Clean previous build artifacts
make          # Build the application
make install  # Install to /usr/local/bin (optional)
```

## Usage

1. Launch the application:
   ```
   ./bin/smdi_manager
   ```

2. Scan for SCSI devices using the "Scan" button
3. Select the appropriate Host Adapter and Target ID
4. Connect to your SMDI device using the "Connect" button
5. Browse, upload, and manage your samples

### Uploading Samples

- Single sample: Operations → Send AIF File...
- Multiple samples: Operations → Send Multiple AIF Files...

When uploading samples, you can specify the starting sample ID. If no ID is specified, the application will use the next available ID.

### Downloading Samples

Right-click on a sample in the list and select "Receive as AIF..." to download it to your filesystem.

### Deleting Samples

Right-click on a sample in the list and select "Delete Sample" to remove it from the sampler.

## Architecture

The application is built using:
- ANSI C90 for maximum compatibility with IRIX 5.3
- Motif 1.2 for the user interface
- Custom SMDI implementation for SCSI communication
- AIF file format support via SGI's Audio File Library

## Troubleshooting

- If the SCSI scan doesn't find your device, check cabling and SCSI termination
- If the application reports "Failed to connect to SMDI device", verify that your device supports the SMDI protocol
- For audio file format issues, ensure that your AIF files are compatible with SGI's Audio File Library

## License

Copyright © 2025 Mike Wolak (mikewolak@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

## Acknowledgments

- Based on the SMDI protocol documentation
- Inspired by similar tools available on other platforms
- Thanks to the SGI IRIX community for continued support of this classic platform