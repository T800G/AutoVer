# Automatic File Version Tool
A tool to set file and product version on PE executables.
![alt text](https://github.com/T800G/AutoVer/blob/master/autover.png "Automatic File Version Tool")<br/>
This tool does not update `FileVersion` or `ProductVersion` strings inside `VS_VERSIONINFO` structure, only `dwFileVersionLS` and `dwProductVersionLS` members of `VS_FIXEDFILEINFO` structure.<br/>
<br/>
Supports setting VersionInfoVersion field in Inno Setup script files (ISS) so you can version installer binaries.<br/>
<br/>
## Using AutoVer with a build system<br/>
<br/>
1. Copy AutoVer.exe to your `PATH` folder or to your build system's executable directory<br/>
(eg. C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin).<br/>
<br/>
2. Add a command to the post-build event in your project, eg.:

>AutoVer.exe "$(TargetPath)" /p

Optional */p* command line switch must follow respective file path:

>AutoVer.exe <file1> /p <file2> /p  ... <fileN> /p
