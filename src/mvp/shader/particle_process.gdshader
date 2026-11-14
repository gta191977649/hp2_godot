shader_type particles;
render_mode disable_velocity, disable_force;
uniform sampler2D step_history : filter_nearest, repeat_disable;
uniform sampler2D group_epochs : filter_nearest, repeat_disable;

uniform float drag;
uniform float gravity;
uniform int atlas_period;
uniform int atlas_cells;

void process() {
    int identity = int(CUSTOM.w);
    int slot = identity & 1023;
    vec4 group = texelFetch(group_epochs, ivec2(slot, 0), 0);
    if (int(group.r) != (identity >> 10)) ACTIVE = false;
    for (int offset = 0; offset < 16 && ACTIVE; ++offset) {
        int packed_life = int(CUSTOM.x);
        int previous = packed_life >> 16;
        int life = packed_life & 65535;
        if (previous == (int(group.b) & 255)) break;
        int index = previous % 16;
        vec4 step = texelFetch(step_history, ivec2(slot, index), 0);
        int ticks = int(texelFetch(step_history, ivec2(slot, index + 16), 0).r);
        int remaining = life - ticks;
        if (remaining <= 0) {
            ACTIVE = false;
            break;
        }
        int packed = int(CUSTOM.z);
        int frame = packed & 255;
        if (atlas_period != 0) {
            frame = (frame + life / atlas_period - remaining / atlas_period) & (atlas_cells - 1);
        }
        vec3 velocity = VELOCITY - drag * abs(VELOCITY) * VELOCITY * step.w;
        velocity.y -= gravity * step.w;
        TRANSFORM[3].xyz += (velocity + step.xyz * float(packed >> 8) / 255.0) * step.w;
        float speed = length(velocity);
        if (speed > 0.0) {
            VELOCITY = trunc(velocity / speed * 127.0) * float(int(speed * 128.0) & 65535) / (128.0 * 127.0);
        } else {
            VELOCITY = vec3(0.0);
        }
        CUSTOM.x = float(remaining | (((previous + 1) & 255) << 16));
        CUSTOM.z = float((packed & 65280) | frame);
    }
}
