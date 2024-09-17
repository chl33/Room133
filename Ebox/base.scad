// This is a base which can clip onto a radar box to help it stand.
include <box.scad>

module base() {
  box_h = outer_dims[2];
  box_l = outer_dims[1];

  slot_h = 8;
  base_h = 3;

  extra_y = 20;
  
  dims = [box_l, box_h + extra_y, slot_h + base_h];

  difference() {
    cube(dims);
    translate([-epsilon, extra_y/2-gap, base_h]) {
      cube([dims[0]+1, box_h+2*gap, outer_dims[1]]);
    }
  }
}

base();
