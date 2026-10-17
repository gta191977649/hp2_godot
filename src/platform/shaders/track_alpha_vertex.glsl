// Retail VU alpha40 modifies RGBA.w only, retaining original ST/Q and RGB.
    gs_rgba.a=hp2_track_alpha40(vu_reflection(source_position,source_normal),
        material_350,material_353,vec3(light_360.x,light_361.x,light_362.x));
