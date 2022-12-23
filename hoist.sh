gcc src/inspection_console.c -lncurses -lm -o bin/inspection
gcc src/command_console.c -lncurses -o bin/command
gcc src/m_x.c -o bin/m_x
gcc src/m_z.c -o bin/m_z
gcc src/world.c -o bin/world
gcc src/master.c -o bin/master

./bin/master
