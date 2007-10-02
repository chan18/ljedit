
1. how to setup develop environment?

	* linux(ubuntu 7.04) - gcc
		libpthread-dev
		libgtk-dev
		libgtkmm-dev
		libglademm-dev
		libsourceview-dev
		libsourceviewmm-dev
		
		# if need use python plugin engine
			python2.5-dev
			pygobject-dev
			pygtk-dev
			pycairo-dev

	* windows - vc8
		pthread-win32
		gtk-dev-2.10.11-win32-1.exe
		gtkmm-win32-devel-2.10.11-1.exe
		gtksourceview-1.8.5			http://ftp.gnome.org/pub/GNOME/binaries/win32/gtksourceview/1.8/
		gtksourceviewmm-vc8.rar			http://ljedit.googlecode.com/files/gtksourceviewmm-vc8-src.rar
		
		# if need use python plugin engine
			python-2.5.1
			pygobject-2.12.3-1.win32-py2.5.exe
			pygtk-2.10.4-1.win32-py2.5.exe
			pycairo-1.2.6-1.win32-py2.5.exe
		

		# gtk has some bugs on windows(chinese language) )
		# 
		#  1. can not use(input/show) Enter in Gtk::TextView
		# 
		#     if you select gtk theme : MS-Windows, you need 
		#       * edit <GTK install path>/share/themes/MS-Windows/gtk-2.0/gtkrc
		#       * find and modify it:
		#                 style "msw-default"
		#                 {
		#                     font_name="simsun 9"   <<< add this line, set font & size
		# 
		#       * your Gtk::TextView will work now!
		# 

