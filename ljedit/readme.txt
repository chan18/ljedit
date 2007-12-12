
1. how to setup develop environment?

	* linux(ubuntu 7.10) - gcc
		libgtk-2.4-dev
		libgtkmm-2.4-dev
		libglademm-2.4-dev
		libsourceview-2.0-dev
		libsourceviewmm-2.0-dev
		python2.5-dev
		python-gtk2-dev
		
		!!! libsourceviewmm-2.0 1.9.2 version need modify gtksourceviewmm-2.0.pc
		!!! change this line :
			!!! Requires: gtksourceview-1.0 gtkmm-2.4
			!!! to 
			!!! Requires: gtksourceview-2.0 gtkmm-2.4

	* windows - vc8
		gtk-dev-2.10.11-win32-1.exe
		gtkmm-win32-devel-2.10.11-1.exe
		python-2.5.1
		pygobject-2.12.3-1.win32-py2.5.exe
		pygtk-2.10.4-1.win32-py2.5.exe
		pycairo-1.2.6-1.win32-py2.5.exe
		
		gtksourceview-2.0, gtksourceviewmm-2.0, glib-2.14.1
			=> http://ljedit.googlecode.com/files/gtksourceviewmm2.0-dev-msvc-with-glib2.14.1.rar
			
			if can not use, download this source & re compiler it
				=> http://ljedit.googlecode.com/files/libgtksourceviewmm-1.9.3%282.0%29-msvc-src.rar

