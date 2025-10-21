### Build
```
docker/run.sh cmake -S . -B build
docker/run.sh cmake --build build
```

### Flash
Note, after reboot to BOOTSEL mode (-f) it doesn't seem to enumerate
in docker.  Running this twice does work though.
```
docker/run.sh picotool load -v -x build/cnc_jogwheel.uf2 -f
```

