# best way #

  1. download and unzip latest puss
> > http://code.google.com/p/ljedit/downloads/list


> 2. download and unzip GTK+ for windows runtime environment into 

&lt;puss&gt;


> > http://ljedit.googlecode.com/files/puss-1.1-environ.tar.bz2


> finally like this:
> 
---

```
    [puss]
        [environ]
            [gtk]
            [pylibs]
        [extends]
        [res]
        puss.exe
```

> run puss.exe & enjoy it!

# DIY way #

  1. download and unzip puss.zip
> > http://ljedit.googlecode.com/files/puss-1.1-last.zip


> 2. download and install GTK+ runtime environment for win32
> > http://ftp.gnome.org/pub/GNOME/binaries/win32


> gtk+ bundle:
> > http://ftp.acc.umu.se/pub/GNOME/binaries/win32/gtk+/2.14/gtk+-bundle_2.14.7-20090119_win32.zip


> need these(use new verison):
> > gtksourceview-2.4
> > libxml2-2.6.27


> regist environ:
> > GTK\_BASEPATH = <gtk path where you unzip them>
> > PATH = %PATH%; <gtk path>\bin


> 3. download and install python-2.5.1 & pygtk
> > http://www.python.org/ftp/python/2.5.1/python-2.5.1.msi
> > http://ftp.gnome.org/pub/GNOME/binaries/win32
> > > pycairo-1.4.12-1.win32-py2.5
> > > pygobject-2.14.1-1.win32-py2.5
> > > pygtk-2.12.1-2.win32-py2.5     # support GtkBuilder

# Developer way #
  1. get puss source

> > http://code.google.com/p/ljedit/source/checkout


> 2. get GTK+ runtime & develop environment
> > http://ftp.gnome.org/pub/GNOME/binaries/win32


> gtk+ bundle:
> > http://ftp.acc.umu.se/pub/GNOME/binaries/win32/gtk+/2.14/gtk+-bundle_2.14.7-20090119_win32.zip


> need these(runtime & dev!!! ):
> > gtksourceview-2.4
> > libxml2-2.6.27


> regist environ:
> > GTK\_BASEPATH = <gtk path where you unzip them>
> > PATH = %PATH%; <gtk path>\bin


> 3. compile with mingw32 or vs2005!

FAQ:
  1. how to generate .lib file from .dll
> download pexports.exe in mingw-utils
    1. pexports xxx.dll > xxx.def
> > 2. lib /DEF:xxx.def xxx.lib