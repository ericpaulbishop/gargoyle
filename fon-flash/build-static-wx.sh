#sudo apt-get install libgtk2.0-dev libgnome2-dev
mkdir static-wx
cd static-wx
wget "http://downloads.sourceforge.net/wxwindows/wxWidgets-2.8.9.tar.gz"
tar xvzf wxWidgets-2.8.9.tar.gz
cd wxWidgets-2.8.9
./configure --enable-optimise --enable-stl --enable-unicode --enable-threads --enable-static --disable-shared --enable-monolithic
make
cd ..
cd ..
ln -s static-wx/wxWidgets-2.8.9/wx-config wx-config
