
1. how to setup develop environment?

	* linux(ubuntu 7.10) - gcc
		libpthread-dev
		libgtk-dev
		libgtkmm-dev
		libglademm-dev
		libsourceview-2.0-dev
		libsourceviewmm-2.0-dev
		
		!!! libsourceviewmm-2.0 1.9.2 version need modify gtksourceviewmm-2.0.pc
		!!! change this line :
			!!! Requires: gtksourceview-1.0 gtkmm-2.4
			!!! to 
			!!! Requires: gtksourceview-2.0 gtkmm-2.4
		
		# if need use python plugin engine
			python2.5-dev
			pygobject-dev
			pygtk-dev
			pycairo-dev

	* windows - vc8
		pthread-win32
		gtk-dev-2.10.11-win32-1.exe
		gtkmm-win32-devel-2.10.11-1.exe
		
		gtksourceview-2.0, gtksourceviewmm-2.0, glib-2.14.1
			=> http://ljedit.googlecode.com/files/libgtksourceviewmm-1.9.3%282.0%29-msvc-src.rar
		
		# if need use python plugin engine
			python-2.5.1
			pygobject-2.12.3-1.win32-py2.5.exe
			pygtk-2.10.4-1.win32-py2.5.exe
			pycairo-1.2.6-1.win32-py2.5.exe

