# FilterFilesMT

![GitHub License](https://img.shields.io/github/license/AndrewGraber/FilterFilesMT)
![GitHub Release](https://img.shields.io/github/v/release/AndrewGraber/FilterFilesMT)
[![Build Status](https://img.shields.io/github/actions/workflow/status/yourusername/FilterFilesMT/build.yml?branch=main)](https://github.com/yourusername/FilterFilesMT/actions)

FilterFilesMT is a fast, multithreaded command-line tool for Windows that recursively searches a directory and filters files based on glob-style (.gitignore) rules. It's ideal for piping filtered file lists from large/deep directories into other programs or scripts.

## Features
- Multithreaded search for maximum performance
- Supports glob-style ignore rules via arguments or input file
- Recursive scanning of directories
- Outputs non-ignored files to standard output for easy piping

## Installation
### Github Releases
Download the latest .exe from the releases page.

### Scoop
```powershell
scoop bucket add mybucket https://github.com/AndrewGraber/scoop-bucket.git
scoop install filterfilesmt
```

### WinGet
```ps
winget install AndrewGraber.FilterFilesMT
```

# Usage
```powershell
filterfilesmt <folder> <num_threads>
```
- `<folder>` - Path to the directory to scan
- `<num_threads> - Number of worker threads to use

### Example
```powershell
filterfilesmt C:\Projects\ 8 > file_list.txt
```
- Reads .filterignore in `C:\Projects` for ignore patterns
- Recursively filters files, ignoring those that match patterns
- Outputs results to standard output (can redirect to file or pipe to another program)

## .filterignore Format
- One glob-style rule per line
- Supports * and most other .gitignore-style patterns
- `?` operator not currently implemented

### Example
```
*.tmp
build/
*.log
```

## Contributing
Contributions are welcome! Feel free to:
- Submit bug reports and feature requests via GitHub Issues
- Open pull requests for fixes or enhancements

## License

MIT License - see LICENSE for details