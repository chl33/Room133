include <ProjectBox/project_box.scad>

board_thickness = 1.6;
pad_space = 2.54;
board_dims = [18.75 * pad_space, 15.5 * pad_space, board_thickness];
mount_offset = pad_space;

module oled() {
  s = inch * 0.1;
  pin_len = 4;
  color("silver") {
    for (i = [0: 3]) {
      translate([s*i+pad_space, pad_space/2, -pin_len]) cube([0.25, 0.25, pin_len]);
    }
  }
  color("gray") cube([12, 38, 1]);
  translate([0, 4, 1]) color("black") cube([12, 28, 1]);
}

module room133_to_oled_frame() {
  u = pad_space;
  translate([u*8.1, 1, 12]) children();
}


module Room133_board() {
  u = pad_space;

  // Board imported from KiCad (VRML) -> Blender
  translate([board_dims[0]/2, board_dims[1]/2, board_dims[2]/2]) color("white")
    import(file="RoomL33a.stl", convexity=3);

  room133_to_oled_frame() oled();
}
