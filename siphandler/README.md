# Build instructions

Build libre, librem and siphandler:

```
cd ../third_party/re
make

cd ../rem
make

cd ../baresip
STATIC=yes make libbaresip.a

cd ../../siphandler
make
```

