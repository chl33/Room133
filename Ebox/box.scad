// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <ProjectBox/project_box.scad>
include <ProjectBox/mounts.scad>
include <ProjectBox/shtc3_window.scad>
include <board.scad>
include <gui.scad>

ones = [1, 1, 1];

wall_thickness = 1;
gap = 0.2;
corner_radius = 2;
mount_offset = pad_space;

space_above_board = 2;
total_above_board = (oled || radar) ? 15 : space_above_board;
space_below_board = 3;
inner_dims = (board_dims
	      + Z*(space_above_board+space_below_board)
	      + 2*gap*ones);
outer_dims = (inner_dims
	      + 2*ones*wall_thickness
	      + [2, 2, 0] * corner_radius);

b1o = radar
  ? [15, 8, outer_dims[2]-wall_thickness]
  : [0, 0, outer_dims[2]-wall_thickness];
b1d = radar
  ? [outer_dims[0]-b1o[0]-8, outer_dims[1]-b1o[1], total_above_board-space_above_board]
  : [37, outer_dims[1]-b1o[1], total_above_board-space_above_board];

module in_Room133_board_frame(board_height=false) {
  zoffset = wall_thickness + (board_height ? space_below_board : 0);
  in_board_frame(outer_dims, board_dims, zoffset) children();
}

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

module Room133_box(top) {
  wall = wall_thickness;
  screw_tab_d = mount_tab ? 10 : 0;

  shtc3_loc = [board_dims[0] - 4.2, 7.3];

  difference() {
    union() {
      project_box(outer_dims,
		  wall_thickness=wall_thickness,
		  gap=gap,
		  snaps_on_sides=false,
		  corner_radius=corner_radius,
		  top=top,
		  screw_tab_d=screw_tab_d);
      if (top) {
	translate(b1o) {
	  rounded_box(b1d, corner_radius);
	}
	in_Room133_board_frame()
	  shtc3_window(shtc3_loc[0], shtc3_loc[1], space_above_board+wall, false);
      } else {
	// Stuff to add on bottom.
	in_Room133_board_frame() at_corners(board_dims, mount_offset)
	  screw_mount(space_below_board, wall, 1.25);
      }
    }
    // Cut outs.
    if (top) {
      // Negative space of bumps.
      translate(b1o+[wall, wall, -epsilon]) rounded_box(b1d-wall*[2, 2, 1], corner_radius);
      // Oled cutout
      if (oled) {
	room133_to_oled_frame() translate ([4, 8, 5]) cube([12, 28, 20]);
      }
      // USB cutout
      translate([35, -1, space_below_board-1+0.001]) cube([11, 4, 6+epsilon]);
      // Connector cutouts
      in_Room133_board_frame() {
	if (motion_light) {
	  // PIRL connector
	  translate([board_dims[0]-7.5, 14, space_below_board+1.6]) cube([7, 13, 50]);
	}
	if (motion) {
	  // PIR connector
	  translate([board_dims[0]-12.8, 27.5, space_below_board+1.6]) cube([7, 11.5, 50]);
	}
	// Hole for temp sensor
	shtc3_window(shtc3_loc[0], shtc3_loc[1], space_above_board+20, true);
	if (leak_check) {
	  translate([board_dims[0]-13.7, 5.5, space_below_board+1.6]) cube([8.4, 8, 50]);
	}
      }
    }
  }
}
