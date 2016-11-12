Cluster In A Box Boostrap Process:

1. Build the server binaries,
  e.g
   user:wwiiolng/$ mkdir build
   user:build/$ cd build
   user:build/$ cmake -G"Ninja" -DPN_DISABLE_AUTH=ON -DPN_BUILD_32BIT=ON ../WW2/src
   user:build/$ ninja -j5
2. Fetch a WW2Mods repo,
3. Run the bootstrap.sh script with the path to your server binaries:
  e.g. $ bootstrap.sh ~/wwiiolng/build
4. Launch the controller,
5. Copy the generated ".tkt" files to your client's data directory.

You can use something like this to start the client:

   ww2.exe -ticket gamename.tkt
      -data 1 -datadir DATA
      -window 1 -width 800 -height 640
      -log 0 -nowor 1 -noinf 1

(each option on the last line is optional and is designed to speed up startup)
