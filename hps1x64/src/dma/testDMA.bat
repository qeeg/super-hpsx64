g++ -w -mwindows -fpermissive -static-libgcc -static-libstdc++ -I../cd/src -I../dma/src -I../gpu/src -I../intc/src -I../mdec/src -I../pio/src -I../sio/src -I../r3000a/src -I../spu/src -I../timer/src -I../databus/src -I../r3000a/src/execution -O3 -o testDMA testDMA.cpp ../cd/src/*.cpp ../dma/src/*.cpp ../gpu/src/*.cpp ../intc/src/*.cpp ../mdec/src/*.cpp ../pio/src/*.cpp ../sio/src/*.cpp ../r3000a/src/*.cpp ../spu/src/*.cpp ../timer/src/*.cpp ../databus/src/*.cpp ../r3000a/src/execution/*.cpp -Wl,-subsystem,console -lopengl32
pause