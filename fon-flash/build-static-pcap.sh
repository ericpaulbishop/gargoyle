mkdir -p static-pcap
cd static-pcap
wget http://www.tcpdump.org/release/libpcap-1.0.0.tar.gz
tar xvzf libpcap-1.0.0.tar.gz
cd libpcap-1.0.0
./configure
make
mv libpcap.a ../..
cp pcap.h ../..
cp -r pcap ../..
cd ../..
