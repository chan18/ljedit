how to install puss on windows?

<best way>
    1. download and unzip puss.rar
        http://ljedit.googlecode.com/files/puss-1.0-last.rar

    2. download and unzip GTK+ for windows runtime environment into [puss]
        http://ljedit.googlecode.com/files/puss-1.0-environ.rar

    3. install python-2.5.1    <optional but some extends write use python>
        http://www.python.org/ftp/python/2.5.1/python-2.5.1.msi 

    finally like this:
    ------------------------------
    [puss]
        [environ]
        [extends]
        [res]
        boot.exe
        puss.exe

    run boot.exe & enjoy it!

<DIY way>
    1. download and unzip puss.rar
        http://ljedit.googlecode.com/files/puss-1.0-last.rar

    2. download and install GTK+ runtime environment for win32
        http://ftp.gnome.org/pub/GNOME/binaries/win32

       need these:
        glib-2.16.1
        gtk-2.12.6            << 2.12.8 or 2.12.9 has bugs when show POPUP_WINDOW!
        gtksourceview-2.0.2
        atk-1.20.0
        cairo-1.4.14
        pango-1.18.4

        gettext-runtime-0.17
        libjpeg-6b-4
        libpng-1.2.8
        libtiff-3.7.1
        libxml2-2.6.27

        win_iconv_dll-tml-20080128

       regist environ:
        GTK_BASEPATH = <gtk path where you unzip them>
        PATH = %PATH%; <gtk path>\bin

    3. download and install python-2.5.1 & pygtk
        http://www.python.org/ftp/python/2.5.1/python-2.5.1.msi
        http://ftp.gnome.org/pub/GNOME/binaries/win32
            pycairo-1.4.12-1.win32-py2.5
            pygobject-2.14.1-1.win32-py2.5
            pygtk-2.12.1-1.win32-py2.5

<Developer way>
    1. get puss source
        http://code.google.com/p/ljedit/source/checkout

    2. get GTK+ runtime & develop environment
        http://ljedit.googlecode.com/files/gtk-dev.rar

       or download from
        http://ftp.gnome.org/pub/GNOME/binaries/win32

       need these(runtime & dev!!! ):
        glib-2.16.1
        gtk-2.12.6        << 2.12.8 or 2.12.9 has bugs when show POPUP_WINDOW!
        gtksourceview-2.0.2
        atk-1.20.0
        cairo-1.4.14
        pango-1.18.4

        gettext-runtime-0.17
        libjpeg-6b-4
        libpng-1.2.8
        libtiff-3.7.1
        libxml2-2.6.27

        win_iconv_dll-tml-20080128

       regist environ:
        GTK_BASEPATH = <gtk path where you unzip them>
        PATH = %PATH%; <gtk path>\bin

    3. compile with mingw32 or vs2005!

FAQ:
    1. how to generate .lib file from .dll
    download pexports.exe in mingw-utils
        1. pexports xxx.dll > xxx.def
        2. lib /DEF:xxx.def xxx.lib


