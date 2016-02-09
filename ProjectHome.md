# free source code editor use gtk+, gtksourceview & pygtk #

  * support linux & windows
  * support c/c++ plugin & python plugin!
  * support c/c++ language auto-complete, preview ...

# connect us #

  * email : [mailto:louisliangjun@gmail.com](mailto:louisliangjun@gmail.com)

# update #
  * 2009-11-07
    * fix some serious bugs

  * 2009-07-26
    * problem :
      * need rm /usr/share/puss before install on linux!!!!
      * need rm old files before use new package!!!!
      * because of old plugin(ljcs) already removed
    * use C rebuild c/c++ language tips plugin, and support
      * F12 : show current tag in preview panel
      * Alt+G/Ctrl+F12 : loop jump to tag implement or declare
      * Ctrl+Shift+space : show function args tip
      * Alt+Right : auto complete
    * add simple find/replace dialog

  * 2009-02-01
    * puss-1.1 windows version finished!
    * puss-1.1-environ for windows finished! (include gtk/pygtk & python runtime environment)
    * puss refactor : change extends/plugins/options architecture
    * now can use new gtk+ on win32(fixed bugs)
    * add puss\_vconsole plugin on win32(use cmd.exe)
    * Ctrl+Tab switch document

  * 2008-04-12
    * publish first alpha version for ubuntu-8.04

  * 2008-04-09
    * support i18n, and add Chinese language, in windows need set LANG=zh\_CN
    * document/left/right/bottom tab set reorderable and detachable
    * add gtk-doc helper extend
    * fix some bugs

  * 2008-04-04
    * publish first usable version for windows
    * port ljedit to puss finish
    * support c/c++ helper
    * support language selector
    * support search tools
    * support options

  * 2008-02-xx
    * rename ljedit to puss
    * reconstruct use gtk+ & gtksourceview
    * remove python from main program(use python as extend)
    * use GtkBuilder replace Glade to build UI

# FAQ #
  * how to install puss on windows ?
    1. see [wiki](http://code.google.com/p/ljedit/w/list)

  * puss for linux(ubuntu 8.04)
    * dependence
      1. gtk-2.12
      1. libgtksourceview-2.0
      1. python-gtk2

  * how to make puss plugin & extend with c/c++
    * see [wiki](http://code.google.com/p/ljedit/w/list)

  * how to make puss plugin & extend with python
    * see see [wiki](http://code.google.com/p/ljedit/w/list)

