Build instructions for Jovian project

This document assumes that we are building under Windows (XP or 7) and using Visual Studio 2013 (VS12).
The link for VS 13 is https://www.visualstudio.com/downloads/download-visual-studio-vs#DownloadFamilies_4

Note: We'll be building Jovian and related tools in 64-bit, assuming a 64-bit install of windows7.

For Windows, we reccomend installing a path editor. We use the version from RedFern
 (http://download.cnet.com/Path-Editor/3000-2094_4-10672356.html)

We recommend putting the source into the root directory of any drive as MinGW seems to have
problems with My Documents and other typical Windows file names.

Also, create a build directory at the same time. Our typical layout looks like:
D:\Jovian\
	build
	Jovian (source)
	MouseOver
	RemoteDataServer

New MingW based install
1. Install minimal MingW
  - Download mingw installer http://sourceforge.net/projects/mingw/files/latest/download?source=files
  - Run the installer (mingw-get-setup.exe)
    - Select the defaults for initial steps
    - When the GUI to install mingw components opens, select "Basic Setup" from the left panel
    - Select "msys-base" from the right panel. Select "Mark for Installation".
    - From the Installation menu, select "Apply Changes"

2. Install Curl
  - Download curl: http://www.confusedbycode.com/curl/#downloads
  - Run installer and follow prompts

3. Install Qt
  - Download from https://download.qt.io/archive/qt/5.5/; Select latest version (currently 5.5.1)
  - Choose 64-bit Qt: qt-opensource-windows-x86-msvc2013_64-5.5.x.exe
  - Follow installation directions
  - If not using a path editor:
    - Control Panel > System > Advanced Setting > Environment Variable
    - Edit the System PATH environment variable
  - Add the Qt bin directory for this version: C:\Qt\Qt5.5.x\5.5\msvc2013_64\bin)

4. Install Python
  - Download https://www.python.org/ftp/python/3.5.0/python-3.5.0-amd64.exe
  - Install, select option Add to PATH

5. (Optional)Install CMake
  - If you choose to skip this step, cmake will automatically be built from source
    but unavailable to the rest of the system
  - Download latest from http://www.cmake.org/cmake/resources/software.html.
  - Choose the version labeled "Windows (Win32 Installer)"
  - Install, select option "Add to PATH for all users"

6. Enable Visual Studio from MinGW
  - From the Start Menu, Select All Programs->Visual Studio 2013->Visual Studio Tools
  - Run "VS2013 x64 Native Tools Command Prompt"
  - In the command prompt, type "cd c:\MinGW\msys\1.0" (assuming a default installation 
    location of MinGW)
  - type "msys.bat". This will open a second command prompt with green text. From
    here on, all operations will be in this window.
  - Since we're now running under MinGW, everything has a Unix syntax, so 
    directories/drives are addressed as /z/Jovian instead of z:\Jovian

7. Build Jovian and MouseOver
  - cd to the Jovian source directory
  - run "./build.cmake -platform windows-x86_64 install <build_location>
    (in our default scenario that directory is /d/Jovian/build)
    There are other arguments to the build script. Run build.cmake to see all
    the options
  - Get a cup of coffee. If this works, a new build can take 1-2 hours to complete.
  - Currently, it won't work and MouseOver will fail (you can check the log file in the
    MouseOver_build directory). To fix this:
    - Launch the MouseOver VS Solution file
    - Right Click on MouseOver
    - Select Linker->General->Additional Library Directories, and add the Boost lib directory.
    - Ensure the compile mode is Release, and compile.

8. Add paths
  - MouseOver needs to find a number of dlls, so add the following paths to PATH
    - Boost lib
    - OpenSceneGraph bin
    - OpenSceneGraph plugins bin (in OSG/bin/plugins-x.y.z)
    - osgWorks bin
    - osgBullet bin
    - Collada bin


------------------------------

Old Version (out of date)


1. Install CMake
  - Download latest from http://www.cmake.org/cmake/resources/software.html.
  - Choose the version labeled "Windows (Win32 Installer)"
  - Install

2. Install Qt
  - Download from https://download.qt.io/archive/qt/5.5/5.5.0/
  - Choose 32-bit Qt: qt-opensource-windows-x86-msvc2013-5.5.0.exe
  - Follow installation directions
 - Control Panel > System > Advanced Setting > Environment Variable
 - Edit the System PATH environment variable
 - Add the Qt bin directory for this version: C:\Qt\Qt5.5.0\5.5\msvc2013\bin)

3. Build Boost
 - Download from http://sourceforge.net/projects/boost/files/boost/1.58.0/
 - Unpack boost
 - Open a VS2013 x86 Native Tools command prompt and change to the directory where you unpacked boost
 - run ".\boostrap.bat"
 - run ".\b2.exe address-model=32 --prefix="C:\Program Files (x86)\Boost_1.58" --build-type=complete install

4. Build Collada
 - Download from http://sourceforge.net/projects/collada-dom/files/Collada%20DOM/Collada%20DOM%202.2/. Select "collada_dom-2.2.zip"
 - Unpack Collada
 - Navigate to collada-dom-2.2 > dom > projects > vc10-1.4
 	- Double click the dom Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "Solution 'dom' in the Solution Explorer; Then right click and select
	  "Build Solution"
	- When Done, Change Debug to Release and repeat last the step
 - Set the COLLADA_DIR environment variable
	- Control Panel > System > Advanced Setting > Environment Variables
- Create a new System environment variable: COLLADA_DIR. Set the value to the path where collada-dom was unpacked.
 - Edit the System PATH environment variable
 - Add the Collada lib and debug directories (%COLLADA_DIR%\dom\build\vc10-1.4;%COLLADA_DIR%\dom\build\vc10-1.4-d)

5. Build OpenSceneGraph
 - If needed, install 7-zip (WinZip is unable to extract these files)
	- Using 7-zip, it's a two step process, first to uncompress, the second to extract.
 - Unpack the OpenSceneGraph-Data from the external_libraries directory in Jovian
 - Unpack OpenSceneGraph from the same directory
 - Run CMake
	- Set "Browse Source" to the unpacked OpenSceneGraph directory
	- Set "Browse Build" to a new directory OpenSceneGraph under build
	- Press Configure
	- When done, Select "Build_OSG_Examples"
	- Select Advanced
	  - Search for JPEG.
	  - Set JPEG_INCLUDE to C:\Program Files\GnuWin32\include
	  - Set JPEG_LIBRARY to C:\Program Files\GnuWin32\lib\jpeg.lib
	  - Search TIFF
	  - Set TIFF_INCLUDE to C:\Program Files\GnuWin32\include
	  - Set TIFF_LIBRARY to C:\Program Files\GnuWin32\lib\libtiff.lib
	- Configure twice more then Generate
 - Goto the OpenSceneGraph build directory
	- Double click the OpenSceneGraph Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "ALL_BUILD" in the Solution Explorer; Then right click and select Build
	- When Done, Scroll the Solution Explorer until you see INSTALL, then select,
	  Right Click and build.
	- When Done, Change Debug to Release and repeat last two steps
 - Set the OSG_DIR environment variable
	- Control Panel > System > Advanced Setting > Environment Variables
	- Create a new user environment variable: OSG_DIR. Set the value to the path
	  where OpenSceneGraph was installed (On Windows typically, "C:\Program Files",
	  on Linux and Mac "/usr/local").
 - Quit Visual Studio
 - Control Panel > System > Advanced Setting > Environment Variable
 - Edit the System PATH environment variable
 - Add the OpenSceneGraph bin directory (typically C:\Program File\OpenSceneGraph\bin)
 - Add User Environment variable OSG_FILE_PATH to the unpacked OpenSceneGraph-Data directory

6. Build Bullet
 - If needed, install 7-zip (WinZip is unable to extract these files)
	- Using 7-zip, it's a two step process, first to uncompress, the second to extract.
 - Unpack bullet-<version> from the external_libraries directory in Jovian
 - Run CMake
	- Set "Browse Source" to the unpacked bullet-<version>
           directory ( the bullet source may have extra cruft such as
           revision number in the archive file. That information will
           not show up in the directory name, e.g., if the archive is
           "bullet-2.79-rev2440.tgz", then the directory will be
           "bullet-2.79").
	- Set "Browse Build" to a new directory bullet under build
	- Press Configure
	- When done, Select "INSTALL_EXTRA_LIBS" and "INSTALL_LIBS"
	- Configure twice more, then Generate
 - Goto the bullet build directory
	- Double click the bullet Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "ALL_BUILD" in the Solution Explorer; Then right click and select Build
	- When Done, Scroll the Solution Explorer until you see INSTALL, then select,
	  Right Click and build.
	- When Done, Change Debug to Release and repeat last two steps
 - Quit Visual Studio

7. Build osgWorks
 - If needed, install 7-zip (WinZip is unable to extract these files)
	- Using 7-zip, it's a two step process, first to uncompress, the second to extract.
 - Unpack osgworks from the external_libraries directory in Jovian
 - Run CMake
	- Set "Browse Source" to the unpacked osgworks directory
	- Set "Browse Build" to a new directory osgworks under build
	- Configure twice, then Generate
 - Goto the bullet build directory
	- Double click the bullet Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "ALL_BUILD" in the Solution Explorer; Then right click and select Build
	- When Done, Scroll the Solution Explorer until you see INSTALL, then select,
	  Right Click and build.
	- When Done, Change Debug to Release and repeat last two steps
 - Quit Visual Studio
 - Control Panel > System > Advanced Setting > Environment Variable
 - Edit the System PATH environment variable
 - Add the osgWorks bin directory (typically C:\Program File\osgWorks\bin)

8. Build osgBullet
 - If needed, install 7-zip (WinZip is unable to extract these files)
	- Using 7-zip, it's a two step process, first to uncompress, the second to extract.
 - Unpack osgbullet from the external_libraries directory in Jovian
 - Run CMake
	- Set "Browse Source" to the unpacked osgbullet directory
	- Set "Browse Build" to a new directory osgbullet under build
	- Configure
	- In all likely hood, CMake will barf an error message about not finding BULLET_INCLUDE_DIR. Check Advanced. Then set BULLET_INCLUDE_DIR to the bullet include in the install (C:\Program Files\BULLET_PHYSICS\include\bullet)
	- Check for any missing Bullet Debug libraries and set those missing manually
	- Configure twice, then Generate
 - Goto the bullet build directory
	- Double click the bullet Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "ALL_BUILD" in the Solution Explorer; Then right click and select Build
	- When Done, Scroll the Solution Explorer until you see INSTALL, then select,
	  Right Click and build.
	- When Done, Change Debug to Release and repeat last two steps
 - Quit Visual Studio
 - Control Panel > System > Advanced Setting > Environment Variable
 - Edit the System PATH environment variable
 - Add the osgBullet bin directory (typically C:\Program File\osgBullet\bin)

9. Build Jovian
 - Create a Jovian directory under build
 - Run CMake
	- Set "Browse Source" to the Jovian src directory
	- Set "Browse Build" to a new directory Jovian under build/Jovian
	- Press Configure
	- Most likely, it will give you an error about not finding Boost_INCLUDE_DIR
	- Set Boost_ROOT to C:/Program Files/Boost_1.58
	- Configure twice more then Generate
 - Goto the Jovian build directory
	- Double click the Jovian Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "Solution Jovian in the Solution Explorer; Then right click and select Build

10. Build MouseOver
 - Create a MouseOver directory under build
 - Run CMake
	- Set "Browse Source" to the Jovian src directory
	- Set "Browse Build" to a new directory Jovian under build/Jovian
	- Press Configure
	- Set JOVIAN_BUILD_DIR and JOVIAN_SOURCE_DIR as appropriate
	- Most likely, it will give you an error about not finding Boost_INCLUDE_DIR
	- Set Boost_ROOT to C:/Program Files/Boost_1.58
	- Configure twice more then Generate
 - Goto the MouseOver build directory
	- Double click the MouseOver Visual Studio Solution icon to launch VS
	- The toolbar should say 'Debug'; if not select it in the drop down menu
	- Left Click "Solution MouseOver in the Solution Explorer; Then right click and select Build
	- I had to add the boost lib dir in MouseOver->properties->Linker->General->Additional Linker Directories

