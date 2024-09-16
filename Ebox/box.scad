// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <ProjectBox/project_box.scad>
include <ProjectBox/mounts.scad>
include <ProjectBox/shtc3_window.scad>
include <board.scad>

ones = [1, 1, 1];

// The walls of the EBox are 1mm thick.
wall_thickness = 1;
// Gap distance between PCB and ebox walls.
gap = 0.2;
// Round the edges of the EBox with a 2mm radius.
corner_radius = 2;

// The basic EBox only has 2mm space above the board.
space_above_board = 2;
// If the PCBA will have an OLED screen the tallest part of the box will be taller.
total_above_board = (oled || radar) ? 15 : space_above_board;
// This is the space below the box, used for the screw mounts.
space_below_board = 3;
// This is the space around the PCBA which the EBox should not intersect.
inner_dims = (board_dims
	      + Z*(space_above_board+space_below_board)
	      + 2*gap*ones);
// This is the size of the outer-surface of the EBox.
outer_dims = (inner_dims
	      + 2*ones*wall_thickness
	      + [2, 2, 0] * corner_radius);

// "Bump" to add on on top of EBox.
// - Zero-sized if neither radar nor OLED is included.
// - Space for radar if included.
// - Space for OLED (with a window) if OLED is included.
// This is the xyz offset of the bump from the 0,0,0 corner of the EBox.
b1o = radar
  ? [15, 8, outer_dims[2]-wall_thickness]
  : [0, 0, outer_dims[2]-wall_thickness];
// This is the outer dimension of the bump.
b1d = radar
  ? [outer_dims[0]-b1o[0]-8, outer_dims[1]-b1o[1], total_above_board-space_above_board]
  : [37, outer_dims[1]-b1o[1], total_above_board-space_above_board];

// This is the position of the PCBA within the EBox.
module in_Room133_board_frame(board_height=false) {
  zoffset = wall_thickness + (board_height ? space_below_board : 0);
  in_board_frame(outer_dims, board_dims, zoffset) children();
}

// This is a window from the top of the EBox with a wall down to the SHTC3 temp/humidity
//  sensor which isolates it from the air inside the resst of the EBox.
module shtc3_window(x, y, z_max, negative) {
  sz = [3, 3, 1];
  w = wall_thickness;
  translate([x, y, space_below_board + board_dims[2]]) {
    if (negative) {
      translate([0, 0, -1]) {
	cube([sz[0], sz[1], z_max+2]);
      }
    } else {
      translate([-w, -w, -0.5]) {
	cube([sz[0] + 2*w, sz[1] + 2*w, z_max]);
      }
    }
  }
}

// This is the module which constructs the EBox for the Room133 device.
module Room133_box(top) {
  wall = wall_thickness;
  screw_tab_d = mount_tab ? 10 : 0;

  shtc3_loc = [board_dims[0] - 4.2, 7.3];

  difference() {
    union() {
      // This is the main body of the EBox, without additional "bump" for OLED/radar.
      project_box(outer_dims,
		  wall_thickness=wall_thickness,
		  gap=gap,
		  corner_radius=corner_radius,
		  top=top,
		  screw_tab_d=screw_tab_d);
      if (top) {
	// Stuff to add on EBox top
	translate(b1o) {
	  // Positive space (outside) of OLED/radar bump.
	  rounded_box(b1d, corner_radius);
	}
	// Outer walls of SHTC3 window/well to isolate sensor from rest of the EBox.
	in_Room133_board_frame()
	  shtc3_window(shtc3_loc[0], shtc3_loc[1], space_above_board+wall, false);
      } else {
	// Stuff to add on EBox bottom
	in_Room133_board_frame() at_corners(board_dims, mount_offset)
	  screw_mount(space_below_board, wall, 1.25);
      }
    }
    // Cut outs.
    if (top) {
      // Negative space of OLED/radar bump.
      translate(b1o+[wall, wall, -epsilon]) rounded_box(b1d-wall*[2, 2, 1], corner_radius);
      if (oled) { // Oled cutout
	room133_to_oled_frame() translate ([4, 8, 5]) cube([12, 28, 20]);
      }
      // USB power-connector cutout
      translate([35, -1, space_below_board-1+0.001]) cube([11, 4, 6+epsilon]);
      // Connector cutouts
      in_Room133_board_frame() {
	if (motion_light) { // Hole for PIRL connector
	  translate([board_dims[0]-7.5, 14, space_below_board+1.6]) cube([7, 13, 50]);
	}
	if (motion) { // Hole for PIR connector
	  translate([board_dims[0]-12.8, 27.5, space_below_board+1.6]) cube([7, 11.5, 50]);
	}
	// Window for temp/humidity sensor
	shtc3_window(shtc3_loc[0], shtc3_loc[1], space_above_board+20, true);
	if (leak_check) {  // Hole for leak-detector connector.
	  translate([board_dims[0]-13.7, 5.5, space_below_board+1.6]) cube([8.4, 8, 50]);
	}
      }
    }
  }
}
