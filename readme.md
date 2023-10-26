before calling the ./test-witsshell.sh, we need to grant permissions:

cd tester
chmod +x run-tests.sh
cd.. ( to go back 1 directory )
chmod +x test-witsshell.sh

now can execute:
./test-witsshell.sh ( shows all 20 tests )


OR :
chmod +x setup.sh
./setup.sh
./test-witsshell.sh ( shows all 20 tests ) or only the .c file: (./witsshell.c)