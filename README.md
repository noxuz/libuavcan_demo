# libuavcanV1 demo on NXP UAVCAN node board
Instructions:

Interconnect 2 UAVCAN board nodes with a single JST-GH cable between both board's CAN2 plugs, and the connectors with the 120 ohm terimanl resistors soldered in the CAN1 ports as shown.

![alt text](board_connection.jpg)

Modify src/main.cpp line 28 with NODE_A or NODE_B for building for each board, NODE_A transmits first.

1. Open S32 Design Sudio.
2. Click File -> Import
3. Under the Git tab, select "Projects from Git" and next.
4. Click "Clone URL" and next.
5. Paste the URL of this repository and click next. https://github.com/noxuz/libuavcan_demo
6. Click the master branch and next.
7. Click "Browse" for the desired destination directory of the project and click next.
8. Choose "Import existing Eclipse projects" and click next and then "Finish".
9. Click in the Hammer Icon in the toolbar for building the project.
10. Click the yellow ligthning shaped icon in the toolbar for flashing the project into the board.
11. In the list at the left of the windowd that shows choose "project_name"_Debug_Segger and click flash with the board connected to the Jlink debugger and powered.
12. Repeat steps 10-13 for the other board but with the macro NODE_A or NODE_B swapped.
13. A green led close to the 5V headers should blink approx each second, read description in top of src/main.cpp, if the green LED from both boards is still, try pressing the reset button in the board that got the NODE_Aprogram flashed into, which is the one that starts the transmission.
14. With an oscilloscope view the frames being transmited at 4Mbit/s data phase and 1Mbit/s in nominal phase.

![alt text](CANFD_oscilloscope.png)
