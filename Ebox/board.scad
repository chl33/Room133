// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <ProjectBox/project_box.scad>
include <ProjectBox/oled91.scad>
include <gui.scad>

board_thickness = 1.6;
// mm between pads in perf-board (1/10 inch)
pad_space = 2.54;
// This is the size of the PCB, for use in the design of the EBox.
board_dims = [18.75 * pad_space, 15.5 * pad_space, board_thickness];
// This is the distance of each edge of the PCB to the center of the
//  screw holes for mounting the board at each corner.
mount_offset = pad_space;

// This is the position of the OLED screen with respect to the PCB.
module room133_to_oled_frame() {
  u = pad_space;
  translate([u*8.1, 1, 12]) children();
}

// This is a model of the Room133 PCBA with OLED screen, for use in EBox design.
module Room133_board() {
  // Board imported from KiCad (VRML) -> meshlab.
  translate([board_dims[0]/2, board_dims[1]/2, board_dims[2]/2]) color("white")
    import(file="RoomL33a.stl", convexity=3);

  // Optionally add the OLED screen
  if (oled) {
    room133_to_oled_frame() oled91();
  }
}
