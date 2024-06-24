# VLC Playlist Creator

This application creates VLC-compatible XSPF playlists from a directory of video files.

## Prerequisites

- CMake (version 3.10 or higher)
- Qt5
- A C++11 compatible compiler
- Sudo privileges for installation

## Building and Installing the Project

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/vlc-playlist-creator.git
   cd vlc-playlist-creator
   ```

2. Run the installer script:
   ```
   chmod +x install.sh
   ./install.sh
   ```

The program will be installed to `/usr/local/bin`.

## Usage

After installation, you can run the program from anywhere by typing:

```
vlc-playlist-creator
```

Use the GUI to select a directory containing video files. The application will create a VLC-compatible XSPF playlist in the selected directory.
